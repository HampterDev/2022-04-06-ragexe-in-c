#ifndef RENDER_SHADOW_FIT_H
#define RENDER_SHADOW_FIT_H

#include <stddef.h>

/* custom: the frame's shadow cascades fitted to what the frame shows (sample distribution shadow
   maps, plans/gpu-renderer.md): the steps the GPU's compute passes take, as pure functions that
   the passes are checked against. The cascades are the ones render/rp_project.c fits to boxes on
   the CPU: SHADOW_FIT_CASCADES maps of SHADOW_FIT_SIZE texels square along the map's light, each
   covering a square across the light that is rounded up to whole 8 units and kept to whole texels,
   so that shadow edges do not crawl while the camera moves.

   A frame is fitted in this order:
   1. shadow_fit_frame (the CPU): the light's axes, the camera, the shadows' reach and the light's
      depth range, the constants of the passes;
   2. shadow_fit_range: over the receivers' view depths, the nearest and the farthest;
   3. shadow_fit_split: that range cut into one slice per cascade;
   4. shadow_fit_extents: over the depths again, each slice's pixels across the light;
   5. shadow_fit_cascades: each slice's square, its matrices and its lookups;
   6. shadow_fit_reaches: whether a caster's box can shade a cascade.

   World and view space are the client's (render/gfx.h): +y is down, and view depth grows away from
   the camera. A depth buffer holds width * height view depths, row by row: the one at
   [j * width + i] is the depth of the receiver (the ground or a model) that the view sees at the
   window coordinates (i, j), Direct3D 9's pixel centres (render/gfx_gpu.c, MARGIN), or 0 where it
   sees none. A pixel receives a shadow when its depth lies from the near plane to the shadows'
   reach (ShadowFitFrame.reach); the others take no part in the fit.

   Floats are rounded as IEEE single precision with every operation rounded on its own (no fused
   multiply-adds), so the results are the same bits on every platform the client builds for and
   from run to run, whatever order the pixels are visited in. The compute passes
   (src/render/shaders/shadow_range.hlsl, shadow_extents.hlsl, shadow_cascades.hlsl) do the same
   steps; where a GPU rounds a division or a square root otherwise, their results differ in the
   last places (tests/test_shadow_fit_gpu.c). */

#define SHADOW_FIT_CASCADES 4
#define SHADOW_FIT_SIZE 2048

/* a box the receivers and casters lie in, world coordinates */
struct ShadowFitBox {
    float min[3], max[3];
};

struct ShadowFitCamera {
    float view[12];   /* world to view, the client's 4x3 (render/gfx.h, GfxWorldView) */
    float proj[2];    /* pixels across per unit of view x and y at a view depth of 1 */
    float centre[2];  /* the window coordinates the view's axis passes through */
    float nearZ, farZ;
    float distance;   /* from the camera to its focus */
};

/* The constants of a frame's passes, which the CPU works out once a frame. */
struct ShadowFitFrame {
    float axis[3][3];         /* the light's axes: two across it, then the way it travels */
    float inverse[12];        /* view to world, a 4x3 as ShadowFitCamera.view */
    /* the pixel (i, j) at view depth d lies across the light, along axis a, at
       pixel[a][3] + d * ((i - centre[0]) * pixel[a][0] + (j - centre[1]) * pixel[a][1] + pixel[a][2]) */
    float pixel[2][4];
    float centre[2];
    float nearZ, reach;       /* the view depths that receive shadows */
    float lightNear, lightRange;  /* along the light: the boxes, and a margin either side */
    float lightView[3];       /* towards the light, view space, unit length */
    float bias;               /* render/gfx.h, GfxShadow.bias */
};

/* The frame's constants for the camera, the light's direction (the way it travels) and the boxes
   that hold every receiver and caster. The shadows reach four times the camera's distance deep,
   and 400 units more, but no deeper than the far plane. 0 where nothing can be fitted: the
   projection or the near plane is not positive, the view cannot be inverted, the light has no
   finite direction, or no box is finite and not empty. */
int shadow_fit_frame(struct ShadowFitFrame *out, const struct ShadowFitCamera *camera, const float *light,
                     const struct ShadowFitBox *boxes, size_t count);

/* the nearest and farthest view depth of the pixels that receive; nearest > farthest where none does */
struct ShadowFitRange {
    float nearest, farthest;
};

void shadow_fit_range(struct ShadowFitRange *out, const struct ShadowFitFrame *frame, const float *depth, int width,
                      int height);

/* The view depths that bound each cascade's slice: split[0] the nearest, split[c + 1] cascade c's
   far depth, the last a unit beyond the farthest. The slices grow with depth, 0.7 of the way from
   even steps to geometric ones. All 0 where no pixel receives. */
void shadow_fit_split(float *split, const struct ShadowFitRange *range);

/* where a slice's pixels lie across the light, along the frame's axes 0 and 1; lo > hi where none
   does */
struct ShadowFitExtent {
    float lo[2], hi[2];
};

/* A pixel belongs to the first slice whose far depth (split[c + 1]) lies beyond it, the cascade the
   receiving shaders look it up in. */
void shadow_fit_extents(struct ShadowFitExtent *out, const struct ShadowFitFrame *frame, const float *split,
                        const float *depth, int width, int height);

/* The frame's cascades, in the conventions of render/gfx.h's GfxShadow. */
struct ShadowFitCascades {
    /* SHADOW_FIT_CASCADES, or 0 where no pixel receives: the receivers then keep the ground's baked
       shadows, as a frame without shadow maps does. The matrices are finite either way. */
    int cascades;
    /* world to each cascade's light space, row vectors: x and y from -1 to 1 across its map, z from
       0 to 1 along the light */
    float world[SHADOW_FIT_CASCADES][16];
    /* the same from view space, GfxShadow.fromView */
    float fromView[SHADOW_FIT_CASCADES][16];
    /* GfxShadow's farDepth, offset, blur and slant */
    float farDepth[SHADOW_FIT_CASCADES];
    float offset[SHADOW_FIT_CASCADES];
    float blur[SHADOW_FIT_CASCADES];
    float slant[SHADOW_FIT_CASCADES];
    /* each map's square across the light, along the frame's axes 0 and 1; lo > hi for a slice
       that no pixel lies in, which no caster reaches */
    float lo[SHADOW_FIT_CASCADES][2], hi[SHADOW_FIT_CASCADES][2];
};

/* Each slice's square: centred on its pixels, half as wide as the wider of their two extents,
   5 percent and 4.5 units more for the soft edge and the lookup's offset, rounded up to a multiple
   of 8 units, and its centre kept to whole texels of its map. A slice that no pixel lies in gets a
   square of 8 units at the light's origin. */
void shadow_fit_cascades(struct ShadowFitCascades *out, const struct ShadowFitFrame *frame, const float *split,
                         const struct ShadowFitExtent *extents);

/* Whether a box can cast into the cascade: centre and half its sizes along its three axes (unit
   vectors, NULL for the world's), world coordinates. The maps are orthographic, so a caster shades
   a cascade wherever it lies along the light, as long as it lies across the light within the
   cascade's square and along it within the frame's depth range. */
int shadow_fit_reaches(const struct ShadowFitFrame *frame, const struct ShadowFitCascades *fit, int cascade,
                       const float *centre, const float *half, const float (*axes)[3]);

#endif
