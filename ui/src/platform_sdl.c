/* Plataforma de desenvolvimento: janela SDL2 no PC (ou SDL_VIDEODRIVER=dummy
 * para testes automáticos). Teclado:
 *   setas = direcional, X = A, Z = B, S = X, A = Y, Q/W = L1/R1, 1/2 = L2/R2
 *   Enter = START, Shift direito = SELECT, Esc = MENU, P = POWER, +/- = volume */
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "platform.h"

static SDL_Window *win;
static SDL_Renderer *ren;
static SDL_Texture *tex;

static int map_key(SDL_Scancode sc)
{
    switch (sc) {
    case SDL_SCANCODE_UP: return IN_UP;
    case SDL_SCANCODE_DOWN: return IN_DOWN;
    case SDL_SCANCODE_LEFT: return IN_LEFT;
    case SDL_SCANCODE_RIGHT: return IN_RIGHT;
    case SDL_SCANCODE_X: return IN_A;
    case SDL_SCANCODE_Z: return IN_B;
    case SDL_SCANCODE_S: return IN_X;
    case SDL_SCANCODE_A: return IN_Y;
    case SDL_SCANCODE_Q: return IN_L1;
    case SDL_SCANCODE_W: return IN_R1;
    case SDL_SCANCODE_1: return IN_L2;
    case SDL_SCANCODE_2: return IN_R2;
    case SDL_SCANCODE_RETURN: return IN_START;
    case SDL_SCANCODE_RSHIFT: return IN_SELECT;
    case SDL_SCANCODE_ESCAPE: return IN_MENU;
    case SDL_SCANCODE_P: return IN_POWER;
    case SDL_SCANCODE_EQUALS: return IN_VOLUP;
    case SDL_SCANCODE_MINUS: return IN_VOLDOWN;
    default: return IN_NONE;
    }
}

int plat_init(canvas_t *c)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "brisa-ui: SDL_Init: %s\n", SDL_GetError());
        return -1;
    }
    c->w = 640;
    c->h = 480;
    const char *res = getenv("BRISA_RES"); /* ex.: 720x720 */
    if (res)
        sscanf(res, "%dx%d", &c->w, &c->h);
    int scale = getenv("BRISA_SCALE") ? atoi(getenv("BRISA_SCALE")) : 1;
    if (scale < 1)
        scale = 1;

    win = SDL_CreateWindow("Brisa", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                           c->w * scale, c->h * scale, 0);
    if (!win) {
        fprintf(stderr, "brisa-ui: janela: %s\n", SDL_GetError());
        return -1;
    }
    ren = SDL_CreateRenderer(win, -1, 0);
    if (!ren)
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, c->w, c->h);
    c->px = calloc((size_t)c->w * (size_t)c->h, sizeof(uint32_t));
    return (ren && tex && c->px) ? 0 : -1;
}

void plat_present(const canvas_t *c)
{
    SDL_UpdateTexture(tex, NULL, c->px, c->w * 4);
    SDL_RenderClear(ren);
    SDL_RenderCopy(ren, tex, NULL, NULL);
    SDL_RenderPresent(ren);
}

int plat_poll(uev_t *ev, int timeout_ms)
{
    SDL_Event e;
    uint64_t deadline = now_ms() + (uint64_t)(timeout_ms > 0 ? timeout_ms : 0);
    for (;;) {
        int left = (int)(deadline - now_ms());
        int got = left > 0 ? SDL_WaitEventTimeout(&e, left) : SDL_PollEvent(&e);
        if (!got)
            return 0;
        memset(ev, 0, sizeof *ev);
        if (e.type == SDL_QUIT) {
            ev->kind = UEV_QUIT;
            return 1;
        }
        if ((e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) && !e.key.repeat) {
            ev->kind = UEV_INPUT;
            ev->btn = map_key(e.key.keysym.scancode);
            ev->pressed = e.type == SDL_KEYDOWN;
            ev->raw_type = 1;
            ev->raw_code = e.key.keysym.scancode;
            ev->raw_value = ev->pressed;
            snprintf(ev->raw_dev, sizeof ev->raw_dev, "teclado");
            return 1;
        }
        if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_EXPOSED) {
            ev->kind = UEV_NONE; /* força redesenho */
            return 1;
        }
    }
}

void plat_release(void) {}
void plat_reacquire(void) {}

void plat_shutdown(void)
{
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
}

const char *plat_name(void)
{
    return "sdl";
}
