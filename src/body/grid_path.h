#ifndef BODY_GRID_PATH_H
#define BODY_GRID_PATH_H

/* The server's walk path search and step time, in freestanding C: no allocation, no globals,
 * no library calls, integer math only.
 *
 * This is the same rule as the server's consumers/spatial/hercules-path.js (searchPath without
 * `easy`, and stepDelayMs; the code is in consumers/shared/src/hercules-path.js), which is the
 * search the server's plain walks run and is itself ported from Hercules path.c path_search.
 * The two must stay in step: for the same grid and endpoints both give the same cells in the
 * same order, also where several shortest paths exist. tools/grid_path_vectors.mjs writes the
 * server's answers to tests/data/grid_path_vectors.txt, tests/test_grid_path.c checks this file
 * against them, and tools/check_grid_path_wasm.mjs checks the wasm build of this file against
 * them and against the server's code directly.
 *
 * The rule:
 * - A* over the 8 neighbours. A straight step costs 10 and a diagonal one 14; the estimate to
 *   the goal is 10 times the Manhattan distance.
 * - A cell's neighbours are tried in the order south-east, east, north-east, north, north-west,
 *   west, south-west, south (north is y + 1). A diagonal step needs both straight cells beside
 *   it walkable as well as its own cell.
 * - The open cells are a binary heap ordered by cost so far plus estimate. Its push, pop and
 *   update are path.c's, which settles the order of cells that tie.
 * - A searched cell is kept in a table of 32 x 32 entries at (x + y * 32) mod 1024. A second
 *   cell that needs an occupied entry ends the search without a path, so the search covers
 *   little more than a 32 x 32 area. An entry holding cell (0,0) counts as free, as in path.c.
 * - A path longer than GRID_PATH_MAX_STEPS steps is no path.
 *
 * Directions are the server's codes: 0 north (y + 1), 1 north-west, 2 west, 3 south-west,
 * 4 south, 5 south-east, 6 east, 7 north-east. The odd ones are diagonal. */

#include <stddef.h>
#include <stdint.h>

#define GRID_PATH_MAX_STEPS 32
/* The widest and tallest map searched; the server's map sizes are 16-bit. */
#define GRID_PATH_MAX_SIDE 0x8000
/* The working memory one search needs, in bytes. It must be aligned for a 32-bit integer. */
#define GRID_PATH_SCRATCH_SIZE 18456

/* One byte per cell, row y at walkable + y * width: 0 blocks the cell, any other value is
 * walkable. Cells outside the map are blocked. The server blocks the cells that are not
 * walkable ground (gat types 1 and 5), the map's last row and last column, and cells under an
 * ice wall; the caller clears those. */
struct GridPathMap {
    int width, height;
    const unsigned char *walkable;
};

/* The cell a step ends on and the direction of the step. */
struct GridPathStep {
    int x, y;
    int dir;
};

/* GRID_PATH_SCRATCH_SIZE, for a caller that cannot read this header (the wasm host). */
size_t grid_path_scratch_size(void);

/* Finds the server's path from (x0,y0) to (x1,y1) and writes its steps, the cells after the
 * start in walking order, to out. Returns the number of steps, 0 when the start is the goal, or
 * -1 when there is no path: an endpoint outside the map, a blocked goal (also when it is the
 * start), no way through, a full table entry, a path longer than GRID_PATH_MAX_STEPS or than
 * max_steps, or arguments that cannot be searched (no map, a map larger than GRID_PATH_MAX_SIDE,
 * scratch missing, misaligned or smaller than GRID_PATH_SCRATCH_SIZE). out is written only when
 * steps are returned. The scratch memory needs no set-up and holds nothing afterwards. */
int grid_path_search(const struct GridPathMap *map, int x0, int y0, int x1, int y1,
                     struct GridPathStep *out, int max_steps, void *scratch, size_t scratch_size);

/* The server's time in milliseconds for one step at walk_speed, the milliseconds per straight
 * cell: walk_speed for a straight step and walk_speed * 14 / 10 rounded down for a diagonal one
 * (diagonal != 0; pass dir & 1). As on the server, a speed of 0 or less counts as 150 and one
 * above 65535 as 65535. */
unsigned grid_path_step_ms(int walk_speed, int diagonal);

#endif
