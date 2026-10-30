#include "std/std.h"
#include "resources/attr.h"
#include "world/path_info.h"
#include "core/numeric.h"
#include "render/math3d.h"
#include <stdlib.h>
#include <string.h>
struct ReferencePathNode {
    int x, y;
    struct ReferencePathNode *parent;
    int g;
    int f;
    int state;
    int dir;
};
SIZE_ASSERT(ReferencePathNode, 0x1c);

#define PATH_INLINE_NODES   0x95
#define PATH_NODE_MAX       0x9249249u

struct ReferencePathFinder {
    struct C3dAttr *attr;
    int tx, ty;
    int nodeCount;
    struct ReferencePathNode nodes[PATH_INLINE_NODES];
    unsigned char unused_node[0x1c];
    struct Tree attrCache;
    unsigned char useHeapNodes;
    struct Vec heapNodes;
    struct Tree nodeIndex;
    struct Vec open;
};
OFFSET_ASSERT(ReferencePathFinder, attrCache, 0x1078);
OFFSET_ASSERT(ReferencePathFinder, open, 0x1098);
SIZE_ASSERT(ReferencePathFinder, 0x10a4);

static void reference_CPathFinder_PurgeAttrCache(struct ReferencePathFinder *self);

struct ReferencePathFinder reference_g_pathFinder;

#define CELL_BLOCKED        1
#define CELL_BLOCKED_SNIPE  5
#define KIND_SHOOT          0xc

struct AttrCacheEntry {
    struct String name;
    struct C3dAttr *attr;
};

struct NodeIndexEntry {
    int key;
    struct ReferencePathNode *node;
};

static int attr_name_less(const void *a, const void *b)
{
    const struct String *x = a, *y = b;

    return string_compare(x, y) < 0;
}

static void attr_entry_destroy(void *value)
{
    string_free(&((struct AttrCacheEntry *)value)->name);
}

static void path_open_tidy(struct Vec *v)
{
    if (v->first) {
        vec_free(v);
        v->first = v->last = v->end = NULL;
    }
}

static void path_nodes_tidy(struct Vec *v)
{
    if (v->first) {
        vec_free(v);
        v->first = v->last = v->end = NULL;
    }
}

void reference_path_index_destroy(struct Tree *t)
{
    tree_free(t, NULL);
}

void reference_path_attr_cache_destroy(struct Tree *t)
{
    tree_free(t, attr_entry_destroy);
}

void reference_path_attr_key_destroy(struct String *s)
{
    string_free(s);
}

static void open_push_heap_by_index(struct ReferencePathNode **first, int hole, int top, struct ReferencePathNode *const *val)
{
    int parent;

    while (top < hole) {
        parent = (hole - 1) >> 1;
        if (first[parent]->f <= (*val)->f)
            break;
        first[hole] = first[parent];
        hole = parent;
    }
    first[hole] = *val;
}

static void open_push(struct Vec *open, struct ReferencePathNode *node)
{
    struct ReferencePathNode **first;
    struct ReferencePathNode *last;
    int count;

    vec_push(open, &node, sizeof node);
    first = (struct ReferencePathNode **)open->first;
    count = (int)((struct ReferencePathNode **)open->last - first);
    if (count >= 2) {
        last = first[count - 1];
        open_push_heap_by_index(first, count - 1, 0, &last);
    }
}

static void open_pop(struct Vec *open)
{
    struct ReferencePathNode **first = (struct ReferencePathNode **)open->first;
    struct ReferencePathNode **last = (struct ReferencePathNode **)open->last;
    struct ReferencePathNode *val;
    int bottom, hole, idx, max_non_leaf;

    if ((char *)last - (char *)first >= 2 * (int)sizeof *first) {
        val = last[-1];
        last[-1] = first[0];
        bottom = (int)(last - 1 - first);
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
    open->last = (struct ReferencePathNode **)open->last - 1;
}

void reference_CPathFinder_Construct(struct ReferencePathFinder *self)
{
    tree_init(&self->attrCache);
    self->heapNodes.first = self->heapNodes.last = self->heapNodes.end = NULL;
    tree_init(&self->nodeIndex);
    self->open.first = self->open.last = self->open.end = NULL;
    self->useHeapNodes = 0;
}

void reference_CPathFinder_Destruct(struct ReferencePathFinder *self)
{
    self->heapNodes.last = self->heapNodes.first;
    self->heapNodes.last = self->heapNodes.first;
    tree_clear(&self->nodeIndex, NULL);
    self->open.last = self->open.first;
    reference_CPathFinder_PurgeAttrCache(self);

    path_open_tidy(&self->open);
    tree_free(&self->nodeIndex, NULL);
    path_nodes_tidy(&self->heapNodes);
    tree_free(&self->attrCache, attr_entry_destroy);
}

void reference_CPathFinder_ClearSearch(struct ReferencePathFinder *self)
{
    self->heapNodes.last = self->heapNodes.first;
    tree_clear(&self->nodeIndex, NULL);
    self->open.last = self->open.first;
}

static void reference_CPathFinder_PurgeAttrCache(struct ReferencePathFinder *self)
{
    struct TreeNode *it;
    struct C3dAttr *attr;

    for (it = tree_first(&self->attrCache); it != self->attrCache.head; it = tree_next(it)) {
        attr = TREE_VALUE(it, struct AttrCacheEntry)->attr;
        if (attr)
            resource_destroy(&attr->res);
    }
    tree_clear(&self->attrCache, attr_entry_destroy);
}

void reference_CPathFinder_SetAttr(struct ReferencePathFinder *self, struct C3dAttr *attr)
{
    self->attr = attr;
}

int reference_CPathFinder_AttrHeight(struct ReferencePathFinder *self)
{
    return self->attr->height;
}

int reference_CPathFinder_AttrWidth(struct ReferencePathFinder *self)
{
    return self->attr->width;
}

BYTE reference_CPathFinder_UseMapAttr(struct ReferencePathFinder *self, struct String name)
{
    struct AttrCacheEntry *entry;
    struct C3dAttr *attr;
    BYTE ok;

    entry = tree_find(&self->attrCache, &name, attr_name_less);
    if (entry) {
        self->attr = entry->attr;
        ok = 1;
    } else {
        attr = (struct C3dAttr *)attribute_resource_create();
        if (!attr) {
            ok = 0;
        } else {
            entry = tree_find(&self->attrCache, &name, attr_name_less);
            if (!entry) {
                struct AttrCacheEntry fresh;

                string_copy(&fresh.name, &name);
                fresh.attr = NULL;
                entry = tree_add(&self->attrCache, &fresh, sizeof fresh, attr_name_less, NULL);
            }
            entry->attr = attr;
            string_append_cstr(&name, ".gat");
            c3d_attr_load(attr, string_cstr(&name));
            self->attr = attr;
            ok = 1;
        }
    }
    string_free(&name);
    return ok;
}

BYTE reference_CPathFinder_CellWalkable(struct ReferencePathFinder *self, int x, int y)
{
    return c3d_attr_cell_walkable(self->attr, x, y);
}

static BYTE reference_CPathFinder_CanStep(struct ReferencePathFinder *self, int x, int y, int nx, int ny,
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

struct ReferencePathNode *reference_CPathFinder_NewNode(struct ReferencePathFinder *self)
{
    int n;

    if (self->useHeapNodes) {
        n = self->nodeCount;
        if (n >= (int)vec_count(&self->heapNodes, sizeof(struct ReferencePathNode)) - 1) {
            vec_resize(&self->heapNodes, self->attr->width * self->attr->height + 1, sizeof(struct ReferencePathNode));
            n = self->nodeCount;
        }
        self->nodeCount = n + 1;
        return (struct ReferencePathNode *)self->heapNodes.first + n;
    }
    n = self->nodeCount;
    if (n >= PATH_INLINE_NODES)
        return NULL;
    self->nodeCount = n + 1;
    return &self->nodes[n];
}

static struct ReferencePathNode *reference_CPathFinder_GetNode(struct ReferencePathFinder *self, int x, int y)
{
    struct NodeIndexEntry *entry;
    struct ReferencePathNode *node;
    int width = self->attr->width, key = width * y + x, n;

    entry = tree_find(&self->nodeIndex, &key, tree_less_int);
    if (entry && entry->node)
        return entry->node;

    n = self->nodeCount;
    if (self->useHeapNodes) {
        if (n >= (int)vec_count(&self->heapNodes, sizeof(struct ReferencePathNode)) - 1) {
            vec_resize(&self->heapNodes, self->attr->height * width + 1, sizeof(struct ReferencePathNode));
            n = self->nodeCount;
        }
        node = (struct ReferencePathNode *)self->heapNodes.first + n;
    } else {
        if (n >= PATH_INLINE_NODES)
            return NULL;
        node = &self->nodes[n];
    }
    self->nodeCount = n + 1;
    if (!node)
        return NULL;
    node->y = y;
    node->x = x;
    node->state = 0;

    key = self->attr->width * y + x;
    entry = tree_get(&self->nodeIndex, &key, sizeof key, sizeof *entry, tree_less_int);
    entry->node = node;
    return node;
}

static BYTE reference_CPathFinder_Visit(struct ReferencePathFinder *self, struct ReferencePathNode *from, int cost, int nx, int ny, int dir)
{
    struct ReferencePathNode **first, **last, **it, *node;
    int g = from->g + cost, i;

    node = reference_CPathFinder_GetNode(self, nx, ny);
    if (!node)
        return 0;
    if (node->state == 0) {
        node->parent = from;
        node->state = 1;
        node->g = g;
        node->f = g + (abs(nx - self->tx) + abs(ny - self->ty)) * 10;
        node->dir = dir;
        open_push(&self->open, node);
        return 1;
    }
    if (node->g <= g)
        return 1;
    node->parent = from;
    node->g = g;
    node->f = g + (abs(nx - self->tx) + abs(ny - self->ty)) * 10;
    node->dir = dir;
    if (node->state == 1) {
        first = (struct ReferencePathNode **)self->open.first;
        last = (struct ReferencePathNode **)self->open.last;
        for (it = first; it != last; it++)
            if ((*it)->x == node->x && (*it)->y == node->y)
                break;
        if (it == last)
            return 1;
        i = (int)(it - first);
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

static int build_path_info(uint32_t startTime, struct ReferencePathNode *last, int speed,
                           float startX, float startY, struct PathInfo *info)
{
    struct PathCell *cells;
    struct ReferencePathNode *n;
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

BYTE reference_CPathFinder_FindPath(struct ReferencePathFinder *self, DWORD startTime, int sx, int sy,
                                      int subX, int subY, int tx, int ty, int speed, struct PathInfo *info,
                                      int kind)
{
    static const struct { int dx, dy, cost, dir; } steps[8] = {
        { 1, -1, 14, 5 }, { 1, 0, 10, 6 }, { 1, 1, 14, 7 }, { 0, 1, 10, 0 },
        { -1, 1, 14, 1 }, { -1, 0, 10, 2 }, { -1, -1, 14, 3 }, { 0, -1, 10, 4 },
    };
    struct ReferencePathNode *node;
    int i, nx, ny;

    self->tx = tx;
    self->ty = ty;
    if (sx == tx && sy == ty)
        return 0;
    reference_CPathFinder_ClearSearch(self);
    self->nodeCount = 0;
    node = reference_CPathFinder_GetNode(self, sx, sy);
    node->state = 1;
    node->parent = NULL;
    node->dir = 0;
    node->g = 0;
    node->f = (abs(sx - self->tx) + abs(sy - self->ty)) * 10;
    open_push(&self->open, node);

    while (self->open.first != self->open.last) {
        node = *(struct ReferencePathNode **)self->open.first;
        open_pop(&self->open);
        if (node->x == tx && node->y == ty) {
            return (BYTE)build_path_info(startTime, node, speed,
                                         start_offset(sx, subX), start_offset(sy, subY), info);
        }
        for (i = 0; i < 8; i++) {
            nx = node->x + steps[i].dx;
            ny = node->y + steps[i].dy;
            if (reference_CPathFinder_CanStep(self, node->x, node->y, nx, ny, kind)
                && !reference_CPathFinder_Visit(self, node, steps[i].cost, nx, ny, steps[i].dir))
                return 0;
        }
        node->state = 2;
    }
    return 0;
}

unsigned char pathfinder_reference(struct C3dAttr *attr, int heap, unsigned long time,
    int sx, int sy, int subx, int suby, int tx, int ty, int speed, struct PathInfo *out, int kind)
{
    struct ReferencePathFinder finder = {0};
    unsigned char ok;
    reference_CPathFinder_Construct(&finder);
    finder.useHeapNodes = (unsigned char)heap;
    reference_CPathFinder_SetAttr(&finder, attr);
    ok = reference_CPathFinder_FindPath(&finder, time, sx, sy, subx, suby, tx, ty, speed, out, kind);
    reference_CPathFinder_Destruct(&finder);
    return ok;
}
