#!/usr/bin/env python3
"""How steady the shadows stay in the motion scenes (tests/scenes/motion), from their logs.

    python3 tests/scenes/run.py --exe ... --game ... --out /tmp/motion --scripts tests/scenes/motion
    python3 tests/scenes/motion_stats.py /tmp/motion                 # one run
    python3 tests/scenes/motion_stats.py /tmp/motion /tmp/motion-old  # against another build's

Per motion (the tag of the log's "fit" lines): the frames, how often a cascade's square changes
size per 100 frames, how far the shadow edges can jump at those changes per 100 frames (a texel of
the larger square, world units), each cascade's mean texel, and the largest part of a texel a square
moved by between two frames it kept its size (0: the texels stayed put). With a second folder each
motion is shown for both, and the exit status is 1 where the first changes size more often than the
second on any motion.
"""
import os
import sys

MAP_SIZE = 2048  # render/gfx.h, GFX_SHADOW_SIZE


def read_fits(folder):
    """{tag: [(frame, cascades, [(radius, x, y, far)] * 4)]} from the folder's .log files"""
    out = {}
    for name in sorted(os.listdir(folder)):
        if not name.endswith(".log"):
            continue
        with open(os.path.join(folder, name), encoding="utf-8", errors="replace") as f:
            for line in f:
                p = line.split()
                if len(p) < 4 or p[0] != "fit" or p[3] == "none":
                    continue
                v = [float(x) for x in p[6:22]]
                squares = [tuple(v[k * 4:k * 4 + 4]) for k in range(4)]
                out.setdefault(p[1], []).append((int(p[2]), int(p[3]), squares))
    return out


def measure(rows):
    changes, jump, worst = 0, 0.0, 0.0
    texel = [[] for _ in range(4)]
    for i, (_, count, squares) in enumerate(rows):
        for k in range(4):
            if count and squares[k][0] > 0:
                texel[k].append(2 * squares[k][0] / MAP_SIZE)
        if i == 0:
            continue
        _, before, last = rows[i - 1]
        if not (count and before):
            continue
        for k in range(4):
            r0, r1 = last[k][0], squares[k][0]
            if r0 <= 0 or r1 <= 0:
                continue
            if r0 != r1:
                changes += 1
                jump += 2 * max(r0, r1) / MAP_SIZE
                continue
            t = 2 * r1 / MAP_SIZE
            for a in (1, 2):
                d = (squares[k][a] - last[k][a]) / t
                worst = max(worst, abs(d - round(d)))
    per = 100.0 / max(len(rows) - 1, 1)
    means = [sum(t) / len(t) if t else 0.0 for t in texel]
    return len(rows), changes * per, jump * per, means, worst


def line(tag, label, rows):
    n, changes, jump, texel, worst = measure(rows)
    print("%-14s %-6s %4d frames: %6.2f size changes /100 frames, edges jump %.3f units /100 frames; "
          "texel %s; kept squares off whole texels by %.2g" % (tag, label, n, changes, jump,
                                                               " ".join("%.3f" % t for t in texel), worst))
    return changes


def main():
    if len(sys.argv) not in (2, 3):
        print(__doc__)
        return 2
    first = read_fits(sys.argv[1])
    second = read_fits(sys.argv[2]) if len(sys.argv) == 3 else None
    more = []
    for tag in sorted(set(first) | set(second or {})):
        a = line(tag, "this", first[tag]) if tag in first else None
        if second is None:
            continue
        b = line(tag, "other", second[tag]) if tag in second else None
        if a is not None and b is not None and a > b:
            more.append(tag)
    if more:
        print("more size changes than the other build: " + ", ".join(more))
    return 1 if more else 0


if __name__ == "__main__":
    sys.exit(main())
