#ifndef WORLD_PATHFINDER_STATE_H
#define WORLD_PATHFINDER_STATE_H

#include <stddef.h>
#include "platform/win_types.h"
#include "world/path_info.h"

struct C3dAttr;
struct PathSearch;
struct PathMapEntry;
struct PathFinder {
    struct C3dAttr *attr;
    struct PathSearch *search;
    struct PathMapEntry *maps;
    size_t map_count, map_capacity;
    unsigned char useHeapNodes;
    /* `attr` as the server's path search reads it (map/grid_path.h), built on first use. */
    unsigned char *walkable;
};

void path_finder_construct(struct PathFinder *self);
void path_finder_destruct(struct PathFinder *self);
unsigned char path_finder_find_path(struct PathFinder *self, DWORD startTime,
    int sx, int sy, int subX, int subY, int tx, int ty, int speed,
    struct PathInfo *info, int kind);
/* This map as the body core reads it (actor/actor.h): its cells and the working memory of a path
   search, with its sides. Returns 0 without a map. */
struct ActorGround;
unsigned char path_finder_body_ground(struct PathFinder *self, struct ActorGround *ground, int *width, int *height);
void path_finder_clear_search(struct PathFinder *self);
void path_finder_set_attr(struct PathFinder *self, struct C3dAttr *attr);
/* A cell of `attr` changed: what was built from the cells is built again on its next use. */
void path_finder_cells_changed(struct PathFinder *self);
unsigned char path_finder_use_map_attr(struct PathFinder *self, const char *name, size_t length);
unsigned char path_finder_cell_walkable(struct PathFinder *self, int x, int y);

extern struct PathFinder g_pathFinder;
#endif
