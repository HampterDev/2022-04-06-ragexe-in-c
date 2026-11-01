"""Regenerate the owned JPEG fixture using Pillow (not the production stb codec)."""

from pathlib import Path
from PIL import Image


image = Image.new("RGB", (17, 19))
colors = ((240, 10, 20), (10, 220, 30), (15, 25, 230), (245, 240, 230))
for y in range(19):
    for x in range(17):
        image.putpixel((x, y), colors[(y >= 9) * 2 + (x >= 8)])
image.save(Path(__file__).with_name("jpeg_quadrants.jpg"), quality=100, subsampling=0)
