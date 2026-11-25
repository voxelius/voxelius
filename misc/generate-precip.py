"""
This is vibecoded, here be dragons. 

Generate the rain and snow textures of the precipitation renderer.

Both are 64x256 RGBA masks for client/world/precip_renderer.cc. One tile
spans one block across U and four blocks down V
(V_TILE_SCALE = 0.25f), and the renderer repeats it along the column. A
streak or a flake that crosses the bottom edge is drawn wrapped, so the
repeat has no seam. The side edges stay empty because U does not repeat.

The alpha channel is binary and the gray level carries the brightness of a
pixel. The hue and the amount belong to the type definition: color and
opacity in data/builtin/scripts/precip/rain.lua, and the biome tint that
multiplies the color. Commit f5db45cf made rain.png a neutral mask for
that reason.

The rain streak lengths and the snow flake shapes repeat the statistics of
the textures that the mod shipped before this generator. The seed moves the
layout only: it writes byte-identical files on every machine.

    python misc/generate-precip.py --seed 7
    python misc/generate-precip.py --seed 7 --type snow -o /tmp/precip
    python misc/generate-precip.py --seed 7 --preview /tmp/precip.png
"""

import argparse
import random
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

WIDTH = 64
HEIGHT = 256

# One seed drives both textures: the mix keeps the two streams independent
RAIN_MIX = 0x9E3779B9
SNOW_MIX = 0x85EBCA6B

DEFAULT_SEED = 7

# Counts measured on the textures the mod shipped before, so a seed starts
# close to them
DEFAULT_RAIN_DROPS = 100
DEFAULT_SNOW_FLAKES = 44

# The mask is neutral, so the brightness is what the old alpha carried
DEFAULT_RAIN_BRIGHTNESS = 0.765
RAIN_BRIGHTNESS_SPREAD = 0.02
RAIN_DIM_CHANCE = 0.08
RAIN_DIM_DROP = 0.05
RAIN_BRIGHTNESS_RANGE = (0.60, 0.85)

# Streak lengths and their weights, measured on the shipped rain.png: a one
# pixel drop stands for a distant streak, and the long ones are rare
RAIN_LENGTHS = (
    (1, 36),
    (2, 9),
    (3, 10),
    (4, 8),
    (5, 12),
    (6, 2),
    (7, 3),
    (8, 3),
    (10, 4),
    (11, 4),
    (15, 1),
    (16, 3),
)
RAIN_LENGTH_WEIGHTS = tuple(weight for _, weight in RAIN_LENGTHS)
RAIN_WIDE_CHANCE = 0.08

SNOW_WHITE = 255

# The four flake shapes of the shipped snow.png
SNOW_SHAPES = (
    ("dot", ((0, 0),), 0.45),
    ("hollow", ((0, -1), (-1, 0), (1, 0), (0, 1)), 0.25),
    ("plus", ((0, -1), (-1, 0), (0, 0), (1, 0), (0, 1)), 0.16),
    ("cross", ((-1, -1), (1, -1), (0, 0), (-1, 1), (1, 1)), 0.14),
)
SNOW_SHAPE_WEIGHTS = tuple(weight for _, _, weight in SNOW_SHAPES)

SKY_TOP = (18, 26, 42)
SKY_BOTTOM = (44, 58, 84)

PANEL_GAP = 12
PANEL_MARGIN = 8
PANEL_HEADER = 20


def blank_texture():
    return Image.new("RGBA", (WIDTH, HEIGHT), (0, 0, 0, 0))


def rain_length(rng):
    length, _ = rng.choices(RAIN_LENGTHS, weights=RAIN_LENGTH_WEIGHTS)[0]

    return length


def rain_level(rng, brightness):
    value = brightness + rng.gauss(0.0, RAIN_BRIGHTNESS_SPREAD)

    if rng.random() < RAIN_DIM_CHANCE:
        value -= RAIN_DIM_DROP

    low, high = RAIN_BRIGHTNESS_RANGE

    return round(min(high, max(low, value)) * 255)


def draw_rain(rng, drops, brightness):
    image = blank_texture()
    pixels = image.load()

    for _ in range(drops):
        length = rain_length(rng)
        width = 2 if rng.random() < RAIN_WIDE_CHANCE else 1
        x = rng.randrange(WIDTH - width + 1)
        y = rng.randrange(HEIGHT)
        level = rain_level(rng, brightness)

        for dy in range(length):
            for dx in range(width):
                # The tile repeats four blocks down, so a streak wraps over the bottom edge
                pixels[x + dx, (y + dy) % HEIGHT] = (level, level, level, 255)

    return image


def pick_shape(rng):
    _, shape, _ = rng.choices(SNOW_SHAPES, weights=SNOW_SHAPE_WEIGHTS)[0]

    return shape


def draw_snow(rng, flakes):
    image = blank_texture()
    pixels = image.load()

    blocked = set()
    placed = 0
    attempts = 0
    max_attempts = flakes * 32

    while placed < flakes and attempts < max_attempts:
        attempts += 1
        shape = pick_shape(rng)
        half = max(abs(dx) for dx, _ in shape)
        x = rng.randint(half, WIDTH - 1 - half)
        y = rng.randrange(HEIGHT)

        cells = [((x + dx) % WIDTH, (y + dy) % HEIGHT) for dx, dy in shape]

        if blocked.intersection(cells):
            continue

        for cell_x, cell_y in cells:
            pixels[cell_x, cell_y] = (SNOW_WHITE, SNOW_WHITE, SNOW_WHITE, 255)

            # A one pixel gap keeps two nearby flakes from merging into one blob
            for offset_x in (-1, 0, 1):
                for offset_y in (-1, 0, 1):
                    blocked.add(((cell_x + offset_x) % WIDTH, (cell_y + offset_y) % HEIGHT))

        placed += 1

    if placed < flakes:
        print("warning: placed {} of {} flakes".format(placed, flakes))

    return image


def sky_panel():
    panel = Image.new("RGBA", (WIDTH, HEIGHT))
    pixels = panel.load()

    for y in range(HEIGHT):
        t = y / (HEIGHT - 1)
        color = tuple(round(a + (b - a) * t) for a, b in zip(SKY_TOP, SKY_BOTTOM))

        for x in range(WIDTH):
            pixels[x, y] = color + (255,)

    return panel


def preview_font():
    try:
        return ImageFont.load_default(size=12)
    except TypeError:
        return ImageFont.load_default()


def make_preview(images, seed, path, scale):
    panel_width = WIDTH * scale
    panel_height = HEIGHT * scale
    width = PANEL_MARGIN * 2 + panel_width * len(images) + PANEL_GAP * (len(images) - 1)
    height = PANEL_MARGIN * 2 + PANEL_HEADER + panel_height

    frame = Image.new("RGB", (width, height), (10, 14, 22))
    draw = ImageDraw.Draw(frame)
    font = preview_font()

    for index, (name, image) in enumerate(images.items()):
        left = PANEL_MARGIN + index * (panel_width + PANEL_GAP)
        panel = sky_panel()
        panel.alpha_composite(image)
        panel = panel.convert("RGB").resize((panel_width, panel_height), Image.NEAREST)
        frame.paste(panel, (left, PANEL_MARGIN + PANEL_HEADER))
        draw.text((left + 2, PANEL_MARGIN + 3), "{} seed {}".format(name, seed), fill=(200, 212, 232), font=font)

    frame.save(path)


def lit_pixels(image):
    counts = image.getchannel("A").histogram()

    return sum(counts[1:])


def main() -> None:
    repo = Path(__file__).resolve().parents[1]
    default_out = repo / "data" / "builtin" / "textures" / "precip"

    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("-o", "--out-dir", type=Path, default=default_out)
    parser.add_argument("--seed", type=int, default=DEFAULT_SEED)
    parser.add_argument(
        "--type",
        choices=("both", "rain", "snow"),
        default="both",
        help="which texture to write (default: both)",
    )
    parser.add_argument(
        "--rain-drops",
        type=int,
        default=DEFAULT_RAIN_DROPS,
        help="rain streaks, one to sixteen pixels each (default: 100)",
    )
    parser.add_argument(
        "--snow-flakes",
        type=int,
        default=DEFAULT_SNOW_FLAKES,
        help="snow flakes, one pixel or a three by three shape each (default: 44)",
    )
    parser.add_argument(
        "--rain-brightness",
        type=float,
        default=DEFAULT_RAIN_BRIGHTNESS,
        help="base brightness of a rain pixel, 0 to 1 (default: 0.765)",
    )
    parser.add_argument(
        "--preview",
        type=Path,
        help="also write a zoomed panel of the textures to this path",
    )
    parser.add_argument("--preview-scale", type=int, default=2, help="preview zoom (default: 2)")

    args = parser.parse_args()

    if args.preview_scale < 1:
        parser.error("--preview-scale must be at least 1")

    images = {}

    if args.type in ("both", "rain"):
        images["rain"] = draw_rain(random.Random(args.seed ^ RAIN_MIX), args.rain_drops, args.rain_brightness)

    if args.type in ("both", "snow"):
        images["snow"] = draw_snow(random.Random(args.seed ^ SNOW_MIX), args.snow_flakes)

    out_dir = args.out_dir.resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    for name, image in images.items():
        path = out_dir / "{}.png".format(name)
        image.save(path)
        print("wrote {}: {} lit pixels".format(path, lit_pixels(image)))

    if args.preview:
        make_preview(images, args.seed, args.preview, args.preview_scale)
        print("wrote", args.preview)


if __name__ == "__main__":
    main()
