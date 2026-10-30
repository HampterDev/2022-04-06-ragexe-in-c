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
OFFSET_ASSERT(Res, lastUse, 0x08);
OFFSET_ASSERT(Res, name, 0x10);
SIZE_ASSERT(Res, 0x110);

#endif
