#!/usr/bin/env python3
"""Generate native application icons from the approved Qwertycoin SVG mark."""

from __future__ import annotations

import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MARK = ROOT / "images" / "brand" / "qwertycoin-mark.svg"
WORDMARK = ROOT / "images" / "brand" / "qwertycoin-wordmark-light.svg"
ICON_DIR = ROOT / "images" / "appicons"


def render(ffmpeg: str, source: Path, width: int, height: int, output: Path) -> None:
    subprocess.run(
        [
            ffmpeg,
            "-hide_banner",
            "-loglevel",
            "error",
            "-i",
            str(source),
            "-vf",
            f"scale={width}:{height}:flags=lanczos",
            "-frames:v",
            "1",
            "-y",
            str(output),
        ],
        check=True,
    )


def write_icns(output: Path, pngs: dict[int, Path]) -> None:
    chunk_types = {
        16: b"icp4",
        32: b"icp5",
        64: b"icp6",
        128: b"ic07",
        256: b"ic08",
        512: b"ic09",
        1024: b"ic10",
    }
    chunks = []
    for size, chunk_type in chunk_types.items():
        payload = pngs[size].read_bytes()
        chunks.append(chunk_type + struct.pack(">I", len(payload) + 8) + payload)
    body = b"".join(chunks)
    output.write_bytes(b"icns" + struct.pack(">I", len(body) + 8) + body)


def main() -> int:
    convert = shutil.which("convert")
    ffmpeg = shutil.which("ffmpeg")
    if not convert or not ffmpeg:
        print("FFmpeg and ImageMagick 'convert' are required", file=sys.stderr)
        return 1
    if not MARK.is_file() or not WORDMARK.is_file():
        print("Approved Qwertycoin SVG sources are missing", file=sys.stderr)
        return 1

    ICON_DIR.mkdir(parents=True, exist_ok=True)
    sizes = (16, 24, 32, 48, 64, 96, 128, 192, 256, 512, 1024)
    generated: dict[int, Path] = {}
    for size in sizes:
        output = ICON_DIR / f"{size}x{size}.png"
        render(ffmpeg, MARK, size, size, output)
        generated[size] = output

    shutil.copyfile(generated[192], ROOT / "images" / "qwertycoin-icon-192.png")
    render(ffmpeg, WORDMARK, 800, 160, ROOT / "images" / "qwertycoin-logo.png")

    with tempfile.TemporaryDirectory(prefix="qwc-icon-") as temporary:
        ico = Path(temporary) / "appicon.ico"
        subprocess.run(
            [convert]
            + [str(generated[size]) for size in (16, 24, 32, 48, 64, 96, 128, 256)]
            + [str(ico)],
            check=True,
        )
        shutil.copyfile(ico, ROOT / "images" / "appicon.ico")

    write_icns(ROOT / "images" / "appicon.icns", generated)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
