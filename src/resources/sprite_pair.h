#ifndef RESOURCES_SPRITE_PAIR_H
#define RESOURCES_SPRITE_PAIR_H

#include "std/std.h"

struct SprRes;
struct ActRes;

struct SpriteResPair {
    struct SprRes *spr;
    struct ActRes *act;
};

#endif
