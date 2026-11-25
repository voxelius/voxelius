"""
This is vibecoded, here be dragons. 

Generate the moon phase atlas.

The atlas is 128x64 with eight 32x32 phase cells, four columns by two
rows, in the order sky_renderer.cc expects: the full moon first, the
waning phases down the top row, the new moon first in the bottom row,
then the waxing phases.

The moon body is a small square in the middle of the cell, close to the
vanilla Minecraft moon. The terminator is a soft, curved penumbra, and
the shadow fades into the transparent halo, so the moon melts into the
night sky.
"""

import math
import os
import random

from PIL import Image

CELL = 32
COLUMNS = 4
ROWS = 2
MOON_BODY = 10

# Lit fraction of the disc for each phase. The top row wanes with the
# lit part on the left, the bottom row waxes with the lit part on the
# right, and index 4 is the new moon.
PHASE_FRACTIONS = (1.0, 0.75, 0.5, 0.375, 0.0, 0.375, 0.5, 0.75)

# The lit shades, from the brightest to the darkest, taken from the
# vanilla moon_phases.png palette
LIT_TONES = (
    (0xD9, 0xE4, 0xFF),
    (0xAF, 0xB8, 0xCC),
    (0x90, 0x97, 0xA5),
    (0x74, 0x7D, 0x93),
    (0x5F, 0x67, 0x7A),
    (0x50, 0x56, 0x66),
)

SHADOW_RIM = (0x2B, 0x2F, 0x38)
HALO_COLOR = (0x8C, 0x9C, 0xC8)

TERMINATOR_CURVE = 0.30
PENUMBRA_FRACTION = 0.32
HALO_STRENGTH = 0.12

OUTPUT = os.path.join(os.path.dirname(__file__), "..", "data", "builtin", "textures", "environment")


def clamp(value, low=0.0, high=1.0):
    return max(low, min(high, value))


def smoothstep(edge0, edge1, value):
    if edge0 == edge1:
        return 0.0 if value < edge0 else 1.0

    t = clamp((value - edge0) / (edge1 - edge0))
    return t * t * (3.0 - 2.0 * t)


def mix(a, b, t):
    return tuple(int(round(a[i] + (b[i] - a[i]) * t)) for i in range(3))


def make_surface(size, seed):
    """Build a lit-surface brightness map with a bright core and craters."""

    rng = random.Random(seed)

    highlights = [
        (rng.uniform(-0.6, 0.6), rng.uniform(-0.6, 0.6), rng.uniform(0.12, 0.28))
        for _ in range(2)
    ]
    craters = [
        (rng.uniform(-0.75, 0.75), rng.uniform(-0.75, 0.75), rng.uniform(0.14, 0.34), rng.uniform(0.15, 0.40))
        for _ in range(size)
    ]

    surface = []

    for y in range(size):
        row = []

        for x in range(size):
            u = (x + 0.5) / size * 2.0 - 1.0
            v = (y + 0.5) / size * 2.0 - 1.0
            radius = math.hypot(u, v)

            value = 0.95 - 0.55 * smoothstep(0.20, 1.05, radius) - 0.03 * v

            for cx, cy, crater_radius, depth in craters:
                distance = math.hypot(u - cx, v - cy)
                if distance < crater_radius:
                    value -= depth * (1.0 - smoothstep(crater_radius * 0.4, crater_radius, distance))

            for cx, cy, highlight_radius in highlights:
                distance = math.hypot(u - cx, v - cy)
                value += 0.12 * math.exp(-((distance / highlight_radius) ** 2) * 2.0)

            row.append(clamp(value))

        surface.append(row)

    return surface


def tone_index(value):
    if value >= 0.80:
        return 0
    if value >= 0.64:
        return 1
    if value >= 0.48:
        return 2
    if value >= 0.33:
        return 3
    if value >= 0.18:
        return 4
    return 5


def draw_halo(pixels, origin_x, origin_y):
    """Draw a soft glow that stays fully transparent at the cell border."""

    for y in range(CELL):
        for x in range(CELL):
            nx = (x + 0.5) / CELL * 2.0 - 1.0
            ny = (y + 0.5) / CELL * 2.0 - 1.0
            radius = math.hypot(nx, ny)
            border = min(x, y, CELL - 1 - x, CELL - 1 - y)

            alpha = (1.0 - smoothstep(0.20, 1.02, radius)) ** 1.5
            alpha *= HALO_STRENGTH * min(1.0, border / 2.0)

            if alpha > 0.002:
                pixels[origin_x + x, origin_y + y] = (*HALO_COLOR, int(255.0 * alpha))


def draw_body(pixels, origin_x, origin_y, phase, surface):
    offset = (CELL - MOON_BODY) // 2
    center = (MOON_BODY - 1) / 2.0

    fraction = PHASE_FRACTIONS[phase]
    lit_width = fraction * MOON_BODY
    # The terminator bulges into the shadow at the equator for a gibbous
    # moon and into the lit side for a crescent, which rounds the shadow
    bulge = -TERMINATOR_CURVE * MOON_BODY * (fraction - 0.5)
    band = max(1.0, MOON_BODY * PENUMBRA_FRACTION)

    for y in range(MOON_BODY):
        ny = (y - center) / center

        for x in range(MOON_BODY):
            source = tone_index(surface[y][x])
            edge = min(x, y, MOON_BODY - 1 - x, MOON_BODY - 1 - y)

            if phase == 4:
                # The new moon is only a faint rim, like in the reference
                if edge == 0:
                    pixels[origin_x + offset + x, origin_y + offset + y] = (*SHADOW_RIM, 255)
                continue

            if phase == 0:
                index = 5 if edge == 0 else max(source, 4) if edge == 1 else source
                pixels[origin_x + offset + x, origin_y + offset + y] = (*LIT_TONES[index], 255)
                continue

            if phase < 4:
                boundary = lit_width + bulge * (1.0 - ny * ny)
                distance = (boundary - 0.5) - x
            else:
                boundary = (MOON_BODY - lit_width) - bulge * (1.0 - ny * ny)
                distance = x - (boundary - 0.5)

            if distance >= band:
                index = 5 if edge == 0 else max(source, 4) if edge == 1 else source
                pixels[origin_x + offset + x, origin_y + offset + y] = (*LIT_TONES[index], 255)
                continue

            t = smoothstep(-band, band, distance)
            index = int(round(source + (len(LIT_TONES) - 1 - source) * (1.0 - t)))
            color = mix(SHADOW_RIM, LIT_TONES[index], t)

            if edge == 0:
                color = LIT_TONES[5]
            elif edge == 1:
                color = LIT_TONES[max(index, 4)]

            alpha = int(round(255.0 * t))

            if alpha > 0:
                pixels[origin_x + offset + x, origin_y + offset + y] = (*color, alpha)


def make_moon_phases():
    halo_layer = Image.new("RGBA", (CELL * COLUMNS, CELL * ROWS), (0, 0, 0, 0))
    body_layer = Image.new("RGBA", (CELL * COLUMNS, CELL * ROWS), (0, 0, 0, 0))
    surface = make_surface(MOON_BODY, 7)

    for phase in range(COLUMNS * ROWS):
        origin_x = (phase % COLUMNS) * CELL
        origin_y = (phase // COLUMNS) * CELL

        draw_halo(halo_layer.load(), origin_x, origin_y)
        draw_body(body_layer.load(), origin_x, origin_y, phase, surface)

    halo_layer.alpha_composite(body_layer)
    halo_layer.save(os.path.join(OUTPUT, "moon.png"))


def main():
    os.makedirs(OUTPUT, exist_ok=True)
    make_moon_phases()
    print("wrote moon.png into", os.path.normpath(OUTPUT))


if __name__ == "__main__":
    main()
