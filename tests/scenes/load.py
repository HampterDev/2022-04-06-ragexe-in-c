#!/usr/bin/env python3
"""Run the load scenes (tests/scenes/load) several times and report what their frames cost.

    python3 tests/scenes/load.py --exe build/ragexe_scenes --game ~/ro --out /tmp/load
    python3 tests/scenes/load.py ... --runs 5 woe_emperium town_prontera     # some scenes
    python3 tests/scenes/load.py ... --msaa 1 --runs 1                         # pictures to compare
    python3 tests/scenes/load.py --compare /tmp/load-old /tmp/load-new          # two summaries

Each run of a scene writes its captures, its log, run.json (exit status, seconds, message boxes)
and <scene>.frames.jsonl (scene.measure: the counters of every measured frame) into
<out>/<scene>/run<N>. A scene run again replaces its earlier runs when every new run ends well;
otherwise the earlier runs stay and the new ones are kept in <out>/<scene>.failed. The counts are
the same in every run; the times are real time and differ by about a fifth between runs, so the
report pools the measured frames of a scene's runs that ended well: for each time the median frame
and the worst one (with the number of the run and the frame of the stretch it was), and the spread
of the runs' own medians. The report covers every scene <out> has runs of, failed sets too; it is
printed and written to <out>/summary.md and <out>/summary.json (--summary-only writes it without
running anything). Exit status 1 when a run fails.
"""
import argparse
import json
import os
import shutil
import statistics
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
COUNTS = ["objects", "draws", "indirect_draws", "draw_calls", "primitives", "instance_bytes", "vertex_bytes",
          "texture_bytes"]
TIMES = ["frame_ms", "script_ms", "view_render_ms", "draw_scene_ms", "present_ms", "packets_ms", "effects_ms",
         "actors_ms", "objects_render_ms", "effects_render_ms", "ui_ms", "atlas_ms", "texels_ms"]


def run_scene(args, scene, root, index):
    out = os.path.join(root, f"run{index}")
    os.makedirs(out, exist_ok=True)
    env = dict(os.environ, RAGEXE_SCENE=os.path.join(args.scenes, scene + ".lua"), RAGEXE_SCENE_OUT=out,
               RAGEXE_MSAA=str(args.msaa), RAGEXE_VSYNC="0")
    if not env.get("SDL_AUDIO_DRIVER") and not env.get("SDL_AUDIODRIVER"):
        env["SDL_AUDIO_DRIVER"] = "dummy"
    if args.frames:
        env["RAGEXE_LOAD_FRAMES"] = str(args.frames)
    started = time.monotonic()
    try:
        proc = subprocess.run([args.exe], cwd=args.game, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                              timeout=args.timeout)
        output, status = proc.stdout.decode("utf-8", "replace"), proc.returncode
    except subprocess.TimeoutExpired as e:
        output, status = (e.stdout or b"").decode("utf-8", "replace") + "\n(timed out)", -1
    except OSError as e:
        output, status = f"(the client did not start: {e})", -1
    with open(os.path.join(out, scene + ".log"), "w", encoding="utf-8") as f:
        f.write(output)
    boxes = sum(1 for line in output.splitlines() if line.startswith("scene: message box"))
    with open(os.path.join(out, "run.json"), "w", encoding="utf-8") as f:
        json.dump({"status": status, "seconds": time.monotonic() - started, "boxes": boxes, "exe": args.exe,
                   "msaa": args.msaa}, f)
    return read_run(out, scene, index)


def read_run(directory, scene, index):
    """Run number `index` as its directory keeps it: run.json, and the frames a run that crashed got to."""
    with open(os.path.join(directory, "run.json"), encoding="utf-8") as f:
        run = json.load(f)
    run["index"] = index
    run["frames"] = []
    path = os.path.join(directory, scene + ".frames.jsonl")
    if os.path.exists(path):
        with open(path, encoding="utf-8") as f:
            for line in f:
                try:
                    run["frames"].append(json.loads(line))
                except json.JSONDecodeError:
                    break  # a run that crashed stops in the middle of a line
    return run


def read_runs(out):
    """Every scene's runs in <out>, in the order of their numbers."""
    scenes = {}
    for scene in sorted(os.listdir(out)):
        root = os.path.join(out, scene)
        if not os.path.isdir(root):
            continue
        numbers = sorted(int(n[3:]) for n in os.listdir(root) if n.startswith("run") and n[3:].isdigit()
                         and os.path.exists(os.path.join(root, n, "run.json")))
        if numbers:
            scenes[scene] = [read_run(os.path.join(root, f"run{i}"), scene, i) for i in numbers]
    return scenes


def summarize(scene, runs):
    """The scene's numbers over every measured frame of its runs."""
    good = [r for r in runs if r["status"] == 0 and r["frames"]]
    pooled = [(r["index"], n, frame) for r in good for n, frame in enumerate(r["frames"], 1)]
    summary = {"scene": scene, "runs": len(runs), "good_runs": len(good), "frames": len(good[0]["frames"]) if good else 0,
               "message_boxes": max((r["boxes"] for r in runs), default=0), "counts": {}, "times": {}}
    if not pooled:
        return summary
    for key in COUNTS:
        values = [frame.get(key, 0) for _, _, frame in pooled]
        summary["counts"][key] = {"median": statistics.median(values), "max": max(values)}
    for key in TIMES:
        if key not in pooled[0][2]:
            continue
        worst = max(pooled, key=lambda p: p[2][key])
        medians = [statistics.median(frame[key] for frame in r["frames"]) for r in good]
        summary["times"][key] = {
            "median": statistics.median(frame[key] for _, _, frame in pooled),
            "worst": worst[2][key], "worst_run": worst[0], "worst_frame": worst[1],
            "run_medians": [min(medians), max(medians)],
            "first": statistics.median(r["frames"][0][key] for r in good),
        }
    return summary


def size(n):
    return f"{n / 1048576:.2f} MB" if n >= 1048576 else f"{n / 1024:.0f} KB" if n >= 1024 else f"{n:.0f} B"


def markdown(summaries, title):
    lines = [f"# {title}", "",
             "Counts: the median frame's (and the largest). Times in ms: the median frame of all runs, the "
             "worst frame (run.frame of the measured stretch), the spread of the runs' medians, and the "
             "first measured frame's median over the runs.", ""]
    lines += ["| scene | runs | frames | objects | draws | instance bytes | vertex bytes | texture bytes |",
              "|---|---|---|---|---|---|---|---|"]
    for s in summaries:
        c = s["counts"]
        if not c:
            lines.append(f"| {s['scene']} | {s['good_runs']}/{s['runs']} | - | - | - | - | - | - |")
            continue
        lines.append(f"| {s['scene']} | {s['good_runs']}/{s['runs']} | {s['frames']} | {c['objects']['median']:.0f} "
                     f"({c['objects']['max']:.0f}) | {c['draws']['median']:.0f} ({c['draws']['max']:.0f}) | "
                     f"{size(c['instance_bytes']['median'])} ({size(c['instance_bytes']['max'])}) | "
                     f"{size(c['vertex_bytes']['median'])} ({size(c['vertex_bytes']['max'])}) | "
                     f"{size(c['texture_bytes']['median'])} ({size(c['texture_bytes']['max'])}) |")
    keys = [k for k in TIMES if any(k in s["times"] for s in summaries)]
    lines += ["", "| scene | time | median | worst (run.frame) | run medians | first frame |", "|---|---|---|---|---|---|"]
    for s in summaries:
        for key in keys:
            t = s["times"].get(key)
            if t:
                lines.append(f"| {s['scene']} | {key} | {t['median']:.2f} | {t['worst']:.2f} ({t['worst_run']}."
                             f"{t['worst_frame']}) | {t['run_medians'][0]:.2f}-{t['run_medians'][1]:.2f} | "
                             f"{t['first']:.2f} |")
    return "\n".join(lines) + "\n"


def compare(a_dir, b_dir):
    with open(os.path.join(a_dir, "summary.json"), encoding="utf-8") as f:
        a = {s["scene"]: s for s in json.load(f)}
    with open(os.path.join(b_dir, "summary.json"), encoding="utf-8") as f:
        b = {s["scene"]: s for s in json.load(f)}
    print(f"A = {a_dir}\nB = {b_dir}\n")
    print("| scene | value | A median | B median | B/A | A worst | B worst |")
    print("|---|---|---|---|---|---|---|")
    for scene in [s for s in a if s in b]:
        for key in ["draws", "instance_bytes", "vertex_bytes", "texture_bytes"]:
            ca, cb = a[scene]["counts"].get(key), b[scene]["counts"].get(key)
            if ca and cb:
                ratio = f"{cb['median'] / ca['median']:.2f}" if ca["median"] else "-"
                print(f"| {scene} | {key} | {ca['median']:.0f} | {cb['median']:.0f} | {ratio} | {ca['max']:.0f} | "
                      f"{cb['max']:.0f} |")
        for key in TIMES:
            ta, tb = a[scene]["times"].get(key), b[scene]["times"].get(key)
            if ta and tb:
                ratio = f"{tb['median'] / ta['median']:.2f}" if ta["median"] else "-"
                print(f"| {scene} | {key} | {ta['median']:.2f} | {tb['median']:.2f} | {ratio} | {ta['worst']:.2f} | "
                      f"{tb['worst']:.2f} |")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--exe", help="the ragexe_scenes executable")
    parser.add_argument("--game", help="the game directory the client runs in")
    parser.add_argument("--out", help="directory for the runs and the summary")
    parser.add_argument("--scenes", default=os.path.join(HERE, "load"), help="directory of the load scenes")
    parser.add_argument("--runs", type=int, default=3, help="runs of each scene")
    parser.add_argument("--msaa", type=int, default=4, help="RAGEXE_MSAA for the runs (players' default 4)")
    parser.add_argument("--frames", type=int,
                        help="measured frames of every scene (RAGEXE_LOAD_FRAMES; by default 600, the arrive scenes 300)")
    parser.add_argument("--timeout", type=int, default=900, help="seconds per run")
    parser.add_argument("--compare", nargs=2, metavar=("A", "B"), help="compare two --out directories' summaries")
    parser.add_argument("--summary-only", action="store_true", help="run nothing, report the runs --out has")
    parser.add_argument("names", nargs="*", help="scene names (default: every scene)")
    args = parser.parse_args()
    if args.compare:
        compare(*args.compare)
        return 0
    if not args.out or not (args.summary_only or (args.exe and args.game)):
        parser.error("--out, and --exe and --game unless --summary-only, are needed")
    # the client runs in --game, so every path it is given is absolute
    args.out, args.scenes = os.path.abspath(args.out), os.path.abspath(args.scenes)
    if not os.path.isdir(args.scenes):
        parser.error(f"no scenes at {args.scenes}")
    if not args.summary_only:
        args.exe, args.game = os.path.abspath(args.exe), os.path.abspath(args.game)
        if not os.path.isfile(args.exe):
            parser.error(f"no client at {args.exe}")
        if not os.path.isdir(args.game):
            parser.error(f"no game directory at {args.game}")
    scenes = sorted(os.path.splitext(f)[0] for f in os.listdir(args.scenes) if f.endswith(".lua"))
    unknown = [n for n in args.names if n not in scenes]
    if unknown:
        parser.error(f"no such scene in {args.scenes}: {', '.join(unknown)}")
    if args.names:
        scenes = [s for s in scenes if s in args.names]
    failed = 0
    for scene in [] if args.summary_only else scenes:
        # the runs go into <scene>.new; when every one ends well they replace the scene's earlier
        # runs, otherwise those stay and the new ones are kept as <scene>.failed
        root = os.path.join(args.out, scene)
        if os.path.isdir(root + ".new"):
            shutil.rmtree(root + ".new")
        scene_failed = 0
        for index in range(1, args.runs + 1):
            run = run_scene(args, scene, root + ".new", index)
            ok = run["status"] == 0 and run["frames"]
            scene_failed += not ok
            median = statistics.median(f["frame_ms"] for f in run["frames"]) if run["frames"] else 0
            print(f"{'ok  ' if ok else 'FAIL'} {scene} run {index}: exit {run['status']}, {len(run['frames'])} frames, "
                  f"median frame {median:.2f} ms ({run['seconds']:.0f} s)", flush=True)
        keep = root if not scene_failed else root + ".failed"
        if os.path.isdir(keep):
            shutil.rmtree(keep)
        os.rename(root + ".new", keep)
        if scene_failed:
            print(f"FAIL {scene}: {scene_failed} of {args.runs} runs failed; they are in {keep}, the earlier runs stay",
                  flush=True)
        failed += scene_failed
    # the report covers every scene --out has runs of, those of earlier invocations too
    os.makedirs(args.out, exist_ok=True)
    runs = read_runs(args.out)
    summaries = [summarize(scene, scene_runs) for scene, scene_runs in runs.items()]
    every = [r for scene_runs in runs.values() for r in scene_runs]
    exes = sorted({os.path.basename(r["exe"]) for r in every})
    msaa = sorted({str(r["msaa"]) for r in every})
    with open(os.path.join(args.out, "summary.json"), "w", encoding="utf-8") as f:
        json.dump(summaries, f, indent=1)
    text = markdown(summaries, f"Load scenes: {', '.join(exes)}, RAGEXE_MSAA={', '.join(msaa)}")
    with open(os.path.join(args.out, "summary.md"), "w", encoding="utf-8") as f:
        f.write(text)
    print(text)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
