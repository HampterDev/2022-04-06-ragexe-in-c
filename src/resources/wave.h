#ifndef RESOURCES_WAVE_H
#define RESOURCES_WAVE_H

#include "fileio/resource_types.h"

struct AudioSound;

/* A sound file, decoded when it loads. A file that can be read loads even when it does not
   decode; it then has no sound and plays nothing. */
struct Wave {
    struct Res res;
    struct AudioSound *sound;
};

void wave_resource_reset(struct Wave *wave);

#endif
