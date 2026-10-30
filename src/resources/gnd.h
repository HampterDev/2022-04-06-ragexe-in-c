#ifndef RESOURCES_GND_H
#define RESOURCES_GND_H

#include "platform/error_message.h"
#include "std/string.h"
#include "resources/resource_ops.h"
#include "fileio/resource_api.h"
#include "fileio/res_mgr.h"
#include "resources/resource_factories.h"
#include "resources/gnd_format.h"

#define GND_MAGIC 0x4e475247u
#define GND_LIGHTMAP_SIZE 256
#define GND_CUBE_SIZE 28

struct GndRes {
    struct Res res;
    BYTE major, minor;
    int width, height;
    float zoom;
    int textureCount, surfaceCount, lightmapCount;
    struct GndWaterRec water;
    int waterSplitW, waterSplitH;
    size_t waterCount;
    struct GndWaterRec *waters;
    const char **textureNames;
    const BYTE *lightmaps;
    struct GndSurfaceRec *surfaces;
    struct GndCellRec *cubes;
    struct File file;
};

BYTE gnd_res_load(struct GndRes *self, const char *name);
void ground_resource_clear(struct GndRes *self);
void ground_resource_finish(struct GndRes *self);

void gnd_res_get_lightmap(const struct GndRes *self, int index, BYTE *out);

#endif
