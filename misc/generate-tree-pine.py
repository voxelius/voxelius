#!/usr/bin/env python3

"""
This is vibecoded, here be dragons. 

Generate pine/spruce-like tree feature JSON.

Unlike oak (stem + box canopy), pine uses a tapered layered crown:
bare lower trunk, then solid leaf tiers that narrow toward a tip.
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


def leaf_offsets(radius: int) -> list[tuple[int, int]]:
    """Solid canopy disk for one tier. Stem center (0,0) is omitted."""
    offsets: list[tuple[int, int]] = []

    if radius <= 0:
        return offsets

    for x in range(-radius, radius + 1):
        for z in range(-radius, radius + 1):
            if x == 0 and z == 0:
                continue

            manhattan = abs(x) + abs(z)
            chebyshev = max(abs(x), abs(z))

            # Filled diamond footprint; drop far corners on wide tiers only.
            if radius >= 3 and manhattan > radius + 1:
                continue
            if chebyshev > radius:
                continue

            offsets.append((x, z))

    return offsets


def radius_sequence(height: int) -> list[int]:
    """
    Per-tier radius from bottom of crown to tip (excluding tip).
    Wider at the base, alternating widths for a stepped spruce silhouette,
    always solid (no holes inside a tier).
    """
    crown_layers = max(height - max(height // 3, 2), 3)
    seq: list[int] = []

    for i in range(crown_layers):
        t = i / max(crown_layers - 1, 1)
        if t < 0.35:
            base = 3
        elif t < 0.7:
            base = 2
        else:
            base = 1

        if i % 2 == 1 and base > 1:
            base -= 1

        seq.append(base)

    return seq


def compute_tree_feature(height: int) -> list[dict]:
    height = max(height, 5)
    # Match oak generator: dirt at y=0, stem from y=1.
    y0 = 1
    parts: list[dict] = []

    parts.append(part(DIRT, 0, -1 + y0, 0, OVERWRITE_DIRT))

    for i in range(height):
        parts.append(part(LOG, 0, i + y0, 0, OVERWRITE_WOOD))

    crown_start = max(height // 3, 2) - 1
    tiers = radius_sequence(height)

    for tier_i, radius in enumerate(tiers):
        y = crown_start + tier_i + y0
        if y > height + y0 - 1:
            break
        for x, z in leaf_offsets(radius):
            parts.append(part(LEAVES, x, y, z, OVERWRITE_WOOD))

    parts.append(part(LEAVES, 0, height + y0, 0, OVERWRITE_WOOD))

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
        default=7,
        help="Stem height in blocks (clamped to >= 5, default: 7)",
    )
    parser.add_argument(
        "--dirt",
        default="builtin:dirt",
        help="Dirt palette block (default: builtin:dirt)",
    )
    parser.add_argument(
        "--log",
        default="builtin:pine_log",
        help="Log palette block (default: builtin:pine_log)",
    )
    parser.add_argument(
        "--leaves",
        default="builtin:pine_leaves",
        help="Leaves palette block (default: builtin:pine_leaves)",
    )
    args = parser.parse_args()

    feature = build_feature(args.height, args.dirt, args.log, args.leaves)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        json.dumps(feature, separators=(",", ":")) + "\n",
        encoding="utf-8",
        newline="\n",
    )
    print(f"Wrote {args.output} ({len(feature['parts'])} parts, height={max(args.height, 5)})")


if __name__ == "__main__":
    main()
