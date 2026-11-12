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
};

/* custom: water is the index of the cell's quad in the map's water (render/gfx.h, gfx_world_water),
   -1 where the cell has none */
struct GndCell {
    float height[4];
    int water;
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

/* custom: the ground's surfaces in the static world (render/gfx.h, gfx_world_mesh), one mesh per
   block of GND_GPU_BLOCK x GND_GPU_BLOCK cells (c3d_ground15_block_mesh, world/world_gpu.c): where
   a block's triangles lie in the world's shared buffers, which cast shadows together, none where
   indexCount is 0; one past the greatest id of its records; and the last frame that queued them
   as casters. */
#define GND_GPU_BLOCK 32

struct GndGpuBlock {
    uint32_t firstIndex, indexCount, baseVertex, ids;
    int castFrame;
};

/* A run of a block's mesh: the surfaces with one texture and one lightmap page, `count` indices
   from `first`. */
struct GndBlockRun {
    struct Texture *texture, *lightmap;
    int first, count;
};

/* The most surfaces of a block, and of its mesh's runs: a top, a front and a right wall a cell. */
#define GND_BLOCK_SURFACES (GND_GPU_BLOCK * GND_GPU_BLOCK * 3)

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
};

#define GND_CELL(g, x, y) ((g)->cells + (size_t)(y) * (g)->width + (x))

void gnd_lightmap_set_reset(struct GndLightmapSet *self);
int gnd_lightmap_set_build(struct GndLightmapSet *self, const struct GndRes *gnd);
float ground_water_height(const struct GndWater *water, int64_t diagonal);
void ground_gpu_free(struct C3dGround15 *self);
/* a corner of cell (x, y) at height h: x, h, z */
static inline void c3d_ground15_corner(const struct C3dGround15 *self, int x, int y, float h, float *pos)
{
    pos[0] = (float)(x - self->width / 2) * self->zoom;
    pos[1] = h;
    pos[2] = (float)(y - self->height / 2) * self->zoom;
}
/* custom: block (bx, by)'s surfaces as one mesh of the static world: four vertices and two
   triangles a surface (a strip, 0 1 2 then 2 1 3), sorted by texture and lightmap page, into verts
   (4 * GND_BLOCK_SURFACES) and indices (6 * GND_BLOCK_SURFACES), their runs into runs
   (GND_BLOCK_SURFACES). Answers the number of runs, 0 for a block without surfaces; *indexCount the
   indices. */
int c3d_ground15_block_mesh(const struct C3dGround15 *self, int bx, int by, struct GfxMeshVertex *verts,
                            WORD *indices, int *indexCount, struct GndBlockRun *runs);
/* custom: the block of water a cell takes its wave from (struct GndWater), -1 for none */
int c3d_ground15_water_block(struct C3dGround15 *self, int x, int y);
/* custom: the cells whose water a frame may draw, into out (NULL: only counted; GndCell.water set
   when out is given): those with a top surface, a block of water, and a corner of that top that
   the wave can rise above. */
size_t c3d_ground15_water_cells(struct C3dGround15 *self, struct GfxWaterCell *out);

#endif
