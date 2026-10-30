#ifndef RESOURCES_GND_FORMAT_H
#define RESOURCES_GND_FORMAT_H

#include "base.h"

struct GndSurfaceRec {
    float u[4], v[4];
    short texture;
    WORD lightmap;
    DWORD colour;
};
OFFSET_ASSERT(GndSurfaceRec, texture, 0x20);
SIZE_ASSERT(GndSurfaceRec, 0x28);

struct GndCellRec {
    float height[4];
    int top;
    int front;
    int right;
};
SIZE_ASSERT(GndCellRec, 0x1c);

struct GndWaterRec {
    float level;
    int type;
    float waveHeight, waveSpeed, wavePitch;
    int animSpeed;
};
SIZE_ASSERT(GndWaterRec, 24);

#endif
