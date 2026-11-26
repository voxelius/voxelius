#!/usr/bin/env python3

"""
This is vibecoded, here be dragons

Generate still/flowing water PNGs via the Minecraft Classic water CA.

https://github.com/ClassiCube/ClassiCube/wiki/MInecraft-Classic-lava-animation-algorithm#water

Still:
  localSoup = sum of horizontal 3-neighbourhood (wrap)
  soup'     = localSoup / 3.3 + pot * 0.8
  pot'      = max(0, pot + flame * 0.05)
  flame'    = flame - 0.1 ; with probability 0.05 set to 0.5

Flowing (TextureWaterFlowFX):
  localSoup = sum of vertical neighbourhood y-2..y (wrap)
  soup'     = localSoup / 3.2 + pot * 0.8
  pot'      = max(0, pot + flame * 0.05)
  flame'    = flame - 0.3 ; with probability 0.2 set to 0.5
  sample soup at (i - tick * size) when coloring (1px/frame downward scroll)

Color (both):
  h = clamp(soup, 0, 1); h2 = h*h
  R = 32 + h2*32 ; G = 50 + h2*64 ; B = 255 ; A = 146 + h2*50

Looping (must be exact — oceans tile the same atlas everywhere):
  Any crossfade/dissolve becomes a world-wide flash when many water tiles animate
  in lockstep. So we do not blend frames.

  Instead: pre-roll one period of Classic-style random flame pulses, replay that
  tape forever, warm up onto the N-periodic orbit, and capture one period.
  Frame N then matches frame 0 exactly (CA + scroll).

  The long-run orbit is over-bright; a soft potHeat cap (--pot-cap) keeps the
  orbit in the same contrast range as the early transient without breaking
  periodicity.

Default --frames 32 matches data/builtin/scripts/fluids/water.lua.
Flowing frame count must be a multiple of size so the scroll phase also closes.
"""

from __future__ import annotations

import argparse
import math
import random
import struct
import zlib
from pathlib import Path



def write_png_rgba(path: Path, width: int, height: int, pixels: bytes) -> None:
    def chunk(tag: bytes, data: bytes) -> bytes:
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    raw = bytearray()
    stride = width * 4
    for y in range(height):
        raw.append(0)
        raw.extend(pixels[y * stride : (y + 1) * stride])

    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + chunk(b"IEND", b"")
    path.write_bytes(png)


def nn_upscale_rgba(src: bytes, src_w: int, src_h: int, scale: int) -> bytes:
    dst_w = src_w * scale
    dst_h = src_h * scale
    out = bytearray(dst_w * dst_h * 4)
    for y in range(dst_h):
        sy = y // scale
        for x in range(dst_w):
            sx = x // scale
            si = (sy * src_w + sx) * 4
            di = (y * dst_w + x) * 4
            out[di : di + 4] = src[si : si + 4]
    return bytes(out)


def tile_2x2_rgba(src: bytes, size: int) -> bytes:
    out_w = size * 2
    out = bytearray(out_w * out_w * 4)
    for y in range(out_w):
        sy = y % size
        for x in range(out_w):
            sx = x % size
            si = (sy * size + sx) * 4
            di = (y * out_w + x) * 4
            out[di : di + 4] = src[si : si + 4]
    return bytes(out)


def write_debug_gif(path: Path, frames_rgba: list[bytes], src_size: int, fps: int = 10) -> None:
    try:
        from PIL import Image
    except ImportError as exc:
        raise SystemExit("Pillow is required for --debug-gif (pip install pillow)") from exc

    tiled_size = src_size * 2
    if 512 % tiled_size != 0:
        raise SystemExit(f"--debug-gif needs 512 divisible by 2*size (got size={src_size})")
    up = 512 // tiled_size
    duration_ms = int(round(1000 / fps))

    images: list[Image.Image] = []
    for rgba in frames_rgba:
        tiled = tile_2x2_rgba(rgba, src_size)
        big = nn_upscale_rgba(tiled, tiled_size, tiled_size, up)
        img = Image.frombytes("RGBA", (512, 512), big)
        bg = Image.new("RGBA", (512, 512), (20, 40, 80, 255))
        composed = Image.alpha_composite(bg, img).convert("P", palette=Image.ADAPTIVE, colors=256)
        images.append(composed)

    images[0].save(path, save_all=True, append_images=images[1:], duration=duration_ms, loop=0, optimize=False)



def soup_to_rgba(soup: float, alpha_min: int, alpha_span: int, grayscale: bool = False) -> tuple[int, int, int, int]:
    h = 0.0 if soup < 0.0 else 1.0 if soup > 1.0 else soup
    h2 = h * h
    r = int(32.0 + h2 * 32.0)
    g = int(50.0 + h2 * 64.0)
    b = 255
    a = int(alpha_min + h2 * alpha_span)
    
    if grayscale:
        lum = int(0.299 * r + 0.587 * g + 0.114 * b)
        r = g = b = lum

    if a < 0:
        a = 0
    elif a > 255:
        a = 255
    return r, g, b, a


def render_soup(
    soup: list[float],
    size: int,
    alpha_min: int,
    alpha_span: int,
    scroll: int = 0,
    grayscale: bool = False,
) -> bytes:
    n = size * size
    out = bytearray(n * 4)
    for i in range(n):
        src = (i - scroll * size) % n
        r, g, b, a = soup_to_rgba(soup[src], alpha_min, alpha_span, grayscale)
        o = i * 4
        out[o] = r
        out[o + 1] = g
        out[o + 2] = b
        out[o + 3] = a
    return bytes(out)


def make_pulse_tape(period: int, cells: int, chance: float, seed: int) -> list[list[bool]]:
    rng = random.Random(seed)
    return [[rng.random() < chance for _ in range(cells)] for _ in range(period)]


def step_still(
    soup: list[float],
    soup_next: list[float],
    pot: list[float],
    flame: list[float],
    size: int,
    pulses: list[bool],
    pot_cap: float,
) -> None:
    for x in range(size):
        for y in range(size):
            local = 0.0
            for xx in (x - 1, x, x + 1):
                local += soup[(xx & (size - 1)) + y * size]
            soup_next[x + y * size] = local / 3.3 + pot[x + y * size] * 0.8

    n = size * size
    for i in range(n):
        pot[i] += flame[i] * 0.05
        if pot[i] < 0.0:
            pot[i] = 0.0
        if pot[i] > pot_cap:
            pot[i] = pot_cap
        flame[i] -= 0.1
        if pulses[i]:
            flame[i] = 0.5

    soup[:], soup_next[:] = soup_next[:], soup[:]


def step_flowing(
    soup: list[float],
    soup_next: list[float],
    pot: list[float],
    flame: list[float],
    size: int,
    pulses: list[bool],
    pot_cap: float,
) -> None:
    for x in range(size):
        for y in range(size):
            local = 0.0
            for yy in range(y - 2, y + 1):
                local += soup[x + (yy & (size - 1)) * size]
            soup_next[x + y * size] = local / 3.2 + pot[x + y * size] * 0.8

    n = size * size
    for i in range(n):
        pot[i] += flame[i] * 0.05
        if pot[i] < 0.0:
            pot[i] = 0.0
        if pot[i] > pot_cap:
            pot[i] = pot_cap
        flame[i] -= 0.3
        if pulses[i]:
            flame[i] = 0.5

    soup[:], soup_next[:] = soup_next[:], soup[:]


def simulate(
    *,
    size: int,
    frames: int,
    seed: int,
    warmup_periods: int,
    pot_cap: float,
    flowing: bool,
    alpha_min: int,
    alpha_span: int,
    grayscale: bool = False,
) -> list[bytes]:
    n = size * size
    soup = [0.0] * n
    soup_next = [0.0] * n
    pot = [0.0] * n
    flame = [0.0] * n

    chance = 0.2 if flowing else 0.05
    tape = make_pulse_tape(frames, n, chance, seed)
    step = step_flowing if flowing else step_still

    tick = 0
    for _ in range(warmup_periods * frames):
        step(soup, soup_next, pot, flame, size, tape[tick % frames], pot_cap)
        tick += 1

    # Exact period on the attractor: frame N matches frame 0 (no dissolve).
    out: list[bytes] = []
    for _ in range(frames):
        step(soup, soup_next, pot, flame, size, tape[tick % frames], pot_cap)
        tick += 1
        scroll = tick if flowing else 0
        out.append(render_soup(soup, size, alpha_min, alpha_span, scroll=scroll, grayscale=grayscale))
    return out



def generate(
    out_dir: Path,
    frames: int,
    size: int,
    seed: int,
    warmup_periods: int,
    pot_cap: float,
    still_prefix: str,
    flowing_prefix: str,
    alpha_min: int,
    alpha_span: int,
    debug_gif: bool,
    grayscale: bool,
) -> None:
    if size & (size - 1):
        raise SystemExit(f"--size must be a power of two (Classic uses bitmask wrap; got {size})")
    if frames % size != 0:
        raise SystemExit(
            f"--frames ({frames}) must be a multiple of --size ({size}) "
            "so flowing scroll phase loops"
        )

    out_dir.mkdir(parents=True, exist_ok=True)
    digits = max(2, int(math.log10(max(frames, 1))) + 1)

    still = simulate(
        size=size,
        frames=frames,
        seed=seed,
        warmup_periods=warmup_periods,
        pot_cap=pot_cap,
        flowing=False,
        alpha_min=alpha_min,
        alpha_span=alpha_span,
        grayscale=grayscale,
    )
    for i, rgba in enumerate(still):
        path = out_dir / f"{still_prefix}_{i + 1:0{digits}d}.png"
        write_png_rgba(path, size, size, rgba)
        print(f"wrote {path.name}")

    flowing_frames = simulate(
        size=size,
        frames=frames,
        seed=seed ^ 0xA5A5A5A5,
        warmup_periods=warmup_periods,
        pot_cap=pot_cap,
        flowing=True,
        alpha_min=alpha_min,
        alpha_span=alpha_span,
        grayscale=grayscale,
    )
    for i, rgba in enumerate(flowing_frames):
        path = out_dir / f"{flowing_prefix}_{i + 1:0{digits}d}.png"
        write_png_rgba(path, size, size, rgba)
        print(f"wrote {path.name}")

    if debug_gif:
        still_gif = out_dir / f"{still_prefix}_debug.gif"
        flowing_gif = out_dir / f"{flowing_prefix}_debug.gif"
        write_debug_gif(still_gif, still, size, fps=10)
        write_debug_gif(flowing_gif, flowing_frames, size, fps=10)
        print(f"wrote {still_gif.name} / {flowing_gif.name} (512x512 @ 10 FPS, 2x2 tiled)")

    print(f"still: {frames} frames (periodic tape, warmup={warmup_periods}, pot_cap={pot_cap})")
    print(f"flowing: {frames} frames (periodic tape + scroll, pot_cap={pot_cap})")
    print(f"alpha: {alpha_min} .. {alpha_min + alpha_span}")


def main() -> None:
    repo = Path(__file__).resolve().parents[1]
    default_out = repo / "data" / "builtin" / "textures" / "block" / "water"

    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("-o", "--out-dir", type=Path, default=default_out)
    parser.add_argument(
        "-n",
        "--frames",
        type=int,
        default=32,
        help="loop length (default: 32, matches water.lua; multiple of --size)",
    )
    parser.add_argument("--size", type=int, default=16, help="texture size, power of two (Classic: 16)")
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument(
        "--warmup",
        type=int,
        default=48,
        help="warmup periods before capture (default: 48)",
    )
    parser.add_argument(
        "--pot-cap",
        type=float,
        default=0.18,
        help="max potHeat (default: 0.18); lower = darker/more contrast, higher = brighter",
    )
    parser.add_argument("--still-prefix", default="water_still")
    parser.add_argument("--flowing-prefix", default="water_flowing")
    parser.add_argument(
        "--alpha-min",
        type=int,
        default=146,
        help="alpha at heat=0 (default: 146, Classic)",
    )
    parser.add_argument(
        "--alpha-max",
        type=int,
        default=196,
        help="alpha at heat=1 (default: 196 = 146+50, Classic)",
    )
    parser.add_argument(
        "--alpha",
        type=int,
        default=None,
        help="constant alpha (overrides --alpha-min/--alpha-max)",
    )
    parser.add_argument(
        "--debug-gif",
        action="store_true",
        help="also write 512x512 debug GIFs at 10 FPS (2x2 tiled NN upscale; needs Pillow)",
    )
    parser.add_argument(
        "--grayscale",
        action="store_true",
        help="generate grayscale textures instead of standard color",
    )
    args = parser.parse_args()

    if args.frames < 1:
        parser.error("--frames must be >= 1")
    if args.size < 1:
        parser.error("--size must be >= 1")
    if args.warmup < 0:
        parser.error("--warmup must be >= 0")
    if args.pot_cap <= 0.0:
        parser.error("--pot-cap must be > 0")

    if args.alpha is not None:
        alpha_min = args.alpha
        alpha_span = 0
    else:
        alpha_min = args.alpha_min
        alpha_span = args.alpha_max - args.alpha_min

    for label, value in (("alpha-min", alpha_min), ("alpha-max", args.alpha_max), ("alpha", args.alpha)):
        if value is not None and not (0 <= value <= 255):
            parser.error(f"--{label} must be in 0..255")

    generate(
        out_dir=args.out_dir,
        frames=args.frames,
        size=args.size,
        seed=args.seed,
        warmup_periods=args.warmup,
        pot_cap=args.pot_cap,
        still_prefix=args.still_prefix,
        flowing_prefix=args.flowing_prefix,
        alpha_min=alpha_min,
        alpha_span=alpha_span,
        debug_gif=args.debug_gif,
        grayscale=args.grayscale,
    )


if __name__ == "__main__":
    main()
