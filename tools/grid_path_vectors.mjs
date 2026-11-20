// Writes the path search's vectors: what the server's walk path search and step time answer for
// a fixed set of grids, which the piece rules holds its map/grid_path.c to
// (game/rules/tests/data/grid_path_vectors.txt, read by its grid_path test) and
// tools/check_grid_path_wasm.mjs holds the wasm build of that file to.
//
//   node tools/grid_path_vectors.mjs <server checkout> <output file>
//
// The piece's file is changed in rules, not in this checkout's submodule: write it into a
// checkout of rules and commit it there.
//
// The answers come from the server's own code, consumers/spatial/hercules-path.js (searchPath
// and stepDelayMs), directions.js and movement-speed.js. The grids come from the generator
// below with a fixed seed, so the same server code always gives the same file.
//
// The file is lines of text:
//   # ...                      a comment
//   T <speed> <diagonal> <ms>  one step at walk speed <speed>, diagonal (1) or straight (0)
//   G <width> <height>         a grid, then <height> rows from y = 0, each <width> characters
//                              from x = 0: '.' walkable, '#' blocked
//   P <x0> <y0> <x1> <y1> <n>  a search on the last grid; <n> is -1 for no path, else <n> steps
//                              follow as <x> <y> <dir>
//   E <grids> <paths> <times>  the last line, the number of G, P and T lines

import { mkdirSync, writeFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

export const DEFAULT_SERVER = "C:\\Users\\Patrick Kuhlmann\\GIT\\ring-buffer\\local\\worktrees\\action-timing";
export const CLIENT_ROOT = resolve(dirname(fileURLToPath(import.meta.url)), "..");
export const VECTORS_FILE = join(CLIENT_ROOT, "game", "rules", "tests", "data", "grid_path_vectors.txt");

const VECTORS_SEED = 0x20220406;

// The server's search and step time over a grid { width, height, cells } (cells: 1 walkable).
export async function loadServer(checkout = DEFAULT_SERVER) {
  const spatial = join(resolve(checkout), "consumers", "spatial");
  const load = (name) => import(pathToFileURL(join(spatial, name)).href);
  const [path, directions, speed] = await Promise.all([
    load("hercules-path.js"), load("directions.js"), load("movement-speed.js"),
  ]);
  return {
    // The steps as { x, y, dir }, [] when the start is the goal, null without a path.
    search(grid, x0, y0, x1, y1) {
      const dirs = path.searchPath({
        x0, y0, x1, y1,
        xs: grid.width,
        ys: grid.height,
        isBlocked: (x, y) => !walkable(grid, x, y),
      });
      if (dirs == null) return null;
      let x = x0, y = y0;
      return dirs.map((dir) => {
        x += directions.DIRX[dir];
        y += directions.DIRY[dir];
        return { x, y, dir };
      });
    },
    // 7 is a diagonal direction, 6 a straight one.
    stepMs(walkSpeed, diagonal) {
      return path.stepDelayMs(diagonal ? 7 : 6, speed.normalizeSpeedMs(walkSpeed));
    },
  };
}

// mulberry32: the same numbers for the same seed on every machine.
export function makeRng(seed) {
  let state = seed >>> 0;
  const next = () => {
    state = (state + 0x6D2B79F5) >>> 0;
    let t = state;
    t = Math.imul(t ^ (t >>> 15), t | 1);
    t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
    return (t ^ (t >>> 14)) >>> 0;
  };
  const int = (n) => next() % n;
  return {
    int,
    range: (lo, hi) => lo + int(hi - lo + 1),
    chance: (percent) => int(100) < percent,
    pick: (list) => list[int(list.length)],
  };
}

// --- grids ---------------------------------------------------------------------------------

function makeGrid(width, height, fill = 1) {
  return { width, height, cells: new Uint8Array(width * height).fill(fill) };
}

function inside(grid, x, y) {
  return x >= 0 && y >= 0 && x < grid.width && y < grid.height;
}

function walkable(grid, x, y) {
  return inside(grid, x, y) && grid.cells[x + y * grid.width] !== 0;
}

function put(grid, x, y, value) {
  if (inside(grid, x, y)) grid.cells[x + y * grid.width] = value;
}

function gridFromRows(rows) {
  const grid = makeGrid(rows[0].length, rows.length);
  rows.forEach((row, y) => [...row].forEach((c, x) => put(grid, x, y, c === "." ? 1 : 0)));
  return grid;
}

// The server's maps never let a unit onto the last row or the last column.
function blockLastRowAndColumn(grid) {
  for (let x = 0; x < grid.width; x++) put(grid, x, grid.height - 1, 0);
  for (let y = 0; y < grid.height; y++) put(grid, grid.width - 1, y, 0);
  return grid;
}

function addNoise(rng, grid, percent) {
  for (let i = 0; i < grid.cells.length; i++) if (rng.chance(percent)) grid.cells[i] = 0;
  return grid;
}

// Walls across the whole grid, each with one to three gaps.
function addWalls(rng, grid, count) {
  for (let n = 0; n < count; n++) {
    const across = rng.chance(50);
    const line = rng.int(across ? grid.height : grid.width);
    const length = across ? grid.width : grid.height;
    const cell = (i) => (across ? [i, line] : [line, i]);
    for (let i = 0; i < length; i++) put(grid, ...cell(i), 0);
    for (let gaps = rng.range(1, 3); gaps > 0; gaps--) put(grid, ...cell(rng.int(length)), 1);
  }
  return grid;
}

// Diagonal walls one or two cells thick: a unit cannot slip between two cells that touch only
// at a corner, so the thin ones block as well as the thick ones.
function addStairs(rng, grid, count) {
  for (let n = 0; n < count; n++) {
    const x0 = rng.range(-grid.height, grid.width + grid.height);
    const lean = rng.chance(50) ? 1 : -1;
    const thick = rng.chance(30);
    for (let y = 0; y < grid.height; y++) {
      put(grid, x0 + lean * y, y, 0);
      if (thick) put(grid, x0 + lean * y + 1, y, 0);
    }
    for (let gaps = rng.range(0, 3); gaps > 0; gaps--) {
      const y = rng.int(grid.height);
      put(grid, x0 + lean * y, y, 1);
      put(grid, x0 + lean * y + 1, y, 1);
    }
  }
  return grid;
}

function checkerGrid(rng, width, height) {
  const grid = makeGrid(width, height);
  const odd = rng.int(2);
  const flip = rng.range(3, 25);
  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) put(grid, x, y, ((x + y) % 2 === odd) !== rng.chance(flip) ? 0 : 1);
  }
  return grid;
}

// Corridors one cell wide, dug depth-first between the cells with odd coordinates, and a few
// extra openings.
function mazeGrid(rng, width, height) {
  const grid = makeGrid(width, height, 0);
  const stack = [[1, 1]];
  put(grid, 1, 1, 1);
  while (stack.length > 0) {
    const [x, y] = stack[stack.length - 1];
    const ways = [[2, 0], [-2, 0], [0, 2], [0, -2]].filter(([dx, dy]) =>
      x + dx > 0 && y + dy > 0 && x + dx < width - 1 && y + dy < height - 1 && !walkable(grid, x + dx, y + dy));
    if (ways.length === 0) {
      stack.pop();
      continue;
    }
    const [dx, dy] = rng.pick(ways);
    put(grid, x + dx / 2, y + dy / 2, 1);
    put(grid, x + dx, y + dy, 1);
    stack.push([x + dx, y + dy]);
  }
  for (let extra = rng.int(1 + ((width * height) >> 4)); extra > 0; extra--) {
    put(grid, rng.range(1, width - 2), rng.range(1, height - 2), 1);
  }
  return grid;
}

// Solid blocks and hollow rooms with a door.
function addRooms(rng, grid, count) {
  for (let n = 0; n < count; n++) {
    const w = rng.range(1, 12), h = rng.range(1, 12);
    const x0 = rng.int(grid.width), y0 = rng.int(grid.height);
    const hollow = w > 2 && h > 2 && rng.chance(50);
    for (let y = y0; y < y0 + h; y++) {
      for (let x = x0; x < x0 + w; x++) {
        const border = x === x0 || y === y0 || x === x0 + w - 1 || y === y0 + h - 1;
        if (!hollow || border) put(grid, x, y, 0);
      }
    }
    if (hollow) {
      if (rng.chance(50)) put(grid, rng.range(x0 + 1, x0 + w - 2), rng.chance(50) ? y0 : y0 + h - 1, 1);
      else put(grid, rng.chance(50) ? x0 : x0 + w - 1, rng.range(y0 + 1, y0 + h - 2), 1);
    }
  }
  return grid;
}

// The kinds of random grid: how many the vector file holds, how many searches each gets, and
// how one is made. `origin` puts most starts in the corner at (0,0), where the search's node
// table treats cell (0,0) specially.
const KINDS = [
  { name: "open", grids: 40, searches: 12, make: (rng) => makeGrid(rng.range(1, 70), rng.range(1, 70)) },
  { name: "noise", grids: 150, searches: 10,
    make: (rng) => addNoise(rng, makeGrid(rng.range(4, 48), rng.range(4, 48)), rng.range(5, 45)) },
  { name: "walls with gaps", grids: 80, searches: 10,
    make: (rng) => addWalls(rng, makeGrid(rng.range(8, 60), rng.range(8, 60)), rng.range(1, 5)) },
  { name: "diagonal walls", grids: 40, searches: 10,
    make: (rng) => addStairs(rng, makeGrid(rng.range(6, 40), rng.range(6, 40)), rng.range(1, 4)) },
  { name: "checkerboard", grids: 30, searches: 8,
    make: (rng) => checkerGrid(rng, rng.range(4, 30), rng.range(4, 30)) },
  { name: "maze", grids: 60, searches: 10,
    make: (rng) => mazeGrid(rng, rng.range(4, 22) * 2 + 1, rng.range(4, 22) * 2 + 1) },
  { name: "rooms", grids: 40, searches: 12,
    make: (rng) => addRooms(rng, makeGrid(rng.range(20, 80), rng.range(20, 80)), rng.range(3, 12)) },
  { name: "corner at the origin", grids: 60, searches: 12, origin: true,
    make: (rng) => {
      const grid = makeGrid(rng.range(33, 44), rng.range(33, 44));
      if (rng.chance(50)) addNoise(rng, grid, rng.range(0, 15));
      else addWalls(rng, grid, rng.range(0, 3));
      return grid;
    } },
  { name: "large", grids: 12, searches: 40,
    make: (rng) => addWalls(rng, addNoise(rng, makeGrid(rng.range(100, 160), rng.range(100, 160)), rng.range(3, 20)), rng.range(0, 4)) },
];

function randomCell(rng, grid) {
  return [rng.int(grid.width), rng.int(grid.height)];
}

// A random cell that is walkable (or blocked) when 30 draws find one.
function randomCellOf(rng, grid, wantWalkable) {
  let cell = randomCell(rng, grid);
  for (let tries = 0; tries < 30 && walkable(grid, ...cell) !== wantWalkable; tries++) cell = randomCell(rng, grid);
  return cell;
}

function borderCell(rng, grid) {
  const [x, y] = randomCell(rng, grid);
  return rng.pick([[x, 0], [x, grid.height - 1], [0, y], [grid.width - 1, y]]);
}

// Endpoints for one search: mostly a walkable start and a goal within reach, and now and then
// the start as the goal, a blocked goal, a cell outside the map, or two cells on the border.
function randomSearch(rng, grid, origin) {
  const kind = rng.int(24);
  if (kind === 0) {
    const cell = randomCell(rng, grid);
    return [...cell, ...cell];
  }
  if (kind === 1) return [...randomCellOf(rng, grid, true), ...randomCellOf(rng, grid, false)];
  if (kind === 2) {
    const out = rng.pick([[-1, 0], [grid.width, 0], [0, -1], [0, grid.height], [-1000, 3], [5, 40000]]);
    const cell = randomCellOf(rng, grid, true);
    return rng.chance(50) ? [...out, ...cell] : [...cell, ...out];
  }
  if (kind <= 5) return [...borderCell(rng, grid), ...borderCell(rng, grid)];

  const start = origin && rng.chance(70)
    ? [rng.int(Math.min(3, grid.width)), rng.int(Math.min(3, grid.height))]
    : randomCellOf(rng, grid, true);
  const reach = rng.pick([1, 2, 3, 5, 8, 12, 16, 20, 26, 32, 36, 48]);
  const clamp = (v, size) => Math.max(0, Math.min(size - 1, v));
  let goal;
  for (let tries = 0; tries < 8; tries++) {
    goal = [clamp(start[0] + rng.range(-reach, reach), grid.width), clamp(start[1] + rng.range(-reach, reach), grid.height)];
    if (walkable(grid, ...goal)) break;
  }
  return [...start, ...goal];
}

// One random grid of a kind with its searches, as { note, grid, searches: [[x0, y0, x1, y1]] }.
function randomSection(rng, kind) {
  const grid = kind.make(rng);
  if (rng.chance(70)) blockLastRowAndColumn(grid);
  const searches = [];
  for (let n = 0; n < kind.searches; n++) searches.push(randomSearch(rng, grid, kind.origin === true));
  return { note: kind.name, grid, searches };
}

// `share` of the vector file's random grids, from the caller's generator.
export function randomSections(rng, share = 1) {
  const sections = [];
  for (const kind of KINDS) {
    for (let n = Math.ceil(kind.grids * share); n > 0; n--) sections.push(randomSection(rng, kind));
  }
  return sections;
}

// Every 3x3 grid, from the centre to each cell and across both diagonals: all the ways the
// cells around a step can be blocked.
function cornerSections() {
  const sections = [];
  for (let bits = 0; bits < 512; bits++) {
    const grid = makeGrid(3, 3);
    for (let i = 0; i < 9; i++) grid.cells[i] = (bits >> i) & 1;
    const searches = [];
    for (let i = 0; i < 9; i++) searches.push([1, 1, i % 3, (i / 3) | 0]);
    searches.push([0, 0, 2, 2], [2, 0, 0, 2]);
    sections.push({ note: bits === 0 ? "every 3x3 grid" : null, grid, searches });
  }
  return sections;
}

// Cases chosen by hand: the server's own tests, the length limit, the node table's entry 0,
// and endpoints on and outside the border.
function chosenSections() {
  const serverMap = (walls = []) => {
    const grid = blockLastRowAndColumn(makeGrid(64, 64));
    for (const [x, y] of walls) put(grid, x, y, 0);
    return grid;
  };
  const open40 = makeGrid(40, 40);
  const ends = (length) => [0, 1, 2, 3, 5, 7].flatMap((from) => [from + 30, from + 31, from + 32, from + 33, length - 1]
    .filter((to) => to < length).flatMap((to) => [[from, to], [to, from]]));
  return [
    { note: "the server's tests: an open 64x64 map", grid: serverMap(),
      searches: [[10, 10, 14, 10], [10, 10, 13, 13], [10, 10, 10, 7], [5, 5, 5, 5], [10, 10, 63, 10], [10, 10, 70, 10],
        [1, 5, 21, 5], [1, 5, 32, 5], [1, 5, 34, 5], [10, 10, 14, 12], [62, 62, 40, 50], [0, 0, 62, 62], [63, 63, 63, 63]] },
    { note: "the server's tests: a blocked goal", grid: serverMap([[12, 10]]), searches: [[10, 10, 12, 10], [12, 10, 12, 10], [12, 10, 10, 10]] },
    { note: "the server's tests: no cutting a corner", grid: serverMap([[11, 10]]), searches: [[10, 10, 11, 11], [11, 11, 10, 10]] },
    { note: "one cell", grid: makeGrid(1, 1), searches: [[0, 0, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1], [-1, 0, 0, 0], [0, -1, 0, 0], [1, 1, 0, 0]] },
    { note: "one blocked cell", grid: makeGrid(1, 1, 0), searches: [[0, 0, 0, 0]] },
    { note: "a row: 32 steps are a path, 33 are not", grid: makeGrid(40, 1), searches: ends(40).map(([a, b]) => [a, 0, b, 0]) },
    { note: "a column: the cells 32 apart share a table entry, and (0,0) gives its entry up", grid: makeGrid(1, 40),
      searches: ends(40).map(([a, b]) => [0, a, 0, b]) },
    { note: "an open 40x40 grid from and to its corners", grid: open40,
      searches: [[0, 0, 32, 31], [0, 0, 31, 31], [0, 0, 32, 32], [0, 0, 0, 32], [0, 0, 32, 0], [0, 0, 20, 31], [0, 0, 31, 20],
        [32, 31, 0, 0], [31, 31, 0, 0], [39, 39, 8, 8], [39, 39, 7, 7], [39, 0, 8, 31], [0, 39, 31, 8], [1, 1, 0, 32], [0, 1, 0, 32],
        [1, 0, 32, 31], [0, 0, 1, 32], [0, 0, 16, 32], [2, 2, 34, 34], [4, 4, 36, 4], [4, 4, 4, 36], [39, 39, 39, 39], [0, 0, 39, 39]] },
    { note: "detours around a wall: 30 and 32 steps are a path, 34 are not", grid: gridFromRows([
        ".".repeat(32),
        "." + "#".repeat(30) + ".",
        ".#" + ".".repeat(28) + "#.",
        ".#" + ".".repeat(28) + "#.",
        ".".repeat(32),
      ]),
      searches: [[2, 2, 2, 0], [9, 2, 9, 0], [13, 2, 13, 0], [14, 2, 14, 0], [15, 2, 15, 0], [16, 2, 16, 0], [17, 2, 17, 0],
        [18, 2, 18, 0], [14, 0, 14, 2], [15, 0, 15, 2], [17, 0, 17, 2], [14, 3, 14, 0], [9, 4, 9, 0], [9, 2, 9, 2], [0, 0, 31, 4]] },
    { note: "a closed room", grid: gridFromRows([
        ".......",
        ".#####.",
        ".#...#.",
        ".#...#.",
        ".#####.",
        ".......",
      ]),
      searches: [[0, 0, 3, 3], [3, 3, 0, 0], [2, 2, 4, 3], [0, 0, 6, 5], [6, 5, 0, 0], [0, 5, 6, 0]] },
    // In the next three a cell waiting in the heap is reached again more cheaply, and the
    // path depends on the heap update re-sorting the entry at that cell's old place too.
    { note: "a cheaper way to a waiting cell: the heap update decides between equal paths", grid: gridFromRows([
        ".#..###",
        ".......",
        "###.#..",
        "....#..",
        "##...#.",
        ".#.....",
        ".#.#.#.",
        "...#.#.",
        ".......",
      ]),
      searches: [[1, 0, 3, 8], [3, 8, 1, 0]] },
    { note: null, grid: gridFromRows([
        "...#..",
        ".##.#.",
        "#.....",
        "#.#...",
        "......",
        ".#..#.",
        "..#.##",
        "#....#",
        "..###.",
        "#.....",
        ".#.#..",
        ".#...#",
      ]),
      searches: [[4, 0, 1, 6], [1, 6, 4, 0]] },
    { note: null, grid: gridFromRows([
        "#.....",
        "......",
        ".....#",
        "......",
        "......",
        "......",
        "......",
        "...#..",
        "#.....",
        "......",
        "....#.",
        "......",
        "......",
      ]),
      searches: [[3, 0, 4, 12], [4, 12, 3, 0]] },
  ];
}

const STEP_SPEEDS = [-2147483648, -5, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 49, 50, 99, 100, 101, 149, 150, 151, 155, 165,
  175, 199, 200, 250, 333, 400, 999, 1000, 4681, 32767, 32768, 65534, 65535, 65536, 100000, 2147483647];

// --- the file ------------------------------------------------------------------------------

// The text of the vector file for this server, and what it holds.
export function buildVectors(server) {
  const rng = makeRng(VECTORS_SEED);
  const lines = [
    "# Walk paths and step times as the server gives them, for map/grid_path.c.",
    "# Recorded from the backend's path search. Do not edit.",
  ];
  const counts = { grids: 0, paths: 0, times: 0, found: 0, empty: 0, none: 0, longest: 0, atLimit: 0 };

  for (const speed of [...STEP_SPEEDS, ...Array.from({ length: 40 }, () => rng.range(1, 2000))]) {
    for (const diagonal of [0, 1]) {
      lines.push(`T ${speed} ${diagonal} ${server.stepMs(speed, diagonal)}`);
      counts.times++;
    }
  }

  for (const { note, grid, searches } of [...chosenSections(), ...cornerSections(), ...randomSections(rng)]) {
    if (note != null) lines.push(`# ${note}`);
    lines.push(`G ${grid.width} ${grid.height}`);
    counts.grids++;
    for (let y = 0; y < grid.height; y++) {
      let row = "";
      for (let x = 0; x < grid.width; x++) row += walkable(grid, x, y) ? "." : "#";
      lines.push(row);
    }
    for (const [x0, y0, x1, y1] of searches) {
      const steps = server.search(grid, x0, y0, x1, y1);
      const tail = steps == null ? "" : steps.map((s) => ` ${s.x} ${s.y} ${s.dir}`).join("");
      lines.push(`P ${x0} ${y0} ${x1} ${y1} ${steps == null ? -1 : steps.length}${tail}`);
      counts.paths++;
      if (steps == null) counts.none++;
      else if (steps.length === 0) counts.empty++;
      else counts.found++;
      if (steps != null) {
        counts.longest = Math.max(counts.longest, steps.length);
        if (steps.length === 32) counts.atLimit++;
      }
    }
  }
  lines.push(`E ${counts.grids} ${counts.paths} ${counts.times}`);
  return { text: lines.join("\n") + "\n", counts };
}

// The vector file's contents as { times: [{ speed, diagonal, ms }], grids: [{ grid, paths:
// [{ x0, y0, x1, y1, steps }] }] } with steps null for no path. Throws on a file that does not
// follow the format or stops early.
export function parseVectors(text) {
  const lines = text.split(/\r?\n/);
  const times = [], grids = [];
  let paths = 0, ended = false;
  const numbers = (line, at) => {
    const values = line.split(" ").slice(1).map(Number);
    if (values.some((v) => !Number.isInteger(v))) throw new Error(`line ${at + 1}: not numbers: ${line}`);
    return values;
  };
  for (let at = 0; at < lines.length; at++) {
    const line = lines[at];
    if (line === "" || line.startsWith("#")) continue;
    if (ended) throw new Error(`line ${at + 1}: text after the E line`);
    const values = numbers(line, at);
    if (line.startsWith("T ") && values.length === 3) {
      times.push({ speed: values[0], diagonal: values[1], ms: values[2] });
    } else if (line.startsWith("G ") && values.length === 2) {
      const grid = makeGrid(values[0], values[1]);
      for (let y = 0; y < grid.height; y++) {
        const row = lines[++at] ?? "";
        if (!new RegExp(`^[.#]{${grid.width}}$`).test(row)) throw new Error(`line ${at + 1}: not a row of ${grid.width} cells`);
        for (let x = 0; x < grid.width; x++) put(grid, x, y, row[x] === "." ? 1 : 0);
      }
      grids.push({ grid, paths: [] });
    } else if (line.startsWith("P ") && grids.length > 0 && values.length === 5 + 3 * Math.max(0, values[4])) {
      const [x0, y0, x1, y1, count] = values;
      const steps = count < 0 ? null : [];
      for (let i = 0; i < count; i++) steps.push({ x: values[5 + 3 * i], y: values[6 + 3 * i], dir: values[7 + 3 * i] });
      grids[grids.length - 1].paths.push({ x0, y0, x1, y1, steps });
      paths++;
    } else if (line.startsWith("E ") && values.length === 3) {
      if (values[0] !== grids.length || values[1] !== paths || values[2] !== times.length) {
        throw new Error(`line ${at + 1}: the file holds ${grids.length} grids, ${paths} paths, ${times.length} times`);
      }
      ended = true;
    } else {
      throw new Error(`line ${at + 1}: not a vector line: ${line}`);
    }
  }
  if (!ended) throw new Error("the file stops before its E line");
  return { times, grids };
}

async function main() {
  const [checkout, output] = process.argv.slice(2);
  if (checkout == null || output == null) throw new Error("usage: node tools/grid_path_vectors.mjs <server checkout> <output file>");
  const { text, counts } = buildVectors(await loadServer(checkout));
  mkdirSync(dirname(resolve(output)), { recursive: true });
  writeFileSync(output, text);
  console.log(`${output}: ${text.length} bytes, ${counts.grids} grids, ${counts.paths} paths `
    + `(${counts.found} found, ${counts.empty} start is goal, ${counts.none} no path; longest ${counts.longest} steps, `
    + `${counts.atLimit} of 32 steps), ${counts.times} step times`);
}

if (process.argv[1] != null && resolve(process.argv[1]).toLowerCase() === fileURLToPath(import.meta.url).toLowerCase()) {
  await main();
}
