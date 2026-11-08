#!/usr/bin/env python3
"""Effect gallery: render every RagEffect ID with the scene harness and write one static page.

    python tools/effect_gallery.py --exe build/ragexe_scenes --game <game dir> --out <dir>
    python tools/effect_gallery.py ... --ids 0-499          # some IDs (default: every traced ID)
    python tools/effect_gallery.py ... --data               # the IDs data/effects/effects.json has
    python tools/effect_gallery.py ... --resume             # skip IDs <out> already has
    python tools/effect_gallery.py --out <dir> --page-only  # rewrite <out>/index.html only

It runs tests/scenes/gallery/effect_gallery.lua with ragexe_scenes in chunks of IDs. The script
launches each ID on the player as ZC_NOTIFY_EFFECT2 does and captures a filmstrip (frames 1, 8,
16, 32 and 64 after the packet by default). When the client dies on an ID, the ID is marked and
the next chunk starts after it. Results go to <out>/effect_gallery.json, the captures to <out>,
one log per chunk to <out>/logs and the page to <out>/index.html, which needs no server.

The page shows for every ID its filmstrip, its effects.json row when it is data (Str, Play),
a "code" tag when rag_effect_process*.c has a case for it, its duration (src/effects/durations.c)
and what the run saw: the effects still on the player at the last frame, objects left behind,
a message box (such as "Invalid Effect Id") or a crash. Captures need game data, so they and the
page stay outside the repository (tests/scenes/README.md, "Effect gallery").
"""
import argparse
import html
import json
import os
import re
import subprocess
import sys
import threading
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SCRIPT = os.path.join(ROOT, "tests", "scenes", "gallery", "effect_gallery.lua")
EFFECTS_JSON = os.path.join(ROOT, "data", "effects", "effects.json")
DURATIONS_C = os.path.join(ROOT, "src", "effects", "durations.c")
PROCESS_DIR = os.path.join(ROOT, "src", "effects", "rag")
# Every ID tests/test_effect_trace.c runs: 0 to EFFECT_DURATION_COUNT - 1 and three beyond.
TRACE_IDS = "0-2384,2385,10000,60000"

START = re.compile(r"^gallery: start (\d+)$")
DONE = re.compile(r"^gallery: done (\d+) alive=([\d,]*) objects=(-?\d+) left=(-?\d+)$")
BOX = re.compile(r'^scene: message box "(.*?)": (.*)$')
CAPTURED = re.compile(r"^scene: captured (.+\.(?:png|jpg)) \(frame (\d+)")


def parse_ids(spec):
    ids = []
    for part in re.split(r"[,\s]+", spec.strip()):
        if not part:
            continue
        m = re.fullmatch(r"(\d+)-(\d+)", part)
        if m:
            ids.extend(range(int(m.group(1)), int(m.group(2)) + 1))
        elif part.isdigit():
            ids.append(int(part))
        else:
            raise ValueError(f"not an ID or range: {part}")
    return sorted(set(ids))


def ranges(ids):
    """[1, 2, 3, 7] -> "1-3,7", to keep the environment variable short."""
    out, i = [], 0
    while i < len(ids):
        j = i
        while j + 1 < len(ids) and ids[j + 1] == ids[j] + 1:
            j += 1
        out.append(str(ids[i]) if i == j else f"{ids[i]}-{ids[j]}")
        i = j + 1
    return ",".join(out)


# ---- what the source says about each ID ----

def load_effect_rows():
    with open(EFFECTS_JSON, encoding="utf-8") as f:
        return {int(k): v for k, v in json.load(f)["RagEffect"].items()}


def load_durations():
    with open(DURATIONS_C, encoding="utf-8") as f:
        text = f.read()
    body = re.search(r"g_effectDurations\[[^\]]*\]\s*=\s*\{(.*?)\};", text, re.S).group(1)
    return [int(v) for v in re.findall(r"-?\d+", body)]


def load_code_cases():
    """{id: (file, first statement)} for every case label of the effect switches in
    rag_effect_process*.c, leaving out rag_runs_every_frame, which only lists IDs."""
    cases = {}
    for name in sorted(os.listdir(PROCESS_DIR)):
        if not re.fullmatch(r"rag_effect_process.*\.c", name):
            continue
        with open(os.path.join(PROCESS_DIR, name), encoding="utf-8", errors="replace") as f:
            lines = f.read().splitlines()
        function = None
        for i, line in enumerate(lines):
            m = re.match(r"^[A-Za-z_][\w\s\*]*?\b(\w+)\s*\([^;]*$", line)
            if m and not line.startswith((" ", "\t")):
                function = m.group(1)
            if function == "rag_runs_every_frame":
                continue
            labels = re.findall(r"\bcase\s+(0x[0-9a-fA-F]+|\d+)\s*:", line)
            if not labels:
                continue
            rest = re.sub(r".*\bcase\s+(?:0x[0-9a-fA-F]+|\d+)\s*:", "", line).strip()
            k = i + 1
            while not rest and k < len(lines) and k < i + 6:
                nxt = lines[k].strip()
                if nxt and not re.match(r"case\s", nxt) and nxt != "{":
                    rest = nxt
                k += 1
            rest = re.sub(r"\s*break;\s*$", "", rest)
            for label in labels:
                cases.setdefault(int(label, 0), (name, rest[:120]))
    return cases


# ---- running the scene ----

def decode(raw):
    """The client's lines are UTF-8, but a message box can carry a CP949 file name."""
    try:
        return raw.decode("utf-8")
    except UnicodeDecodeError:
        return raw.decode("cp949", "replace")


def run_chunk(args, ids, log_path, results):
    """Runs the gallery script over ids; returns the IDs it finished. Fills results."""
    env = dict(os.environ, RAGEXE_SCENE=SCRIPT, RAGEXE_SCENE_OUT=os.path.abspath(args.out),
               RAGEXE_GALLERY_IDS=ranges(ids), RAGEXE_GALLERY_FRAMES=args.frames,
               RAGEXE_GALLERY_CROP=args.crop, RAGEXE_GALLERY_JPEG=str(args.jpeg),
               RAGEXE_GALLERY_MAP=args.map, RAGEXE_GALLERY_JOB=str(args.job),
               RAGEXE_GALLERY_SETTLE=str(args.settle))
    proc = subprocess.Popen([os.path.abspath(args.exe)], cwd=args.game, env=env, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)
    last = [time.monotonic()]
    stalled = [False]

    def watchdog():
        while proc.poll() is None:
            if time.monotonic() - last[0] > args.stall:
                stalled[0] = True
                proc.kill()
                return
            time.sleep(1)

    threading.Thread(target=watchdog, daemon=True).start()
    finished, current, crash = [], None, []
    with open(log_path, "w", encoding="utf-8", newline="") as log:
        for raw in proc.stdout:
            line = decode(raw)
            last[0] = time.monotonic()
            log.write(line)
            line = line.rstrip("\r\n")
            if crash or line == "scene: crashed":
                crash.append(line)
                continue
            m = START.match(line)
            if m:
                current = int(m.group(1))
                results[str(current)] = {"frames": {}, "boxes": []}
                continue
            m = CAPTURED.match(line)
            if m and current is not None:
                name = os.path.basename(m.group(1))
                after = re.search(r"_f(\d+)\.\w+$", name)
                if after:
                    results[str(current)]["frames"][str(int(after.group(1)))] = name
                continue
            m = BOX.match(line)
            if m and current is not None:
                results[str(current)]["boxes"].append(f"{m.group(1)}: {m.group(2)}")
                continue
            m = DONE.match(line)
            if m:
                r = results[m.group(1)]
                r["alive"] = [int(v) for v in m.group(2).split(",") if v]
                r["objects"], r["left"] = int(m.group(3)), int(m.group(4))
                r["status"] = "ok"
                finished.append(int(m.group(1)))
                current = None
    proc.wait()
    if current is not None:
        r = results[str(current)]
        r["status"] = "hang" if stalled[0] else "crash"
        r["crash"] = " / ".join(l.split(None, 1)[1].strip() for l in crash
                                if l.startswith(("Exception:", "At:", "Checkpoint:"))) or (
            f"no output for {args.stall} s" if stalled[0] else f"exit status {proc.returncode}")
        finished.append(current)
    elif proc.returncode != 0 and not finished:
        raise SystemExit(f"ragexe_scenes failed before the first effect (exit status {proc.returncode}); "
                         f"see {log_path}")
    return finished


def run(args, ids, results_path, results):
    os.makedirs(os.path.join(args.out, "logs"), exist_ok=True)
    todo = list(ids)
    chunk_no = 0
    started = time.monotonic()
    while todo:
        chunk = todo[:args.chunk]
        chunk_no += 1
        log_path = os.path.join(args.out, "logs", f"chunk_{chunk[0]:05d}_{chunk_no:03d}.log")
        finished = run_chunk(args, chunk, log_path, results)
        done = set(finished)
        if not done:
            raise SystemExit(f"no effect finished in {log_path}; stopping")
        # IDs after the last one the chunk reached go into the next chunk.
        reached = max(done)
        todo = [i for i in chunk if i > reached and i not in done] + todo[len(chunk):]
        bad = ", ".join("%d %s" % (i, results[str(i)]["status"]) for i in finished
                        if results[str(i)]["status"] != "ok")
        with open(results_path, "w", encoding="utf-8") as f:
            json.dump(results, f, indent=0, sort_keys=True)
        print(f"{len(ids) - len(todo)}/{len(ids)} IDs, {time.monotonic() - started:.0f} s" + (f"; {bad}" if bad else ""),
              flush=True)


# ---- the page ----

PAGE = """<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Effect Gallery</title>
<style>
:root{--bg:#f4f4f2;--fg:#1d1d1b;--muted:#6b6b66;--card:#fff;--line:#d9d9d4;--accent:#1f5fbf;
--data:#2e7d32;--play:#00838f;--code:#6a3fb5;--none:#8a8a84;--warn:#b26a00;--bad:#c62828}
@media (prefers-color-scheme:dark){:root:not([data-theme=light]){--bg:#121212;--fg:#e9e9e6;--muted:#9a9a94;
--card:#1d1d1c;--line:#333331;--accent:#7fb0ff;--data:#7cc782;--play:#5ecfd9;--code:#b99cf0;--none:#8a8a84;
--warn:#f0b35a;--bad:#ff7b72}}
:root[data-theme=dark]{--bg:#121212;--fg:#e9e9e6;--muted:#9a9a94;--card:#1d1d1c;--line:#333331;--accent:#7fb0ff;
--data:#7cc782;--play:#5ecfd9;--code:#b99cf0;--none:#8a8a84;--warn:#f0b35a;--bad:#ff7b72}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--fg);font:14px/1.4 system-ui,sans-serif}
header{position:sticky;top:0;z-index:2;background:var(--bg);border-bottom:1px solid var(--line);padding:12px 16px;
display:flex;flex-wrap:wrap;gap:8px 16px;align-items:center}
h1{font-size:18px;margin:0 8px 0 0}input[type=search]{flex:1 1 220px;min-width:0;padding:6px 10px;border:1px solid var(--line);
border-radius:6px;background:var(--card);color:var(--fg);font:inherit}
.chips{display:flex;flex-wrap:wrap;gap:6px}.chips button{border:1px solid var(--line);background:var(--card);color:var(--fg);
border-radius:999px;padding:4px 10px;font:inherit;cursor:pointer}.chips button[aria-pressed=true]{background:var(--fg);color:var(--bg)}
label{display:flex;gap:6px;align-items:center;color:var(--muted)}#count{color:var(--muted);font-variant-numeric:tabular-nums}
main{display:grid;grid-template-columns:repeat(auto-fill,minmax(250px,1fr));gap:12px;padding:16px}
.card{background:var(--card);border:1px solid var(--line);border-radius:8px;overflow:hidden;display:flex;flex-direction:column}
.view{position:relative;aspect-ratio:var(--ar);background:#000;cursor:pointer}.view img{width:100%;height:100%;object-fit:cover;display:block}
.view .f{position:absolute;right:6px;bottom:6px;background:#000a;color:#fff;font-size:11px;padding:1px 5px;border-radius:4px}
.strip{display:grid;grid-template-columns:repeat(var(--n),1fr);gap:2px;padding:2px;background:var(--line)}
.strip img{width:100%;aspect-ratio:var(--ar);object-fit:cover;display:block;cursor:pointer;opacity:.75}.strip img.on{opacity:1;outline:2px solid var(--accent);outline-offset:-2px}
.body{padding:8px 10px;display:flex;flex-direction:column;gap:6px}
.title{display:flex;gap:8px;align-items:baseline}.id{font-weight:600;font-size:16px;font-variant-numeric:tabular-nums}
.hex{color:var(--muted);font-family:ui-monospace,monospace;font-size:12px}
.tags{display:flex;flex-wrap:wrap;gap:4px}.tag{font-size:11px;padding:1px 6px;border-radius:4px;border:1px solid currentColor}
.t-str{color:var(--data)}.t-play{color:var(--play)}.t-code{color:var(--code)}.t-none{color:var(--none)}.t-warn{color:var(--warn)}.t-bad{color:var(--bad)}
.meta{color:var(--muted);font-size:12px;overflow-wrap:anywhere}code,pre{font-family:ui-monospace,monospace;font-size:11px}
details summary{cursor:pointer;color:var(--muted);font-size:12px}pre{margin:4px 0 0;max-height:240px;overflow:auto;background:var(--bg);
padding:6px;border-radius:4px;white-space:pre-wrap;overflow-wrap:anywhere}
.missing{display:grid;place-items:center;height:100%;color:#aaa;font-size:12px;padding:8px;text-align:center}
@media (max-width:540px){main{padding:16px 16px;grid-template-columns:1fr}header{padding:10px 16px}}
</style></head><body>
<header><h1>Effect gallery</h1>
<input type="search" id="q" placeholder="ID, range (100-200), file, function…" aria-label="Search">
<div class="chips" id="kinds"></div>
<label><input type="checkbox" id="anim" checked> animate on hover</label>
<span id="count"></span></header>
<main id="grid"></main>
<script>
const DATA = __DATA__;
const META = __META__;
const KINDS = [["all","All"],["data","Data (Str/Play)"],["play","Play"],["str","Str"],["code","Code"],["none","No case"],
  ["issue","Crash/box/left over"]];
let kind = "all";
const chips = document.getElementById("kinds");
for (const [k, label] of KINDS) {
  const b = document.createElement("button"); b.textContent = label; b.dataset.k = k;
  b.setAttribute("aria-pressed", k === kind); b.onclick = () => { kind = k;
    for (const x of chips.children) x.setAttribute("aria-pressed", x.dataset.k === k); render(); };
  chips.append(b);
}
const esc = s => String(s).replace(/[&<>"]/g, c => ({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;"}[c]));
function matches(e, q) {
  if (!q) return true;
  for (const part of q.split(/[\\s,]+/).filter(Boolean)) {
    const r = part.match(/^(\\d+)-(\\d+)$/);
    if (r) { if (e.id >= +r[1] && e.id <= +r[2]) return true; continue; }
    if (/^\\d+$/.test(part)) { if (e.id === +part) return true; continue; }
    if (/^0x[0-9a-f]+$/i.test(part)) { if (e.id === parseInt(part, 16)) return true; continue; }
    if (e.text.includes(part.toLowerCase())) return true;
  }
  return false;
}
function ofKind(e) {
  switch (kind) {
    case "data": return !!(e.row && (e.row.Str || e.row.Play));
    case "play": return !!(e.row && e.row.Play);
    case "str": return !!(e.row && e.row.Str);
    case "code": return !!e.code;
    case "none": return !e.code && !(e.row && e.row.Play);
    case "issue": return e.status !== "ok" || (e.boxes && e.boxes.length) || e.left > 0;
  }
  return true;
}
for (const e of DATA) e.text = [e.code ? e.code[0] + " " + e.code[1] : "", e.row ? JSON.stringify(e.row) : "",
  (e.boxes || []).join(" "), e.crash || ""].join(" ").toLowerCase();
const grid = document.getElementById("grid");
const io = new IntersectionObserver(es => { for (const x of es) if (x.isIntersecting) { fill(x.target); io.unobserve(x.target); } },
  {rootMargin: "600px"});
document.documentElement.style.setProperty("--ar", META.aspect);
document.documentElement.style.setProperty("--n", META.frames.length);
function card(e) {
  const c = document.createElement("article"); c.className = "card"; c.e = e;
  const tags = [];
  if (e.row && e.row.Str) tags.push(`<span class="tag t-str" title="effects.json Str">Str</span>`);
  if (e.row && e.row.Play) tags.push(`<span class="tag t-play" title="effects.json Play">Play</span>`);
  if (e.code) tags.push(`<span class="tag t-code" title="${esc(e.code[0])}">code</span>`);
  if (!e.code && !(e.row && e.row.Play)) tags.push(`<span class="tag t-none" title="no case in rag_effect_process*.c">no case</span>`);
  if (e.status === "crash" || e.status === "hang") tags.push(`<span class="tag t-bad">${e.status}</span>`);
  else if (!e.status) tags.push(`<span class="tag t-none">not run</span>`);
  if (e.boxes && e.boxes.length) tags.push(`<span class="tag t-warn" title="${esc(e.boxes.join("\\n"))}">message box</span>`);
  if (e.left > 0) tags.push(`<span class="tag t-warn" title="objects left after clearing">left ${e.left}</span>`);
  if (e.alive && e.alive.length) tags.push(`<span class="tag t-none" title="effects on the player at frame ${META.frames.at(-1)}">alive at ${META.frames.at(-1)}</span>`);
  const meta = [];
  if (e.duration !== null) meta.push(`duration ${e.duration >= 99999999 ? "∞" : e.duration}`);
  if (e.alive && e.alive.length) meta.push(`on player: ${e.alive.join(", ")}`);
  if (e.objects !== undefined) meta.push(`objects +${Math.max(0, e.objects)}`);
  const lines = [];
  if (e.code) lines.push(`<div class="meta"><code>${esc(e.code[0])}</code>: <code>${esc(e.code[1])}</code></div>`);
  if (e.boxes && e.boxes.length) lines.push(`<div class="meta">${esc(e.boxes.join(" · "))}</div>`);
  if (e.crash) lines.push(`<div class="meta">${esc(e.crash)}</div>`);
  if (e.row) lines.push(`<details><summary>effects.json</summary><pre>${esc(JSON.stringify(e.row, null, 1))}</pre></details>`);
  c.innerHTML = `<div class="view"></div><div class="strip"></div><div class="body">
    <div class="title"><span class="id">${e.id}</span><span class="hex">0x${e.id.toString(16)}</span></div>
    <div class="tags">${tags.join("")}</div><div class="meta">${meta.join(" · ")}</div>${lines.join("")}</div>`;
  io.observe(c);
  return c;
}
function show(c, i) {
  const e = c.e, f = META.frames[i], file = e.frames && e.frames[f];
  const v = c.querySelector(".view");
  v.innerHTML = file ? `<img src="${esc(file)}" alt="effect ${e.id}, frame ${f}"><span class="f">frame ${f}</span>`
                     : `<div class="missing">${e.status ? "no capture" : "not run"}</div>`;
  c.querySelectorAll(".strip img").forEach((x, k) => x.classList.toggle("on", k === i));
  c.cur = i;
}
function fill(c) {
  const e = c.e, s = c.querySelector(".strip");
  s.innerHTML = META.frames.map((f, i) => e.frames && e.frames[f]
    ? `<img loading="lazy" src="${esc(e.frames[f])}" alt="frame ${f}" title="frame ${f}" data-i="${i}">` : `<span></span>`).join("");
  s.onclick = ev => { if (ev.target.dataset.i) { stop(c); show(c, +ev.target.dataset.i); } };
  const v = c.querySelector(".view");
  v.onmouseenter = () => { if (document.getElementById("anim").checked) play(c); };
  v.onmouseleave = () => stop(c);
  v.onclick = () => c.timer ? stop(c) : play(c);
  show(c, Math.min(2, META.frames.length - 1));
}
function play(c) { stop(c); let i = 0; show(c, i); c.timer = setInterval(() => show(c, i = (i + 1) % META.frames.length), 350); }
function stop(c) { clearInterval(c.timer); c.timer = null; }
function render() {
  const q = document.getElementById("q").value.trim();
  const list = DATA.filter(e => ofKind(e) && matches(e, q));
  grid.replaceChildren(...list.map(card));
  document.getElementById("count").textContent = `${list.length} of ${DATA.length}`;
}
let t; document.getElementById("q").oninput = () => { clearTimeout(t); t = setTimeout(render, 150); };
render();
</script></body></html>
"""


def write_page(args, results, ids):
    rows = load_effect_rows()
    durations = load_durations()
    cases = load_code_cases()
    frames = [int(f) for f in args.frames.split(",") if f.strip()]
    shown = sorted(set(ids) | {int(k) for k in results})
    entries = []
    for i in shown:
        r = results.get(str(i), {})
        entries.append({
            "id": i, "row": rows.get(i), "code": list(cases[i]) if i in cases else None,
            "duration": durations[i] if 0 <= i < len(durations) else None,
            "frames": r.get("frames"), "status": r.get("status"), "alive": r.get("alive"),
            "objects": r.get("objects"), "left": r.get("left"), "boxes": r.get("boxes"), "crash": r.get("crash"),
        })
    if args.crop == "full":
        aspect = "4/3"
    else:
        w, h = args.crop.split("x")
        aspect = f"{w}/{h}"
    meta = {"frames": [str(f) for f in frames], "aspect": aspect}
    page = PAGE.replace("__DATA__", json.dumps(entries, ensure_ascii=False).replace("</", "<\\/")).replace(
        "__META__", json.dumps(meta))
    path = os.path.join(args.out, "index.html")
    with open(path, "w", encoding="utf-8") as f:
        f.write(page)
    return path


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--exe", help="the ragexe_scenes executable")
    parser.add_argument("--game", help="the game directory the client runs in")
    parser.add_argument("--out", required=True, help="directory for captures, logs, results and index.html")
    parser.add_argument("--ids", default=TRACE_IDS, help=f"IDs and ranges (default {TRACE_IDS})")
    parser.add_argument("--data", action="store_true", help="only the RagEffect IDs of data/effects/effects.json")
    parser.add_argument("--resume", action="store_true", help="skip IDs that <out>/effect_gallery.json has")
    parser.add_argument("--page-only", action="store_true", help="only rewrite index.html from the results")
    parser.add_argument("--frames", default="1,8,16,32,64", help="frames after the packet to capture")
    parser.add_argument("--crop", default="448x384", help="WxH around the frame's centre, or full")
    parser.add_argument("--jpeg", type=int, default=85, help="JPEG quality, 0 for PNG")
    parser.add_argument("--map", default="prt_fild08,200,200", help="map,x,y the player stands on")
    parser.add_argument("--job", type=int, default=0, help="the player's job")
    parser.add_argument("--settle", type=int, default=180, help="frames to wait for an effect's leftovers")
    parser.add_argument("--chunk", type=int, default=400, help="IDs per client run")
    parser.add_argument("--stall", type=int, default=60, help="seconds without output before a run is killed")
    args = parser.parse_args()

    ids = sorted(load_effect_rows()) if args.data else parse_ids(args.ids)
    os.makedirs(args.out, exist_ok=True)
    results_path = os.path.join(args.out, "effect_gallery.json")
    results = {}
    if os.path.exists(results_path):
        with open(results_path, encoding="utf-8") as f:
            results = json.load(f)
    if not args.page_only:
        if not args.exe or not args.game:
            parser.error("--exe and --game are needed unless --page-only")
        todo = [i for i in ids if not (args.resume and results.get(str(i), {}).get("status"))]
        run(args, todo, results_path, results)
    path = write_page(args, results, ids)
    print(f"gallery: {os.path.abspath(path)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
