#ifndef RESOURCES_EMBLEM_TYPES_H
#define RESOURCES_EMBLEM_TYPES_H

#include "resources/bitmap_types.h"

struct EmblemRes {
    struct Res res;
    struct GifRes picture;
};
OFFSET_ASSERT(EmblemRes, picture, 0x110);

#endif
