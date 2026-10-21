#!/usr/bin/env python3
"""Monta as imagens do README a partir das capturas do `make test`.

uso: showcase.py <pasta-das-capturas> <pasta-de-saída>
Gera telas.png (três telas lado a lado) e uma PNG por tela.
"""
import os
import sys

from PIL import Image, ImageDraw, ImageFilter

SHOTS = [("1-sistemas", "sistemas"), ("2-jogos", "jogos"), ("4-ajustes", "ajustes"),
         ("5-energia", "energia")]
SCALE = 2          # telas em 1280x960 para ficar nítido em telas retina
RADIUS = 28
GAP = 48
MARGIN = 72
BG_TOP = (21, 26, 38)
BG_BOT = (8, 11, 17)


def rounded(im, r):
    mask = Image.new("L", im.size, 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, im.width - 1, im.height - 1), r, fill=255)
    out = im.convert("RGBA")
    out.putalpha(mask)
    return out


def gradient(w, h):
    bg = Image.new("RGB", (w, h))
    px = bg.load()
    for y in range(h):
        t = y / max(h - 1, 1)
        c = tuple(int(a + (b - a) * t) for a, b in zip(BG_TOP, BG_BOT))
        for x in range(w):
            px[x, y] = c
    return bg


def main():
    src, dst = sys.argv[1], sys.argv[2]
    os.makedirs(dst, exist_ok=True)
    screens = []
    for shot, name in SHOTS:
        im = Image.open(os.path.join(src, shot + ".png")).convert("RGB")
        im = im.resize((im.width * SCALE, im.height * SCALE), Image.LANCZOS)
        im.save(os.path.join(dst, name + ".png"), optimize=True)
        screens.append(rounded(im, RADIUS))

    row = screens[:3]
    sw, sh = row[0].size
    w = MARGIN * 2 + sw * len(row) + GAP * (len(row) - 1)
    h = MARGIN * 2 + sh
    canvas = gradient(w, h).convert("RGBA")

    shadow = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(shadow)
    for i in range(len(row)):
        x = MARGIN + i * (sw + GAP)
        d.rounded_rectangle((x, MARGIN + 18, x + sw, MARGIN + 18 + sh), RADIUS, fill=(0, 0, 0, 170))
    canvas = Image.alpha_composite(canvas, shadow.filter(ImageFilter.GaussianBlur(24)))

    for i, s in enumerate(row):
        canvas.alpha_composite(s, (MARGIN + i * (sw + GAP), MARGIN))
    canvas.convert("RGB").save(os.path.join(dst, "telas.png"), optimize=True)


if __name__ == "__main__":
    main()
