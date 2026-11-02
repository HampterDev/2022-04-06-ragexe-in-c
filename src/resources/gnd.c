#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include "resources/gnd_format.h"
#include "fileio/file.h"
#include "resources/gnd.h"

static void gnd_init(struct GndRes *gnd)
{
    memset(gnd, 0, sizeof *gnd);
    res_construct(&gnd->res);
    gnd->res.ops = &ground_resource_ops;
    file_construct(&gnd->file);
    gnd->zoom = 10.0f;
}

struct Res *ground_resource_create(void)
{
    struct GndRes *gnd = malloc(sizeof *gnd);

    if (!gnd)
        return NULL;
    gnd_init(gnd);
    return &gnd->res;
}

void gnd_res_get_lightmap(const struct GndRes *self, int index, BYTE *out)
{
    if (!out)
        return;
    if (index < 0 || index >= self->lightmapCount || !self->lightmaps) {
        memset(out, 0, GND_LIGHTMAP_SIZE);
        return;
    }
    memcpy(out, self->lightmaps + (size_t)index * GND_LIGHTMAP_SIZE, GND_LIGHTMAP_SIZE);
}

static BYTE gnd_read_water(struct GndRes *self)
{
    struct File *fp = &self->file;
    struct GndWaterRec water;
    const BYTE *records;
    uint64_t total;
    size_t count, stride, i;

    if (!file_read(fp, &water, sizeof water)
            || !file_read(fp, &self->waterSplitW, 4)
            || !file_read(fp, &self->waterSplitH, 4)
            || self->waterSplitW < 0 || self->waterSplitH < 0)
        return 0;
    self->water = water;
    if (self->minor != 8 && self->minor != 9)
        return 1;
    total = (uint64_t)self->waterSplitW * (uint64_t)self->waterSplitH;
    if (total > SIZE_MAX / sizeof *self->waters)
        return 0;
    count = (size_t)total;
    stride = self->minor == 8 ? sizeof(float) : sizeof water;
    records = file_consume(fp, count, stride);
    if (!records)
        return 0;
    if (count) {
        self->waters = malloc(count * sizeof *self->waters);
        if (!self->waters)
            return 0;
    }
    self->waterCount = count;
    for (i = 0; i < count; i++) {
        if (self->minor == 8)
            memcpy(&water.level, records + i * stride, sizeof water.level);
        else
            memcpy(&water, records + i * stride, sizeof water);
        self->waters[i] = water;
    }
    return 1;
}

static BYTE gnd_surface_valid(const struct GndRes *self, int index)
{
    const struct GndSurfaceRec *surface;
    if (index < 0 || index >= self->surfaceCount)
        return 1;
    surface = &self->surfaces[index];
    return surface->texture >= 0 && surface->texture < self->textureCount
        && surface->lightmap < self->lightmapCount;
}

static BYTE gnd_parse(struct GndRes *self)
{
    struct File *fp = &self->file;
    const BYTE *names, *end, *surfaces, *cubes;
    uint64_t cubeCount;
    int nameLen, gridX, gridY, gridCell, i;

    if (!file_read(fp, &self->major, 1) || !file_read(fp, &self->minor, 1)
            || self->major != 1 || self->minor <= 6
            || !file_read(fp, &self->width, 4) || !file_read(fp, &self->height, 4)
            || !file_read(fp, &self->zoom, 4)
            || !file_read(fp, &self->textureCount, 4) || !file_read(fp, &nameLen, 4)
            || self->width < 0 || self->height < 0 || self->textureCount < 0
            || (self->textureCount && nameLen < 0))
        return 0;
    cubeCount = (uint64_t)self->height * (uint64_t)self->width;
    if (cubeCount > INT_MAX / GND_CUBE_SIZE)
        return 0;
    if ((size_t)self->textureCount > SIZE_MAX / sizeof *self->textureNames)
        return 0;
    names = fp->buffer + fp->position;
    end = fp->buffer + fp->size;
    if (nameLen > 0 && !file_consume(fp, (size_t)self->textureCount, (size_t)nameLen))
        return 0;
    for (i = 0; i < self->textureCount; i++) {
        const BYTE *name = names + (size_t)i * nameLen;
        if (!memchr(name, 0, (size_t)(end - name)))
            return 0;
    }
    if (self->textureCount) {
        self->textureNames = malloc((size_t)self->textureCount * sizeof *self->textureNames);
        if (!self->textureNames)
            return 0;
    }
    for (i = 0; i < self->textureCount; i++)
        self->textureNames[i] = (const char *)names + (size_t)i * nameLen;
    if (!file_read(fp, &self->lightmapCount, 4) || self->lightmapCount < 0
            || !file_read(fp, &gridX, 4) || !file_read(fp, &gridY, 4)
            || !file_read(fp, &gridCell, 4))
        return 0;
    self->lightmaps = file_consume(fp, (size_t)self->lightmapCount, GND_LIGHTMAP_SIZE);
    if (!self->lightmaps || !file_read(fp, &self->surfaceCount, 4) || self->surfaceCount < 0)
        return 0;
    surfaces = file_consume(fp, (size_t)self->surfaceCount, sizeof *self->surfaces);
    cubes = file_consume(fp, (size_t)cubeCount, sizeof *self->cubes);
    if (!surfaces || !cubes)
        return 0;
    if (self->surfaceCount) {
        size_t bytes = (size_t)self->surfaceCount * sizeof *self->surfaces;
        self->surfaces = malloc(bytes);
        if (!self->surfaces)
            return 0;
        memcpy(self->surfaces, surfaces, bytes);
    }
    if (cubeCount) {
        size_t bytes = (size_t)cubeCount * sizeof *self->cubes;
        self->cubes = malloc(bytes);
        if (!self->cubes)
            return 0;
        memcpy(self->cubes, cubes, bytes);
    }
    for (i = 0; i < (int)cubeCount; i++) {
        const struct GndCellRec *cube = &self->cubes[i];
        if (!gnd_surface_valid(self, cube->top) || !gnd_surface_valid(self, cube->front)
                || !gnd_surface_valid(self, cube->right))
            return 0;
    }
    return self->minor < 8 || gnd_read_water(self);
}

BYTE gnd_res_load(struct GndRes *self, const char *name)
{
    DWORD magic;
    ground_resource_clear(self);
    if (file_open(&self->file, name, 0, 1, 0)
            && file_read(&self->file, &magic, sizeof magic)
            && magic == GND_MAGIC && gnd_parse(self)) {
        return 1;
    }
    ground_resource_clear(self);
    return 0;
}

void ground_resource_clear(struct GndRes *self)
{
    free(self->textureNames);
    free(self->waters);
    free(self->surfaces);
    free(self->cubes);
    self->textureNames = NULL;
    self->waters = NULL;
    self->waterCount = 0;
    self->lightmaps = NULL;
    self->surfaces = NULL;
    self->cubes = NULL;
    self->textureCount = self->lightmapCount = self->surfaceCount = 0;
    self->width = self->height = 0;
    self->major = self->minor = 0;
    self->waterSplitW = self->waterSplitH = 0;
    memset(&self->water, 0, sizeof self->water);
    self->zoom = 10.0f;
    file_close(&self->file);
}

void ground_resource_finish(struct GndRes *self)
{
    ground_resource_clear(self);
    file_destruct(&self->file);
    res_destruct(&self->res);
}

static void ground_resource_destroy(struct Res *resource)
{
    struct GndRes *self = (struct GndRes *)resource;
    ground_resource_finish(self);
    free(self);
}
static int ground_resource_load(struct Res *resource, const char *name)
{
    return gnd_res_load((struct GndRes *)resource, name);
}
const struct ResourceOps ground_resource_ops = {
    .destroy = ground_resource_destroy,
    .load = ground_resource_load,
};
