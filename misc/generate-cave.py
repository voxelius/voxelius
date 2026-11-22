#!/usr/bin/env python3

"""
This is vibecoded, here be dragons. 

Generate the wall textures of the parallax cave background.

The client draws the background as four walls, back to front, that scroll over
the sky gradient at different speeds. Each wall is one texture, one window tall
at the 1:1 texel scale, and it tiles left to right without a seam.

A wall is a grid of 16-texel blocks: a ragged ceiling, a ragged floor, spikes
under the ceiling and over the floor, holes that open it to the sky, loose
chunks in the opening and a few columns that split it. Every block takes one of
the stone textures of the game, so the wall is made of the same stone units as
the interface buttons. The light comes from the openings: the rock near a gap
is lit by the sky behind it, the mass falls off into shadow, and the far walls
mix in more of the sky colour.

Only the front wall is baked with a small blur, because the camera looks into
the cave through it and it is out of focus.

Needs numpy and Pillow:

  python misc/generate-cave.py
  python misc/generate-cave.py --preview build/cave_preview.png
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter

ROOT = Path(__file__).resolve().parent.parent
STONE_ROOT = ROOT / "data" / "builtin" / "textures" / "block" / "stone"
DEFAULT_OUTPUT = ROOT / "data" / "builtin" / "textures" / "gui"

TILE_WIDTH = 1536
TILE_HEIGHT = 544

# One block of the cave, and one stone texture of the game
BLOCK = 16
COLUMNS = TILE_WIDTH // BLOCK
ROWS = TILE_HEIGHT // BLOCK

# Wavelength of the ceiling and floor profiles, in blocks
CEILING_WAVELENGTH = 14
FLOOR_WAVELENGTH = 20

SKY_TOP = np.array([0.204, 0.373, 0.588], np.float32)
SKY_BOTTOM = np.array([0.749, 0.867, 0.925], np.float32)


@dataclass(frozen=True)
class Wall:
    name: str
    ceiling: int
    floor: int
    ragged: float
    spikes: int
    holes: int
    rubble: int
    pillars: int
    pillar_width: tuple[int, int]
    pillar_span: tuple[float, float]
    blur: float
    haze: float
    tint: float
    seed: int


# Back to front: the far wall closes most of the view, the front one is a thin
# dark frame that the camera looks through
WALLS = (
    Wall(
        name="background_back.png",
        ceiling=10,
        floor=12,
        ragged=2.6,
        spikes=34,
        holes=14,
        rubble=14,
        pillars=3,
        pillar_width=(1, 4),
        pillar_span=(0.05, 0.95),
        blur=0.0,
        haze=0.30,
        tint=1.00,
        seed=1101,
    ),
    Wall(
        name="background_mid.png",
        ceiling=8,
        floor=10,
        ragged=2.2,
        spikes=26,
        holes=9,
        rubble=16,
        pillars=2,
        pillar_width=(2, 5),
        pillar_span=(0.10, 1.00),
        blur=0.0,
        haze=0.14,
        tint=0.95,
        seed=2203,
    ),
    Wall(
        name="background_near.png",
        ceiling=6,
        floor=7,
        ragged=1.8,
        spikes=18,
        holes=5,
        rubble=10,
        pillars=2,
        pillar_width=(4, 7),
        pillar_span=(0.00, 1.00),
        blur=0.0,
        haze=0.05,
        tint=1.02,
        seed=3307,
    ),
    Wall(
        name="background_front.png",
        ceiling=3,
        floor=4,
        ragged=1.4,
        spikes=10,
        holes=2,
        rubble=3,
        pillars=1,
        pillar_width=(5, 9),
        pillar_span=(0.00, 1.00),
        blur=1.3,
        haze=0.00,
        tint=0.92,
        seed=4409,
    ),
)


def sky_rows(rows: int) -> np.ndarray:
    """The sky gradient colour of each texture row."""

    k = np.linspace(0.0, 1.0, rows, dtype=np.float32)[:, None]
    return SKY_TOP[None, :] + (SKY_BOTTOM - SKY_TOP)[None, :] * k


def stone_variants() -> list[np.ndarray]:
    return [np.asarray(Image.open(path).convert("RGB"), np.float32) / 255.0 for path in sorted(STONE_ROOT.glob("stone_*.png"))]


def noise_1d(samples: int, frequency: int, rng: np.random.Generator) -> np.ndarray:
    """Value noise on a ring of `frequency` samples, so it wraps."""

    lattice = rng.random(frequency, dtype=np.float32)

    x = np.arange(samples, dtype=np.float32) * frequency / samples
    x0 = np.floor(x).astype(np.int64)
    t = x - x0
    t = t * t * (3.0 - 2.0 * t)

    left = lattice[x0 % frequency]
    right = lattice[(x0 + 1) % frequency]
    return left + (right - left) * t


def fbm_1d(samples: int, frequency: int, rng: np.random.Generator, octaves: int = 3, gain: float = 0.5) -> np.ndarray:
    total = np.zeros(samples, np.float32)
    amplitude = 1.0
    norm = 0.0
    scale = 1

    for _ in range(octaves):
        total += amplitude * noise_1d(samples, frequency * scale, rng)
        norm += amplitude
        amplitude *= gain
        scale *= 2

    return total / norm


def neighbour_count(mask: np.ndarray, radius: int) -> np.ndarray:
    total = np.zeros(mask.shape, np.int32)
    for dy in range(-radius, radius + 1):
        for dx in range(-radius, radius + 1):
            if dy == 0 and dx == 0:
                continue
            total += np.roll(np.roll(mask, dy, 0), dx, 1)
    return total


def tidy(mask: np.ndarray) -> np.ndarray:
    """Even out the spikes so the wall reads in blocks instead of noise."""

    rocks = neighbour_count(mask, 1)
    return (mask | ((~mask) & (rocks >= 7))) & (~(mask & (rocks <= 1)))


def drop_small(mask: np.ndarray, min_blocks: int) -> np.ndarray:
    """Remove every rock island with fewer than min_blocks blocks."""

    rows, cols = mask.shape
    seen = np.zeros(mask.shape, bool)
    result = mask.copy()

    for start_y, start_x in zip(*np.nonzero(mask)):
        if seen[start_y, start_x]:
            continue

        stack = [(int(start_y), int(start_x))]
        seen[start_y, start_x] = True
        island = []

        while stack:
            y, x = stack.pop()
            island.append((y, x))

            for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                ny = (y + dy) % rows
                nx = (x + dx) % cols
                if mask[ny, nx] and not seen[ny, nx]:
                    seen[ny, nx] = True
                    stack.append((ny, nx))

        if len(island) < min_blocks:
            for y, x in island:
                result[y, x] = False

    return result


def blob(shape: tuple[int, int], width: int, height: int, rng: np.random.Generator) -> np.ndarray:
    """A ragged block: width by height, with the corners bitten off."""

    mask = np.zeros(shape, bool)

    half_w = 0.5 * (width - 1)
    half_h = 0.5 * (height - 1)

    for dy in range(height):
        for dx in range(width):
            if ((dx - half_w) / (half_w + 1.0)) ** 2 + ((dy - half_h) / (half_h + 1.0)) ** 2 > 1.0:
                continue
            if rng.random() < 0.18:
                continue
            mask[dy % shape[0], dx % shape[1]] = True

    return mask


def stamp(mask: np.ndarray, piece: np.ndarray, column: int, row: int) -> None:
    """Paste a piece with its top left corner at column, row."""

    height, width = mask.shape

    for y in range(piece.shape[0]):
        for x in range(piece.shape[1]):
            if piece[y, x]:
                mask[(row + y) % height, (column + x) % width] = True


def hang(mask: np.ndarray, column: int, length: int, half_width: int, limit: int, rng: np.random.Generator) -> None:
    """Hang a tapering spike of rock under the ceiling, no deeper than limit."""

    for dx in range(-half_width, half_width + 1):
        reach = int(round(length * (1.0 - abs(dx) / (half_width + 1.0)) + rng.uniform(-0.5, 0.5)))

        if reach <= 0:
            continue

        x = (column + dx) % mask.shape[1]
        rows = np.nonzero(mask[:limit, x])[0]
        start = int(rows.max()) + 1 if rows.size else limit
        mask[start : min(limit, start + reach), x] = True


def rise(mask: np.ndarray, column: int, length: int, half_width: int, limit: int, rng: np.random.Generator) -> None:
    """Raise a tapering spike of rock over the floor, no higher than limit."""

    height = mask.shape[0]
    bottom = height - limit

    for dx in range(-half_width, half_width + 1):
        reach = int(round(length * (1.0 - abs(dx) / (half_width + 1.0)) + rng.uniform(-0.5, 0.5)))

        if reach <= 0:
            continue

        x = (column + dx) % mask.shape[1]
        rows = np.nonzero(mask[bottom:, x])[0]
        start = int(rows.min()) + bottom if rows.size else bottom
        mask[max(bottom, start - reach + 1) : start + 1, x] = True


def pillar(mask: np.ndarray, column: int, width: int, row_from: int, row_to: int, rng: np.random.Generator) -> None:
    """A ragged column of rock between two rows; it splits the opening."""

    for dx in range(width):
        x = (column + dx) % mask.shape[1]
        start = max(0, row_from + int(rng.integers(-1, 2)))
        end = min(mask.shape[0], row_to + int(rng.integers(-2, 3)))
        mask[start:end, x] = True


def build_wall_mask(wall: Wall, rng: np.random.Generator) -> np.ndarray:
    ceiling = wall.ceiling + wall.ragged * (fbm_1d(COLUMNS, CEILING_WAVELENGTH, rng) - 0.5) * 2.0
    floor = wall.floor + wall.ragged * (fbm_1d(COLUMNS, FLOOR_WAVELENGTH, rng) - 0.5) * 2.0

    ceiling_rows = np.clip(np.round(ceiling).astype(int), 1, ROWS // 2)
    floor_rows = np.clip(np.round(floor).astype(int), 1, ROWS // 2)

    mask = np.zeros((ROWS, COLUMNS), bool)

    for column in range(COLUMNS):
        mask[: ceiling_rows[column], column] = True
        mask[ROWS - floor_rows[column] :, column] = True

    for _ in range(wall.spikes):
        column = int(rng.integers(0, COLUMNS))
        length = int(rng.integers(2, 7))
        half_width = int(rng.integers(0, 2))

        if rng.random() < 0.6:
            hang(mask, column, length, half_width, ROWS // 2, rng)
        else:
            rise(mask, column, length, half_width, ROWS // 2, rng)

    low, high = wall.pillar_span
    for _ in range(wall.pillars):
        width = int(rng.integers(wall.pillar_width[0], wall.pillar_width[1] + 1))
        column = int(rng.integers(0, COLUMNS))
        pillar(mask, column, width, int(low * ROWS), int(high * ROWS), rng)

    mask = tidy(mask)

    # Loose chunks in the opening, so the wall is not a clean frame
    for _ in range(wall.rubble):
        column = int(rng.integers(0, COLUMNS))
        row = int(rng.integers(0, ROWS))
        stamp(mask, blob((ROWS, COLUMNS), int(rng.integers(1, 5)), int(rng.integers(1, 4)), rng), column, row)

    # Holes that open the wall to the sky
    for _ in range(wall.holes):
        rocks = np.argwhere(mask)

        if rocks.size == 0:
            continue

        row, column = rocks[rng.integers(0, len(rocks))]
        hole = np.zeros((ROWS, COLUMNS), bool)

        stamp(hole, blob((ROWS, COLUMNS), int(rng.integers(1, 5)), int(rng.integers(1, 4)), rng), int(column), int(row))
        mask = mask & (~hole)

    return drop_small(mask, 4)


def box_blur(field: np.ndarray, radius: int) -> np.ndarray:
    result = field
    for axis in (0, 1):
        total = np.zeros_like(result)
        for offset in range(-radius, radius + 1):
            total += np.roll(result, offset, axis)
        result = total / (2 * radius + 1)
    return result


def stretch(cells: np.ndarray) -> np.ndarray:
    return np.repeat(np.repeat(cells, BLOCK, axis=0), BLOCK, axis=1)


def shade_wall(mask: np.ndarray, wall: Wall, variants: list[np.ndarray], rng: np.random.Generator) -> np.ndarray:
    height = mask.shape[0] * BLOCK
    width = mask.shape[1] * BLOCK

    gap = stretch(~mask).astype(np.float32)

    # How much of the opening light reaches a texel: the near field lights the
    # rim, the far field gives the falloff into the mass of the wall
    rim = box_blur(gap, 2)
    glow = box_blur(box_blur(gap, 7), 7)

    shade = 0.40 + 0.48 * glow + 0.34 * rim * rim

    open_up = stretch(np.roll(~mask, 1, axis=0))
    open_down = stretch(np.roll(~mask, -1, axis=0))
    open_left = stretch(np.roll(~mask, 1, axis=1))
    open_right = stretch(np.roll(~mask, -1, axis=1))

    row_in_block = (np.arange(height) % BLOCK)[:, None]
    col_in_block = (np.arange(width) % BLOCK)[None, :]

    weight = np.ones((height, width), np.float32)
    weight *= np.where(open_up & (row_in_block < 2), 1.18, 1.0)
    weight *= np.where(open_down & (row_in_block >= BLOCK - 2), 0.84, 1.0)
    weight *= np.where(open_left & (col_in_block < 2), 1.05, 1.0)
    weight *= np.where(open_right & (col_in_block >= BLOCK - 2), 0.95, 1.0)

    grain = np.zeros((height, width, 3), np.float32)
    variant_of = rng.integers(0, len(variants), mask.shape)

    for index, variant in enumerate(variants):
        selected = stretch(variant_of == index)
        grain[selected] = np.tile(variant, (mask.shape[0], mask.shape[1], 1))[selected]

    rgb = grain * (shade * weight)[:, :, None] * wall.tint

    # Rock close to an opening picks up the colour of the sky behind it, and
    # the haze fakes the air between the walls
    fog = sky_rows(height)[:, None, :]
    light = np.clip(wall.haze + 0.20 * rim, 0.0, 0.85)
    rgb = rgb * (1.0 - light[:, :, None]) + fog * 0.95 * light[:, :, None]

    alpha = stretch(mask).astype(np.float32)
    return np.concatenate((np.clip(rgb, 0.0, 1.0), alpha[:, :, None]), axis=2)


def to_image(rgba: np.ndarray) -> Image.Image:
    data = (np.clip(rgba, 0.0, 1.0) * 255.0 + 0.5).astype(np.uint8)
    return Image.fromarray(data, "RGBA")


def blur_wall(rgba: np.ndarray, radius: float) -> np.ndarray:
    """Bake the depth of field: the front wall is out of focus."""

    if radius <= 0.0:
        return rgba

    image = to_image(rgba).filter(ImageFilter.GaussianBlur(radius))
    return np.asarray(image, np.float32) / 255.0


def preview(walls: list[tuple[str, np.ndarray]]) -> Image.Image:
    sky = np.tile(sky_rows(TILE_HEIGHT)[:, None, :], (1, TILE_WIDTH, 1))
    canvas = Image.fromarray((sky * 255.0).astype(np.uint8), "RGB").convert("RGBA")

    for _, rgba in walls:
        canvas.alpha_composite(to_image(rgba))

    return canvas.convert("RGB")


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate the parallax cave wall textures.")
    parser.add_argument("-o", "--out-dir", type=Path, default=DEFAULT_OUTPUT, help="directory for the wall PNGs")
    parser.add_argument("--preview", type=Path, default=None, help="also write the walls stacked over the sky")
    args = parser.parse_args()

    args.out_dir.mkdir(parents=True, exist_ok=True)

    variants = stone_variants()
    walls = []

    for wall in WALLS:
        rng = np.random.default_rng(wall.seed)
        mask = build_wall_mask(wall, rng)
        rgba = blur_wall(shade_wall(mask, wall, variants, rng), wall.blur)

        walls.append((wall.name, rgba))
        to_image(rgba).save(args.out_dir / wall.name)
        print("wrote", args.out_dir / wall.name)

    if args.preview:
        args.preview.parent.mkdir(parents=True, exist_ok=True)
        preview(walls).save(args.preview)
        print("wrote", args.preview)


if __name__ == "__main__":
    main()
