/* Plataforma do aparelho: framebuffer (/dev/fb0) + evdev (/dev/input/event*).
 * Não usa GPU: a interface aparece antes mesmo de o driver Mali carregar. */
#include <errno.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <linux/kd.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "common.h"
#include "platform.h"

#define MAX_DEVS 16
#define QLEN 128

static int fb_fd = -1, tty_fd = -1;
static uint8_t *fb_mem;
static size_t fb_len;
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;
static int fb_pages, fb_page;

struct dev {
    int fd;
    char name[32];
    int has_stick;
    int ax_min, ax_max, ay_min, ay_max;
    int hat_x, hat_y, stick_x, stick_y; /* -1, 0 ou 1 */
};
static struct dev devs[MAX_DEVS];
static int ndevs;

static uint8_t keymap[KEY_CNT];

static uev_t queue[QLEN];
static int q_head, q_tail;

/* ---------- mapeamento de botões ---------- */

static void map_defaults(void)
{
    static const int def[][2] = {
        /* controle embutido (convenção do kernel; ajuste em input.conf) */
        {BTN_DPAD_UP, IN_UP}, {BTN_DPAD_DOWN, IN_DOWN},
        {BTN_DPAD_LEFT, IN_LEFT}, {BTN_DPAD_RIGHT, IN_RIGHT},
        {BTN_EAST, IN_A}, {BTN_SOUTH, IN_B}, {BTN_NORTH, IN_X}, {BTN_WEST, IN_Y},
        {BTN_TL, IN_L1}, {BTN_TR, IN_R1}, {BTN_TL2, IN_L2}, {BTN_TR2, IN_R2},
        {BTN_SELECT, IN_SELECT}, {BTN_START, IN_START}, {BTN_MODE, IN_MENU},
        {KEY_POWER, IN_POWER}, {KEY_VOLUMEUP, IN_VOLUP}, {KEY_VOLUMEDOWN, IN_VOLDOWN},
        /* teclado USB */
        {KEY_UP, IN_UP}, {KEY_DOWN, IN_DOWN}, {KEY_LEFT, IN_LEFT}, {KEY_RIGHT, IN_RIGHT},
        {KEY_ENTER, IN_A}, {KEY_BACKSPACE, IN_B}, {KEY_ESC, IN_MENU},
        {KEY_TAB, IN_START}, {KEY_SPACE, IN_SELECT},
    };
    memset(keymap, 0, sizeof keymap);
    for (size_t i = 0; i < sizeof def / sizeof def[0]; i++)
        keymap[def[i][0]] = (uint8_t)def[i][1];
}

/* input.conf: "A = 305" ou "A = 305, 28". Um botão citado no arquivo perde
 * os códigos padrão e fica só com os códigos listados. */
static void map_load(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return;
    int cleared[IN_COUNT] = {0};
    char line[256];
    while (fgets(line, sizeof line, f)) {
        char *s = line;
        while (*s == ' ' || *s == '\t')
            s++;
        if (*s == '#' || *s == '\n' || !*s)
            continue;
        char *eq = strchr(s, '=');
        if (!eq)
            continue;
        *eq = 0;
        char name[32];
        if (sscanf(s, "%31s", name) != 1)
            continue;
        int btn = in_from_name(name);
        if (btn == IN_NONE)
            continue;
        if (!cleared[btn]) {
            for (int k = 0; k < KEY_CNT; k++)
                if (keymap[k] == btn)
                    keymap[k] = 0;
            cleared[btn] = 1;
        }
        for (char *tok = strtok(eq + 1, ", \t\r\n"); tok; tok = strtok(NULL, ", \t\r\n")) {
            long code = strtol(tok, NULL, 0);
            if (code > 0 && code < KEY_CNT)
                keymap[code] = (uint8_t)btn;
        }
    }
    fclose(f);
}

/* ---------- fila de eventos ---------- */

static void q_push(const struct dev *d, int btn, int pressed, int type, int code, int value)
{
    int next = (q_tail + 1) % QLEN;
    if (next == q_head)
        return; /* fila cheia: descarta */
    uev_t *e = &queue[q_tail];
    memset(e, 0, sizeof *e);
    e->kind = UEV_INPUT;
    e->btn = btn;
    e->pressed = pressed;
    e->raw_type = type;
    e->raw_code = code;
    e->raw_value = value;
    snprintf(e->raw_dev, sizeof e->raw_dev, "%s", d->name);
    q_tail = next;
}

static int q_pop(uev_t *ev)
{
    if (q_head == q_tail)
        return 0;
    *ev = queue[q_head];
    q_head = (q_head + 1) % QLEN;
    return 1;
}

/* Direção digital vinda de um eixo: solta a anterior, aperta a nova. */
static void axis_dir(struct dev *d, int *state, int nv, int neg, int pos, int code, int value)
{
    if (nv == *state)
        return;
    if (*state)
        q_push(d, *state < 0 ? neg : pos, 0, EV_ABS, code, value);
    if (nv)
        q_push(d, nv < 0 ? neg : pos, 1, EV_ABS, code, value);
    *state = nv;
}

static int stick_dir(int v, int min, int max, int cur)
{
    if (max <= min)
        return 0;
    float n = (2.0f * (v - min) / (float)(max - min)) - 1.0f;
    if (n > 0.6f) return 1;
    if (n < -0.6f) return -1;
    if (n > -0.4f && n < 0.4f) return 0;
    return cur; /* histerese */
}

static void handle(struct dev *d, const struct input_event *e)
{
    if (e->type == EV_KEY) {
        if (e->value == 2) /* autorrepetição do teclado: a interface faz a sua */
            return;
        int btn = e->code < KEY_CNT ? keymap[e->code] : IN_NONE;
        q_push(d, btn, e->value != 0, EV_KEY, e->code, e->value);
    } else if (e->type == EV_ABS) {
        int v = e->value;
        switch (e->code) {
        case ABS_HAT0X:
            axis_dir(d, &d->hat_x, v < 0 ? -1 : v > 0, IN_LEFT, IN_RIGHT, e->code, v);
            break;
        case ABS_HAT0Y:
            axis_dir(d, &d->hat_y, v < 0 ? -1 : v > 0, IN_UP, IN_DOWN, e->code, v);
            break;
        case ABS_X:
            if (d->has_stick)
                axis_dir(d, &d->stick_x, stick_dir(v, d->ax_min, d->ax_max, d->stick_x), IN_LEFT, IN_RIGHT, e->code, v);
            break;
        case ABS_Y:
            if (d->has_stick)
                axis_dir(d, &d->stick_y, stick_dir(v, d->ay_min, d->ay_max, d->stick_y), IN_UP, IN_DOWN, e->code, v);
            break;
        }
    }
}

static void inputs_open(void)
{
    ndevs = 0;
    for (int i = 0; i < 32 && ndevs < MAX_DEVS; i++) {
        char path[32];
        snprintf(path, sizeof path, "/dev/input/event%d", i);
        int fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0)
            continue;
        struct dev *d = &devs[ndevs++];
        memset(d, 0, sizeof *d);
        d->fd = fd;
        if (ioctl(fd, EVIOCGNAME(sizeof d->name - 1), d->name) < 0)
            snprintf(d->name, sizeof d->name, "event%d", i);
        struct input_absinfo ax, ay;
        if (ioctl(fd, EVIOCGABS(ABS_X), &ax) == 0 && ioctl(fd, EVIOCGABS(ABS_Y), &ay) == 0 &&
            ax.maximum > ax.minimum && ay.maximum > ay.minimum) {
            d->has_stick = 1;
            d->ax_min = ax.minimum;
            d->ax_max = ax.maximum;
            d->ay_min = ay.minimum;
            d->ay_max = ay.maximum;
        }
    }
}

static void inputs_drain(void)
{
    struct input_event buf[64];
    for (int i = 0; i < ndevs; i++) {
        while (read(devs[i].fd, buf, sizeof buf) > 0)
            ;
        devs[i].hat_x = devs[i].hat_y = devs[i].stick_x = devs[i].stick_y = 0;
    }
    q_head = q_tail = 0;
}

/* ---------- framebuffer ---------- */

static int fb_open(void)
{
    fb_fd = open("/dev/fb0", O_RDWR | O_CLOEXEC);
    if (fb_fd < 0) {
        perror("brisa-ui: /dev/fb0");
        return -1;
    }
    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo) < 0 || ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo) < 0) {
        perror("brisa-ui: FBIOGET");
        return -1;
    }
    /* um jogo pode ter deixado a tela deslocada para a segunda página */
    vinfo.xoffset = vinfo.yoffset = 0;
    ioctl(fb_fd, FBIOPAN_DISPLAY, &vinfo);
    ioctl(fb_fd, FBIOBLANK, FB_BLANK_UNBLANK);

    fb_pages = vinfo.yres_virtual >= vinfo.yres * 2 &&
               finfo.smem_len >= finfo.line_length * vinfo.yres * 2 ? 2 : 1;
    fb_page = 0;
    fb_len = finfo.smem_len;
    fb_mem = mmap(NULL, fb_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    if (fb_mem == MAP_FAILED) {
        perror("brisa-ui: mmap fb");
        fb_mem = NULL;
        return -1;
    }
    return 0;
}

static void fb_close(void)
{
    if (fb_mem)
        munmap(fb_mem, fb_len);
    if (fb_fd >= 0)
        close(fb_fd);
    fb_mem = NULL;
    fb_fd = -1;
}

static inline uint32_t pack(uint32_t p)
{
    uint32_t r = (p >> 16) & 255, g = (p >> 8) & 255, b = p & 255;
    uint32_t v = ((r >> (8 - vinfo.red.length)) << vinfo.red.offset) |
                 ((g >> (8 - vinfo.green.length)) << vinfo.green.offset) |
                 ((b >> (8 - vinfo.blue.length)) << vinfo.blue.offset);
    if (vinfo.transp.length)
        v |= ((1u << vinfo.transp.length) - 1) << vinfo.transp.offset;
    return v;
}

int plat_init(canvas_t *c)
{
    map_defaults();
    const char *etc = getenv("BRISA_ETC");
    const char *data = getenv("BRISA_DATA");
    char path[512];
    snprintf(path, sizeof path, "%s/input.conf", etc ? etc : "/etc/brisa");
    map_load(path);
    /* ajuste do usuário no cartão de jogos (editável no PC), vale por cima */
    snprintf(path, sizeof path, "%s/input.conf", data ? data : "/mnt/sd1/Brisa");
    map_load(path);

    if (fb_open() < 0)
        return -1;

    /* tira o console de texto da frente (cursor piscando etc.) */
    tty_fd = open("/dev/tty0", O_RDWR | O_CLOEXEC);
    if (tty_fd >= 0)
        ioctl(tty_fd, KDSETMODE, KD_GRAPHICS);

    c->w = (int)vinfo.xres;
    c->h = (int)vinfo.yres;
    c->px = calloc((size_t)c->w * (size_t)c->h, sizeof(uint32_t));
    inputs_open();
    return c->px ? 0 : -1;
}

void plat_present(const canvas_t *c)
{
    if (!fb_mem)
        return;
    int page = fb_pages == 2 ? !fb_page : 0;
    uint8_t *base = fb_mem + (size_t)page * vinfo.yres * finfo.line_length;
    int w = c->w < (int)vinfo.xres ? c->w : (int)vinfo.xres;
    int h = c->h < (int)vinfo.yres ? c->h : (int)vinfo.yres;
    int direct = vinfo.bits_per_pixel == 32 && vinfo.red.offset == 16 &&
                 vinfo.green.offset == 8 && vinfo.blue.offset == 0;

    for (int y = 0; y < h; y++) {
        const uint32_t *src = &c->px[y * c->w];
        uint8_t *dst = base + (size_t)y * finfo.line_length;
        if (direct) {
            memcpy(dst, src, (size_t)w * 4);
        } else if (vinfo.bits_per_pixel == 32) {
            uint32_t *d32 = (uint32_t *)dst;
            for (int x = 0; x < w; x++)
                d32[x] = pack(src[x]);
        } else if (vinfo.bits_per_pixel == 16) {
            uint16_t *d16 = (uint16_t *)dst;
            for (int x = 0; x < w; x++)
                d16[x] = (uint16_t)pack(src[x]);
        }
    }
    if (fb_pages == 2) {
        vinfo.yoffset = (unsigned)page * vinfo.yres;
        ioctl(fb_fd, FBIOPAN_DISPLAY, &vinfo);
        fb_page = page;
    }
}

int plat_poll(uev_t *ev, int timeout_ms)
{
    if (q_pop(ev))
        return 1;
    struct pollfd pfd[MAX_DEVS];
    for (int i = 0; i < ndevs; i++) {
        pfd[i].fd = devs[i].fd;
        pfd[i].events = POLLIN;
        pfd[i].revents = 0;
    }
    if (poll(pfd, (nfds_t)ndevs, timeout_ms) <= 0)
        return 0;
    for (int i = 0; i < ndevs; i++) {
        if (!(pfd[i].revents & POLLIN))
            continue;
        struct input_event buf[32];
        ssize_t n;
        while ((n = read(devs[i].fd, buf, sizeof buf)) > 0)
            for (ssize_t k = 0; k < n / (ssize_t)sizeof buf[0]; k++)
                handle(&devs[i], &buf[k]);
    }
    return q_pop(ev);
}

void plat_release(void)
{
    if (fb_mem) {
        memset(fb_mem, 0, fb_len);
        vinfo.yoffset = 0;
        ioctl(fb_fd, FBIOPAN_DISPLAY, &vinfo);
    }
    fb_close();
}

void plat_reacquire(void)
{
    fb_open();
    if (tty_fd >= 0)
        ioctl(tty_fd, KDSETMODE, KD_GRAPHICS);
    inputs_drain();
}

void plat_shutdown(void)
{
    fb_close();
    for (int i = 0; i < ndevs; i++)
        close(devs[i].fd);
    if (tty_fd >= 0) {
        ioctl(tty_fd, KDSETMODE, KD_TEXT);
        close(tty_fd);
    }
}

const char *plat_name(void)
{
    return "fbdev";
}
