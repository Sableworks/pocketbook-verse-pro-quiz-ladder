#!/usr/bin/env python3
"""Write a high-contrast 256x256 launcher PNG (white field, black diamond, ladder)."""
from __future__ import annotations

import os
import struct
import zlib


def chunk(tag: bytes, data: bytes) -> bytes:
    crc = zlib.crc32(tag + data) & 0xFFFFFFFF
    return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", crc)


def main() -> None:
    n = 256
    rows = []
    cx = cy = n / 2
    for y in range(n):
        row = [255] * (n * 3)
        for x in range(n):
            dx = abs(x - cx) / 92.0
            dy = abs(y - cy + 8) / 118.0
            in_d = dx + dy <= 1.0
            edge = abs(dx + dy - 1.0) < 0.035
            ladder = False
            # two rails
            if 78 <= y <= 186 and ((88 <= x <= 100) or (156 <= x <= 168)):
                ladder = True
            # five rungs
            if 100 <= x <= 156:
                for ry in (86, 108, 130, 152, 174):
                    if ry <= y <= ry + 8:
                        ladder = True
            i = x * 3
            if in_d:
                row[i : i + 3] = (0, 0, 0) if not ladder else (255, 255, 255)
            if edge:
                row[i : i + 3] = (0, 0, 0)
        rows.append(b"\x00" + bytes(row))
    raw = b"".join(rows)
    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", n, n, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 9))
    png += chunk(b"IEND", b"")
    here = os.path.dirname(os.path.abspath(__file__))
    out = os.path.join(os.path.dirname(here), "quizladder.png")
    with open(out, "wb") as f:
        f.write(png)
    print("wrote", out, len(png), "bytes")


if __name__ == "__main__":
    main()
