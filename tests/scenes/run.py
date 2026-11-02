#!/usr/bin/env python3
"""Run the scene scripts with ragexe_scenes and compare the captures with golden images.

    python3 tests/scenes/run.py --exe build/ragexe_scenes --game ~/ro --out /tmp/scenes
    python3 tests/scenes/run.py ... --golden ~/scene-golden            # compare
    python3 tests/scenes/run.py ... --golden ~/scene-golden --update   # accept the captures

The client runs in the game directory (--game), so the captures depend on that game data and on
the renderer; golden images belong to one data set and one machine type and are kept outside
the repository. Each run writes <out>/report.html with the golden, the capture and the
difference side by side. Exit status 1 when a scene fails or a capture differs.
"""
import argparse
import html
import os
import re
import shutil
import struct
import subprocess
import sys
import time
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
CAPTURED = re.compile(r"^scene: captured (.+\.png) \(frame (\d+)[^)]*\)$")


def read_png(path):
    """8-bit RGB or RGBA, not interlaced (what the harness and most tools write). Returns
    (width, height, channels, bytearray of rows without filter bytes)."""
    with open(path, "rb") as f:
        data = f.read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"{path}: not a PNG")
    pos, idat, header = 8, [], None
    while pos < len(data):
        length, kind = struct.unpack(">I4s", data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + length]
        if kind == b"IHDR":
            header = struct.unpack(">IIBBBBB", body)
        elif kind == b"IDAT":
            idat.append(body)
        elif kind == b"IEND":
            break
        pos += 12 + length
    width, height, depth, color, _, _, interlace = header
    channels = {2: 3, 6: 4}.get(color)
    if depth != 8 or channels is None or interlace:
        raise ValueError(f"{path}: only 8-bit RGB/RGBA PNGs without interlacing are supported")
    raw = zlib.decompress(b"".join(idat))
    stride = width * channels
    out = bytearray(stride * height)
    prev = bytearray(stride)
    for y in range(height):
        kind = raw[y * (stride + 1)]
        line = bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        if kind == 1:
            for i in range(channels, stride):
                line[i] = (line[i] + line[i - channels]) & 0xff
        elif kind == 2:
            for i in range(stride):
                line[i] = (line[i] + prev[i]) & 0xff
        elif kind == 3:
            for i in range(stride):
                left = line[i - channels] if i >= channels else 0
                line[i] = (line[i] + ((left + prev[i]) >> 1)) & 0xff
        elif kind == 4:
            for i in range(stride):
                a = line[i - channels] if i >= channels else 0
                b = prev[i]
                c = prev[i - channels] if i >= channels else 0
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                line[i] = (line[i] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 0xff
        out[y * stride:(y + 1) * stride] = line
        prev = line
    return width, height, channels, out


def write_png(path, width, height, rgb):
    rows = b"".join(b"\0" + bytes(rgb[y * width * 3:(y + 1) * width * 3]) for y in range(height))

    def chunk(kind, body):
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body))

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)) +
                chunk(b"IDAT", zlib.compress(rows, 6)) + chunk(b"IEND", b""))


def rgb_of(width, height, channels, pixels):
    if channels == 3:
        return pixels
    out = bytearray(width * height * 3)
    out[0::3], out[1::3], out[2::3] = pixels[0::4], pixels[1::4], pixels[2::4]
    return out


def compare(golden_path, actual_path, diff_path, tolerance):
    """Returns (differing pixels, largest channel difference, message). Differing pixels are
    drawn red over a dimmed copy of the capture in diff_path."""
    gw, gh, gc, gp = read_png(golden_path)
    aw, ah, ac, ap = read_png(actual_path)
    if (gw, gh) != (aw, ah):
        return gw * gh, 255, f"size {aw}x{ah}, golden {gw}x{gh}"
    g, a = rgb_of(gw, gh, gc, gp), rgb_of(aw, ah, ac, ap)
    if g == a:
        return 0, 0, "identical"
    diff = bytearray(x // 3 for x in a)
    count = largest = 0
    for i in range(0, len(a), 3):
        d = max(abs(a[i] - g[i]), abs(a[i + 1] - g[i + 1]), abs(a[i + 2] - g[i + 2]))
        if d > largest:
            largest = d
        if d > tolerance:
            count += 1
            diff[i:i + 3] = b"\xff\x00\x00"
    write_png(diff_path, aw, ah, diff)
    return count, largest, f"{count} pixels differ by more than {tolerance} (largest difference {largest})"


def run_scene(args, script, out):
    env = dict(os.environ, RAGEXE_SCENE=os.path.abspath(script), RAGEXE_SCENE_OUT=os.path.abspath(out))
    started = time.monotonic()
    try:
        proc = subprocess.run([os.path.abspath(args.exe)], cwd=args.game, env=env, stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT, timeout=args.timeout)
        output, status = proc.stdout.decode("utf-8", "replace"), proc.returncode
    except subprocess.TimeoutExpired as e:
        output, status = (e.stdout or b"").decode("utf-8", "replace") + "\n(timed out)", -1
    captures = [(m.group(1), int(m.group(2))) for m in map(CAPTURED.match, output.splitlines()) if m]
    log = os.path.join(out, os.path.splitext(os.path.basename(script))[0] + ".log")
    with open(log, "w", encoding="utf-8") as f:
        f.write(output)
    return status, captures, time.monotonic() - started, log


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--exe", required=True, help="the ragexe_scenes executable")
    parser.add_argument("--game", required=True, help="the game directory the client runs in")
    parser.add_argument("--out", required=True, help="directory for captures, logs, diffs and report.html")
    parser.add_argument("--scripts", default=os.path.join(HERE, "scripts"), help="directory of scene scripts")
    parser.add_argument("--golden", help="directory of golden images to compare with")
    parser.add_argument("--update", action="store_true", help="copy the captures into --golden")
    parser.add_argument("--tolerance", type=int, default=0, help="per-channel difference still counted equal")
    parser.add_argument("--max-pixels", type=int, default=0, help="differing pixels still accepted")
    parser.add_argument("--timeout", type=int, default=600, help="seconds per scene")
    parser.add_argument("scenes", nargs="*", help="scene names (default: every script)")
    args = parser.parse_args()
    if args.update and not args.golden:
        parser.error("--update needs --golden")

    scripts = sorted(f for f in os.listdir(args.scripts) if f.endswith(".lua"))
    if args.scenes:
        scripts = [f for f in scripts if os.path.splitext(f)[0] in args.scenes]
    os.makedirs(args.out, exist_ok=True)
    if args.golden:
        os.makedirs(args.golden, exist_ok=True)

    rows, failed = [], 0
    for name in scripts:
        status, captures, seconds, log = run_scene(args, os.path.join(args.scripts, name), args.out)
        scene = os.path.splitext(name)[0]
        if status != 0 or not captures:
            failed += 1
            rows.append((scene, None, "fail", f"exit status {status}, {len(captures)} captures", log))
            print(f"FAIL {scene}: exit status {status} ({os.path.relpath(log)})")
            continue
        for path, frame in captures:
            capture = os.path.basename(path)
            golden = os.path.join(args.golden, capture) if args.golden else None
            if args.update:
                shutil.copyfile(path, golden)
                verdict, message = "updated", f"frame {frame}, golden updated"
            elif golden and os.path.exists(golden):
                diff = os.path.join(args.out, os.path.splitext(capture)[0] + ".diff.png")
                count, _, message = compare(golden, path, diff, args.tolerance)
                verdict = "pass" if count <= args.max_pixels else "fail"
                failed += verdict == "fail"
            elif golden:
                verdict, message = "new", "no golden image yet"
            else:
                verdict, message = "captured", f"frame {frame}"
            rows.append((scene, capture, verdict, message, log))
            print(f"{verdict.upper():8} {scene}/{capture}: {message} ({seconds:.0f} s)")

    write_report(args, rows)
    print(f"report: {os.path.join(os.path.abspath(args.out), 'report.html')}")
    return 1 if failed else 0


def write_report(args, rows):
    out = os.path.abspath(args.out)
    colors = {"pass": "#2e7d32", "fail": "#c62828", "new": "#ef6c00", "updated": "#1565c0", "captured": "#455a64"}

    def img(path, label):
        if not path or not os.path.exists(path):
            return f"<figure><div class=none>no {label}</div><figcaption>{label}</figcaption></figure>"
        rel = html.escape(os.path.relpath(path, out).replace(os.sep, "/"))
        return f"<figure><a href='{rel}'><img src='{rel}' loading=lazy></a><figcaption>{label}</figcaption></figure>"

    parts = []
    for scene, capture, verdict, message, log in rows:
        head = (f"<h2><span class=badge style='background:{colors[verdict]}'>{verdict}</span> "
                f"{html.escape(scene)}{' / ' + html.escape(capture) if capture else ''}</h2>"
                f"<p>{html.escape(message)} &middot; <a href='{html.escape(os.path.relpath(log, out))}'>log</a></p>")
        figs = ""
        if capture:
            stem = os.path.splitext(capture)[0]
            golden = os.path.join(args.golden, capture) if args.golden else None
            figs = "<div class=row>" + img(golden, "golden") + img(os.path.join(out, capture), "capture")
            figs += img(os.path.join(out, stem + ".diff.png"), "difference") + "</div>"
        parts.append(f"<section>{head}{figs}</section>")
    page = ("<!doctype html><meta charset=utf-8><meta name=viewport content='width=device-width'>"
            "<title>Scene captures</title><style>"
            ":root{color-scheme:light dark;--bg:#fafafa;--fg:#222;--card:#fff;--line:#ddd}"
            "@media (prefers-color-scheme:dark){:root{--bg:#151515;--fg:#e8e8e8;--card:#202020;--line:#333}}"
            "body{margin:0;padding:16px;background:var(--bg);color:var(--fg);font:14px system-ui,sans-serif}"
            "section{background:var(--card);border:1px solid var(--line);border-radius:8px;padding:12px;margin:0 0 16px}"
            "h2{font-size:16px;margin:0 0 4px}.badge{color:#fff;border-radius:4px;padding:1px 6px;font-size:12px;"
            "text-transform:uppercase;margin-right:6px}.row{display:grid;grid-template-columns:repeat(auto-fit,"
            "minmax(260px,1fr));gap:8px}figure{margin:0}img{width:100%;image-rendering:pixelated;display:block}"
            ".none{aspect-ratio:4/3;display:grid;place-items:center;border:1px dashed var(--line)}"
            "figcaption{font-size:12px;opacity:.7}a{color:inherit}</style>"
            "<h1 style='font-size:20px'>Scene captures</h1>" + "".join(parts))
    with open(os.path.join(out, "report.html"), "w", encoding="utf-8") as f:
        f.write(page)


if __name__ == "__main__":
    sys.exit(main())
