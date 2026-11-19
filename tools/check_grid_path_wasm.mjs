// Checks build/wasm/grid_path.wasm (src/body/grid_path.c built by tools/build_grid_path_wasm.bat)
// against the server's walk path search and step time:
//
//   node tools/check_grid_path_wasm.mjs [server checkout] [wasm file]
//
//   1. tests/data/grid_path_vectors.txt is what the server's code gives now for the generator
//      in tools/grid_path_vectors.mjs, so the file tests/test_grid_path.c holds the native
//      build to is not stale.
//   2. The wasm gives every path and step time of that file.
//   3. The wasm and the server's code, asked directly, agree on fresh random grids (a fixed
//      seed that is not the vector file's) and on the step time at every walk speed.
//
// Prints one line per part and exits 0, or prints the first difference and exits 1.

import { readFileSync } from "node:fs";
import { join } from "node:path";
import {
  CLIENT_ROOT, DEFAULT_SERVER, VECTORS_FILE, buildVectors, loadServer, makeRng, parseVectors, randomSections,
} from "./grid_path_vectors.mjs";

const FRESH_SEED = 0x5EED0002;
const MAX_STEPS = 32;
// Room for more steps than the longest path, so that the search's own limit is what refuses a
// longer one.
const OUT_STEPS = MAX_STEPS + 8;
// A step is three 32-bit integers: x, y, dir. The map is width, height and the cells' address.
const STEP_BYTES = 12;
const MAP_BYTES = 12;
const UNTOUCHED = -77;

// The wasm's search and step time with the server wrapper's shape (see loadServer).
function loadWasm(file) {
  const { exports } = new WebAssembly.Instance(new WebAssembly.Module(readFileSync(file)), {});
  const { memory } = exports;
  const scratchBytes = exports.grid_path_scratch_size();
  // The host's buffers follow the module's own data: the map, the steps, the scratch memory,
  // then the grid's cells.
  const mapAt = (exports.__heap_base.value + 15) & ~15;
  const outAt = mapAt + 16;
  const scratchAt = outAt + OUT_STEPS * STEP_BYTES;
  const cellsAt = scratchAt + scratchBytes;
  if (MAP_BYTES > outAt - mapAt || scratchAt % 4 !== 0) throw new Error("the buffers overlap or are misaligned");
  let loaded = null;

  function load(grid) {
    const need = cellsAt + grid.cells.length - memory.buffer.byteLength;
    if (need > 0) memory.grow(Math.ceil(need / 65536));
    new Uint8Array(memory.buffer).set(grid.cells, cellsAt);
    new Int32Array(memory.buffer, mapAt, 3).set([grid.width, grid.height, cellsAt]);
    loaded = grid;
  }

  return {
    search(grid, x0, y0, x1, y1) {
      if (loaded !== grid) load(grid);
      const out = new Int32Array(memory.buffer, outAt, OUT_STEPS * 3);
      out.fill(UNTOUCHED);
      const count = exports.grid_path_search(mapAt, x0, y0, x1, y1, outAt, OUT_STEPS, scratchAt, scratchBytes);
      if (count < -1 || count > MAX_STEPS) throw new Error(`the wasm returned ${count} steps`);
      for (let i = Math.max(0, count) * 3; i < out.length; i++) {
        if (out[i] !== UNTOUCHED) throw new Error("the wasm wrote past the steps it returned");
      }
      if (count < 0) return null;
      return Array.from({ length: count }, (_, i) => ({ x: out[3 * i], y: out[3 * i + 1], dir: out[3 * i + 2] }));
    },
    stepMs: (walkSpeed, diagonal) => exports.grid_path_step_ms(walkSpeed, diagonal ? 1 : 0) >>> 0,
  };
}

const show = (steps) => (steps == null ? "no path" : `${steps.length} steps [${steps.map((s) => `${s.x},${s.y}/${s.dir}`).join(" ")}]`);

function samePath(a, b) {
  if (a == null || b == null) return a == null && b == null;
  return a.length === b.length && a.every((s, i) => s.x === b[i].x && s.y === b[i].y && s.dir === b[i].dir);
}

function rowsOf(grid) {
  const rows = [];
  for (let y = 0; y < grid.height; y++) {
    rows.push(Array.from(grid.cells.subarray(y * grid.width, (y + 1) * grid.width), (c) => (c ? "." : "#")).join(""));
  }
  return rows.join("\n");
}

function differ(what, detail) {
  console.error(`check_grid_path_wasm: ${what}\n${detail}`);
  process.exit(1);
}

function comparePath(what, grid, [x0, y0, x1, y1], got, want, wantName) {
  if (samePath(got, want)) return;
  differ(`${what}: (${x0},${y0}) to (${x1},${y1}) on a ${grid.width}x${grid.height} grid`,
    `the wasm:  ${show(got)}\n${wantName}: ${show(want)}\n${grid.cells.length <= 4096 ? rowsOf(grid) : "(grid too large to print)"}`);
}

const [checkout = DEFAULT_SERVER, wasmFile = join(CLIENT_ROOT, "build", "wasm", "grid_path.wasm")] = process.argv.slice(2);
const server = await loadServer(checkout);
const wasm = loadWasm(wasmFile);

// 1. The committed vectors are the server's answers.
const committed = readFileSync(VECTORS_FILE, "utf8").replace(/\r\n/g, "\n");
if (committed !== buildVectors(server).text) {
  differ("tests/data/grid_path_vectors.txt is not what the server's code gives",
    "the server's search changed, or the file was edited: run tools/grid_path_vectors.mjs and look at the difference");
}
console.log("vectors: tests/data/grid_path_vectors.txt is what the server's code gives");

// 2. The wasm gives the vectors.
const vectors = parseVectors(committed);
let vectorPaths = 0;
for (const { grid, paths } of vectors.grids) {
  for (const { x0, y0, x1, y1, steps } of paths) {
    comparePath("vector", grid, [x0, y0, x1, y1], wasm.search(grid, x0, y0, x1, y1), steps, "the vector");
    vectorPaths++;
  }
}
for (const { speed, diagonal, ms } of vectors.times) {
  if (wasm.stepMs(speed, diagonal) !== ms) {
    differ(`vector: a step at speed ${speed} (diagonal ${diagonal})`, `the wasm: ${wasm.stepMs(speed, diagonal)} ms\nthe vector: ${ms} ms`);
  }
}
console.log(`wasm == vectors: ${vectorPaths} paths on ${vectors.grids.length} grids, ${vectors.times.length} step times`);

// 3. The wasm and the server, asked directly.
const fresh = randomSections(makeRng(FRESH_SEED), 2);
let freshPaths = 0, freshFound = 0;
for (const { grid, searches } of fresh) {
  for (const ends of searches) {
    const want = server.search(grid, ...ends);
    comparePath("fresh grid", grid, ends, wasm.search(grid, ...ends), want, "the server");
    freshPaths++;
    if (want != null && want.length > 0) freshFound++;
  }
}
let speeds = 0;
for (let speed = -3; speed <= 70000; speed++) {
  for (const diagonal of [0, 1]) {
    if (wasm.stepMs(speed, diagonal) !== server.stepMs(speed, diagonal)) {
      differ(`a step at speed ${speed} (diagonal ${diagonal})`,
        `the wasm: ${wasm.stepMs(speed, diagonal)} ms\nthe server: ${server.stepMs(speed, diagonal)} ms`);
    }
    speeds++;
  }
}
console.log(`wasm == server: ${freshPaths} paths (${freshFound} found) on ${fresh.length} fresh grids, ${speeds} step times`);
