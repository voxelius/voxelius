#!/usr/bin/env python3

"""
This is vibecoded, here be dragons. 

Render feature JSON without launching the game.

The renderer is feedback only: no lighting model, no ambient occlusion, and a
preview exposure on the dark leaf masks. A mushroom gets a second panel from
below so the cap underside is visible.

    python misc/preview-features.py
    python misc/preview-features.py data/builtin/features/oak_tree_01.json --out build/preview --scale 24
"""

from __future__ import annotations

import argparse
import json
from dataclasses import dataclass
from functools import lru_cache
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont

REPO_ROOT = Path(__file__).resolve().parents[1]
TEXTURE_ROOT = REPO_ROOT / "data" / "builtin" / "textures" / "block"
FEATURE_ROOT = REPO_ROOT / "data" / "builtin" / "features"

BACKGROUND = (26, 30, 36)
FOREGROUND = (226, 230, 236)
DIM = (138, 146, 156)

TEX_SIZE = 16


# ---------------------------------------------------------------- materials

@dataclass(frozen=True)
class Material:
    top: tuple[np.ndarray, ...]
    side: tuple[np.ndarray, ...]
    bottom: tuple[np.ndarray, ...] = ()

    def face_textures(self, axis: str, face: str) -> tuple[np.ndarray, ...]:
        if face == "-y":
            return self.bottom if self.bottom else self.side
        if axis == "x":
            return self.top if face == "x" else self.side
        if axis == "z":
            return self.top if face == "z" else self.side
        return self.top if face == "y" else self.side


@lru_cache(maxsize=None)
def load_texture(relative: str, tint: tuple[float, float, float] = (1.0, 1.0, 1.0), exposure: float = 1.0,
    mask: str | None = None, mask_tint: tuple[float, float, float] = (1.0, 1.0, 1.0), rotate: int = 0) -> np.ndarray:
    image = Image.open(TEXTURE_ROOT / relative).convert("RGBA")
    data = np.asarray(image, dtype=np.float32)[..., :3] / 255.0

    if mask is not None:
        mask_image = Image.open(TEXTURE_ROOT / mask).convert("RGBA")
        mask_value = np.asarray(mask_image, dtype=np.float32)[..., 0:1] / 255.0
        data = data * (1.0 + mask_value * (np.asarray(mask_tint, dtype=np.float32) - 1.0))

    if rotate:
        data = np.rot90(data, rotate)

    return np.clip(data * np.asarray(tint, dtype=np.float32) * exposure, 0.0, 1.0)


def texture_set(*specs: tuple[str, float]) -> tuple[np.ndarray, ...]:
    return tuple(load_texture(path, exposure=exposure) for path, exposure in specs)


def build_materials() -> dict[str, Material]:
    grass_tint = (0.45, 0.72, 0.30)
    moss_tint = (0.55, 0.72, 0.34)

    return {
        "builtin:dirt": Material(
            top=texture_set(("dirt/dirt_01.png", 1.25)),
            side=texture_set(("dirt/dirt_03.png", 1.25)),
        ),
        "builtin:grass": Material(
            top=(load_texture("grass/grass_01.png", tint=grass_tint, exposure=1.8),),
            side=(load_texture("grass/grass_side_01.png", exposure=1.3,
                mask="grass/grass_side_overlay.png", mask_tint=grass_tint),),
        ),
        "builtin:oak_log": Material(
            top=texture_set(("oak/oak_log.png", 1.25)),
            side=(load_texture("oak/oak_wood_01.png", exposure=2.6, rotate=1),
                load_texture("oak/oak_wood_02.png", exposure=2.6, rotate=1)),
            bottom=texture_set(("oak/oak_log.png", 1.25)),
        ),
        "builtin:oak_leaves": Material(
            top=(load_texture("oak/oak_leaves.png", tint=(0.32, 0.60, 0.20), exposure=3.2),),
            side=(load_texture("oak/oak_leaves.png", tint=(0.30, 0.56, 0.19), exposure=3.0),),
        ),
        "builtin:pine_log": Material(
            top=texture_set(("pine/pine_log.png", 1.4)),
            side=(load_texture("pine/pine_wood_01.png", exposure=3.2, rotate=1),
                load_texture("pine/pine_wood_02.png", exposure=3.2, rotate=1)),
            bottom=texture_set(("pine/pine_log.png", 1.4)),
        ),
        "builtin:pine_leaves": Material(
            top=(load_texture("pine/pine_leaves_01.png", tint=(0.22, 0.48, 0.30), exposure=4.2),),
            side=(load_texture("pine/pine_leaves_02.png", tint=(0.20, 0.46, 0.29), exposure=4.0),
                load_texture("pine/pine_leaves_03.png", tint=(0.20, 0.46, 0.29), exposure=4.0)),
        ),
        "builtin:spruce_log": Material(
            top=texture_set(("spruce/spruce_log.png", 1.2)),
            side=(load_texture("spruce/spruce_wood_01.png", exposure=2.6, rotate=1),
                load_texture("spruce/spruce_wood_02.png", exposure=2.6, rotate=1)),
            bottom=texture_set(("spruce/spruce_log.png", 1.2)),
        ),
        "builtin:spruce_leaves": Material(
            top=(load_texture("spruce/spruce_leaves.png", tint=(0.20, 0.44, 0.28), exposure=4.0),),
            side=(load_texture("spruce/spruce_leaves.png", tint=(0.18, 0.42, 0.27), exposure=3.8),),
        ),
        "builtin:birch_log": Material(
            top=texture_set(("birch/birch_log.png", 0.9)),
            side=(load_texture("birch/birch_wood_01.png", exposure=0.85, rotate=1),
                load_texture("birch/birch_wood_02.png", exposure=0.85, rotate=1)),
            bottom=texture_set(("birch/birch_log.png", 0.9)),
        ),
        "builtin:birch_leaves": Material(
            top=(load_texture("birch/birch_leaves.png", tint=(0.55, 0.72, 0.30), exposure=3.0),),
            side=(load_texture("birch/birch_leaves.png", tint=(0.52, 0.68, 0.28), exposure=2.9),),
        ),
        "builtin:willow_log": Material(
            top=texture_set(("willow/willow_log.png", 1.15)),
            side=(load_texture("willow/willow_wood_01.png", exposure=1.35, rotate=1),
                load_texture("willow/willow_wood_02.png", exposure=1.35, rotate=1)),
            bottom=texture_set(("willow/willow_log.png", 1.15)),
        ),
        "builtin:willow_leaves": Material(
            top=(load_texture("willow/willow_leaves.png", tint=(0.52, 0.60, 0.34), exposure=3.0),),
            side=(load_texture("willow/willow_leaves.png", tint=(0.50, 0.58, 0.32), exposure=2.9),),
        ),
        "builtin:cobble": Material(
            top=texture_set(("cobble/cobble_01.png", 1.2), ("cobble/cobble_02.png", 1.2)),
            side=texture_set(("cobble/cobble_01.png", 1.2), ("cobble/cobble_02.png", 1.2)),
        ),
        "builtin:mossy_cobble": Material(
            top=(
                load_texture("cobble/cobble_01.png", exposure=1.2, mask="cobble/cobblemoss_mask.png", mask_tint=moss_tint),
                load_texture("cobble/cobble_02.png", exposure=1.2, mask="cobble/cobblemoss_mask.png", mask_tint=moss_tint),
            ),
            side=(
                load_texture("cobble/cobble_01.png", exposure=1.2, mask="cobble/cobblemoss_mask.png", mask_tint=moss_tint),
                load_texture("cobble/cobble_02.png", exposure=1.2, mask="cobble/cobblemoss_mask.png", mask_tint=moss_tint),
            ),
        ),
        "builtin:mushroom_stem": Material(
            top=texture_set(("mushroom/mushroom_stem.png", 1.05)),
            side=(load_texture("mushroom/mushroom_stem.png", exposure=1.05, rotate=1),),
            bottom=texture_set(("mushroom/mushroom_stem.png", 1.05)),
        ),
        "builtin:red_mushroom_cap": Material(
            top=texture_set(("mushroom/red_mushroom_cap_01.png", 0.95), ("mushroom/red_mushroom_cap_02.png", 0.95),
                ("mushroom/red_mushroom_cap_03.png", 0.95), ("mushroom/red_mushroom_cap_04.png", 0.95)),
            side=texture_set(("mushroom/red_mushroom_cap_01.png", 0.9), ("mushroom/red_mushroom_cap_02.png", 0.9)),
            bottom=texture_set(("mushroom/mushroom_trama_01.png", 1.5), ("mushroom/mushroom_trama_02.png", 1.5),
                ("mushroom/mushroom_trama_03.png", 1.5)),
        ),
        "builtin:brown_mushroom_cap": Material(
            top=texture_set(("mushroom/brown_mushroom_cap_01.png", 1.0), ("mushroom/brown_mushroom_cap_02.png", 1.0)),
            side=texture_set(("mushroom/brown_mushroom_cap_01.png", 0.95), ("mushroom/brown_mushroom_cap_02.png", 0.95)),
            bottom=texture_set(("mushroom/mushroom_trama_01.png", 1.45), ("mushroom/mushroom_trama_03.png", 1.45)),
        ),
    }


# -------------------------------------------------------------------- tree

@dataclass(frozen=True)
class BlockCell:
    block: str
    axis: str = "y"


@dataclass
class Tree:
    cells: dict[tuple[int, int, int], BlockCell]
    name: str = ""

    def bounds(self) -> tuple[tuple[int, int, int], tuple[int, int, int]]:
        xs = [pos[0] for pos in self.cells]
        ys = [pos[1] for pos in self.cells]
        zs = [pos[2] for pos in self.cells]
        return (min(xs), min(ys), min(zs)), (max(xs), max(ys), max(zs))

    def is_mushroom(self) -> bool:
        return any(cell.block.endswith("mushroom_cap") for cell in self.cells.values())


def load_feature(path: Path) -> Tree:
    data = json.loads(path.read_text(encoding="utf-8"))
    palette = [entry["block"] for entry in data["palette"]]
    cells = {}
    for part in data["parts"]:
        offset = part["offset"]
        entry = data["palette"][part["block"]]
        orientation = entry.get("states", {}).get("orientation")
        axis = "y"
        if orientation == "east":
            axis = "x"
        elif orientation == "north":
            axis = "z"
        cells[(offset[0], offset[1], offset[2])] = BlockCell(palette[part["block"]], axis)
    return Tree(cells, path.stem)


# ---------------------------------------------------------------- renderer

AXIS_X = np.array([1.0, 0.5])
AXIS_Z = np.array([-1.0, 0.5])

FACES_ABOVE = (
    ("y", (0, 1, 0), (1, 0, 0), (0, 0, 1), 1.00),
    ("x", (1, 0, 0), (0, 0, 1), (0, 1, 0), 0.80),
    ("z", (0, 0, 1), (1, 0, 0), (0, 1, 0), 0.64),
)

FACES_BELOW = (
    ("-y", (0, 0, 0), (1, 0, 0), (0, 0, 1), 0.78),
    ("x", (1, 0, 0), (0, 0, 1), (0, 1, 0), 0.80),
    ("z", (0, 0, 1), (1, 0, 0), (0, 1, 0), 0.64),
)

NEIGHBOURS = {
    "y": (0, 1, 0),
    "-y": (0, -1, 0),
    "x": (1, 0, 0),
    "z": (0, 0, 1),
}


def rotate_cells(cells: dict[tuple[int, int, int], BlockCell], yaw: int) -> dict[tuple[int, int, int], BlockCell]:
    rotated = dict(cells)
    for _ in range(yaw % 4):
        step = {}
        for (x, y, z), cell in rotated.items():
            axis = cell.axis
            if axis == "x":
                axis = "z"
            elif axis == "z":
                axis = "x"
            step[(z, y, -x)] = BlockCell(cell.block, axis)
        rotated = step
    return rotated


def render_iso(tree: Tree, *, scale: int = 20, supersample: int = 2, ground: bool = True, yaw: int = 0,
    below: bool = False) -> Image.Image:
    materials = build_materials()
    cells = rotate_cells(tree.cells, yaw)

    if ground and not below:
        cells = add_ground(cells, margin=1)

    faces = FACES_BELOW if below else FACES_ABOVE
    axis_y = np.array([0.0, 1.0 if below else -1.0])
    depth_sign = np.array([1.0, -1.0 if below else 1.0, 1.0])
    raster_scale = float(scale * supersample)

    def project(x: float, y: float, z: float) -> np.ndarray:
        return x * AXIS_X + z * AXIS_Z + y * axis_y

    corners = [(0, 0, 0), (1, 0, 0), (0, 0, 1), (1, 0, 1), (0, 1, 0), (1, 1, 0), (0, 1, 1), (1, 1, 1)]
    points = []
    for (x, y, z) in cells:
        for corner in corners:
            points.append(project(x + corner[0], y + corner[1], z + corner[2]))
    points = np.asarray(points)
    minimum = points.min(axis=0) - 1.0
    maximum = points.max(axis=0) + 1.0

    width = max(int(np.ceil((maximum[0] - minimum[0]) * raster_scale)), 8)
    height = max(int(np.ceil((maximum[1] - minimum[1]) * raster_scale)), 8)

    color = np.zeros((height, width, 3), dtype=np.float32)
    color[:] = np.asarray(BACKGROUND, dtype=np.float32) / 255.0
    ramp = np.linspace(1.25, 0.85, height, dtype=np.float32)[:, None, None]
    color *= ramp
    depth = np.full((height, width), -1e9, dtype=np.float32)

    for (x, y, z), cell in cells.items():
        material = materials.get(cell.block) or materials["builtin:dirt"]
        block_seed = (x * 73856093 ^ y * 19349663 ^ z * 83492791) & 0x7FFFFFFF
        for name, corner, u_dir, v_dir, shade in faces:
            neighbor = (x + NEIGHBOURS[name][0], y + NEIGHBOURS[name][1], z + NEIGHBOURS[name][2])
            if neighbor in cells:
                continue

            origin = project(x + corner[0], y + corner[1], z + corner[2]) * raster_scale
            edge_u = project(x + corner[0] + u_dir[0], y + corner[1] + u_dir[1], z + corner[2] + u_dir[2]) * raster_scale - origin
            edge_v = project(x + corner[0] + v_dir[0], y + corner[1] + v_dir[1], z + corner[2] + v_dir[2]) * raster_scale - origin

            rasterize(color, depth, origin - minimum * raster_scale, edge_u, edge_v, material, cell.axis, name, shade,
                (x + corner[0], y + corner[1], z + corner[2]), u_dir, v_dir, depth_sign, block_seed)

    image = (np.clip(color, 0.0, 1.0) * 255.0).astype(np.uint8)
    result = Image.fromarray(image, "RGB")
    if supersample > 1:
        result = result.resize((width // supersample, height // supersample), Image.LANCZOS)
    if below:
        result = result.transpose(Image.FLIP_TOP_BOTTOM)
    return result


def add_ground(cells: dict[tuple[int, int, int], BlockCell], margin: int) -> dict[tuple[int, int, int], BlockCell]:
    xs = [pos[0] for pos in cells]
    zs = [pos[2] for pos in cells]
    result = dict(cells)
    for x in range(min(xs) - margin, max(xs) + margin + 1):
        for z in range(min(zs) - margin, max(zs) + margin + 1):
            if (x, 0, z) not in result:
                result[(x, 0, z)] = BlockCell("builtin:grass")
    return result


def rasterize(color: np.ndarray, depth: np.ndarray, origin: np.ndarray, edge_u: np.ndarray, edge_v: np.ndarray,
    material: Material, axis: str, face: str, shade: float, world_origin: tuple[int, int, int],
    u_dir: tuple[int, int, int], v_dir: tuple[int, int, int], depth_sign: np.ndarray, block_seed: int) -> None:
    height, width = depth.shape
    corners = np.array([origin, origin + edge_u, origin + edge_v, origin + edge_u + edge_v])
    low = np.floor(corners.min(axis=0)) - 2.0
    high = np.ceil(corners.max(axis=0)) + 2.0
    x0 = max(int(low[0]), 0)
    y0 = max(int(low[1]), 0)
    x1 = min(int(high[0]) + 1, width)
    y1 = min(int(high[1]) + 1, height)
    if x1 <= x0 or y1 <= y0:
        return

    det = edge_u[0] * edge_v[1] - edge_u[1] * edge_v[0]
    if abs(det) < 1e-9:
        return

    xs = np.arange(x0, x1, dtype=np.float32) + 0.5
    ys = np.arange(y0, y1, dtype=np.float32) + 0.5
    dx = xs[None, :] - origin[0]
    dy = ys[:, None] - origin[1]
    u = (edge_v[1] * dx - edge_v[0] * dy) / det
    v = (-edge_u[1] * dx + edge_u[0] * dy) / det
    mask = (u >= 0.0) & (u < 1.0) & (v >= 0.0) & (v < 1.0)
    if not mask.any():
        return

    textures = material.face_textures(axis, face)
    texture = textures[block_seed % len(textures)]
    tu = np.clip((u * TEX_SIZE).astype(np.int32), 0, TEX_SIZE - 1)
    tv = np.clip((v * TEX_SIZE).astype(np.int32), 0, TEX_SIZE - 1)
    texels = texture[TEX_SIZE - 1 - tv, tu] * shade

    depth_origin = float(np.dot(np.asarray(world_origin, dtype=np.float32), depth_sign))
    depth_u = float(np.dot(np.asarray(u_dir, dtype=np.float32), depth_sign))
    depth_v = float(np.dot(np.asarray(v_dir, dtype=np.float32), depth_sign))
    pixel_depth = depth_origin + u * depth_u + v * depth_v

    view_color = color[y0:y1, x0:x1]
    view_depth = depth[y0:y1, x0:x1]
    visible = mask & (pixel_depth > view_depth)
    view_color[visible] = texels[visible]
    view_depth[visible] = pixel_depth[visible]


# --------------------------------------------------------------- elevation

def flat_color(block: str) -> np.ndarray:
    material = build_materials().get(block)
    if material is None:
        material = build_materials()["builtin:dirt"]
    pixels = np.concatenate([texture.reshape(-1, 3) for texture in material.side], axis=0)
    return pixels.mean(axis=0)


def elevation(tree: Tree, view: str = "front", scale: int = 6) -> Image.Image:
    if view == "front":
        axis_a = lambda pos: pos[0]
        depth = lambda pos: pos[2]
    else:
        axis_a = lambda pos: pos[2]
        depth = lambda pos: pos[0]

    nearest: dict[tuple[int, int], tuple[int, str]] = {}
    for pos, cell in tree.cells.items():
        pixel = (axis_a(pos), pos[1])
        current = nearest.get(pixel)
        if current is None or depth(pos) < current[0]:
            nearest[pixel] = (depth(pos), cell.block)

    a_values = [pixel[0] for pixel in nearest]
    b_values = [pixel[1] for pixel in nearest]
    a0, a1 = min(a_values), max(a_values)
    b0, b1 = min(b_values), max(b_values)

    image = np.zeros(((b1 - b0 + 1) * scale, (a1 - a0 + 1) * scale, 3), dtype=np.float32)
    image[:] = np.asarray(BACKGROUND, dtype=np.float32) / 255.0

    for (a, b), (_, block) in nearest.items():
        x0 = (a - a0) * scale
        y0 = (b1 - b) * scale
        image[y0:y0 + scale, x0:x0 + scale] = flat_color(block)

    return Image.fromarray((np.clip(image, 0.0, 1.0) * 255.0).astype(np.uint8), "RGB")


def ascii_view(tree: Tree) -> list[str]:
    role_character = {"dirt": ".", "log": "#", "leaves": "*"}
    rows: dict[int, dict[int, str]] = {}
    rank = {".": 0, "*": 1, "#": 2}

    def role_of(block: str) -> str:
        if block.endswith("_leaves"):
            return "leaves"
        if block.endswith(("_log", "_wood", "_stem")):
            return "log"
        if block.endswith("_cobble"):
            return "log"
        if block.endswith("_cap"):
            return "leaves"
        return "dirt"

    for (x, y, z), cell in tree.cells.items():
        row = rows.setdefault(y, {})
        character = role_character[role_of(cell.block)]
        if x not in row or rank[character] > rank[row[x]]:
            row[x] = character

    xs = [x for row in rows.values() for x in row]
    x0, x1 = min(xs), max(xs)
    lines = []
    for y in range(max(rows), -1, -1):
        row = rows.get(y, {})
        lines.append("".join(row.get(x, " ") for x in range(x0, x1 + 1)).rstrip())
    return lines


# -------------------------------------------------------------------- sheet

def load_font(size: int):
    return ImageFont.load_default(size=size)


def make_panel(tree: Tree, scale: int, below: bool) -> Image.Image:
    images = [render_iso(tree, scale=scale)]
    if below:
        images.append(render_iso(tree, scale=scale, ground=False, below=True))
    detail = max(4, int(scale * 0.75))
    front = elevation(tree, "front", scale=detail)
    side = elevation(tree, "side", scale=detail)
    gap = 10
    width = sum(image.width for image in images) + gap * len(images)
    width += max(front.width, side.width)
    height = max(max(image.height for image in images), front.height + side.height + gap)

    panel = Image.new("RGB", (width, height), BACKGROUND)
    x = 0
    for image in images:
        panel.paste(image, (x, height - image.height))
        x += image.width + gap
    panel.paste(front, (x, height - (front.height + side.height + gap)))
    panel.paste(side, (x, height - side.height))
    return panel


def compose(panels: list[tuple[Image.Image, str]], title: str, path: Path, columns: int) -> None:
    pad = 14
    label_height = 24
    title_height = 34
    font = load_font(12)
    measure = ImageDraw.Draw(Image.new("RGB", (8, 8)))

    columns = min(columns, len(panels))
    rows = (len(panels) + columns - 1) // columns
    col_widths = []
    for column in range(columns):
        values = [max(panel.width, int(measure.textlength(label, font=font)) + 12)
            for index, (panel, label) in enumerate(panels) if index % columns == column]
        col_widths.append(max(values) if values else 0)
    row_heights = []
    for row in range(rows):
        values = [panel.height for index, (panel, _) in enumerate(panels) if index // columns == row]
        row_heights.append(max(values) if values else 0)

    width = sum(col_widths) + pad * (columns + 1)
    height = title_height + sum(row_heights) + rows * (label_height + pad) + pad
    canvas = Image.new("RGB", (width, height), BACKGROUND)
    draw = ImageDraw.Draw(canvas)
    draw.text((pad, 8), title, font=load_font(18), fill=FOREGROUND)

    y = title_height
    for row in range(rows):
        x = pad
        baseline = y + row_heights[row]
        for column in range(columns):
            index = row * columns + column
            if index >= len(panels):
                break
            panel, label = panels[index]
            canvas.paste(panel, (x, baseline - panel.height))
            draw.text((x, baseline + 6), label, font=font, fill=FOREGROUND)
            x += col_widths[column] + pad
        y = baseline + label_height + pad

    path.parent.mkdir(parents=True, exist_ok=True)
    canvas.save(path)
    print(f"wrote {path} ({canvas.width}x{canvas.height})")


def collect_paths(arguments: list[str]) -> list[Path]:
    roots = [Path(argument) for argument in arguments] if arguments else [FEATURE_ROOT]
    paths: list[Path] = []
    for root in roots:
        if root.is_dir():
            paths.extend(sorted(root.glob("*.json")))
        elif root.is_file():
            paths.append(root)
        else:
            raise SystemExit(f"not a feature file or directory: {root}")
    return paths


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("features", nargs="*", help="feature JSON files or directories")
    parser.add_argument("--out", type=Path, default=REPO_ROOT / "build" / "preview-features")
    parser.add_argument("--scale", type=int, default=20)
    parser.add_argument("--columns", type=int, default=4)
    parser.add_argument("--ascii", action="store_true", help="print the text silhouettes")
    args = parser.parse_args()

    paths = collect_paths(args.features)
    if not paths:
        raise SystemExit("no feature JSON found")

    panels = []
    for path in paths:
        tree = load_feature(path)
        lo, hi = tree.bounds()
        panel = make_panel(tree, args.scale, tree.is_mushroom())
        panel_path = args.out / f"{path.stem}.png"
        panel_path.parent.mkdir(parents=True, exist_ok=True)
        panel.save(panel_path)
        panels.append((panel, f"{path.stem}  h={hi[1] + 1} w={hi[0] - lo[0] + 1}"))
        if args.ascii:
            print(f"== {path.stem}")
            print("\n".join(ascii_view(tree)))
            print()
        print(f"wrote {panel_path}")

    compose(panels, "Feature preview", args.out / "_sheet.png", args.columns)


if __name__ == "__main__":
    main()
