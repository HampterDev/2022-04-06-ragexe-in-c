#ifndef GAMEMODE_MAGIC_STATIC_H
#define GAMEMODE_MAGIC_STATIC_H

#include "core/once.h"
#include <stdlib.h>

#define MAGIC_STATIC(guard, build, object, dtor) \
    do { \
        if (once_begin(&(guard))) { \
            build(&(object)); \
            atexit(dtor); \
            once_complete(&(guard)); \
        } \
    } while (0)

#endif
