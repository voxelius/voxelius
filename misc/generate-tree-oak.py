#!/usr/bin/env python3

"""
This is vibecoded, here be dragons. Well, not exactly vibecoded. The old
tree generator was written in C++ and was inside overworld.cc generator and
since we're currently only have one world type and features are now stored as
JSON files, the generator's code has been forward-ported by an LLM into a script

Generate tree feature JSON from the classic stem + canopy layout.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path


DIRT = 0
LOG = 1
LEAVES = 2

OVERWRITE_DIRT = ["turf", "xfoil"]
OVERWRITE_WOOD = ["gas", "xfoil"]


def part(block: int, x: int, y: int, z: int, overwrite: list[str]) -> dict:
    return {
        "block": block,
        "offset": [x, y, z],
        "overwrite": overwrite,
    }


def compute_tree_feature(height: int) -> list[dict]:
    height = max(height, 4)
    # C++ used dirt at y=-1 and stem from y=0; JSON places dirt at y=0.
    y0 = 1
    parts: list[dict] = []

    parts.append(part(DIRT, 0, -1 + y0, 0, OVERWRITE_DIRT))

    for i in range(height):
        parts.append(part(LOG, 0, i + y0, 0, OVERWRITE_WOOD))

    leaves_start = height - 3
    leaves_thick_end = height - 2
    leaves_thin_end = height - 1

    # Thin 3x3 ring (no center — stem occupies it).
    for i in range(leaves_start, leaves_thin_end + 1):
        y = i + y0
        for x, z in (
            (-1, -1), (-1, 0), (-1, 1),
            (0, -1),           (0, 1),
            (1, -1),  (1, 0),  (1, 1),
        ):
            parts.append(part(LEAVES, x, y, z, OVERWRITE_WOOD))

    y = height + y0
    for x, z in ((-1, 0), (0, -1), (0, 0), (0, 1), (1, 0)):
        parts.append(part(LEAVES, x, y, z, OVERWRITE_WOOD))

    for i in range(leaves_start, leaves_thick_end + 1):
        y = i + y0
        for x, z in (
            (-1, -2), (-1, 2),
            (-2, -1), (-2, -2), (-2, 0), (-2, 1), (-2, 2),
            (0, -2), (0, 2),
            (1, -2), (1, 2),
            (2, -1), (2, -2), (2, 0), (2, 1), (2, 2),
        ):
            parts.append(part(LEAVES, x, y, z, OVERWRITE_WOOD))

    return parts


def build_feature(height: int, dirt: str, log: str, leaves: str) -> dict:
    return {
        "palette": [
            {"block": dirt},
            {"block": log},
            {"block": leaves},
        ],
        "parts": compute_tree_feature(height),
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "-o", "--output",
        type=Path,
        required=True,
        help="Output feature JSON path",
    )
    parser.add_argument(
        "--height",
        type=int,
        default=4,
        help="Stem height in blocks (clamped to >= 4, default: 4)",
    )
    parser.add_argument(
        "--dirt",
        default="builtin:dirt",
        help="Dirt palette block (default: builtin:dirt)",
    )
    parser.add_argument(
        "--log",
        default="builtin:oak_log",
        help="Log palette block (default: builtin:oak_log)",
    )
    parser.add_argument(
        "--leaves",
        default="builtin:oak_leaves",
        help="Leaves palette block (default: builtin:oak_leaves)",
    )
    args = parser.parse_args()

    feature = build_feature(args.height, args.dirt, args.log, args.leaves)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        json.dumps(feature, separators=(",", ":")) + "\n",
        encoding="utf-8",
        newline="\n",
    )
    print(f"Wrote {args.output} ({len(feature['parts'])} parts, height={max(args.height, 4)})")


if __name__ == "__main__":
    main()
