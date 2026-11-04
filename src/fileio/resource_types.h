#ifndef FILEIO_RESOURCE_TYPES_H
#define FILEIO_RESOURCE_TYPES_H

#include "fileio/hash_types.h"

struct ResourceOps;

struct Res {
    const struct ResourceOps *ops;
    int lockCount;
    unsigned lastUse;
    int typeIndex;
    struct Hash name;
};

#endif
