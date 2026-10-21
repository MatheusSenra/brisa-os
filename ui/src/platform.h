#ifndef BRISA_PLATFORM_H
#define BRISA_PLATFORM_H

#include "common.h"

/* Implementado por platform_fbdev.c (aparelho) ou platform_sdl.c (PC). */

int plat_init(canvas_t *c);              /* aloca a tela no tamanho do display */
void plat_present(const canvas_t *c);
int plat_poll(uev_t *ev, int timeout_ms); /* 1 = evento, 0 = tempo esgotado */

/* Antes de abrir um jogo: solta o display, mas continua lendo os controles
 * para os atalhos globais (volume, brilho, POWER). */
void plat_release(void);
/* Depois do jogo: retoma o display e descarta eventos acumulados. */
void plat_reacquire(void);

void plat_shutdown(void);
const char *plat_name(void);

#endif
