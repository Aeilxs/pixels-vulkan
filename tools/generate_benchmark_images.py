#!/usr/bin/env python3

from __future__ import annotations

import argparse
import struct
import zlib
from pathlib import Path


BENCHMARKS = {
    "500k": (707, 707),    # 499 849 particles
    "1m":   (1024, 1024),  # 1 048 576 particles
    "2m":   (1448, 1448),  # 2 096 704 particles
    "4m":   (2048, 2048),  # 4 194 304 particles
}


def png_chunk(chunk_type: bytes, data: bytes) -> bytes:
    return (
        struct.pack(">I", len(data))
        + chunk_type
        + data
        + struct.pack(">I", zlib.crc32(chunk_type + data) & 0xFFFFFFFF)
    )


def pixel(x: int, y: int, width: int, height: int) -> tuple[int, int, int, int]:
    # Deterministic colorful gradient.
    # Alpha stays 255 everywhere so gap=1 gives exactly width * height particles.
    r = (x * 255) // max(width - 1, 1)
    g = (y * 255) // max(height - 1, 1)
    b = ((x + y) * 255) // max(width + height - 2, 1)

    return r, g, b, 255


def generate_png(path: Path, width: int, height: int) -> None:
    raw = bytearray()

    for y in range(height):
        # PNG filter type 0: no filtering.
        raw.append(0)

        for x in range(width):
            raw.extend(pixel(x, y, width, height))

    header = struct.pack(
        ">IIBBBBB",
        width,
        height,
        8,  # bit depth
        6,  # color type: RGBA
        0,  # compression
        0,  # filter
        0,  # interlace
    )

    png = bytearray(b"\x89PNG\r\n\x1a\n")
    png += png_chunk(b"IHDR", header)
    png += png_chunk(b"IDAT", zlib.compress(raw, level=9))
    png += png_chunk(b"IEND", b"")

    path.write_bytes(png)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate deterministic Pixel Storm benchmark PNGs."
    )
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default=Path("assets/benchmarks"),
        help="Output directory (default: assets/benchmarks)",
    )

    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)

    for name, (width, height) in BENCHMARKS.items():
        particle_count = width * height
        path = args.output / f"bench-{name}.png"

        print(
            f"Generating {path}: "
            f"{width}x{height} = {particle_count:,} particles"
        )

        generate_png(path, width, height)
        print("Ok.")

if __name__ == "__main__":
    main()