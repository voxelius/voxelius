#!/usr/bin/env python3

"""
This is vibecoded, here be dragons. 

Generate the builtin tree and scatter feature JSON.

Each species is a seeded shape builder, so rerunning with the same seed writes
the same files. The output is the format that shared/res/feature.cc loads:
a palette of block entries and a list of parts with an offset and the tags the
part may overwrite.
"""

from __future__ import annotations

import argparse
import json
import random
from dataclasses import dataclass, field
from pathlib import Path

DIRT = "dirt"
LOG = "log"
LEAVES = "leaves"
ROCK = "rock"
ROCK2 = "rock2"
STEM = "stem"
CAP = "cap"

AXIS_Y = "y"
AXIS_X = "x"
AXIS_Z = "z"

ROLE_OVERWRITE = {
    DIRT: ["turf", "xfoil"],
    LOG: ["gas", "xfoil"],
    LEAVES: ["gas", "xfoil"],
    ROCK: ["turf", "gas", "xfoil"],
    ROCK2: ["turf", "gas", "xfoil"],
    STEM: ["gas", "xfoil"],
    CAP: ["gas", "xfoil"],
}

# A log and the mushroom stem are state blocks: the palette entry always names
# the orientation, otherwise the variant comes from the block default.
AXIS_ORIENTATION = {
    "y": "up",
    "x": "east",
    "z": "north",
}

Pos = tuple[int, int, int]


def hash_unit(seed: int, x: int, y: int, z: int) -> float:
    """Return a stable pseudo-random value in [0, 1) for one grid cell."""
    h = (seed * 0x9E3779B1) ^ (x * 0x85EBCA6B) ^ (y * 0xC2B2AE35) ^ (z * 0x27D4EB2F)
    h &= 0xFFFFFFFF
    h ^= h >> 15
    h = (h * 0x2545F491) & 0xFFFFFFFF
    h ^= h >> 13
    return h / 4294967296.0


@dataclass(frozen=True)
class Cell:
    role: str
    axis: str = AXIS_Y


@dataclass
class Shape:
    cells: dict[Pos, Cell] = field(default_factory=dict)

    def put(self, pos: Pos, role: str, axis: str = AXIS_Y) -> None:
        self.cells[pos] = Cell(role, axis)

    def put_if_empty(self, pos: Pos, role: str, axis: str = AXIS_Y) -> None:
        if pos not in self.cells:
            self.cells[pos] = Cell(role, axis)

    def has_role(self, pos: Pos, role: str) -> bool:
        cell = self.cells.get(pos)
        return cell is not None and cell.role == role

    def bounds(self) -> tuple[Pos, Pos]:
        xs = [pos[0] for pos in self.cells]
        ys = [pos[1] for pos in self.cells]
        zs = [pos[2] for pos in self.cells]
        return (min(xs), min(ys), min(zs)), (max(xs), max(ys), max(zs))

    def count(self, role: str) -> int:
        return sum(1 for cell in self.cells.values() if cell.role == role)

    def visible_trunk(self) -> int:
        """Count the trunk blocks that stand in the open below the first leaves."""
        leaf_layers = [pos[1] for pos, cell in self.cells.items() if cell.role == LEAVES]
        if leaf_layers:
            return max(0, min(leaf_layers) - 1)
        y = 1
        while self.has_role((0, y, 0), LOG):
            y += 1
        return y - 1

    def disk(self, y: int, radius: int, role: str, *, ragged: float = 0.0, seed: int = 0,
        center: tuple[int, int] = (0, 0), kind: str = "round") -> None:
        cx, cz = center
        for dx in range(-radius, radius + 1):
            for dz in range(-radius, radius + 1):
                if not disk_contains(dx, dz, radius, kind):
                    continue
                edge = max(abs(dx), abs(dz)) >= max(radius - 1, 1)
                if radius >= 2 and edge and ragged > 0.0 and hash_unit(seed, dx, 0, dz) < ragged:
                    continue
                self.put((cx + dx, y, cz + dz), role)

    def ellipsoid(self, center: Pos, radii: Pos, role: str, *, ragged: float = 0.0, seed: int = 0,
        min_y: int = -512) -> None:
        cx, cy, cz = center
        rx, ry, rz = radii
        for dx in range(-rx, rx + 1):
            for dy in range(-ry, ry + 1):
                for dz in range(-rz, rz + 1):
                    if cy + dy < min_y:
                        continue
                    t = (dx / rx) ** 2 + (dy / ry) ** 2 + (dz / rz) ** 2
                    if t > 1.0:
                        continue
                    if t > 0.62 and ragged > 0.0 and hash_unit(seed, dx, dy, dz) < ragged:
                        continue
                    self.put((cx + dx, cy + dy, cz + dz), role)

    def rim_cells(self, y: int, radius: int, kind: str = "round") -> list[Pos]:
        """Return the outer ring of a disk on layer y."""
        ring = []
        for dx in range(-radius, radius + 1):
            for dz in range(-radius, radius + 1):
                if not disk_contains(dx, dz, radius, kind):
                    continue
                if max(abs(dx), abs(dz)) < radius:
                    continue
                ring.append((dx, y, dz))
        return ring


def disk_contains(dx: int, dz: int, radius: int, kind: str = "round") -> bool:
    if kind == "square":
        return max(abs(dx), abs(dz)) <= radius
    if kind == "plus":
        return abs(dx) + abs(dz) <= radius
    return dx * dx + dz * dz <= radius * radius + radius // 2


def feature_json(shape: Shape, blocks: dict[str, str]) -> dict:
    palette: list[dict] = []
    index: dict[tuple[str, str | None], int] = {}

    def slot(role: str, axis: str) -> int:
        block = blocks[role]
        orientation = AXIS_ORIENTATION.get(axis) if role in (LOG, STEM) else None
        key = (block, orientation)
        if key in index:
            return index[key]
        entry: dict = {"block": block}
        if orientation is not None:
            entry["states"] = {"orientation": orientation}
        index[key] = len(palette)
        palette.append(entry)
        return index[key]

    parts = []
    for pos in sorted(shape.cells, key=lambda p: (p[1], p[0], p[2])):
        cell = shape.cells[pos]
        parts.append({
            "block": slot(cell.role, cell.axis),
            "offset": list(pos),
            "overwrite": ROLE_OVERWRITE[cell.role],
        })

    return {"palette": palette, "parts": parts}


def trunk(shape: Shape, height: int, *, x: int = 0, z: int = 0, wide: int = 1) -> None:
    for y in range(1, height + 1):
        for dx in range(wide):
            for dz in range(wide):
                shape.put((x + dx, y, z + dz), LOG)


def hang_from_rim(shape: Shape, layer: int, radius: int, seed: int, chance: float, depth: int = 1) -> None:
    for (dx, _, dz) in shape.rim_cells(layer, radius):
        if hash_unit(seed, dx, layer, dz) >= chance:
            continue
        for step in range(1, depth + 1):
            shape.put_if_empty((dx, layer - step, dz), LEAVES)


def build_oak(rng: random.Random) -> Shape:
    """Broad dome on a short trunk: 7 wide, 6 tall crown, 4-5 visible trunk blocks."""
    shape = Shape()
    shape.put((0, 0, 0), DIRT)
    visible = rng.randint(4, 5)
    seed = rng.randrange(1 << 30)
    lean = rng.choice((-1, 0, 0, 1))
    drift = rng.choice((-1, 0, 0, 1))
    layers = (
        (visible + 1, 2),
        (visible + 2, 3),
        (visible + 3, 3),
        (visible + 4, 2),
        (visible + 5, 1),
        (visible + 6, 0),
    )
    for y, radius in layers:
        center = (lean, drift) if y >= visible + 3 else (0, 0)
        shape.disk(y, radius, LEAVES, ragged=0.16, seed=seed + y, center=center)

    # A ragged skirt hides the flat bottom of the dome.
    hang_from_rim(shape, visible + 2, 2, seed + 101, 0.4)

    trunk(shape, visible + 3)
    return shape


def build_birch(rng: random.Random) -> Shape:
    """Tall pale trunk with a narrow teardrop crown: 5 wide, 7 tall crown."""
    shape = Shape()
    shape.put((0, 0, 0), DIRT)
    visible = rng.randint(3, 5)
    seed = rng.randrange(1 << 30)
    shape.ellipsoid((0, visible + 3, 0), (2, 3, 2), LEAVES, ragged=0.20, seed=seed)
    shape.put((0, visible + 6, 0), LEAVES)
    trunk(shape, visible + 2)
    return shape


def build_spruce(rng: random.Random) -> Shape:
    """Narrow conical crown with drooping branch tips: 7 wide, crown from y=2 to the tip."""
    shape = Shape()
    shape.put((0, 0, 0), DIRT)
    height = rng.randint(9, 12)
    seed = rng.randrange(1 << 30)
    for y in range(2, height + 1):
        t = (y - 2) / max(height - 2, 1)
        radius = max(int(round(3.4 * (1.0 - t))), 1)
        shape.disk(y, radius, LEAVES, ragged=0.10, seed=seed + y)
        for (dx, _, dz) in shape.rim_cells(y, radius):
            if y <= 2:
                continue
            if hash_unit(seed + 77, dx, y, dz) < 0.45:
                shape.put_if_empty((dx, y - 1, dz), LEAVES)
    shape.put((0, height + 1, 0), LEAVES)
    trunk(shape, height)
    return shape


def build_pine(rng: random.Random) -> Shape:
    """Bare tall trunk with a small irregular crown at the top and one side branch."""
    shape = Shape()
    shape.put((0, 0, 0), DIRT)
    height = rng.randint(9, 12)
    seed = rng.randrange(1 << 30)
    direction = rng.choice(((1, 0), (-1, 0), (0, 1), (0, -1)))
    branch_y = height - rng.randint(3, 5)
    length = rng.randint(1, 2)
    for step in range(1, length + 1):
        pos = (direction[0] * step, branch_y, direction[1] * step)
        shape.put(pos, LOG, AXIS_X if direction[0] else AXIS_Z)

    jitter = (-1, 0, 1)
    # The middle clump is centred so the trunk stays covered, the outer two lean.
    shape.disk(height - 2, 2, LEAVES, ragged=0.26, seed=seed + 1, center=(rng.choice(jitter), rng.choice(jitter)))
    shape.disk(height - 1, 2, LEAVES, ragged=0.12, seed=seed + 2)
    shape.disk(height, 2, LEAVES, ragged=0.26, seed=seed + 3, center=(rng.choice(jitter), rng.choice(jitter)))
    shape.put_if_empty((0, height + 1, 0), LEAVES)

    end = length + 1
    shape.disk(branch_y, 2, LEAVES, ragged=0.28, seed=seed + 201, center=(direction[0] * end, direction[1] * end))
    shape.disk(branch_y + 1, 1, LEAVES, ragged=0.1, seed=seed + 202, center=(direction[0] * end, direction[1] * end))

    trunk(shape, height - 1)
    return shape


def build_swamp(rng: random.Random) -> Shape:
    """Low wide flat crown on a short trunk with a drooping rim."""
    shape = Shape()
    shape.put((0, 0, 0), DIRT)
    height = rng.randint(3, 4)
    seed = rng.randrange(1 << 30)
    shape.disk(height + 1, 4, LEAVES, ragged=0.22, seed=seed + 1)
    shape.disk(height + 2, 3, LEAVES, ragged=0.18, seed=seed + 2)
    shape.disk(height + 3, 1, LEAVES, ragged=0.0, seed=seed + 3)

    for (dx, _, dz) in shape.rim_cells(height + 1, 4):
        if hash_unit(seed + 3, dx, height, dz) < 0.5:
            shape.put_if_empty((dx, height, dz), LEAVES)

    for (rx, rz) in ((-1, 0), (1, 1)):
        shape.put((rx, 1, rz), LOG)

    trunk(shape, height)
    return shape


def build_willow(rng: random.Random) -> Shape:
    """Weeping willow: broad crown of loose clumps with long hanging leaf strands."""
    shape = Shape()
    shape.put((0, 0, 0), DIRT)
    height = rng.randint(8, 10)
    seed = rng.randrange(1 << 30)
    crown = height - 1
    layers = ((crown, 3), (crown + 1, 3), (crown + 2, 2), (crown + 3, 1))
    for y, radius in layers:
        shape.disk(y, radius, LEAVES, ragged=0.32, seed=seed + y)

    # Long strands fall from the rim, which is the whole point of a willow.
    for (dx, _, dz) in shape.rim_cells(crown, 3):
        if hash_unit(seed + 11, dx, crown, dz) >= 0.4:
            continue
        length = 2 + int(hash_unit(seed + 12, dx, crown, dz) * 3)
        for step in range(1, length + 1):
            shape.put_if_empty((dx, crown - step, dz), LEAVES)

    trunk(shape, height)
    return shape


def build_boulder(rng: random.Random) -> Shape:
    """Mossy rock dome with a few plain cobble blocks, half buried in the turf."""
    shape = Shape()
    seed = rng.randrange(1 << 30)
    radius_x = rng.randint(2, 3)
    radius_z = rng.randint(2, 3)
    height = rng.randint(2, 3)
    shape.ellipsoid((0, 1, 0), (radius_x, height, radius_z), ROCK, ragged=0.30, seed=seed, min_y=1)

    if rng.random() < 0.7:
        direction = rng.choice(((1, 0), (-1, 0), (0, 1), (0, -1)))
        shape.ellipsoid((direction[0] * (radius_x - 1), 1, direction[1] * (radius_z - 1)), (1, 1, 1), ROCK,
            ragged=0.35, seed=seed + 1, min_y=1)

    # A broken ring of rock at ground level reads as a half buried stone.
    for (x, y, z) in list(shape.cells):
        if y != 1:
            continue
        if hash_unit(seed + 23, x, 0, z) < 0.5:
            shape.put_if_empty((x, 0, z), ROCK)

    for pos, cell in list(shape.cells.items()):
        if cell.role == ROCK and hash_unit(seed + 7, *pos) < 0.20:
            shape.cells[pos] = Cell(ROCK2)

    return shape


def build_red_mushroom(rng: random.Random) -> Shape:
    """Huge red mushroom: 1x1 stem and a 7 wide cap in three layers."""
    shape = Shape()
    seed = rng.randrange(1 << 30)
    height = rng.randint(3, 5)
    shape.disk(height, 3, CAP, ragged=0.14, seed=seed + 1)
    shape.disk(height + 1, 2, CAP, ragged=0.10, seed=seed + 2)
    shape.disk(height + 2, 1, CAP, ragged=0.0, seed=seed + 3)
    for y in range(1, height + 1):
        shape.put((0, y, 0), STEM)
    return shape


def build_brown_mushroom(rng: random.Random) -> Shape:
    """Huge brown mushroom: 1x1 stem and a flatter 7 wide cap in two layers."""
    shape = Shape()
    seed = rng.randrange(1 << 30)
    height = rng.randint(3, 4)
    shape.disk(height, 3, CAP, ragged=0.20, seed=seed + 1)
    shape.disk(height + 1, 2, CAP, ragged=0.12, seed=seed + 2)
    for y in range(1, height + 1):
        shape.put((0, y, 0), STEM)
    return shape


def tall_mushroom(rng: random.Random, radii: tuple[int, ...]) -> Shape:
    """Tall mushroom: a bare stem as tall as a birch and a narrow clean cap."""
    shape = Shape()
    height = rng.randint(6, 8)
    for y in range(1, height + 1):
        shape.put((0, y, 0), STEM)
    for index, radius in enumerate(radii):
        shape.disk(height + index, radius, CAP)
    return shape


def build_red_mushroom_tall(rng: random.Random) -> Shape:
    """Tall red mushroom: 5 wide cap, four clean layers, no hanging edge blocks."""
    return tall_mushroom(rng, (2, 2, 2, 1))


def build_brown_mushroom_tall(rng: random.Random) -> Shape:
    """Tall brown mushroom: 3 wide cap, three clean layers, no hanging edge blocks."""
    return tall_mushroom(rng, (1, 1, 0))


@dataclass(frozen=True)
class Species:
    key: str
    title: str
    blocks: dict[str, str]
    build: object
    note: str

    @property
    def suffix(self) -> str:
        return "_tree" if LEAVES in self.blocks else ""

    def variant_name(self, index: int) -> str:
        return f"{self.key}{self.suffix}_{index + 1:02d}"

    def variants(self, count: int, seed: int) -> list[Shape]:
        result = []
        for index in range(count):
            rng = random.Random(f"{seed}:{self.key}:{index}")
            result.append(self.build(rng))
        return result


SPECIES = {
    species.key: species
    for species in (
        Species(
            key="oak",
            title="Oak",
            blocks={DIRT: "builtin:dirt", LOG: "builtin:oak_log", LEAVES: "builtin:oak_leaves"},
            build=build_oak,
            note="broad dome on a short trunk",
        ),
        Species(
            key="birch",
            title="Birch",
            blocks={DIRT: "builtin:dirt", LOG: "builtin:birch_log", LEAVES: "builtin:birch_leaves"},
            build=build_birch,
            note="tall pale trunk with a narrow crown",
        ),
        Species(
            key="spruce",
            title="Spruce",
            blocks={DIRT: "builtin:dirt", LOG: "builtin:spruce_log", LEAVES: "builtin:spruce_leaves"},
            build=build_spruce,
            note="conical crown from y=2, drooping tips",
        ),
        Species(
            key="pine",
            title="Pine",
            blocks={DIRT: "builtin:dirt", LOG: "builtin:pine_log", LEAVES: "builtin:pine_leaves"},
            build=build_pine,
            note="bare trunk with a crown at the top and one side branch",
        ),
        Species(
            key="swamp",
            title="Swamp oak",
            blocks={DIRT: "builtin:dirt", LOG: "builtin:oak_log", LEAVES: "builtin:oak_leaves"},
            build=build_swamp,
            note="low wide crown on a short trunk",
        ),
        Species(
            key="willow",
            title="Willow",
            blocks={DIRT: "builtin:dirt", LOG: "builtin:willow_log", LEAVES: "builtin:willow_leaves"},
            build=build_willow,
            note="broad crown with long hanging strands",
        ),
        Species(
            key="boulder",
            title="Boulder",
            blocks={ROCK: "builtin:mossy_cobble", ROCK2: "builtin:cobble"},
            build=build_boulder,
            note="mossy rock dome half buried in the turf",
        ),
        Species(
            key="red_mushroom",
            title="Red mushroom",
            blocks={STEM: "builtin:mushroom_stem", CAP: "builtin:red_mushroom_cap"},
            build=build_red_mushroom,
            note="wide 7 block cap in three layers",
        ),
        Species(
            key="red_mushroom_tall",
            title="Tall red mushroom",
            blocks={STEM: "builtin:mushroom_stem", CAP: "builtin:red_mushroom_cap"},
            build=build_red_mushroom_tall,
            note="narrow 5 wide cap on a birch-high stem",
        ),
        Species(
            key="brown_mushroom",
            title="Brown mushroom",
            blocks={STEM: "builtin:mushroom_stem", CAP: "builtin:brown_mushroom_cap"},
            build=build_brown_mushroom,
            note="wide 7 block cap in two layers",
        ),
        Species(
            key="brown_mushroom_tall",
            title="Tall brown mushroom",
            blocks={STEM: "builtin:mushroom_stem", CAP: "builtin:brown_mushroom_cap"},
            build=build_brown_mushroom_tall,
            note="narrow 3 wide cap on a birch-high stem",
        ),
    )
}


def write_feature(path: Path, feature: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(feature, separators=(",", ":")) + "\n", encoding="utf-8", newline="\n")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--species", default="", help="comma separated keys, default is all")
    parser.add_argument("--variants", type=int, default=4, help="variants per species (default: 4)")
    parser.add_argument("--seed", type=int, default=0, help="base seed (default: 0)")
    parser.add_argument("--out", type=Path, default=None, help="output directory")
    parser.add_argument("--dry-run", action="store_true", help="print the plan without writing")
    parser.add_argument("--list", action="store_true", help="list the species and exit")
    args = parser.parse_args()

    if args.list:
        for key, species in SPECIES.items():
            print(f"{key:24s} {species.title:24s} {species.note}")
        return

    keys = [key.strip() for key in args.species.split(",")] if args.species else list(SPECIES)
    unknown = [key for key in keys if key not in SPECIES]
    if unknown:
        raise SystemExit(f"unknown species: {', '.join(unknown)}")

    out = args.out or Path(__file__).resolve().parents[1] / "data" / "builtin" / "features"
    total = 0

    for key in keys:
        species = SPECIES[key]
        for index, shape in enumerate(species.variants(args.variants, args.seed)):
            name = species.variant_name(index)
            feature = feature_json(shape, species.blocks)
            (lo, hi) = shape.bounds()
            width = hi[0] - lo[0] + 1
            height = hi[1] + 1
            if args.dry_run:
                print(f"{name}: {len(feature['parts'])} parts, {height} tall, {width} wide")
            else:
                write_feature(out / f"{name}.json", feature)
                print(f"{name}.json: {len(feature['parts'])} parts, {height} tall, {width} wide")
            total += 1

    print(f"{total} features written to {out}")


if __name__ == "__main__":
    main()
