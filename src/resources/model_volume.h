#ifndef RESOURCES_MODEL_VOLUME_H
#define RESOURCES_MODEL_VOLUME_H

#include "base.h"

struct VolumeBox {
    float size[3];
    float pos[3];
    float rot[3];
    float corners[8][3];
    float matrix[12];
    float inverse[12];
    int flags;
};
OFFSET_ASSERT(VolumeBox, corners, 0x24);
OFFSET_ASSERT(VolumeBox, inverse, 0xb4);
SIZE_ASSERT(VolumeBox, 0xe8);

#endif
