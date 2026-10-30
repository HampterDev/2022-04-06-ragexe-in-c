#ifndef ASYNCWORK_WORK_POOL_INTERNAL_H
#define ASYNCWORK_WORK_POOL_INTERNAL_H

#include "web/work_pool.h"
#include "web/work.h"
#include <stddef.h>

struct WorkPoolType {
    const char *name;
    size_t size;
    void (*initialize)(void *work);
    void (*dispose)(void *work);
    struct AsyncWorkOps operations;
};

struct WorkPoolMgr *work_pool_create_with_types(const struct WorkPoolType *types, size_t count);

#endif
