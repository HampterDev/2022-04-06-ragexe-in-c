#ifndef RESOURCES_ATTR_TYPES_H
#define RESOURCES_ATTR_TYPES_H

#include "fileio/resource_types.h"

struct AttrCell {
    float height[4];
    int type;
};
SIZE_ASSERT(AttrCell, 20);

enum {
    ATTR_CELL_WALKABLE = 0,
    ATTR_CELL_BLOCKED = 1,
    ATTR_CELL_CLIFF = 5
};

struct C3dAttr {
    struct Res res;
    int width, height;
    int zoom;
    struct AttrCell *cells;
};

#endif
