#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype.h"
#pragma GCC diagnostic pop

#include "font.h"
#include "gfx.h"

#define GCACHE 1024

struct glyph {
    uint32_t cp;
    int used;
    int w, h, xoff, yoff;
    int adv; /* em unidades da fonte */
    unsigned char *bmp;
};

struct font {
    stbtt_fontinfo info;
    unsigned char *data;
    float scale;
    int ascent, descent, line;
    struct glyph cache[GCACHE];
};

font_t *font_load(const char *path, float px)
{
    FILE *fp = fopen(path, "rb");
    if (!fp)
        return NULL;
    fseek(fp, 0, SEEK_END);
    long n = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    font_t *f = calloc(1, sizeof *f);
    f->data = malloc((size_t)n);
    if (fread(f->data, 1, (size_t)n, fp) != (size_t)n ||
        !stbtt_InitFont(&f->info, f->data, stbtt_GetFontOffsetForIndex(f->data, 0))) {
        fclose(fp);
        free(f->data);
        free(f);
        return NULL;
    }
    fclose(fp);
    f->scale = stbtt_ScaleForPixelHeight(&f->info, px);
    int a, d, g;
    stbtt_GetFontVMetrics(&f->info, &a, &d, &g);
    f->ascent = (int)ceilf(a * f->scale);
    f->descent = (int)floorf(d * f->scale);
    f->line = f->ascent - f->descent + (int)(g * f->scale);
    return f;
}

void font_free(font_t *f)
{
    if (!f)
        return;
    for (int i = 0; i < GCACHE; i++)
        if (f->cache[i].used)
            stbtt_FreeBitmap(f->cache[i].bmp, NULL);
    free(f->data);
    free(f);
}

int font_ascent(const font_t *f) { return f->ascent; }
int font_line(const font_t *f) { return f->line; }

int font_center_baseline(const font_t *f, int y, int h)
{
    /* centraliza a altura das maiúsculas, que é o que o olho percebe */
    int cap = (f->ascent * 72) / 100;
    return y + (h + cap) / 2;
}

/* Decodifica UTF-8; bytes inválidos viram Latin-1 (nomes vindos de FAT). */
static uint32_t utf8_next(const char **s)
{
    const unsigned char *p = (const unsigned char *)*s;
    uint32_t c = p[0];
    if (c < 0x80) {
        *s += 1;
        return c;
    }
    int n = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : c >= 0xC0 ? 1 : -1;
    if (n < 0) {
        *s += 1;
        return c;
    }
    uint32_t cp = c & (0x3Fu >> n);
    for (int i = 1; i <= n; i++) {
        if ((p[i] & 0xC0) != 0x80) {
            *s += 1;
            return c;
        }
        cp = (cp << 6) | (p[i] & 0x3F);
    }
    *s += n + 1;
    return cp;
}

static struct glyph *glyph_get(font_t *f, uint32_t cp)
{
    if (!stbtt_FindGlyphIndex(&f->info, (int)cp))
        cp = '?';
    unsigned h = cp % GCACHE;
    struct glyph *g = NULL;
    for (int k = 0; k < GCACHE; k++) {
        struct glyph *e = &f->cache[(h + k) % GCACHE];
        if (e->used && e->cp == cp)
            return e;
        if (!e->used) {
            g = e;
            break;
        }
    }
    if (!g) { /* cache cheio: reaproveita o slot natural */
        g = &f->cache[h];
        stbtt_FreeBitmap(g->bmp, NULL);
    }
    int adv, lsb;
    stbtt_GetCodepointHMetrics(&f->info, (int)cp, &adv, &lsb);
    g->cp = cp;
    g->used = 1;
    g->adv = adv;
    g->bmp = stbtt_GetCodepointBitmap(&f->info, 0, f->scale, (int)cp, &g->w, &g->h, &g->xoff, &g->yoff);
    return g;
}

static int run(canvas_t *c, font_t *f, int x, int baseline, const char *s, size_t len, uint32_t col)
{
    const char *end = s + len;
    float pen = 0;
    uint32_t prev = 0;
    while (s < end && *s) {
        uint32_t cp = utf8_next(&s);
        if (prev)
            pen += stbtt_GetCodepointKernAdvance(&f->info, (int)prev, (int)cp) * f->scale;
        struct glyph *g = glyph_get(f, cp);
        if (c && g->bmp)
            gfx_mask(c, x + (int)lroundf(pen) + g->xoff, baseline + g->yoff, g->bmp, g->w, g->h, g->w, col);
        pen += g->adv * f->scale;
        prev = cp;
    }
    return (int)ceilf(pen);
}

int font_width(font_t *f, const char *s)
{
    return run(NULL, f, 0, 0, s, strlen(s), 0);
}

int font_draw(canvas_t *c, font_t *f, int x, int baseline, const char *s, uint32_t col)
{
    return run(c, f, x, baseline, s, strlen(s), col);
}

int font_draw_fit(canvas_t *c, font_t *f, int x, int baseline, const char *s, uint32_t col, int maxw)
{
    size_t len = strlen(s);
    if (run(NULL, f, 0, 0, s, len, 0) <= maxw)
        return run(c, f, x, baseline, s, len, col);

    static const char ell[] = "\xE2\x80\xA6"; /* … */
    int ew = font_width(f, ell);
    const char *p = s, *fit = s;
    while (*p) {
        const char *q = p;
        utf8_next(&q);
        if (run(NULL, f, 0, 0, s, (size_t)(q - s), 0) + ew > maxw)
            break;
        fit = p = q;
    }
    /* não termina o corte em espaço */
    while (fit > s && fit[-1] == ' ')
        fit--;
    int w = run(c, f, x, baseline, s, (size_t)(fit - s), col);
    return w + run(c, f, x + w, baseline, ell, sizeof ell - 1, col);
}
