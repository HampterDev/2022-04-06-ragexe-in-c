#ifndef FILEIO_RESOURCE_OPS_H
#define FILEIO_RESOURCE_OPS_H

#include "fileio/resource_types.h"

struct ResourceOps {
    void (*destroy)(struct Res *resource);
    int (*load)(struct Res *resource, const char *name);
    void (*update)(struct Res *resource);
    int (*is_resident)(const struct Res *resource);
};

static inline void resource_destroy(struct Res *resource)
{
    if (resource)
        resource->ops->destroy(resource);
}

static inline int resource_load(struct Res *resource, const char *name)
{
    return resource->ops->load(resource, name);
}

static inline void resource_update(struct Res *resource)
{
    if (resource->ops->update)
        resource->ops->update(resource);
}

static inline int resource_is_resident(const struct Res *resource)
{
    return resource->ops->is_resident && resource->ops->is_resident(resource);
}

#endif
