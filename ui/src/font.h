#ifndef BRISA_FONT_H
#define BRISA_FONT_H

#include "common.h"

typedef struct font font_t;

font_t *font_load(const char *path, float px);
void font_free(font_t *f);

int font_ascent(const font_t *f);
int font_line(const font_t *f);
/* Linha de base que centraliza o texto verticalmente em [y, y+h). */
int font_center_baseline(const font_t *f, int y, int h);

int font_width(font_t *f, const char *utf8);
/* Desenha a partir da linha de base; devolve a largura. */
int font_draw(canvas_t *c, font_t *f, int x, int baseline, const char *utf8, uint32_t col);
/* Igual a font_draw, mas corta com "…" se passar de maxw. */
int font_draw_fit(canvas_t *c, font_t *f, int x, int baseline, const char *utf8, uint32_t col, int maxw);

#endif
