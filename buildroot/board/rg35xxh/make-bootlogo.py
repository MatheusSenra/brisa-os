#!/usr/bin/env python3
"""Gera o bootlogo.bmp do Brisa OS que o bootloader mostra ao ligar.

uso: make-bootlogo.py <saída.bmp>
Mesmo formato do logo da Anbernic: BMP 640x480, 24 bits, sem compressão.
Mesmas cores e marca da interface e do banner do README.
"""
import math
import sys

from PIL import Image, ImageDraw, ImageFilter, ImageFont

W, H = 640, 480
BG_TOP = (21, 26, 38)
BG_BOT = (11, 14, 21)
ACCENT = (79, 209, 197)
TEXT = (233, 237, 243)
DIM = (141, 151, 171)
FONTS = ["/usr/share/fonts/truetype/dejavu/", "/usr/share/fonts/dejavu/"]


def font(name, size):
    for d in FONTS:
        try:
            return ImageFont.truetype(d + name, size)
        except OSError:
            pass
    raise SystemExit("fonte DejaVu não encontrada (rode 'make deps')")


def wave(draw, y0, amp, phase, width, alpha):
    pts = []
    for x in range(-20, W + 21, 4):
        t = x / W
        y = y0 + amp * math.sin(2 * math.pi * (t * 1.3 + phase))
        pts.append((x, y))
    # mais forte no meio, sumindo nas bordas, como no banner
    for i in range(len(pts) - 1):
        t = abs(pts[i][0] / W - 0.5) * 2
        a = int(alpha * max(0.0, 1 - t ** 1.6))
        draw.line([pts[i], pts[i + 1]], fill=ACCENT + (a,), width=width)


def main():
    img = Image.new("RGB", (W, H))
    px = img.load()
    for y in range(H):
        t = y / (H - 1)
        c = tuple(int(a + (b - a) * t) for a, b in zip(BG_TOP, BG_BOT))
        for x in range(W):
            px[x, y] = c

    layer = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(layer)
    wave(d, 318, 14, 0.10, 3, 150)
    wave(d, 342, 12, 0.35, 2, 90)
    wave(d, 296, 10, 0.62, 2, 60)
    img = Image.alpha_composite(img.convert("RGBA"), layer.filter(ImageFilter.GaussianBlur(0.6)))

    d = ImageDraw.Draw(img)
    title = font("DejaVuSans-Bold.ttf", 62)
    sub = font("DejaVuSans.ttf", 19)
    text = "Brisa OS"
    tw = d.textlength(text, font=title)
    dot = 22
    gap = 18
    x0 = (W - (dot + gap + tw)) / 2
    ty = 172
    asc, _ = title.getmetrics()
    cap = asc * 0.72
    cy = ty + asc - cap / 2
    # ponto desenhado em 4x e reduzido, para a borda sair suave
    ss = 4
    disc = Image.new("L", (dot * ss, dot * ss), 0)
    ImageDraw.Draw(disc).ellipse((0, 0, dot * ss - 1, dot * ss - 1), fill=255)
    disc = disc.resize((dot, dot), Image.LANCZOS)
    img.paste(Image.new("RGBA", (dot, dot), ACCENT + (255,)), (int(x0), int(cy - dot / 2)), disc)
    d.text((x0 + dot + gap, ty), text, font=title, fill=TEXT)

    s = "Firmware leve para o RG35XX H"
    d.text(((W - d.textlength(s, font=sub)) / 2, ty + 92), s, font=sub, fill=DIM)

    img.convert("RGB").save(sys.argv[1], format="BMP")


if __name__ == "__main__":
    main()
