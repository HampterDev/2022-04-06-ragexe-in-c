#include "world/pathfinder_state.h"
#include "resources/attr.h"
#include "core/numeric.h"
#include "render/math3d.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum { PATH_LOCAL_NODES = 149, CELL_BLOCKED = 1, CELL_BLOCKED_SNIPE = 5, KIND_SHOOT = 12 };

struct PathNode {
    int x, y;
    struct PathNode *parent;
    int64_t g, f;
    int state, dir;
    size_t slot;
};
struct PathSearch {
    struct PathNode *nodes;
    struct PathNode **open, **index;
    size_t capacity, index_capacity, limit, node_count, open_count;
    int tx, ty;
};
struct PathMapEntry {
    char *file;
    size_t name_length;
    struct C3dAttr *attr;
};

struct PathFinder g_pathFinder;

void path_finder_construct(struct PathFinder *self)
{
    memset(self, 0, sizeof *self);
}

void path_finder_clear_search(struct PathFinder *self)
{
    struct PathSearch *search = self->search;
    size_t i;
    if (!search)
        return;
    for (i = 0; i < search->node_count; ++i)
        search->index[search->nodes[i].slot] = NULL;
    search->node_count = search->open_count = 0;
}

void path_finder_destruct(struct PathFinder *self)
{
    size_t i;
    if (self->search) {
        free(self->search->nodes);
        free(self->search->open);
        free(self->search->index);
        free(self->search);
    }
    for (i = 0; i < self->map_count; ++i) {
        c3d_attr_destroy(self->maps[i].attr);
        free(self->maps[i].file);
    }
    free(self->maps);
    memset(self, 0, sizeof *self);
}

void path_finder_set_attr(struct PathFinder *self, struct C3dAttr *attr)
{
    if (self->attr != attr)
        path_finder_clear_search(self);
    self->attr = attr;
}

static int map_name_compare(const char *name, size_t length, const struct PathMapEntry *entry)
{
    size_t common = length < entry->name_length ? length : entry->name_length;
    int result = memcmp(name, entry->file, common);
    if (result)
        return result;
    return (length > entry->name_length) - (length < entry->name_length);
}

unsigned char path_finder_use_map_attr(struct PathFinder *self, const char *name, size_t length)
{
    size_t low = 0, high = self->map_count, middle, capacity;
    struct PathMapEntry entry, *maps;
    if (!name || length > SIZE_MAX - 5)
        return 0;
    while (low < high) {
        middle = low + (high - low) / 2;
        if (map_name_compare(name, length, &self->maps[middle]) > 0)
            low = middle + 1;
        else
            high = middle;
    }
    if (low < self->map_count && !map_name_compare(name, length, &self->maps[low])) {
        path_finder_set_attr(self, self->maps[low].attr);
        return 1;
    }
    entry.file = malloc(length + 5);
    if (!entry.file)
        return 0;
    memcpy(entry.file, name, length);
    memcpy(entry.file + length, ".gat", 5);
    entry.name_length = length;
    entry.attr = (struct C3dAttr *)attribute_resource_create();
    if (!entry.attr)
        goto failed;
    if (!c3d_attr_load(entry.attr, entry.file))
        goto failed;
    if (self->map_count == self->map_capacity) {
        const size_t maximum = SIZE_MAX / sizeof *maps;
        if (self->map_capacity == maximum)
            goto failed;
        capacity = self->map_capacity ? self->map_capacity : 8;
        capacity = capacity > maximum / 2 ? maximum : capacity * 2;
        maps = realloc(self->maps, capacity * sizeof *maps);
        if (!maps)
            goto failed;
        self->maps = maps;
        self->map_capacity = capacity;
    }
    memmove(self->maps + low + 1, self->maps + low,
            (self->map_count - low) * sizeof *self->maps);
    self->maps[low] = entry;
    ++self->map_count;
    path_finder_set_attr(self, entry.attr);
    return 1;
failed:
    if (entry.attr)
        c3d_attr_destroy(entry.attr);
    free(entry.file);
    return 0;
}

unsigned char path_finder_cell_walkable(struct PathFinder *self, int x, int y)
{
    return c3d_attr_cell_walkable(self->attr, x, y);
}

static int prepare_search(struct PathFinder *self)
{
    struct PathSearch *search = self->search;
    struct PathNode *nodes;
    struct PathNode **open, **index;
    size_t limit = PATH_LOCAL_NODES, index_capacity = 2;
    path_finder_clear_search(self);
    if (self->useHeapNodes) {
        if ((size_t)self->attr->height > SIZE_MAX / (size_t)self->attr->width)
            return 0;
        limit = (size_t)self->attr->width * (size_t)self->attr->height;
    }
    if (limit > SIZE_MAX / sizeof *nodes || limit > SIZE_MAX / sizeof *open)
        return 0;
    while (index_capacity / 2 < limit) {
        if (index_capacity > SIZE_MAX / 2 / sizeof *index)
            return 0;
        index_capacity *= 2;
    }
    if (!search) {
        search = calloc(1, sizeof *search);
        if (!search)
            return 0;
        self->search = search;
    }
    if (search->capacity < limit) {
        nodes = malloc(limit * sizeof *nodes);
        open = malloc(limit * sizeof *open);
        index = calloc(index_capacity, sizeof *index);
        if (!nodes || !open || !index) {
            free(nodes); free(open); free(index);
            return 0;
        }
        free(search->nodes); free(search->open); free(search->index);
        search->nodes = nodes; search->open = open; search->index = index;
        search->capacity = limit; search->index_capacity = index_capacity;
    }
    search->limit = limit;
    return 1;
}

static struct PathNode *path_finder_get_node(struct PathSearch *search, int x, int y)
{
    size_t slot = ((uint32_t)x * UINT32_C(2654435761) ^ (uint32_t)y * UINT32_C(2246822507))
                  & (search->index_capacity - 1);
    struct PathNode *node;
    while ((node = search->index[slot]) != NULL) {
        if (node->x == x && node->y == y)
            return node;
        slot = (slot + 1) & (search->index_capacity - 1);
    }
    if (search->node_count == search->limit)
        return NULL;
    node = &search->nodes[search->node_count++];
    node->x = x; node->y = y; node->state = 0; node->slot = slot;
    search->index[slot] = node;
    return node;
}

static void open_push_heap_by_index(struct PathNode **first, size_t hole, size_t top, struct PathNode *const *val)
{
    size_t parent;

    while (top < hole) {
        parent = (hole - 1) >> 1;
        if (first[parent]->f <= (*val)->f)
            break;
        first[hole] = first[parent];
        hole = parent;
    }
    first[hole] = *val;
}

static void open_push(struct PathSearch *search, struct PathNode *node)
{
    size_t count = ++search->open_count;
    search->open[count - 1] = node;
    if (count >= 2)
        open_push_heap_by_index(search->open, count - 1, 0, &node);
}

static void open_pop(struct PathSearch *search)
{
    struct PathNode **first = search->open;
    struct PathNode **last = search->open + search->open_count;
    struct PathNode *val;
    size_t bottom, hole, idx, max_non_leaf;

    if (search->open_count >= 2) {
        val = last[-1];
        last[-1] = first[0];
        bottom = (size_t)(last - 1 - first);
        hole = 0;
        idx = 0;
        max_non_leaf = (bottom - 1) >> 1;
        while (idx < max_non_leaf) {
            idx = 2 * idx + 2;
            if (first[idx]->f > first[idx - 1]->f)
                idx--;
            first[hole] = first[idx];
            hole = idx;
        }
        if (idx == max_non_leaf && (bottom & 1) == 0) {
            first[hole] = first[bottom - 1];
            hole = bottom - 1;
        }
        open_push_heap_by_index(first, hole, 0, &val);
    }
    --search->open_count;
}

static BYTE path_finder_can_step(struct PathFinder *self, int x, int y, int nx, int ny,
                                    int kind)
{
    struct C3dAttr *attr;
    struct AttrCell *cell;

    if (nx < 0 || ny < 0)
        return 0;
    attr = self->attr;
    if (nx >= attr->width || ny >= attr->height)
        return 0;
    cell = c3d_attr_get_cell(attr, nx, ny);
    if (!cell || cell->type == CELL_BLOCKED)
        return 0;
    if ((BYTE)kind != KIND_SHOOT && cell->type == CELL_BLOCKED_SNIPE)
        return 0;
    if (x != nx && y != ny) {
        cell = c3d_attr_get_cell(attr, x, ny);
        if (!cell || cell->type == CELL_BLOCKED || cell->type == CELL_BLOCKED_SNIPE)
            return 0;
        cell = c3d_attr_get_cell(attr, nx, y);
        if (!cell || cell->type == CELL_BLOCKED || cell->type == CELL_BLOCKED_SNIPE)
            return 0;
    }
    return 1;
}

static BYTE path_finder_visit(struct PathFinder *self, struct PathNode *from, int cost, int nx, int ny, int dir)
{
    struct PathNode **first, **last, **it, *node;
    struct PathSearch *search = self->search;
    int64_t g = from->g + cost;
    size_t i;

    node = path_finder_get_node(search, nx, ny);
    if (!node)
        return 0;
    if (node->state == 0) {
        node->parent = from;
        node->state = 1;
        node->g = g;
        node->f = g + ((int64_t)abs(nx - search->tx) + abs(ny - search->ty)) * 10;
        node->dir = dir;
        open_push(search, node);
        return 1;
    }
    if (node->g <= g)
        return 1;
    node->parent = from;
    node->g = g;
    node->f = g + ((int64_t)abs(nx - search->tx) + abs(ny - search->ty)) * 10;
    node->dir = dir;
    if (node->state == 1) {
        first = search->open;
        last = search->open + search->open_count;
        for (it = first; it != last; it++)
            if ((*it)->x == node->x && (*it)->y == node->y)
                break;
        if (it == last)
            return 1;
        i = (size_t)(it - first);
        if (i + 1 >= 2) {
            node = *it;
            open_push_heap_by_index(first, i, 0, &node);
        }
        return 1;
    }
    if (node->state == 2)
        node->state = 1;
    return 1;
}

static int build_path_info(uint32_t startTime, struct PathNode *last, int speed,
                           float startX, float startY, struct PathInfo *info)
{
    struct PathCell *cells;
    struct PathNode *n;
    int count = 0, i, diag;
    float dx, dy, dist;

    for (n = last; n; n = n->parent)
        count++;
    if (!path_info_resize(info, count))
        return 0;
    info->startX = startX;
    info->startY = startY;
    for (i = count, n = last; i > 0; i--, n = n->parent) {
        cells = info->cells;
        cells[i - 1].x = n->x;
        cells[i - 1].y = n->y;
        cells[i - 1].dir = n->dir;
    }
    diag = (int)((float)speed * 1.414f);
    cells = info->cells;
    if (!count) return 1;
    cells[0].time = startTime;
    if (count < 2) return 1;
    dy = (float)cells[1].y - info->startY;
    dx = (float)cells[1].x - info->startX;
    dist = float_sqrt(dy * dy + dx * dx);
    cells[1].time = number_truncate_low32((float)speed * dist) + startTime;
    for (i = 2; i < count; i++)
        cells[i].time = ((cells[i].dir & 1) ? (DWORD)diag : (DWORD)speed) + cells[i - 1].time;
    return 1;
}

static float start_offset(int cell, int sub)
{
    float f = (float)sub * 0.0625f;

    if (cell > 0)
        f = (float)((double)cell - 0.5 + (double)f);
    return f;
}

BYTE path_finder_find_path(struct PathFinder *self, DWORD startTime, int sx, int sy,
                                      int subX, int subY, int tx, int ty, int speed, struct PathInfo *info,
                                      int kind)
{
    static const struct { int dx, dy, cost, dir; } steps[8] = {
        { 1, -1, 14, 5 }, { 1, 0, 10, 6 }, { 1, 1, 14, 7 }, { 0, 1, 10, 0 },
        { -1, 1, 14, 1 }, { -1, 0, 10, 2 }, { -1, -1, 14, 3 }, { 0, -1, 10, 4 },
    };
    struct PathNode *node;
    struct PathSearch *search;
    int i, nx, ny;

    if (sx == tx && sy == ty)
        return 0;
    if (!c3d_attr_get_cell(self->attr, sx, sy) || !c3d_attr_get_cell(self->attr, tx, ty)
        || !prepare_search(self))
        return 0;
    search = self->search;
    search->tx = tx; search->ty = ty;
    node = path_finder_get_node(search, sx, sy);
    if (!node)
        return 0;
    node->state = 1;
    node->parent = NULL;
    node->dir = 0;
    node->g = 0;
    node->f = ((int64_t)abs(sx - tx) + abs(sy - ty)) * 10;
    open_push(search, node);

    while (search->open_count) {
        node = search->open[0];
        open_pop(search);
        if (node->x == tx && node->y == ty) {
            return (BYTE)build_path_info(startTime, node, speed,
                                         start_offset(sx, subX), start_offset(sy, subY), info);
        }
        for (i = 0; i < 8; i++) {
            nx = node->x + steps[i].dx;
            ny = node->y + steps[i].dy;
            if (path_finder_can_step(self, node->x, node->y, nx, ny, kind)
                && !path_finder_visit(self, node, steps[i].cost, nx, ny, steps[i].dir))
                return 0;
        }
        node->state = 2;
    }
    return 0;
}
