#include <math.h>
#include <stdio.h>

#include "gfx.h"

static inline uint32_t mix(uint32_t d, uint32_t s, unsigned a)
{
    unsigned ia = 255 - a;
    unsigned r = (((s >> 16) & 255) * a + ((d >> 16) & 255) * ia + 127) / 255;
    unsigned g = (((s >> 8) & 255) * a + ((d >> 8) & 255) * ia + 127) / 255;
    unsigned b = ((s & 255) * a + (d & 255) * ia + 127) / 255;
    return 0xFF000000u | (r << 16) | (g << 8) | b;
}

static inline void put(canvas_t *c, int x, int y, uint32_t col, unsigned a)
{
    uint32_t *p = &c->px[y * c->w + x];
    if (a >= 255)
        *p = col | 0xFF000000u;
    else if (a > 0)
        *p = mix(*p, col, a);
}

static int clip(const canvas_t *c, int *x, int *y, int *w, int *h)
{
    if (*x < 0) { *w += *x; *x = 0; }
    if (*y < 0) { *h += *y; *y = 0; }
    if (*x + *w > c->w) *w = c->w - *x;
    if (*y + *h > c->h) *h = c->h - *y;
    return *w > 0 && *h > 0;
}

void gfx_fill(canvas_t *c, int x, int y, int w, int h, uint32_t col)
{
    unsigned a = col >> 24;
    if (a == 0 || !clip(c, &x, &y, &w, &h))
        return;
    for (int j = y; j < y + h; j++) {
        uint32_t *row = &c->px[j * c->w];
        if (a == 255) {
            for (int i = x; i < x + w; i++)
                row[i] = col;
        } else {
            for (int i = x; i < x + w; i++)
                row[i] = mix(row[i], col, a);
        }
    }
}

void gfx_rrect(canvas_t *c, int x, int y, int w, int h, int r, uint32_t col)
{
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    if (r <= 0) {
        gfx_fill(c, x, y, w, h, col);
        return;
    }
    unsigned a0 = col >> 24;

    /* faixa do meio sem cantos: preenchimento direto */
    gfx_fill(c, x, y + r, w, h - 2 * r, col);

    /* faixas de cima e de baixo: cobertura antisserrilhada nos cantos */
    for (int j = 0; j < h; j++) {
        if (j >= r && j < h - r)
            continue;
        int py = y + j;
        if (py < 0 || py >= c->h)
            continue;
        float dy = j < r ? (float)r - (j + 0.5f) : (j + 0.5f) - (float)(h - r);
        for (int i = 0; i < w; i++) {
            int px = x + i;
            if (px < 0 || px >= c->w)
                continue;
            float dx = 0;
            if (i < r)
                dx = (float)r - (i + 0.5f);
            else if (i >= w - r)
                dx = (i + 0.5f) - (float)(w - r);
            float cov = (float)r - sqrtf(dx * dx + dy * dy) + 0.5f;
            if (cov <= 0)
                continue;
            if (cov > 1)
                cov = 1;
            put(c, px, py, col, (unsigned)(a0 * cov + 0.5f));
        }
    }
}

void gfx_vgrad(canvas_t *c, int x, int y, int w, int h, uint32_t top, uint32_t bottom)
{
    if (h <= 0)
        return;
    for (int j = 0; j < h; j++) {
        unsigned t = (unsigned)(j * 255 / (h > 1 ? h - 1 : 1));
        gfx_fill(c, x, y + j, w, 1, mix(top, bottom, t));
    }
}

void gfx_mask(canvas_t *c, int x, int y, const uint8_t *mask, int mw, int mh, int stride, uint32_t col)
{
    unsigned a0 = col >> 24;
    for (int j = 0; j < mh; j++) {
        int py = y + j;
        if (py < 0 || py >= c->h)
            continue;
        for (int i = 0; i < mw; i++) {
            int px = x + i;
            if (px < 0 || px >= c->w)
                continue;
            unsigned m = mask[j * stride + i];
            if (m)
                put(c, px, py, col, (m * a0 + 127) / 255);
        }
    }
}

int gfx_save_bmp(const canvas_t *c, const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f)
        return -1;
    int rowsz = (c->w * 3 + 3) & ~3;
    uint32_t datasz = (uint32_t)rowsz * (uint32_t)c->h;
    uint8_t hdr[54] = {'B', 'M'};
    uint32_t v;
#define LE32(off, val) do { v = (val); hdr[off] = v; hdr[off + 1] = v >> 8; hdr[off + 2] = v >> 16; hdr[off + 3] = v >> 24; } while (0)
    LE32(2, 54 + datasz);
    LE32(10, 54);
    LE32(14, 40);
    LE32(18, (uint32_t)c->w);
    LE32(22, (uint32_t)c->h);
    hdr[26] = 1;
    hdr[28] = 24;
    LE32(34, datasz);
#undef LE32
    fwrite(hdr, 1, sizeof hdr, f);
    uint8_t row[4 * 4096];
    for (int y = c->h - 1; y >= 0; y--) {
        for (int x = 0; x < c->w; x++) {
            uint32_t p = c->px[y * c->w + x];
            row[x * 3] = p & 255;
            row[x * 3 + 1] = (p >> 8) & 255;
            row[x * 3 + 2] = (p >> 16) & 255;
        }
        for (int k = c->w * 3; k < rowsz; k++)
            row[k] = 0;
        fwrite(row, 1, (size_t)rowsz, f);
    }
    fclose(f);
    return 0;
}
