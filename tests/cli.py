"""CLI integration checks using only Python's standard library."""
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import zlib

binary = str(Path(sys.argv[1]).resolve())
fonts = Path(sys.argv[2]).resolve()
env = os.environ.copy()
env.pop("SBTEX_FONTS_DIR", None)


def run(*args, ok=True, extra_env=None):
    result = subprocess.run([binary, *args], cwd=work, env=env | (extra_env or {}), capture_output=True)
    assert (result.returncode == 0) == ok, (args, result.returncode, result.stderr)
    return result


def png(data, expected):
    assert data[:8] == b"\x89PNG\r\n\x1a\n"
    assert struct.unpack(">II", data[16:24]) == expected
    offset, compressed = 8, b""
    while offset < len(data):
        size = struct.unpack(">I", data[offset:offset + 4])[0]
        tag = data[offset + 4:offset + 8]
        payload = data[offset + 8:offset + 8 + size]
        crc = struct.unpack(">I", data[offset + 8 + size:offset + 12 + size])[0]
        assert zlib.crc32(tag + payload) == crc
        if tag == b"IDAT":
            compressed += payload
        offset += size + 12
    raw = zlib.decompress(compressed)
    assert raw
    # Decode Cairo's RGB/RGBA output to check rendered colours, including
    # the white paper in RGB fonts and transparent pixels in RGBA fonts.
    assert data[24] == 8 and data[25] in (2, 6)
    channels = 3 if data[25] == 2 else 4
    stride = expected[0] * channels
    previous = bytearray(stride)
    rows = []
    for y in range(expected[1]):
        start = y * (stride + 1)
        kind = raw[start]
        row = bytearray(raw[start + 1:start + 1 + stride])
        for x in range(stride):
            left = row[x - channels] if x >= channels else 0
            above = previous[x]
            corner = previous[x - channels] if x >= channels else 0
            p = left + above - corner
            distances = [abs(p - left), abs(p - above), abs(p - corner)]
            paeth = (left, above, corner)[distances.index(min(distances))]
            row[x] = (row[x] + (0, left, above, (left + above) // 2, paeth)[kind]) & 255
        rows.append([tuple(row[x:x + 3]) for x in range(0, stride, channels)])
        previous = row
    return rows


with tempfile.TemporaryDirectory() as directory:
    work = Path(directory)
    assert b"Usage:" in run("--help").stdout
    for flag in ("-i", "-o", "-f", "--fonts-dir", "--columns", "-bg", "-color"):
        assert b"missing value" in run(flag, ok=False).stderr
    run(ok=False)
    run("--unknown", ok=False)
    for value in ("0", "-1", "129", "3x", "999999999999"):
        run("-i", "A", "--columns", value, ok=False)
    run("-i", "A", "-f", "../sans", ok=False)
    run("-i", "A", "-f", "absent", ok=False)
    run("-i", "A", "--fonts-dir", str(work / "absent"), ok=False)
    run("-i", "A", extra_env={"SBTEX_FONTS_DIR": str(work / "absent")}, ok=False)
    png(run("-i", "abc", "-v").stdout, (340, 140))
    assert run("-i", "abc").stdout == run("-i", "ABC").stdout
    for flag in ("-bg", "-color"):
        for value in ("", "fff", "#ff", "#ffff", "#1234567", "#ggg", "#12 456", "#-ff"):
            assert b"hex colour" in run("-i", "A", flag, value, ok=False).stderr
    short = run("-i", "AB", "-bg", "#123", "-color", "#FaC").stdout
    long = run("-i", "AB", "--bg", "#112233", "--color", "#ffaacc").stdout
    assert short == long
    rows = png(short, (240, 140))
    assert rows[0][0] == (17, 34, 51)
    for start in (20, 120):  # A is RGB, B is RGBA.
        pixels = {pixel for row in rows[20:120] for pixel in row[start:start + 100]}
        if start == 20:
            assert (255, 170, 204) in pixels
        else:  # B contains coloured ink, so its luminance affects coverage.
            assert any(r > 100 for r, g, b in pixels)
        assert (17, 34, 51) in pixels
        assert (255, 255, 255) not in pixels
    blank = png(run("-i", "", "-bg", "#abc").stdout, (140, 140))
    assert all(pixel == (170, 187, 204) for row in blank for pixel in row)
    foreground = png(run("-i", "A", "-color", "#f00").stdout, (140, 140))
    assert foreground[0][0] == (255, 255, 255)
    assert any((255, 0, 0) in row for row in foreground)
    background = png(run("-i", "A", "-bg", "#abc").stdout, (140, 140))
    assert any((0, 0, 0) in row for row in background)
    png(run("-i", "ABCD", "--columns", "2").stdout, (240, 240))
    png(run("-i", "A\nB").stdout, (140, 240))
    png(run("-i", "").stdout, (140, 140))
    fallback = run("-i", "JQ?")
    png(fallback.stdout, (340, 140))
    assert fallback.stderr.count(b"missing glyph") == 3
    run("-i", "é", ok=False)
    run("-i", "\n" * 256, ok=False)
    run("-i", "A" * 32768, "--columns", "128", ok=False)
    output = work / "render.png"
    run("-i", "AB", "-o", str(output))
    png(output.read_bytes(), (240, 140))
    run("-i", "A", "-o", str(work), ok=False)
    custom = work / "custom fonts"
    shutil.copytree(fonts, custom)
    png(run("-i", "A", extra_env={"SBTEX_FONTS_DIR": str(custom)}).stdout, (140, 140))
    png(run("-i", "A", "--fonts-dir", str(custom),
            extra_env={"SBTEX_FONTS_DIR": "/nonexistent"}).stdout, (140, 140))
    (custom / "sans/glyphs/A.png").write_bytes(b"broken PNG")
    output.write_bytes(b"keep existing file")
    error = run("-i", "A", "--fonts-dir", str(custom), "-o", str(output), ok=False)
    assert b"A.png" in error.stderr
    assert output.read_bytes() == b"keep existing file"
    (custom / "sans/glyphs/A.png").unlink()
    png(run("-i", "A", "--fonts-dir", str(custom)).stdout, (140, 140))
    (custom / "_fail.png").unlink()
    assert b"_fail.png" in run("-i", "A", "--fonts-dir", str(custom), ok=False).stderr
print("CLI, PNG, font discovery, fallback, and error checks passed")
