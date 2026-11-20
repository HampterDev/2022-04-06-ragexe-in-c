#!/usr/bin/env python3
"""Run scenes under two memory layouts and fail where their frames differ.

    python3 tests/scenes/layouts.py --exe build/ragexe_scenes --game ~/ro --out /tmp/layouts
    python3 tests/scenes/layouts.py ... woe_emperium town_prontera          # other load scenes
    python3 tests/scenes/layouts.py ... --scripts tests/scenes/scripts       # the regular scenes

A scene drawn from recorded packets with a clock that moves one frame per frame must give the same
frames in every run. Each scene runs once with RAGEXE_SCENE_LAYOUT=1 and once with 2 (the harness
then takes and fills heap blocks of other sizes before the client starts and fills the stack below
it with another byte every frame), so an order kept by addresses or a read of memory nothing wrote
shows as a difference here rather than now and then. Compared: every capture's pixels, every
capture's counters and every measured frame's counters (scene.measure), the times left out. By
default the load scenes arrive_prontera and town_prontera (tests/scenes/load), half a minute in all,
at RAGEXE_MSAA=1. A read of memory nothing wrote need not differ in every pair of runs (what it reads
can be left over from the client's own work), so a scene that passes once may still fail later.
Exit status 1 when a run fails or anything differs.
"""
import argparse
import json
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from run import read_png  # noqa: E402

LAYOUTS = (1, 2)


def counts(record):
    """A counters object without its times."""
    return {k: v for k, v in record.items() if not k.endswith("_ms")}


def run_layout(args, scene, layout):
    out = os.path.join(args.out, scene, f"layout{layout}")
    os.makedirs(out, exist_ok=True)
    for name in os.listdir(out):
        os.remove(os.path.join(out, name))
    env = dict(os.environ, RAGEXE_SCENE=os.path.join(args.scripts, scene + ".lua"), RAGEXE_SCENE_OUT=out,
               RAGEXE_MSAA=str(args.msaa), RAGEXE_VSYNC="0", RAGEXE_SCENE_LAYOUT=str(layout))
    if not env.get("SDL_AUDIO_DRIVER") and not env.get("SDL_AUDIODRIVER"):
        env["SDL_AUDIO_DRIVER"] = "dummy"
    try:
        proc = subprocess.run([args.exe], cwd=args.game, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                              timeout=args.timeout)
        output, status = proc.stdout.decode("utf-8", "replace"), proc.returncode
    except subprocess.TimeoutExpired as e:
        output, status = (e.stdout or b"").decode("utf-8", "replace") + "\n(timed out)", -1
    with open(os.path.join(out, scene + ".log"), "w", encoding="utf-8") as f:
        f.write(output)
    return out, status


def differences(a, b):
    """What differs between the outputs of two runs of one scene."""
    found = []
    names = sorted(set(os.listdir(a)) | set(os.listdir(b)))
    for name in names:
        pa, pb = os.path.join(a, name), os.path.join(b, name)
        if name.endswith(".log"):
            continue
        if not os.path.exists(pa) or not os.path.exists(pb):
            found.append(f"{name}: written by one run only")
        elif name.endswith(".png"):
            ia, ib = read_png(pa), read_png(pb)
            if ia[:3] != ib[:3]:
                found.append(f"{name}: {ia[0]}x{ia[1]} / {ib[0]}x{ib[1]}")
            elif ia[3] != ib[3]:
                n = ia[2]
                pixels = sum(1 for i in range(0, len(ia[3]), n) if ia[3][i:i + n] != ib[3][i:i + n])
                found.append(f"{name}: {pixels} pixels differ")
        elif name.endswith(".counters.json"):
            with open(pa, encoding="utf-8") as fa, open(pb, encoding="utf-8") as fb:
                ca, cb = counts(json.load(fa)), counts(json.load(fb))
            if ca != cb:
                keys = sorted(k for k in set(ca) | set(cb) if ca.get(k) != cb.get(k))
                found.append(f"{name}: " + ", ".join(f"{k} {ca.get(k)} / {cb.get(k)}" for k in keys))
        elif name.endswith(".frames.jsonl"):
            with open(pa, encoding="utf-8") as fa, open(pb, encoding="utf-8") as fb:
                la = [counts(json.loads(line)) for line in fa if line.strip()]
                lb = [counts(json.loads(line)) for line in fb if line.strip()]
            if len(la) != len(lb):
                found.append(f"{name}: {len(la)} / {len(lb)} frames")
            for x, y in zip(la, lb):
                if x != y:
                    keys = sorted(k for k in set(x) | set(y) if x.get(k) != y.get(k))
                    found.append(f"{name}: frame {x.get('frame')}: "
                                 + ", ".join(f"{k} {x.get(k)} / {y.get(k)}" for k in keys))
    return found


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--exe", required=True, help="the ragexe_scenes executable")
    parser.add_argument("--game", required=True, help="the game directory the client runs in")
    parser.add_argument("--out", required=True, help="directory for the runs")
    parser.add_argument("--scripts", default=os.path.join(HERE, "load"), help="directory of the scenes")
    parser.add_argument("--msaa", type=int, default=1, help="RAGEXE_MSAA for the runs")
    parser.add_argument("--timeout", type=int, default=900, help="seconds per run")
    parser.add_argument("names", nargs="*", help="scene names (default: arrive_prontera town_prontera)")
    args = parser.parse_args()
    # the client runs in --game, so every path it is given is absolute
    args.exe, args.game, args.out, args.scripts = (os.path.abspath(p) for p in (args.exe, args.game, args.out,
                                                                                args.scripts))
    failed = 0
    for scene in args.names or ["arrive_prontera", "town_prontera"]:
        if not os.path.isfile(os.path.join(args.scripts, scene + ".lua")):
            parser.error(f"no scene {scene} in {args.scripts}")
        runs = [run_layout(args, scene, layout) for layout in LAYOUTS]
        bad = [f"layout {layout}: exit {status}" for layout, (_, status) in zip(LAYOUTS, runs) if status]
        found = bad or differences(runs[0][0], runs[1][0])
        for line in found[:20]:
            print(f"  {line}")
        if len(found) > 20:
            print(f"  ... and {len(found) - 20} more")
        print(f"{'FAIL' if found else 'ok  '} {scene}", flush=True)
        failed += bool(found)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
