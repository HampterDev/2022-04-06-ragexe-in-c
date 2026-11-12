#!/usr/bin/env python3
"""tools/shaders/check.py: fails when a shader header under src/render/shaders or tests/shaders is
not what tools/shaders made from the sources there now: a <name>.hlsl whose source (with the files it
includes) hashes otherwise than the `source` line of <name>.h records, a tools/shaders/shaders.c
(the tool, with its flags and binding rules) that hashes otherwise than its `tool` line, an output
after the header's comment that hashes otherwise than its `output` line (an edit by hand), a
source without a header, or a header without a source. Regenerate with tools\\shaders\\build.bat.
Runs in `build.bat check` and as the test shader_sources."""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DIRECTORIES = [os.path.join(ROOT, "src", "render", "shaders"), os.path.join(ROOT, "tests", "shaders")]
TOOL = os.path.join(ROOT, "tools", "shaders", "shaders.c")
INCLUDE = re.compile(rb'#include "([^"]+)"')
LINE = re.compile(rb"^   (source|tool|output) ([0-9a-f]{16})(?: \*/)?\r?$", re.M)
BASIS = 0xCBF29CE484222325


def fnv(h, data):
    """FNV-1a, 64 bits, over text with its line ends made LF (tools/shaders/shaders.c)."""
    data = data.replace(b"\r\n", b"\n")
    for byte in data:
        h = ((h ^ byte) * 0x100000001B3) & 0xFFFFFFFFFFFFFFFF
    return h


def source_hash(directory, name, h, seen):
    """The file, then each file it includes (named from the directory of the file that includes it,
    as dxc and tools/shaders/shaders.c name them), depth first in the order they appear, each path
    once."""
    path = directory + "/" + name
    if path in seen:
        return h
    seen.add(path)
    with open(path, "rb") as f:
        text = f.read()
    h = fnv(h, text)
    for included in INCLUDE.findall(text):
        h = source_hash(path[:path.rindex("/")], included.decode(), h, seen)
    return h


def check(directory, tool, problems):
    names = sorted(os.listdir(directory))
    sources = [n[:-5] for n in names if n.endswith(".hlsl")]
    headers = [n[:-2] for n in names if n.endswith(".h") and n != "shader_code.h"]
    for base in sources:
        header = os.path.join(directory, base + ".h")
        if not os.path.exists(header):
            problems.append(f"{base}.hlsl has no {base}.h")
            continue
        with open(header, "rb") as f:
            text = f.read().replace(b"\r\n", b"\n")
        end = text.find(b"*/\n")
        found = {k.decode(): v.decode() for k, v in LINE.findall(text[:end + 2] if end >= 0 else b"")}
        want = {
            "source": "%016x" % source_hash(directory, base + ".hlsl", BASIS, set()),
            "tool": tool,
            "output": "%016x" % fnv(BASIS, text[end + 3:]) if end >= 0 else None,
        }
        what = {"source": f"made before {base}.hlsl or a file it includes changed",
                "tool": "made before tools/shaders/shaders.c changed", "output": "edited after it was made"}
        for key in ("source", "tool", "output"):
            if key not in found:
                problems.append(f"{base}.h has no {key} line")
            elif found[key] != want[key]:
                problems.append(f"{base}.h: {what[key]} ({key} {want[key]}, header {found[key]})")
    for base in headers:
        if base not in sources:
            problems.append(f"{base}.h has no {base}.hlsl")


def main():
    problems = []
    with open(TOOL, "rb") as f:
        tool = "%016x" % fnv(BASIS, f.read())
    for directory in DIRECTORIES:
        check(directory, tool, problems)
    for p in problems:
        print("check_shaders: " + p)
    if problems:
        print("check_shaders: regenerate with tools\\shaders\\build.bat")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
