"""
This is vibecoded, here be dragons. 

Generate the temporary sun and moon sprites.

Both are square, like the ones the game is inspired by. The sun is one
32x32 image: a bright square core inside an opaque black square, with a
radial glow that additive blending turns into a halo. The moon is a 4x2
atlas of eight 32x32 phase cells, each a square core with the unlit side
painted black, wrapped in a soft transparent glow.
"""

import math
import os
import random

from PIL import Image

SUN_SIZE = 32
MOON_CELL = 32
MOON_COLUMNS = 4
MOON_ROWS = 2

SUN_CORE = 0.44
MOON_CORE = 0.72

MOON_SHADOW = (0.010, 0.010, 0.018)
MOON_RIM = (0.06, 0.07, 0.10)

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
                # by alpha, so the hue stays here and alpha carries it
                intensity = (1.0 - smoothstep(0.30, 1.10, radius)) * 0.18
                color = (1.0, 1.0, 0.42)
                alpha = intensity

            pixels[x, y] = (
                int(255 * color[0]),
                int(255 * color[1]),
                int(255 * color[2]),
                int(255 * alpha),
            )

    image.save(os.path.join(OUTPUT, "sun.png"))


def is_lit(phase, u):
    if phase == 0:
        return True

    if phase == 4:
        return False

    if phase < 4:
        # Waning: the lit part retreats from the right
        return u < 1.0 - 0.25 * phase

    # Waxing: the lit part grows from the right
    return u > 1.0 - 0.25 * (phase - 4)


def lit_fraction(phase):
    if phase == 0:
        return 1.0

    if phase == 4:
        return 0.0

    if phase < 4:
        return 1.0 - 0.25 * phase

    return 0.25 * (phase - 4)


def make_moon_phases():
    width = MOON_CELL * MOON_COLUMNS
    height = MOON_CELL * MOON_ROWS

    image = Image.new("RGBA", (width, height))
    pixels = image.load()

    random.seed(7)
    craters = []

    for _ in range(16):
        craters.append((random.random(), random.random(), 0.04 + random.random() * 0.07))

    for phase in range(MOON_ROWS * MOON_COLUMNS):
        origin_x = (phase % MOON_COLUMNS) * MOON_CELL
        origin_y = (phase // MOON_COLUMNS) * MOON_CELL
        phase_light = lit_fraction(phase)

        for y in range(MOON_CELL):
            for x in range(MOON_CELL):
                u = (x + 0.5) / MOON_CELL
                v = (y + 0.5) / MOON_CELL
                nx = u * 2.0 - 1.0
                ny = v * 2.0 - 1.0
                square = max(abs(nx), abs(ny))

                core_offset = 0.5 - MOON_CORE * 0.5
                core_u = (u - core_offset) / MOON_CORE

                if square <= MOON_CORE:
                    # The body is a full square; the dark side is
                    # shadow, not a hole, so it covers the sky
                    if not is_lit(phase, core_u):
                        # A rim around the shadow keeps the moon's shape
                        # readable against a dark sky; the terminator gets
                        # the same treatment so the phase is visible
                        core_min = round(core_offset * MOON_CELL)
                        core_max = round((core_offset + MOON_CORE) * MOON_CELL) - 1
                        on_edge = x == core_min or x == core_max or y == core_min or y == core_max
                        on_terminator = False

                        if phase != 0 and phase != 4:
                            boundary = 1.0 - 0.25 * phase if phase < 4 else 1.0 - 0.25 * (phase - 4)
                            boundary_u = core_offset + boundary * MOON_CORE
                            on_terminator = abs(u - boundary_u) <= 1.2 / MOON_CELL

                        color = MOON_RIM if on_edge or on_terminator else MOON_SHADOW

                        pixels[origin_x + x, origin_y + y] = (
                            int(255 * color[0]),
                            int(255 * color[1]),
                            int(255 * color[2]),
                            255,
                        )
                        continue

                    shade = 0.90 + 0.10 * math.sin(u * math.pi) * math.sin(v * math.pi)

                    for cx, cy, radius in craters:
                        distance = math.hypot(u - cx, v - cy)

                        if distance < radius:
                            edge = 1.0 - smoothstep(radius * 0.5, radius, distance)
                            shade *= 1.0 - 0.22 * edge

                    color = (0.88 * shade, 0.91 * shade, 0.96 * shade)

                    pixels[origin_x + x, origin_y + y] = (
                        int(255 * color[0]),
                        int(255 * color[1]),
                        int(255 * color[2]),
                        255,
                    )
                    continue

                # The glow around the core is transparent, so the moon
                # only lights the sky instead of blacking out a bigger square.
                # The nearest core column decides whether this side is lit,
                # which keeps the reflection off the shadowed side
                falloff = min(1.0, max(0.0, (square - MOON_CORE) / (1.0 - MOON_CORE)))
                nearest_lit = is_lit(phase, min(1.0, max(0.0, core_u)))
                side_light = 1.0 if nearest_lit else 0.20
                halo = (1.0 - falloff) ** 2 * 0.20 * phase_light * side_light

                pixels[origin_x + x, origin_y + y] = (
                    int(255 * 0.62),
                    int(255 * 0.68),
                    int(255 * 0.82),
                    int(255 * halo),
                )

    image.save(os.path.join(OUTPUT, "moon_phases.png"))


def main():
    os.makedirs(OUTPUT, exist_ok=True)
    make_sun()
    make_moon_phases()
    print("wrote sun.png and moon_phases.png into", os.path.normpath(OUTPUT))


if __name__ == "__main__":
    main()
