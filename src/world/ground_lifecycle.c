#include "render/gfx.h"
#include "render/texture.h"
#include "world/ground.h"
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>

void gnd_lightmap_set_reset(struct GndLightmapSet *self)
{
    size_t i;
    if (self->pages) {
        for (i = self->pageCount; i != 0; --i) {
            struct Texture *page = self->pages[i - 1];
            if (page)
                texture_destroy(page);
        }
    }
    free(self->pages);
    free(self->lightmaps);
    self->pages = NULL;
    self->lightmaps = NULL;
    self->pageCount = self->count = 0;
}

/* custom: the ground's static meshes (world/ground.h); world/ground_draw.c builds them */
static void gpu_block_free(struct GndGpuBlock *b)
{
    gfx_mesh_destroy(b->mesh);
    free(b->draws);
    free(b->half);
    memset(b, 0, sizeof *b);
}

void ground_gpu_free(struct C3dGround15 *self)
{
    int i;

    if (self->gpuBlocks)
        for (i = 0; i < self->gpuCols * self->gpuRows; i++)
            gpu_block_free(&self->gpuBlocks[i]);
    free(self->gpuBlocks);
    self->gpuBlocks = NULL;
    self->gpuCols = self->gpuRows = 0;
    self->gpuState = 0;
}

struct C3dGround15 *ground_new(void)
{
    struct C3dGround15 *ground = calloc(1, sizeof *ground);
    if (!ground)
        return NULL;
    ground->zoom = 10.0f;
    ground->blocksX = ground->blocksY = 1;
    return ground;
}

void ground_free(struct C3dGround15 *ground)
{
    if (ground) {
        c3d_ground15_clear(ground);
        free(ground);
    }
}

void ground_set_attribute_map(struct C3dGround15 *ground, struct C3dAttr *attr)
{
    ground->attr = attr;
}

void c3d_ground15_clear(struct C3dGround15 *self)
{
    ground_gpu_free(self);
    free(self->waters);
    free(self->cells);
    free(self->colours);
    free(self->surfaces);
    gnd_lightmap_set_reset(&self->lightmaps);
    self->waters = NULL;
    self->cells = NULL;
    self->colours = NULL;
    self->surfaces = NULL;
    self->waterCount = self->cellCount = self->colourCount = self->surfaceCount = 0;
    self->width = self->height = 0;
    self->blocksX = self->blocksY = 1;
    self->waterPerBlock = 0;
}

int c3d_ground15_create(struct C3dGround15 *self, int width, int height)
{
    size_t count, i;
    c3d_ground15_clear(self);
    if (width <= 0 || height <= 0 || (size_t)width > SIZE_MAX / (size_t)height)
        return 0;
    count = (size_t)width * height;
    if (count > SIZE_MAX / sizeof *self->cells
            || count > SIZE_MAX / (4 * sizeof *self->colours) || count > INT_MAX / 4)
        return 0;
    self->cells = calloc(count, sizeof *self->cells);
    self->colours = calloc(count * 4, sizeof *self->colours);
    if (!self->cells || !self->colours) {
        c3d_ground15_clear(self);
        return 0;
    }
    self->cellCount = count;
    self->colourCount = count * 4;
    self->width = width;
    self->height = height;
    for (i = 0; i < count; ++i) {
        self->cells[i].colour[0] = self->cells[i].colour[1] = self->cells[i].colour[2] = 255;
        self->cells[i].lastFrame = -1;
    }
    for (i = 0; i < self->colourCount; ++i)
        self->colours[i].r = self->colours[i].g = self->colours[i].b = 1.0f;
    return 1;
}
