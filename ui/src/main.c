/* brisa-ui: interface do Brisa OS.
 *
 * Variáveis de ambiente (os padrões são os caminhos do aparelho):
 *   BRISA_ETC   /etc/brisa        systems.conf, input.conf
 *   BRISA_DATA  /mnt/sd1/Brisa    settings.conf, recent.txt, wifi.conf
 *   BRISA_ROMS  /mnt/sd1/Roms     uma subpasta por sistema
 *   BRISA_BIN   /usr/bin          brisa-launch, brisa-net, brisa-power, brisa-volume
 *   BRISA_SCRIPT                  roteiro de teste: "DOWN,A,SHOT:x.bmp,QUIT"
 */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "font.h"
#include "gfx.h"
#include "platform.h"
#include "settings.h"
#include "sysinfo.h"
#include "systems.h"

/* ---------- tema ---------- */

#define COL_BG_TOP    RGB(0x15, 0x1a, 0x26)
#define COL_BG_BOT    RGB(0x0b, 0x0e, 0x15)
#define COL_PANEL     RGB(0x19, 0x1f, 0x2c)
#define COL_LINE      RGB(0x2a, 0x33, 0x45)
#define COL_TEXT      RGB(0xe9, 0xed, 0xf3)
#define COL_DIM       RGB(0x8d, 0x97, 0xab)
#define COL_ACCENT    RGB(0x4f, 0xd1, 0xc5)
#define COL_ON_ACCENT RGB(0x08, 0x1d, 0x1f)
#define COL_BAD       RGB(0xef, 0x6b, 0x6b)

#define MAX_RECENT 30
#define REPEAT_DELAY 350
#define REPEAT_RATE 70
/* Segurar POWER desliga direito antes que o chip de energia corte à força
 * (o que corrompe o cartão). Um toque rápido abre o menu de energia. */
#define POWER_HOLD 2000
#define MAX_SCRIPT 128

typedef enum { SCR_SYSTEMS, SCR_GAMES, SCR_SETTINGS, SCR_ABOUT, SCR_INPUT, SCR_COUNT } screen_t;

enum { SET_BRIGHT, SET_WIFI, SET_SSH, SET_PERF, SET_RESUME, SET_INPUT, SET_ABOUT, SET_REBOOT, SET_POWEROFF, SET_COUNT };

static const char *const perf_ids[] = {"powersave", "balanced", "performance"};
static const char *const perf_names[] = {"Economia", "Equilibrado", "Máximo"};

static struct {
    canvas_t c;
    font_t *f_title, *f_row, *f_small, *f_big;
    int hdr_h, ftr_h, row_h, pad;

    const char *etc, *data, *roms, *bin;
    char path_settings[512], path_recent[512];

    system_t sys[MAX_SYSTEMS];
    int nsys;
    int vis[MAX_SYSTEMS + 1]; /* linhas da tela inicial; -1 = Recentes */
    int nvis;

    char *recent_path[MAX_RECENT];
    int recent_sys[MAX_RECENT];
    int nrecent;

    game_t *games;
    int ngames;
    int games_sys; /* sistema da lista aberta; -1 = Recentes */

    screen_t scr;
    int sel[SCR_COUNT], top[SCR_COUNT];

    int dialog, dialog_sel;
    char toast[160];
    uint64_t toast_until;

    char rawlog[12][112];
    int nraw;
    char about[8][2][96];
    int nabout;

    int held, menu_down;
    uint64_t next_repeat;
    uint64_t power_since; /* POWER apertado desde (0 = solto) */

    char *script[MAX_SCRIPT];
    int nscript, script_pos;

    int dirty, running, last_min;
} A;

static int S(int v) { return v * A.c.h / 480; }

static const char *env_or(const char *k, const char *def)
{
    const char *v = getenv(k);
    return v && *v ? v : def;
}

static void toast(const char *msg)
{
    snprintf(A.toast, sizeof A.toast, "%s", msg);
    A.toast_until = now_ms() + 2500;
    A.dirty = 1;
}

/* ---------- comandos externos ---------- */

static pid_t spawn(const char *cmd, const char *a1, const char *a2)
{
    char path[512];
    snprintf(path, sizeof path, "%s/%s", A.bin, cmd);
    pid_t pid = fork();
    if (pid == 0) {
        setpgid(0, 0);
        execl(path, cmd, a1, a2, (char *)NULL);
        _exit(127);
    }
    return pid;
}

static int run_wait(const char *cmd, const char *arg)
{
    int st = 0;
    pid_t pid = spawn(cmd, arg, NULL);
    if (pid < 0)
        return -1;
    while (waitpid(pid, &st, 0) < 0 && errno == EINTR)
        ;
    return WIFEXITED(st) ? WEXITSTATUS(st) : -1;
}

static void run_capture(const char *cmd, const char *arg, char *out, size_t n)
{
    char line[600];
    snprintf(line, sizeof line, "'%s/%s' %s 2>/dev/null", A.bin, cmd, arg);
    out[0] = 0;
    FILE *p = popen(line, "r");
    if (!p)
        return;
    if (fgets(out, (int)n, p))
        out[strcspn(out, "\r\n")] = 0;
    pclose(p);
}

/* ---------- jogos recentes ---------- */

static int sys_index(const char *id)
{
    for (int i = 0; i < A.nsys; i++)
        if (strcmp(A.sys[i].id, id) == 0)
            return i;
    return -1;
}

static void recent_load(void)
{
    FILE *f = fopen(A.path_recent, "r");
    if (!f)
        return;
    char line[1400];
    while (A.nrecent < MAX_RECENT && fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = 0;
        char *bar = strchr(line, '|');
        if (!bar)
            continue;
        *bar = 0;
        int si = sys_index(line);
        if (si < 0 || access(bar + 1, R_OK) != 0)
            continue;
        A.recent_sys[A.nrecent] = si;
        A.recent_path[A.nrecent++] = strdup(bar + 1);
    }
    fclose(f);
}

static void recent_save(void)
{
    char tmp[600];
    snprintf(tmp, sizeof tmp, "%s.tmp", A.path_recent);
    FILE *f = fopen(tmp, "w");
    if (!f)
        return;
    for (int i = 0; i < A.nrecent; i++)
        fprintf(f, "%s|%s\n", A.sys[A.recent_sys[i]].id, A.recent_path[i]);
    fclose(f);
    rename(tmp, A.path_recent);
}

static void recent_push(int si, const char *path)
{
    char *p = strdup(path);
    int i;
    for (i = 0; i < A.nrecent; i++)
        if (strcmp(A.recent_path[i], path) == 0)
            break;
    if (i < A.nrecent) {
        free(A.recent_path[i]);
    } else if (A.nrecent == MAX_RECENT) {
        i = MAX_RECENT - 1;
        free(A.recent_path[i]);
    } else {
        i = A.nrecent++;
    }
    for (; i > 0; i--) {
        A.recent_path[i] = A.recent_path[i - 1];
        A.recent_sys[i] = A.recent_sys[i - 1];
    }
    A.recent_path[0] = p;
    A.recent_sys[0] = si;
}

/* ---------- listas ---------- */

static void build_vis(void)
{
    A.nvis = 0;
    if (A.nrecent > 0)
        A.vis[A.nvis++] = -1;
    for (int i = 0; i < A.nsys; i++)
        if (A.sys[i].count > 0)
            A.vis[A.nvis++] = i;
    if (A.sel[SCR_SYSTEMS] >= A.nvis)
        A.sel[SCR_SYSTEMS] = A.nvis > 0 ? A.nvis - 1 : 0;
}

static void rescan(void)
{
    systems_count(A.sys, A.nsys, A.roms);
    build_vis();
    A.dirty = 1;
}

static void open_list(int si, int keep_sel)
{
    games_free(A.games, A.ngames);
    A.games = NULL;
    A.ngames = 0;
    if (si < 0) {
        A.games = calloc((size_t)A.nrecent + 1, sizeof *A.games);
        for (int i = 0; i < A.nrecent; i++) {
            char name[256];
            game_display_name(A.recent_path[i], name, sizeof name);
            A.games[i].name = strdup(name);
            A.games[i].path = strdup(A.recent_path[i]);
            A.games[i].sys = A.recent_sys[i];
        }
        A.ngames = A.nrecent;
    } else {
        A.ngames = games_list(&A.sys[si], si, A.roms, &A.games);
    }
    A.games_sys = si;
    if (!keep_sel || A.sel[SCR_GAMES] >= A.ngames)
        A.sel[SCR_GAMES] = A.top[SCR_GAMES] = 0;
    A.scr = SCR_GAMES;
    A.dirty = 1;
}

static int list_rows(void)
{
    int avail = A.c.h - A.hdr_h - A.ftr_h - S(20);
    int rows = avail / A.row_h;
    return rows < 1 ? 1 : rows;
}

static void move(int *sel, int n, int d)
{
    if (n <= 0)
        return;
    int v = *sel + d;
    if (d == 1 || d == -1) { /* passo simples dá a volta na lista */
        if (v < 0) v = n - 1;
        if (v >= n) v = 0;
    } else {
        if (v < 0) v = 0;
        if (v >= n) v = n - 1;
    }
    *sel = v;
}

/* ---------- desenho ---------- */

typedef void (*row_fn)(int i, char *l, size_t ln, char *r, size_t rn);

static void draw_list(int n, int sel, int *top, row_fn fn)
{
    int rows = list_rows();
    int y0 = A.hdr_h + S(10);
    if (sel >= 0) {
        if (sel < *top) *top = sel;
        if (sel >= *top + rows) *top = sel - rows + 1;
    }
    if (*top > n - rows) *top = n - rows;
    if (*top < 0) *top = 0;

    int x = A.pad, w = A.c.w - 2 * A.pad;
    int bar = n > rows;
    if (bar)
        w -= S(12);

    for (int r = 0; r < rows && *top + r < n; r++) {
        int i = *top + r, y = y0 + r * A.row_h;
        char l[300] = "", rt[80] = "";
        fn(i, l, sizeof l, rt, sizeof rt);
        int on = i == sel;
        if (on)
            gfx_rrect(&A.c, x, y + S(2), w, A.row_h - S(4), S(10), COL_ACCENT);
        int bl = font_center_baseline(A.f_row, y, A.row_h);
        int rw = rt[0] ? font_width(A.f_row, rt) : 0;
        if (rw)
            font_draw(&A.c, A.f_row, x + w - S(14) - rw, bl, rt, on ? COL_ON_ACCENT : COL_DIM);
        font_draw_fit(&A.c, A.f_row, x + S(14), bl, l, on ? COL_ON_ACCENT : COL_TEXT,
                      w - S(28) - (rw ? rw + S(16) : 0));
    }

    if (bar) {
        int tx = A.c.w - A.pad - S(4), th = rows * A.row_h;
        int kh = th * rows / n;
        if (kh < S(24)) kh = S(24);
        int ky = y0 + (th - kh) * *top / (n - rows);
        gfx_rrect(&A.c, tx, y0, S(4), th, S(2), COL_LINE);
        gfx_rrect(&A.c, tx, ky, S(4), kh, S(2), COL_DIM);
    }
}

static void draw_header(const char *title)
{
    gfx_fill(&A.c, 0, 0, A.c.w, A.hdr_h, COL_PANEL);
    gfx_fill(&A.c, 0, A.hdr_h - 1, A.c.w, 1, COL_LINE);
    int d = S(10);
    gfx_rrect(&A.c, A.pad, (A.hdr_h - d) / 2, d, d, d / 2, COL_ACCENT);
    font_draw_fit(&A.c, A.f_title, A.pad + d + S(10), font_center_baseline(A.f_title, 0, A.hdr_h),
                  title, COL_TEXT, A.c.w / 2);

    int x = A.c.w - A.pad;
    int bl = font_center_baseline(A.f_small, 0, A.hdr_h);
    char clk[8];
    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm);
    strftime(clk, sizeof clk, "%H:%M", &tm);
    A.last_min = tm.tm_min;
    x -= font_width(A.f_small, clk);
    font_draw(&A.c, A.f_small, x, bl, clk, COL_TEXT);
    x -= S(16);

    int chg, pct = sys_battery(&chg);
    if (pct >= 0) {
        int bw = S(24), bh = S(12), by = (A.hdr_h - bh) / 2;
        uint32_t col = chg ? COL_ACCENT : pct <= 15 ? COL_BAD : COL_TEXT;
        x -= S(3);
        gfx_fill(&A.c, x, by + bh / 3, S(3), bh - 2 * (bh / 3), col);
        x -= bw;
        gfx_rrect(&A.c, x, by, bw, bh, S(3), col);
        gfx_rrect(&A.c, x + S(2), by + S(2), bw - S(4), bh - S(4), S(2), COL_PANEL);
        gfx_fill(&A.c, x + S(3), by + S(3), (bw - S(6)) * pct / 100, bh - S(6), col);
        char p[16];
        snprintf(p, sizeof p, "%d%%", pct);
        x -= S(6) + font_width(A.f_small, p);
        font_draw(&A.c, A.f_small, x, bl, p, COL_DIM);
        x -= S(16);
    }

    char ip[64];
    if (sys_ip(ip, sizeof ip)) {
        /* três barrinhas de sinal */
        int bw = S(4), gap = S(2), base = A.hdr_h / 2 + S(6);
        x -= 3 * bw + 2 * gap;
        for (int i = 0; i < 3; i++) {
            int bh = S(5) + i * S(4);
            gfx_fill(&A.c, x + i * (bw + gap), base - bh, bw, bh, COL_ACCENT);
        }
    }
}

/* hints: pares tecla/rótulo */
static void draw_footer(const char *const *hints, int n, const char *right)
{
    int y = A.c.h - A.ftr_h;
    gfx_fill(&A.c, 0, y, A.c.w, A.ftr_h, COL_PANEL);
    gfx_fill(&A.c, 0, y, A.c.w, 1, COL_LINE);
    int x = A.pad, bl = font_center_baseline(A.f_small, y, A.ftr_h);
    for (int i = 0; i + 1 < n; i += 2) {
        int kw = font_width(A.f_small, hints[i]) + S(14), kh = S(22);
        if (kw < kh)
            kw = kh;
        gfx_rrect(&A.c, x, y + (A.ftr_h - kh) / 2, kw, kh, kh / 2, COL_LINE);
        font_draw(&A.c, A.f_small, x + (kw - font_width(A.f_small, hints[i])) / 2, bl, hints[i], COL_TEXT);
        x += kw + S(6);
        x += font_draw(&A.c, A.f_small, x, bl, hints[i + 1], COL_DIM) + S(16);
    }
    if (right) {
        int w = font_width(A.f_small, right);
        font_draw(&A.c, A.f_small, A.c.w - A.pad - w, bl, right, COL_DIM);
    }
}

static void draw_center(font_t *f, int y, const char *s, uint32_t col)
{
    int w = font_width(f, s);
    if (w > A.c.w - 2 * A.pad)
        font_draw_fit(&A.c, f, A.pad, y, s, col, A.c.w - 2 * A.pad);
    else
        font_draw(&A.c, f, (A.c.w - w) / 2, y, s, col);
}

static void row_system(int i, char *l, size_t ln, char *r, size_t rn)
{
    int si = A.vis[i];
    if (si < 0) {
        snprintf(l, ln, "Recentes");
        snprintf(r, rn, "%d", A.nrecent);
    } else {
        snprintf(l, ln, "%s", A.sys[si].name);
        if (A.sys[si].count == 1)
            snprintf(r, rn, "1 jogo");
        else
            snprintf(r, rn, "%d jogos", A.sys[si].count);
    }
}

static void row_game(int i, char *l, size_t ln, char *r, size_t rn)
{
    snprintf(l, ln, "%s", A.games[i].name);
    if (A.games_sys < 0)
        snprintf(r, rn, "%s", A.sys[A.games[i].sys].name);
}

static void row_setting(int i, char *l, size_t ln, char *r, size_t rn)
{
    static const char *const labels[SET_COUNT] = {
        "Brilho", "Wi-Fi", "SSH e SFTP", "Desempenho nos jogos", "Continuar de onde parou",
        "Testar controles", "Sobre", "Reiniciar", "Desligar",
    };
    snprintf(l, ln, "%s", labels[i]);
    switch (i) {
    case SET_BRIGHT:
        snprintf(r, rn, "\xE2\x97\x80  %d  \xE2\x96\xB6", settings_get_int("brightness", 7));
        break;
    case SET_WIFI:
        snprintf(r, rn, "%s", settings_get_int("wifi", 0) ? "Ligado" : "Desligado");
        break;
    case SET_SSH:
        snprintf(r, rn, "%s", settings_get_int("ssh", 0) ? "Ligado" : "Desligado");
        break;
    case SET_PERF: {
        const char *p = settings_get("perf", "balanced");
        int k = 1;
        for (int j = 0; j < 3; j++)
            if (strcmp(p, perf_ids[j]) == 0)
                k = j;
        snprintf(r, rn, "%s", perf_names[k]);
        break;
    }
    case SET_RESUME:
        snprintf(r, rn, "%s", settings_get_int("resume", 1) ? "Sim" : "Não");
        break;
    }
}

static void row_about(int i, char *l, size_t ln, char *r, size_t rn)
{
    snprintf(l, ln, "%s", A.about[i][0]);
    snprintf(r, rn, "%s", A.about[i][1]);
}

static void draw_toast(void)
{
    int w = font_width(A.f_small, A.toast) + S(36);
    if (w > A.c.w - 2 * A.pad)
        w = A.c.w - 2 * A.pad;
    int h = S(36), x = (A.c.w - w) / 2, y = A.c.h - A.ftr_h - h - S(12);
    gfx_rrect(&A.c, x, y, w, h, h / 2, RGB(0x26, 0x30, 0x42));
    font_draw_fit(&A.c, A.f_small, x + S(18), font_center_baseline(A.f_small, y, h), A.toast, COL_TEXT, w - S(36));
}

static void draw_dialog(void)
{
    static const char *const opts[] = {"Desligar", "Reiniciar", "Cancelar"};
    gfx_fill(&A.c, 0, 0, A.c.w, A.c.h, RGBA(0, 0, 0, 170));
    int w = S(300), h = S(64) + 3 * A.row_h + S(16);
    int x = (A.c.w - w) / 2, y = (A.c.h - h) / 2;
    gfx_rrect(&A.c, x, y, w, h, S(16), COL_PANEL);
    font_draw(&A.c, A.f_title, x + S(22), y + S(42), "Energia", COL_TEXT);
    for (int i = 0; i < 3; i++) {
        int ry = y + S(60) + i * A.row_h;
        if (i == A.dialog_sel)
            gfx_rrect(&A.c, x + S(12), ry, w - S(24), A.row_h - S(4), S(10), COL_ACCENT);
        font_draw(&A.c, A.f_row, x + S(26), font_center_baseline(A.f_row, ry, A.row_h - S(4)), opts[i],
                  i == A.dialog_sel ? COL_ON_ACCENT : COL_TEXT);
    }
}

static void draw(void)
{
    gfx_vgrad(&A.c, 0, 0, A.c.w, A.c.h, COL_BG_TOP, COL_BG_BOT);

    switch (A.scr) {
    case SCR_SYSTEMS: {
        draw_header("Brisa");
        static const char *const h[] = {"A", "Abrir", "START", "Ajustes", "SELECT", "Atualizar"};
        if (A.nvis == 0) {
            int y = A.c.h / 2 - S(40);
            draw_center(A.f_big, y, "Nenhum jogo encontrado", COL_TEXT);
            draw_center(A.f_small, y + S(40), "Copie as ROMs para a pasta Roms do cartão,", COL_DIM);
            draw_center(A.f_small, y + S(64), "uma subpasta por sistema: GB, GBA, SNES, MD, PS...", COL_DIM);
            draw_center(A.f_small, y + S(100), A.roms, COL_ACCENT);
            draw_footer(h + 2, 4, NULL);
        } else {
            int total = 0;
            for (int i = 0; i < A.nsys; i++)
                total += A.sys[i].count;
            char right[32];
            snprintf(right, sizeof right, "%d jogos", total);
            draw_list(A.nvis, A.sel[SCR_SYSTEMS], &A.top[SCR_SYSTEMS], row_system);
            draw_footer(h, 6, right);
        }
        break;
    }
    case SCR_GAMES: {
        draw_header(A.games_sys < 0 ? "Recentes" : A.sys[A.games_sys].name);
        static const char *const h[] = {"A", "Jogar", "B", "Voltar", "L1/R1", "Página"};
        char right[32];
        snprintf(right, sizeof right, "%d/%d", A.ngames ? A.sel[SCR_GAMES] + 1 : 0, A.ngames);
        if (A.ngames == 0)
            draw_center(A.f_row, A.c.h / 2, "Nenhum jogo nesta pasta", COL_DIM);
        draw_list(A.ngames, A.sel[SCR_GAMES], &A.top[SCR_GAMES], row_game);
        draw_footer(h, 6, right);
        break;
    }
    case SCR_SETTINGS: {
        draw_header("Ajustes");
        static const char *const h[] = {"A", "Selecionar", "\xE2\x97\x80\xE2\x96\xB6", "Alterar", "B", "Voltar"};
        draw_list(SET_COUNT, A.sel[SCR_SETTINGS], &A.top[SCR_SETTINGS], row_setting);
        draw_footer(h, 6, NULL);
        break;
    }
    case SCR_ABOUT: {
        draw_header("Sobre");
        static const char *const h[] = {"B", "Voltar"};
        draw_list(A.nabout, -1, &A.top[SCR_ABOUT], row_about);
        draw_footer(h, 2, NULL);
        break;
    }
    case SCR_INPUT: {
        draw_header("Testar controles");
        static const char *const h[] = {"B", "Voltar", "POWER", "Voltar"};
        int y = A.hdr_h + S(34);
        font_draw(&A.c, A.f_small, A.pad, y, "Aperte os botões. Use os códigos em /etc/brisa/input.conf.", COL_DIM);
        if (A.nraw == 0)
            draw_center(A.f_row, A.c.h / 2, "Esperando um botão...", COL_TEXT);
        for (int i = 0; i < A.nraw; i++)
            font_draw_fit(&A.c, A.f_small, A.pad, y + S(32) + i * S(26), A.rawlog[i],
                          i == 0 ? COL_ACCENT : COL_TEXT, A.c.w - 2 * A.pad);
        draw_footer(h, 4, NULL);
        break;
    }
    default:
        break;
    }

    if (A.toast_until && now_ms() < A.toast_until)
        draw_toast();
    if (A.dialog)
        draw_dialog();
}

/* ---------- ações ---------- */

static void volume(int btn, int show)
{
    char out[16];
    run_capture("brisa-volume", btn == IN_VOLUP ? "up" : "down", out, sizeof out);
    if (show && out[0]) {
        char msg[48];
        snprintf(msg, sizeof msg, "Volume %s%%", out);
        toast(msg);
    }
}

static void brightness(int d, int show)
{
    int v = settings_get_int("brightness", 7) + d;
    if (v < 1) v = 1;
    if (v > 10) v = 10;
    settings_set_int("brightness", v);
    settings_save();
    bright_set(v);
    if (show) {
        char msg[32];
        snprintf(msg, sizeof msg, "Brilho %d", v);
        toast(msg);
    }
    A.dirty = 1;
}

static void power(const char *action)
{
    gfx_vgrad(&A.c, 0, 0, A.c.w, A.c.h, COL_BG_TOP, COL_BG_BOT);
    draw_center(A.f_big, A.c.h / 2, strcmp(action, "reboot") == 0 ? "Reiniciando..." : "Desligando...", COL_TEXT);
    plat_present(&A.c);
    settings_save();
    recent_save();
    run_wait("brisa-power", action);
    /* no aparelho não volta; no PC o script de teste só registra */
    A.dialog = 0;
    toast("Comando de energia enviado");
}

static void launch(const game_t *g)
{
    const system_t *s = &A.sys[g->sys];

    gfx_vgrad(&A.c, 0, 0, A.c.w, A.c.h, COL_BG_TOP, COL_BG_BOT);
    draw_center(A.f_small, A.c.h / 2 - S(34), s->name, COL_ACCENT);
    draw_center(A.f_big, A.c.h / 2 + S(4), g->name, COL_TEXT);
    draw_center(A.f_small, A.c.h / 2 + S(44), "Abrindo...", COL_DIM);
    plat_present(&A.c);

    char *path = strdup(g->path);
    int si = g->sys;
    recent_push(si, path);
    recent_save();

    plat_release();
    pid_t pid = spawn("brisa-launch", s->id, path);
    int status = 0, killed = 0;
    uint64_t kill_at = 0;
    int menu = 0;
    while (pid > 0) {
        pid_t r = waitpid(pid, &status, WNOHANG);
        if (r == pid || (r < 0 && errno != EINTR))
            break;
        uev_t ev;
        /* atalhos globais durante o jogo */
        if (plat_poll(&ev, 100) && ev.kind == UEV_INPUT) {
            if (ev.btn == IN_MENU) {
                menu = ev.pressed;
            } else if (ev.pressed) {
                if (ev.btn == IN_VOLUP || ev.btn == IN_VOLDOWN)
                    volume(ev.btn, 0);
                else if (menu && (ev.btn == IN_UP || ev.btn == IN_DOWN))
                    brightness(ev.btn == IN_UP ? 1 : -1, 0);
                else if (ev.btn == IN_POWER && !killed) {
                    /* o RetroArch salva SRAM e o estado ao receber SIGTERM */
                    kill(pid, SIGTERM);
                    killed = 1;
                    kill_at = now_ms() + 6000;
                }
            }
        }
        if (killed == 1 && now_ms() > kill_at) {
            kill(-pid, SIGKILL);
            killed = 2;
        }
    }
    plat_reacquire();
    A.held = A.menu_down = 0;

    if (pid < 0 || (!killed && WIFEXITED(status) && WEXITSTATUS(status) != 0))
        toast("Não foi possível abrir o jogo (veja Brisa/logs no cartão)");

    build_vis();
    if (A.scr == SCR_GAMES && A.games_sys < 0)
        open_list(-1, 0);
    free(path);
    A.dirty = 1;
}

static void open_about(void)
{
    char buf[96];
    A.nabout = 0;
#define ABOUT(k, ...) do { snprintf(A.about[A.nabout][0], 96, "%s", k); \
                           snprintf(A.about[A.nabout][1], 96, __VA_ARGS__); A.nabout++; } while (0)
    snprintf(buf, sizeof buf, "%s/version", A.etc);
    FILE *f = fopen(buf, "r");
    char ver[32] = "dev";
    if (f) {
        if (fgets(ver, sizeof ver, f))
            ver[strcspn(ver, "\r\n")] = 0;
        fclose(f);
    }
    ABOUT("Brisa OS", "%s (%s)", BRISA_VERSION, ver);
    sys_kernel(buf, sizeof buf);
    ABOUT("Sistema", "%s", buf);
    ABOUT("Memória", "%ld MB", sys_ram_mb());
    ABOUT("Rede", "%s", sys_ip(buf, sizeof buf) ? buf : "sem conexão");
    long mb = sys_free_mb(A.roms);
    if (mb >= 0)
        ABOUT("Espaço livre", "%.1f GB", mb / 1024.0);
    ABOUT("Pasta de ROMs", "%s", A.roms);
    ABOUT("Vídeo", "%s %dx%d", plat_name(), A.c.w, A.c.h);
#undef ABOUT
    A.top[SCR_ABOUT] = 0;
    A.scr = SCR_ABOUT;
}

static void setting_change(int item, int d)
{
    switch (item) {
    case SET_BRIGHT:
        brightness(d, 0);
        return;
    case SET_WIFI: {
        int on = !settings_get_int("wifi", 0);
        settings_set_int("wifi", on);
        settings_save();
        char p[600];
        snprintf(p, sizeof p, "%s/wifi.conf", A.data);
        if (on && access(p, R_OK) != 0)
            toast("Crie Brisa/wifi.conf no cartão (veja wifi.conf.exemplo)");
        else
            toast(on ? "Wi-Fi ligado, conectando..." : "Wi-Fi desligado");
        spawn("brisa-net", "apply", NULL);
        break;
    }
    case SET_SSH: {
        int on = !settings_get_int("ssh", 0);
        settings_set_int("ssh", on);
        settings_save();
        toast(on ? "SSH e SFTP ligados (usuário root)" : "SSH e SFTP desligados");
        spawn("brisa-net", "apply", NULL);
        break;
    }
    case SET_PERF: {
        const char *p = settings_get("perf", "balanced");
        int k = 1;
        for (int j = 0; j < 3; j++)
            if (strcmp(p, perf_ids[j]) == 0)
                k = j;
        k = (k + (d < 0 ? 2 : 1)) % 3;
        settings_set("perf", perf_ids[k]);
        settings_save();
        break;
    }
    case SET_RESUME:
        settings_set_int("resume", !settings_get_int("resume", 1));
        settings_save();
        break;
    }
    A.dirty = 1;
}

/* ---------- entrada ---------- */

static void on_press(int b)
{
    A.dirty = 1;

    if (A.dialog) {
        if (b == IN_UP || b == IN_DOWN)
            move(&A.dialog_sel, 3, b == IN_UP ? -1 : 1);
        else if (b == IN_B || b == IN_POWER)
            A.dialog = 0;
        else if (b == IN_A) {
            if (A.dialog_sel == 0)
                power("poweroff");
            else if (A.dialog_sel == 1)
                power("reboot");
            else
                A.dialog = 0;
        }
        return;
    }
    if (b == IN_POWER) {
        A.dialog = 1;
        A.dialog_sel = 0;
        return;
    }
    if (b == IN_VOLUP || b == IN_VOLDOWN) {
        volume(b, 1);
        return;
    }
    if (A.menu_down && (b == IN_UP || b == IN_DOWN)) {
        brightness(b == IN_UP ? 1 : -1, 1);
        return;
    }

    int page = list_rows();
    switch (A.scr) {
    case SCR_SYSTEMS:
        if (b == IN_UP || b == IN_DOWN)
            move(&A.sel[SCR_SYSTEMS], A.nvis, b == IN_UP ? -1 : 1);
        else if (b == IN_L1 || b == IN_R1)
            move(&A.sel[SCR_SYSTEMS], A.nvis, b == IN_L1 ? -page : page);
        else if (b == IN_A && A.nvis > 0)
            open_list(A.vis[A.sel[SCR_SYSTEMS]], 0);
        else if (b == IN_START) {
            A.scr = SCR_SETTINGS;
        } else if (b == IN_SELECT) {
            rescan();
            toast("Lista de jogos atualizada");
        }
        break;
    case SCR_GAMES:
        if (b == IN_UP || b == IN_DOWN)
            move(&A.sel[SCR_GAMES], A.ngames, b == IN_UP ? -1 : 1);
        else if (b == IN_L1 || b == IN_R1 || b == IN_LEFT || b == IN_RIGHT)
            move(&A.sel[SCR_GAMES], A.ngames, (b == IN_L1 || b == IN_LEFT) ? -page : page);
        else if (b == IN_A && A.ngames > 0)
            launch(&A.games[A.sel[SCR_GAMES]]);
        else if (b == IN_B) {
            A.scr = SCR_SYSTEMS;
            for (int i = 0; i < A.nvis; i++)
                if (A.vis[i] == A.games_sys)
                    A.sel[SCR_SYSTEMS] = i;
        }
        break;
    case SCR_SETTINGS: {
        int it = A.sel[SCR_SETTINGS];
        if (b == IN_UP || b == IN_DOWN)
            move(&A.sel[SCR_SETTINGS], SET_COUNT, b == IN_UP ? -1 : 1);
        else if (b == IN_B || b == IN_START)
            A.scr = SCR_SYSTEMS;
        else if (b == IN_LEFT || b == IN_RIGHT) {
            if (it <= SET_RESUME)
                setting_change(it, b == IN_LEFT ? -1 : 1);
        } else if (b == IN_A) {
            if (it == SET_BRIGHT)
                setting_change(it, 1);
            else if (it <= SET_RESUME)
                setting_change(it, 1);
            else if (it == SET_INPUT) {
                A.nraw = 0;
                A.scr = SCR_INPUT;
            } else if (it == SET_ABOUT)
                open_about();
            else if (it == SET_REBOOT)
                power("reboot");
            else if (it == SET_POWEROFF)
                power("poweroff");
        }
        break;
    }
    case SCR_ABOUT:
        if (b == IN_B || b == IN_A)
            A.scr = SCR_SETTINGS;
        break;
    default:
        break;
    }
}

static int repeats(int b)
{
    return b == IN_UP || b == IN_DOWN || b == IN_LEFT || b == IN_RIGHT || b == IN_L1 || b == IN_R1;
}

static void log_raw(const uev_t *ev)
{
    const char *type = ev->raw_type == 1 ? "KEY" : ev->raw_type == 3 ? "ABS" : "?";
    memmove(A.rawlog[1], A.rawlog[0], sizeof A.rawlog[0] * 11);
    snprintf(A.rawlog[0], sizeof A.rawlog[0], "%s   %s %d = %d   \xE2\x86\x92 %s",
             ev->raw_dev, type, ev->raw_code, ev->raw_value, in_name(ev->btn));
    if (A.nraw < 12)
        A.nraw++;
}

static void on_event(const uev_t *ev)
{
    if (ev->kind == UEV_QUIT) {
        A.running = 0;
        return;
    }
    if (ev->kind != UEV_INPUT) {
        A.dirty = 1;
        return;
    }
    if (A.scr == SCR_INPUT) {
        log_raw(ev);
        A.dirty = 1;
        if (ev->pressed && (ev->btn == IN_B || ev->btn == IN_POWER))
            A.scr = SCR_SETTINGS;
        return;
    }
    if (ev->btn == IN_MENU)
        A.menu_down = ev->pressed;
    if (ev->btn == IN_POWER) {
        /* decide ao soltar: toque abre o menu; 2 s segurando desliga (ver main) */
        if (ev->pressed) {
            A.power_since = now_ms();
        } else if (A.power_since) {
            A.power_since = 0;
            on_press(IN_POWER);
        }
        return;
    }
    if (ev->pressed) {
        on_press(ev->btn);
        if (repeats(ev->btn)) {
            A.held = ev->btn;
            A.next_repeat = now_ms() + REPEAT_DELAY;
        }
    } else if (ev->btn == A.held) {
        A.held = 0;
    }
}

/* ---------- roteiro de teste ---------- */

static void script_load(void)
{
    const char *s = getenv("BRISA_SCRIPT");
    if (!s)
        return;
    char *copy = strdup(s);
    for (char *t = strtok(copy, ","); t && A.nscript < MAX_SCRIPT; t = strtok(NULL, ","))
        A.script[A.nscript++] = strdup(t);
    free(copy);
}

static void script_step(void)
{
    const char *t = A.script[A.script_pos++];
    if (strncmp(t, "SHOT:", 5) == 0) {
        gfx_save_bmp(&A.c, t + 5);
    } else if (strcmp(t, "QUIT") == 0) {
        A.running = 0;
    } else {
        uev_t ev = {.kind = UEV_INPUT, .btn = in_from_name(t), .pressed = 1};
        on_event(&ev);
        ev.pressed = 0;
        on_event(&ev);
    }
}

/* ---------- main ---------- */

static font_t *load_font(const char *file, float px)
{
    const char *dirs[] = {getenv("BRISA_FONT_DIR"), "/usr/share/fonts/dejavu",
                          "/usr/share/fonts/truetype/dejavu", NULL};
    for (int i = 0; i < 3; i++) {
        if (!dirs[i])
            continue;
        char p[512];
        snprintf(p, sizeof p, "%s/%s", dirs[i], file);
        font_t *f = font_load(p, px);
        if (f)
            return f;
    }
    return NULL;
}

/* brisa-ui --splash "texto": desenha uma tela de aviso e sai, deixando-a no
 * display (usado pelo rcS em tarefas longas, como preparar o cartão). */
static int splash(const char *msg)
{
    gfx_vgrad(&A.c, 0, 0, A.c.w, A.c.h, COL_BG_TOP, COL_BG_BOT);
    const char *title = "Brisa OS";
    int d = S(14), gap = S(14);
    int tw = font_width(A.f_big, title);
    int x = (A.c.w - (d + gap + tw)) / 2, y = A.c.h / 2 - S(10);
    gfx_rrect(&A.c, x, y - (font_ascent(A.f_big) * 72 / 100 + d) / 2, d, d, d / 2, COL_ACCENT);
    font_draw(&A.c, A.f_big, x + d + gap, y, title, COL_TEXT);
    draw_center(A.f_row, y + S(44), msg, COL_DIM);
    draw_center(A.f_small, y + S(74), "Isso acontece só uma vez.", COL_DIM);
    plat_present(&A.c);
    const char *shot = getenv("BRISA_SHOT");
    if (shot)
        gfx_save_bmp(&A.c, shot);
    /* sem plat_shutdown: a tela fica como está até a interface abrir */
    return 0;
}

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);
    const char *splash_msg = argc > 2 && strcmp(argv[1], "--splash") == 0 ? argv[2] : NULL;
    A.etc = env_or("BRISA_ETC", "/etc/brisa");
    A.data = env_or("BRISA_DATA", "/mnt/sd1/Brisa");
    A.roms = env_or("BRISA_ROMS", "/mnt/sd1/Roms");
    A.bin = env_or("BRISA_BIN", "/usr/bin");
    if (!splash_msg)
        mkdir(A.data, 0755);
    snprintf(A.path_settings, sizeof A.path_settings, "%s/settings.conf", A.data);
    snprintf(A.path_recent, sizeof A.path_recent, "%s/recent.txt", A.data);

    settings_load(A.path_settings);
    setenv("TZ", settings_get("tz", "<-03>3"), 1); /* horário de Brasília */
    tzset();

    if (plat_init(&A.c) < 0)
        return 1;

    A.hdr_h = S(46);
    A.ftr_h = S(40);
    A.row_h = S(44);
    A.pad = S(16);
    A.f_title = load_font("DejaVuSans-Bold.ttf", (float)S(21));
    A.f_big = load_font("DejaVuSans-Bold.ttf", (float)S(26));
    A.f_row = load_font("DejaVuSans.ttf", (float)S(19));
    A.f_small = load_font("DejaVuSans.ttf", (float)S(15));
    if (!A.f_title || !A.f_big || !A.f_row || !A.f_small) {
        fprintf(stderr, "brisa-ui: fonte DejaVu Sans não encontrada\n");
        plat_shutdown();
        return 1;
    }
    if (splash_msg)
        return splash(splash_msg);

    bright_set(settings_get_int("brightness", 7));

    char conf[512];
    snprintf(conf, sizeof conf, "%s/systems.conf", A.etc);
    A.nsys = systems_load(conf, A.sys, MAX_SYSTEMS);
    systems_count(A.sys, A.nsys, A.roms);
    recent_load();
    build_vis();
    script_load();

    A.running = A.dirty = 1;
    while (A.running) {
        if (A.dirty) {
            draw();
            plat_present(&A.c);
            A.dirty = 0;
        }
        if (A.script_pos < A.nscript) {
            script_step();
            continue;
        }
        if (A.nscript && A.script_pos >= A.nscript)
            break;

        uint64_t now = now_ms();
        if (A.power_since && now - A.power_since >= POWER_HOLD) {
            A.power_since = 0;
            power("poweroff");
            continue;
        }
        int timeout = 1000;
        if (A.held)
            timeout = A.next_repeat > now ? (int)(A.next_repeat - now) : 0;
        else if (A.toast_until > now && A.toast_until - now < 1000)
            timeout = (int)(A.toast_until - now);
        if (A.power_since) {
            int left = (int)(A.power_since + POWER_HOLD - now);
            if (left < timeout)
                timeout = left;
        }

        uev_t ev;
        if (plat_poll(&ev, timeout)) {
            on_event(&ev);
        } else {
            now = now_ms();
            if (A.held && now >= A.next_repeat) {
                on_press(A.held);
                A.next_repeat = now + REPEAT_RATE;
            }
            if (A.toast_until && now >= A.toast_until) {
                A.toast_until = 0;
                A.dirty = 1;
            }
            time_t t = time(NULL);
            struct tm tm;
            localtime_r(&t, &tm);
            if (tm.tm_min != A.last_min)
                A.dirty = 1;
        }
        while (waitpid(-1, NULL, WNOHANG) > 0)
            ;
    }

    settings_save();
    plat_shutdown();
    return 0;
}
