#ifndef RENDER_SHADOW_FIT_GPU_H
#define RENDER_SHADOW_FIT_GPU_H

/* custom: the shadow fit (render/shadow_fit.h) as the device's compute passes read and write it
   (src/render/shaders/shadow_fit.hlsli and cascades.hlsli): the passes' constants, the state they
   reduce into, the history they keep from frame to frame, and the cascades they leave, packed from
   and read back into the C's terms. Pure: render/gpu_world.c packs what it uploads and reads what it
   downloads with it, and tests/test_shadow_fit_gpu.c runs the passes on the same bytes. */

#include "render/shadow_fit.h"
#include "render/shadow_fit_gpu_tile.h"

#include <stdint.h>

/* The passes' constants (shadow_fit.hlsli, Frame): 16-byte vectors. */
struct ShadowFitGpuFrame {
    float pixelX[4], pixelY[4];  /* ShadowFitFrame.pixel[0] and [1] */
    float screen[4];             /* its centre's x and y, its screen's width and height */
    float depths[4];             /* its near plane, its reach, its lightNear and its lightRange */
    uint32_t size[4];            /* the depth pass's targets' width and height, which the passes read */
    float axisX[4], axisY[4], axisZ[4];  /* its axes */
    float inverse[12];
    float lightView[4];          /* towards the light, view space; w: the bias */
    float ambient[4];            /* the ambient light, 0 to 1; w: a shadow's darkness, at least 1 / 255 */
};

/* The state the range and the extents reduce into, uints: the nearest and the farthest depth's
   bits, then per cascade the least x and y and the greatest x and y across the light, each as
   shadow_fit_gpu_order orders it. */
#define SHADOW_FIT_GPU_EXTENTS 2
#define SHADOW_FIT_GPU_STATE (SHADOW_FIT_GPU_EXTENTS + 4 * SHADOW_FIT_CASCADES)

/* The range and the extents take the depth targets a tile of SHADOW_FIT_GPU_TILE by
   SHADOW_FIT_GPU_TILE pixels a group (render/shadow_fit_gpu_tile.h): their dispatch's groups
   across and down for targets of width by height pixels. */
void shadow_fit_gpu_groups(uint32_t width, uint32_t height, uint32_t groups[2]);

/* The history the cascades pass keeps from frame to frame, floats: the squares' half sizes, then
   the range the split was cut from (nearest, farthest) and two unused; all 0 for none (a new
   world). */
#define SHADOW_FIT_GPU_HISTORY 8

/* The cascades the fit leaves (cascades.hlsli, Cascades), which the receivers, the casters and the
   casters' cull read: 16-byte vectors, row-major matrices. */
struct ShadowFitGpuCascades {
    float world[SHADOW_FIT_CASCADES][16];
    float fromView[SHADOW_FIT_CASCADES][16];
    float farDepth[4], offset[4], blur[4], slant[4];
    float square[SHADOW_FIT_CASCADES][4];  /* the least x and y, then the greatest */
    float kept[4];                         /* the squares' half sizes kept for the next frame */
    float previous[4];                     /* the frame before's, which they were kept from */
    float range[4];                        /* the range the split was cut from, then the frame before's */
    float planes[SHADOW_FIT_CASCADES * 6][4];
    float lightView[4];                    /* w: the bias */
    float ambient[4];                      /* w: the darkness */
    uint32_t count[4];                     /* x: the cascades, SHADOW_FIT_CASCADES or 0 */
};

/* The constants of a frame fitted to depth targets of width by height pixels, with the map's
   ambient light (clamped to 0 to 1) and a shadow's darkness (at least 1 / 255). */
void shadow_fit_gpu_frame(const struct ShadowFitFrame *frame, int width, int height, const float *ambient,
                          float darkness, struct ShadowFitGpuFrame *out);
/* The state a frame's fit starts from: nothing seen, the nearest depth at infinity, every extent
   empty. */
void shadow_fit_gpu_state(uint32_t *state);
/* A float as a uint that orders as the float does (+0 above -0), and back. */
uint32_t shadow_fit_gpu_order(float x);
float shadow_fit_gpu_unorder(uint32_t u);

/* What the passes left, in the C's terms: the range and the extents they found (the empty state
   reads nearest +infinity, farthest 0), the squares and the range they were kept from, the range
   the split was cut from, and the cascades. */
struct ShadowFitGpuRead {
    struct ShadowFitRange range;
    struct ShadowFitExtent extents[SHADOW_FIT_CASCADES];
    float previous[SHADOW_FIT_CASCADES];
    struct ShadowFitRange previousRange, kept;
    struct ShadowFitCascades cascades;
};

void shadow_fit_gpu_read(const uint32_t *state, const struct ShadowFitGpuCascades *result, struct ShadowFitGpuRead *out);

#endif
