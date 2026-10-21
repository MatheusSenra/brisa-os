#ifndef BRISA_GFX_H
#define BRISA_GFX_H

#include "common.h"

#define RGB(r, g, b) (0xFF000000u | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))
#define RGBA(r, g, b, a) (((uint32_t)(a) << 24) | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))
#define WITH_ALPHA(c, a) (((c) & 0x00FFFFFFu) | ((uint32_t)(a) << 24))

/* Todas as funções recortam nas bordas e respeitam o alfa da cor. */
void gfx_fill(canvas_t *c, int x, int y, int w, int h, uint32_t col);
void gfx_rrect(canvas_t *c, int x, int y, int w, int h, int r, uint32_t col);
void gfx_vgrad(canvas_t *c, int x, int y, int w, int h, uint32_t top, uint32_t bottom);
/* Pinta uma máscara de 8 bits (cobertura de glifo) com a cor dada. */
void gfx_mask(canvas_t *c, int x, int y, const uint8_t *mask, int mw, int mh, int stride, uint32_t col);

int gfx_save_bmp(const canvas_t *c, const char *path);

#endif
