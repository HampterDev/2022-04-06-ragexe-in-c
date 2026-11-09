#include "actor/weather.h"
#ifndef WORLD_GROUND_H
#define WORLD_GROUND_H

#include "world/world.h"
#include "resources/gnd.h"
#include "world/ground_api.h"

#include "render/texture_types.h"
#include "render/primitive_types.h"
#include <stdint.h>

struct GndVertex {
    float pos[3];
    DWORD colour;
    float u, v;
    float lu, lv;
};
SIZE_ASSERT(GndVertex, 0x20);

struct GndLightmap {
    BYTE centre[3];
    struct Texture *page;
    float uv[4][2];
    BYTE quarter[4][4];
};

struct GndSurface {
    struct GndVertex v[4];
    struct Texture *texture;
    struct Texture *lightmapPage;
    struct GndLightmap *lightmap;
    DWORD colour;
    int halfAlpha;
};

struct GndCell {
    float height[4];
    float water[4][3];
    struct GndSurface *top;
    struct GndSurface *rightWall;
    struct GndSurface *frontWall;
    int lastFrame;
    BYTE colour[3];
};

struct GndColour { float r, g, b; };
SIZE_ASSERT(GndColour, 12);

struct GndWater {
    float level;
    int type;
    float waveHeight, phaseStep, wavePitch;
    int stepsPerPicture;
    int step;
    int picture;
    float phase;
    struct Texture *texture;
};

struct GndLightmapSet {
    struct GndLightmap *lightmaps;
    size_t count;
    struct Texture **pages;
    size_t pageCount;
};

/* custom: the ground's surfaces as static meshes that the device projects (render/gfx.h), one per
   block of GND_GPU_BLOCK x GND_GPU_BLOCK cells, built on their first draw (world/ground_draw.c).
   A draw is a run of surfaces with one texture and one lightmap page; the first plainCount draws
   hold the surfaces without halfAlpha, the rest those with it. The first castCount indices are
   those plain draws', which cast shadows; castFrame is the last frame that queued them. */
#define GND_GPU_BLOCK 32

struct GndGpuDraw {
    struct Texture *texture, *lightmap;
    int first, count;
};

struct GndGpuBlock {
    struct gfx_mesh *mesh;
    struct GndGpuDraw *draws;
    int drawCount, plainCount;
    int lastFrame, castCount, castFrame;
};

struct C3dGround15 {
    struct C3dAttr *attr;
    int width, height;
    float zoom;
    struct GndLightmapSet lightmaps;
    int waterType;
    struct GndSurface *surfaces;
    size_t surfaceCount;
    struct GndColour *colours;
    size_t colourCount;
    struct GndCell *cells;
    size_t cellCount;
    int blocksX, blocksY;
    struct GndWater *waters;
    size_t waterCount;
    BYTE waterPerBlock;
    struct GndGpuBlock *gpuBlocks;
    int gpuCols, gpuRows;
    int gpuState;
};

#define GND_CELL(g, x, y) ((g)->cells + (size_t)(y) * (g)->width + (x))

void gnd_lightmap_set_reset(struct GndLightmapSet *self);
int gnd_lightmap_set_build(struct GndLightmapSet *self, const struct GndRes *gnd);
float ground_water_height(const struct GndWater *water, int64_t diagonal);
void ground_gpu_free(struct C3dGround15 *self);

#endif
