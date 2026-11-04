#ifndef RESOURCES_WAVE_H
#define RESOURCES_WAVE_H

#include "fileio/resource_types.h"

struct Wave {
    struct Res res;
    void *data;
    int milesOwned;
};

void wave_resource_reset(struct Wave *wave);

#endif
