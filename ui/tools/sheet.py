#!/usr/bin/env python3
"""Converte as capturas .bmp do `make test` em PNG e monta folha.png com todas."""
import glob
import os
import sys

from PIL import Image

d = sys.argv[1]
shots = sorted(glob.glob(os.path.join(d, "*.bmp")))
ims = []
for f in shots:
    im = Image.open(f)
    im.save(f[:-4] + ".png")
    ims.append(im)
if ims:
    w, h = ims[0].size
    cols, gap = 2, 10
    rows = (len(ims) + cols - 1) // cols
    sheet = Image.new("RGB", (w * cols + gap * (cols - 1), h * rows + gap * (rows - 1)), (40, 40, 40))
    for i, im in enumerate(ims):
        sheet.paste(im, ((i % cols) * (w + gap), (i // cols) * (h + gap)))
    sheet.save(os.path.join(d, "folha.png"))
