#ifndef FILEIO_HASH_TYPES_H
#define FILEIO_HASH_TYPES_H

#include "base.h"

struct Hash {
    unsigned hash;
    char name[0xfc];
};
SIZE_ASSERT(Hash, 0x100);

#endif
