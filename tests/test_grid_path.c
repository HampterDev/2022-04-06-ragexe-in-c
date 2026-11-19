/* Checks src/body/grid_path.c against the server's answers in tests/data/grid_path_vectors.txt
 * (written by tools/grid_path_vectors.mjs, which describes the lines): every path with the same
 * cells and directions in the same order, and every step time.
 *
 *   test_grid_path <vector file>
 *
 * Prints one summary line and returns 0 when everything matches, else prints the first
 * difference and returns 1. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "body/grid_path.h"

/* Room for more steps than the longest path, so that the search's own limit is what refuses a
 * longer one. */
#define OUT_ROOM (GRID_PATH_MAX_STEPS + 8)
/* Marks the steps after a path's last, which the search must leave alone. */
#define UNTOUCHED (-77)

static uint32_t scratch[(GRID_PATH_SCRATCH_SIZE + 3) / 4];
static unsigned long line_number;

static int fail(const char *what)
{
    printf("grid_path: line %lu: %s\n", line_number, what);
    return 1;
}

/* Returns the next line without its line end, or NULL at the end of the text. */
static char *next_line(char **cursor)
{
    char *line = *cursor, *end;
    if (!*line) return NULL;
    end = line + strcspn(line, "\r\n");
    *cursor = end;
    if (**cursor == '\r') ++*cursor;
    if (**cursor == '\n') ++*cursor;
    *end = 0;
    ++line_number;
    return line;
}

/* Reads `count` integers from *text and moves it past them. Returns 0 when one is missing. */
static int read_ints(char **text, long *out, int count)
{
    int i;
    for (i = 0; i < count; ++i) {
        char *end;
        out[i] = strtol(*text, &end, 10);
        if (end == *text) return 0;
        *text = end;
    }
    return 1;
}

static int run_search(const struct GridPathMap *map, const long *ends, struct GridPathStep *out, int max_steps)
{
    int i;
    for (i = 0; i < OUT_ROOM; ++i) out[i].x = out[i].y = out[i].dir = UNTOUCHED;
    return grid_path_search(map, (int)ends[0], (int)ends[1], (int)ends[2], (int)ends[3], out, max_steps,
                            scratch, sizeof scratch);
}

/* One P line: the search gives the expected steps, writes nothing after them, and gives no path
 * when the caller has room for one step fewer. */
static int check_path(const struct GridPathMap *map, char *text)
{
    struct GridPathStep out[OUT_ROOM];
    long head[5], want[3];
    int count, i;

    if (!map->walkable) return fail("a P line before the first grid");
    if (!read_ints(&text, head, 5) || head[4] < -1 || head[4] > GRID_PATH_MAX_STEPS) return fail("not a P line");
    count = run_search(map, head, out, OUT_ROOM);
    if (count != head[4]) {
        printf("grid_path: line %lu: (%ld,%ld) to (%ld,%ld) on a %dx%d grid: %d steps, the server gives %ld\n",
               line_number, head[0], head[1], head[2], head[3], map->width, map->height, count, head[4]);
        return 1;
    }
    for (i = 0; i < count; ++i) {
        if (!read_ints(&text, want, 3)) return fail("a P line with too few steps");
        if (out[i].x != want[0] || out[i].y != want[1] || out[i].dir != want[2]) {
            printf("grid_path: line %lu: (%ld,%ld) to (%ld,%ld): step %d is (%d,%d) direction %d, "
                   "the server gives (%ld,%ld) direction %ld\n",
                   line_number, head[0], head[1], head[2], head[3], i + 1, out[i].x, out[i].y, out[i].dir,
                   want[0], want[1], want[2]);
            return 1;
        }
    }
    if (*text) return fail("a P line with text after its steps");
    for (i = count < 0 ? 0 : count; i < OUT_ROOM; ++i) {
        if (out[i].x != UNTOUCHED || out[i].y != UNTOUCHED || out[i].dir != UNTOUCHED)
            return fail("the search wrote past the steps it returned");
    }
    if (count > 0 && run_search(map, head, out, count - 1) != -1)
        return fail("a path longer than max_steps was returned");
    if (count > 0 && out[0].x != UNTOUCHED) return fail("a refused path was written");
    return 0;
}

/* The arguments the search refuses, on a 2x1 grid with a one-step path. */
static int check_arguments(void)
{
    static const unsigned char cells[2] = {1, 1};
    struct GridPathMap map = {2, 1, cells};
    struct GridPathMap no_cells = {2, 1, NULL};
    struct GridPathMap too_wide = {GRID_PATH_MAX_SIDE + 1, 1, cells};
    struct GridPathStep out[1];

    if (grid_path_scratch_size() != GRID_PATH_SCRATCH_SIZE) return fail("grid_path_scratch_size");
    if (grid_path_search(&map, 0, 0, 1, 0, out, 1, scratch, sizeof scratch) != 1 || out[0].x != 1 || out[0].y != 0
        || out[0].dir != 6) return fail("the one-step path");
    if (grid_path_search(&map, 0, 0, 1, 0, NULL, 1, scratch, sizeof scratch) != -1) return fail("no room for steps");
    if (grid_path_search(&map, 0, 0, 0, 0, NULL, 0, scratch, sizeof scratch) != 0) return fail("start is goal");
    if (grid_path_search(NULL, 0, 0, 1, 0, out, 1, scratch, sizeof scratch) != -1) return fail("no map");
    if (grid_path_search(&no_cells, 0, 0, 1, 0, out, 1, scratch, sizeof scratch) != -1) return fail("no cells");
    if (grid_path_search(&too_wide, 0, 0, 1, 0, out, 1, scratch, sizeof scratch) != -1) return fail("too wide");
    if (grid_path_search(&map, 0, 0, 1, 0, out, 1, NULL, sizeof scratch) != -1) return fail("no scratch");
    if (grid_path_search(&map, 0, 0, 1, 0, out, 1, scratch, GRID_PATH_SCRATCH_SIZE - 1) != -1)
        return fail("small scratch");
    if (grid_path_search(&map, 0, 0, 1, 0, out, 1, (char *)scratch + 1, sizeof scratch - 1) != -1)
        return fail("misaligned scratch");
    return 0;
}

int main(int argc, char **argv)
{
    struct GridPathMap map = {0, 0, NULL};
    unsigned char *cells = NULL;
    unsigned long grids = 0, paths = 0, times = 0;
    char *text, *cursor, *line;
    long size, v[3];
    int ended = 0;
    FILE *file;

    if (argc != 2) { printf("usage: test_grid_path <vector file>\n"); return 1; }
    file = fopen(argv[1], "rb");
    if (!file) { printf("grid_path: cannot open %s\n", argv[1]); return 1; }
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, 0, SEEK_SET);
    text = size >= 0 ? malloc((size_t)size + 1) : NULL;
    if (!text || fread(text, 1, (size_t)size, file) != (size_t)size) { printf("grid_path: cannot read %s\n", argv[1]); return 1; }
    text[size] = 0;
    fclose(file);

    /* The search must not depend on what the scratch memory holds. */
    memset(scratch, 0xA5, sizeof scratch);
    if (check_arguments()) return 1;

    cursor = text;
    while ((line = next_line(&cursor)) != NULL) {
        char *rest = line + 1;
        if (!*line || *line == '#') continue;
        if (ended) return fail("text after the E line");
        if (*line == 'T') {
            if (!read_ints(&rest, v, 3) || *rest) return fail("not a T line");
            if (grid_path_step_ms((int)v[0], (int)v[1]) != (unsigned long)v[2]) {
                printf("grid_path: line %lu: a step at speed %ld (diagonal %ld) takes %u ms, the server gives %ld\n",
                       line_number, v[0], v[1], grid_path_step_ms((int)v[0], (int)v[1]), v[2]);
                return 1;
            }
            ++times;
        } else if (*line == 'G') {
            long x, y;
            if (!read_ints(&rest, v, 2) || *rest || v[0] < 1 || v[1] < 1 || v[0] > GRID_PATH_MAX_SIDE
                || v[1] > GRID_PATH_MAX_SIDE) return fail("not a G line");
            free(cells);
            cells = malloc((size_t)v[0] * (size_t)v[1]);
            if (!cells) return fail("no memory for the grid");
            for (y = 0; y < v[1]; ++y) {
                line = next_line(&cursor);
                if (!line || strlen(line) != (size_t)v[0] || strspn(line, ".#") != (size_t)v[0])
                    return fail("not a grid row");
                for (x = 0; x < v[0]; ++x) cells[y * v[0] + x] = line[x] == '.';
            }
            map.width = (int)v[0];
            map.height = (int)v[1];
            map.walkable = cells;
            ++grids;
        } else if (*line == 'P') {
            if (check_path(&map, rest)) return 1;
            ++paths;
        } else if (*line == 'E') {
            if (!read_ints(&rest, v, 3) || *rest) return fail("not an E line");
            if ((unsigned long)v[0] != grids || (unsigned long)v[1] != paths || (unsigned long)v[2] != times)
                return fail("the E line's counts are not the file's");
            ended = 1;
        } else {
            return fail("not a vector line");
        }
    }
    if (!ended) return fail("the file stops before its E line");
    free(cells);
    free(text);
    printf("grid_path: %lu paths on %lu grids and %lu step times match the server\n", paths, grids, times);
    return 0;
}
