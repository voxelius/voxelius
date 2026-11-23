"""
This is vibecoded, here be dragons. 

Generate the sun sprite.

The sun is a 32x32 image: a small bright square core with a radial
glow that additive blending turns into a halo.
"""

import math
import os

from PIL import Image

SUN_SIZE = 32
SUN_CORE = 0.25

OUTPUT = os.path.join(os.path.dirname(__file__), "..", "data", "builtin", "textures", "environment")


def smoothstep(edge0, edge1, value):
    if edge0 >= edge1:
        return 0.0 if value < edge0 else 1.0

    t = min(1.0, max(0.0, (value - edge0) / (edge1 - edge0)))
    return t * t * (3.0 - 2.0 * t)


def mix(a, b, t):
    return tuple(a[i] + (b[i] - a[i]) * t for i in range(len(a)))


def make_sun():
    image = Image.new("RGBA", (SUN_SIZE, SUN_SIZE))
    pixels = image.load()

    for y in range(SUN_SIZE):
        for x in range(SUN_SIZE):
            nx = (x + 0.5) / SUN_SIZE * 2.0 - 1.0
            ny = (y + 0.5) / SUN_SIZE * 2.0 - 1.0
            square = max(abs(nx), abs(ny))
            radius = math.hypot(nx, ny)

            if square <= SUN_CORE:
                core = min(1.0, radius / 0.70)
                color = mix((1.0, 1.0, 0.88), (1.0, 0.835, 0.29), smoothstep(0.0, 0.75, core))
                alpha = 1.0
            else:
                # The glow is nearly circular. The shader premultiplies
                # by alpha, so the hue stays here and alpha carries it.
                # It fades out before the cell border, or the quad clips it
                intensity = (1.0 - smoothstep(0.22, 0.96, radius)) * 0.15
                color = (1.0, 1.0, 0.42)
                alpha = intensity

            pixels[x, y] = (
                int(255 * color[0]),
                int(255 * color[1]),
                int(255 * color[2]),
                int(255 * alpha),
            )

    image.save(os.path.join(OUTPUT, "sun.png"))


def main():
    os.makedirs(OUTPUT, exist_ok=True)
    make_sun()
    print("wrote sun.png into", os.path.normpath(OUTPUT))


if __name__ == "__main__":
    main()
