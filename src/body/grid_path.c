#include "body/grid_path.h"

/* The same rule as the server's consumers/spatial/hercules-path.js; see grid_path.h. Each
 * function names the server function it follows. */

#define MOVE_COST 10
#define MOVE_DIAGONAL_COST 14
#define DEFAULT_SPEED_MS 150
#define MAX_SPEED_MS 0xFFFF

/* The table has one entry per (x + y * 32) mod 1024. One more node holds the cell that takes
 * entry 0 over from cell (0,0), so that cell (0,0) stays a node of its own. */
#define TABLE_SIZE (GRID_PATH_MAX_STEPS * GRID_PATH_MAX_STEPS)
#define SPARE_NODE TABLE_SIZE
#define NODE_COUNT (TABLE_SIZE + 1)
#define NO_PARENT 0xFFFFu

enum { NODE_EMPTY, NODE_OPEN, NODE_CLOSED };
enum { ALLOW_NORTH = 1, ALLOW_WEST = 2, ALLOW_SOUTH = 4, ALLOW_EAST = 8 };

struct Node {
    int32_t g, f;       /* cost from the start; g plus the estimate to the goal */
    int16_t x, y;
    uint16_t parent;    /* node index of the cell before this one, NO_PARENT at the start */
    uint8_t state;
};

struct Scratch {
    struct Node node[NODE_COUNT];
    uint16_t heap[NODE_COUNT];  /* node indices; an open node is in the heap exactly once */
    uint16_t heap_count;
    uint16_t entry0;            /* the node table entry 0 stands for: 0, later SPARE_NODE */
};

/* GRID_PATH_SCRATCH_SIZE is the scratch size; written without _Static_assert, which the
   tests that compile this file in the compiler's default mode do not have. */
typedef char grid_path_scratch_size_is_right[sizeof(struct Scratch) == GRID_PATH_SCRATCH_SIZE ? 1 : -1];

size_t grid_path_scratch_size(void)
{
    return GRID_PATH_SCRATCH_SIZE;
}

static int blocked(const struct GridPathMap *map, int x, int y)
{
    return map->walkable[(size_t)y * (size_t)map->width + (size_t)x] == 0;
}

static int32_t heap_f(const struct Scratch *s, int i)
{
    return s->node[s->heap[i]].f;
}

static void heap_swap(struct Scratch *s, int i, int j)
{
    uint16_t t = s->heap[i];
    s->heap[i] = s->heap[j];
    s->heap[j] = t;
}

/* siftDown: moves entry i towards the root, not above `top`, while its parent has a larger f. */
static void heap_rise(struct Scratch *s, int top, int i)
{
    while (i > top) {
        int parent = (i - 1) >> 1;
        if (heap_f(s, parent) <= heap_f(s, i)) break;
        heap_swap(s, parent, i);
        i = parent;
    }
}

/* siftUp: moves entry `top` down to a leaf, each time below the child with the smaller f (the
 * right one when they tie), then lets it rise again as far as `top`. */
static void heap_sink(struct Scratch *s, int top)
{
    int i = top;
    int left = i * 2 + 1;
    while (left < s->heap_count) {
        int right = left + 1;
        int child = (right >= s->heap_count || heap_f(s, left) < heap_f(s, right)) ? left : right;
        heap_swap(s, i, child);
        i = child;
        left = i * 2 + 1;
    }
    heap_rise(s, top, i);
}

/* heapPush */
static void heap_push(struct Scratch *s, unsigned node)
{
    s->heap[s->heap_count] = (uint16_t)node;
    heap_rise(s, 0, s->heap_count);
    s->heap_count++;
}

/* heapPop: removes the root; the last entry takes its place and sinks. */
static void heap_pop(struct Scratch *s)
{
    s->heap_count--;
    if (s->heap_count == 0) return;
    s->heap[0] = s->heap[s->heap_count];
    heap_sink(s, 0);
}

/* heapUpdate: puts a node whose f got smaller back in order. Returns 1 when the node is not in
 * the heap. */
static int heap_update(struct Scratch *s, unsigned node)
{
    int i = 0;
    while (i < s->heap_count && s->heap[i] != node) i++;
    if (i == s->heap_count) return 1;
    heap_rise(s, 0, i);
    heap_sink(s, i);
    return 0;
}

static int distance(int a, int b)
{
    return a < b ? b - a : a - b;
}

/* addPath: reaches cell (x,y) with cost g from the node `parent`. A known cell takes the new
 * way when it is cheaper and is opened again. A new cell gets its table entry. Returns 1 when
 * the entry belongs to another cell; an entry that holds cell (0,0) is taken over. */
static int add_node(struct Scratch *s, int x, int y, int g, unsigned parent, int x1, int y1)
{
    int f = g + MOVE_COST * (distance(x, x1) + distance(y, y1));
    unsigned entry = (unsigned)(x + y * GRID_PATH_MAX_STEPS) & (TABLE_SIZE - 1);
    unsigned index = entry == 0 ? s->entry0 : entry;
    struct Node *node = &s->node[index];

    if (node->state != NODE_EMPTY) {
        if (node->x == x && node->y == y) {
            if (g < node->g) {
                node->g = g;
                node->parent = (uint16_t)parent;
                node->f = f;
                if (node->state == NODE_CLOSED) heap_push(s, index);
                else if (heap_update(s, index)) return 1;
                node->state = NODE_OPEN;
            }
            return 0;
        }
        if (node->x != 0 || node->y != 0) return 1;
        index = SPARE_NODE;
        s->entry0 = SPARE_NODE;
        node = &s->node[index];
    }
    node->g = g;
    node->f = f;
    node->x = (int16_t)x;
    node->y = (int16_t)y;
    node->parent = (uint16_t)parent;
    node->state = NODE_OPEN;
    heap_push(s, index);
    return 0;
}

/* searchPath, then aStarPath. */
int grid_path_search(const struct GridPathMap *map, int x0, int y0, int x1, int y1,
                     struct GridPathStep *out, int max_steps, void *scratch, size_t scratch_size)
{
    /* walk_choices: the direction of a step, at [1 - dy][dx + 1]. */
    static const unsigned char step_dir[3][3] = {{1, 0, 7}, {2, 0, 6}, {3, 4, 5}};
    struct Scratch *s = scratch;
    unsigned current, index;
    int i, steps;

    if (!map || !map->walkable || map->width <= 0 || map->height <= 0
        || map->width > GRID_PATH_MAX_SIDE || map->height > GRID_PATH_MAX_SIDE) return -1;
    if (!scratch || scratch_size < sizeof *s || ((uintptr_t)scratch & (_Alignof(struct Scratch) - 1))) return -1;
    if (!out || max_steps < 0) max_steps = 0;

    if (x0 < 0 || x0 >= map->width || y0 < 0 || y0 >= map->height) return -1;
    if (x1 < 0 || x1 >= map->width || y1 < 0 || y1 >= map->height || blocked(map, x1, y1)) return -1;
    if (x0 == x1 && y0 == y1) return 0;

    for (i = 0; i < NODE_COUNT; i++) s->node[i].state = NODE_EMPTY;
    s->heap_count = 0;
    s->entry0 = 0;
    add_node(s, x0, y0, 0, NO_PARENT, x1, y1);

    for (;;) {
        int x, y, g, allowed = 0, full = 0;

        if (s->heap_count == 0) return -1;
        current = s->heap[0];
        heap_pop(s);
        x = s->node[current].x;
        y = s->node[current].y;
        g = s->node[current].g;
        s->node[current].state = NODE_CLOSED;
        if (x == x1 && y == y1) break;

        if (y < map->height - 1 && !blocked(map, x, y + 1)) allowed |= ALLOW_NORTH;
        if (y > 0 && !blocked(map, x, y - 1)) allowed |= ALLOW_SOUTH;
        if (x < map->width - 1 && !blocked(map, x + 1, y)) allowed |= ALLOW_EAST;
        if (x > 0 && !blocked(map, x - 1, y)) allowed |= ALLOW_WEST;

#define CAN(dirs) ((allowed & (dirs)) == (dirs))
#define REACH(dx, dy, cost) (full += add_node(s, x + (dx), y + (dy), g + (cost), current, x1, y1))
        if (CAN(ALLOW_SOUTH | ALLOW_EAST) && !blocked(map, x + 1, y - 1)) REACH(1, -1, MOVE_DIAGONAL_COST);
        if (CAN(ALLOW_EAST)) REACH(1, 0, MOVE_COST);
        if (CAN(ALLOW_NORTH | ALLOW_EAST) && !blocked(map, x + 1, y + 1)) REACH(1, 1, MOVE_DIAGONAL_COST);
        if (CAN(ALLOW_NORTH)) REACH(0, 1, MOVE_COST);
        if (CAN(ALLOW_NORTH | ALLOW_WEST) && !blocked(map, x - 1, y + 1)) REACH(-1, 1, MOVE_DIAGONAL_COST);
        if (CAN(ALLOW_WEST)) REACH(-1, 0, MOVE_COST);
        if (CAN(ALLOW_SOUTH | ALLOW_WEST) && !blocked(map, x - 1, y - 1)) REACH(-1, -1, MOVE_DIAGONAL_COST);
        if (CAN(ALLOW_SOUTH)) REACH(0, -1, MOVE_COST);
#undef REACH
#undef CAN
        if (full) return -1;
    }

    /* The goal's parents lead back to the start; g falls along them, so they end there. */
    steps = 0;
    for (index = current; s->node[index].parent != NO_PARENT; index = s->node[index].parent) {
        if (++steps > GRID_PATH_MAX_STEPS) return -1;
    }
    if (steps > max_steps) return -1;
    index = current;
    for (i = steps; i-- > 0; ) {
        const struct Node *node = &s->node[index];
        const struct Node *from = &s->node[node->parent];
        out[i].x = node->x;
        out[i].y = node->y;
        out[i].dir = step_dir[1 - (node->y - from->y)][node->x - from->x + 1];
        index = node->parent;
    }
    return steps;
}

/* stepDelayMs at the speed normalizeSpeedMs (movement-speed.js) gives an entity. */
unsigned grid_path_step_ms(int walk_speed, int diagonal)
{
    unsigned speed = walk_speed <= 0 ? DEFAULT_SPEED_MS
                   : walk_speed > MAX_SPEED_MS ? MAX_SPEED_MS : (unsigned)walk_speed;
    return diagonal ? speed * MOVE_DIAGONAL_COST / MOVE_COST : speed;
}
