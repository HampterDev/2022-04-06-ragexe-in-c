#ifndef RESOURCES_WAVE_H
#define RESOURCES_WAVE_H

#include "fileio/resource_types.h"

struct Wave {
    struct Res res;
    void *data;
    int milesOwned;
};
OFFSET_ASSERT(Wave, data, 0x110);
SIZE_ASSERT(Wave, 0x118);

void wave_resource_reset(struct Wave *wave);

#endif
