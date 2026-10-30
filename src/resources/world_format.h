#ifndef RESOURCES_WORLD_FORMAT_H
#define RESOURCES_WORLD_FORMAT_H

#include "base.h"
#include "resources/world_records.h"

struct RswModelRec {
    char fileName[80], nodeName[80];
    float pos[3], rot[3], scale[3];
};
SIZE_ASSERT(RswModelRec, 196);

struct RswLightRec {
    char name[80];
    float pos[3];
    int colour[3];
    float range;
};
SIZE_ASSERT(RswLightRec, 108);

struct RswEffectRec {
    char name[80];
    float pos[3];
    int id;
    float emitSpeed, param[4];
};
SIZE_ASSERT(RswEffectRec, 116);

struct RswSoundRec {
    char name[80], waveName[80];
    float pos[3], volume;
    int width, height;
    float range, cycle;
};
OFFSET_ASSERT(RswSoundRec, cycle, 188);
SIZE_ASSERT(RswSoundRec, 192);
SIZE_ASSERT(RswBounds, 48);

#endif
