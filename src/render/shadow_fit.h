#ifndef RENDER_SHADOW_FIT_H
#define RENDER_SHADOW_FIT_H

#include <stddef.h>

/* custom: the frame's shadow cascades fitted to what the frame shows (sample distribution shadow
   maps, plans/gpu-renderer.md): the steps the GPU's compute passes take (render/gfx_gpu.c), as pure
   functions that the passes are checked against. SHADOW_FIT_CASCADES maps of SHADOW_FIT_SIZE texels
   square along the map's light, each covering a square across the light that is rounded up to whole
   8 units and moved by whole texels. The range of depths the view is cut into slices from, and each
   slice's square, are kept from frame to frame while they still serve, so that shadow edges do not
   jump while the camera moves.

   A frame is fitted in this order:
   1. shadow_fit_frame and shadow_fit_bounds (the CPU): the light's axes, the camera, the shadows'
      reach and the light's depth range, the constants of the passes;
   2. shadow_fit_range: over the receivers' view depths, the nearest and the farthest;
   3. shadow_fit_keep: the range the frame cuts its slices from, from that and the frame before's;
   4. shadow_fit_split: that range cut into one slice per cascade;
   5. shadow_fit_extents: over the depths again, each slice's pixels across the light;
   6. shadow_fit_cascades: each slice's square, from the frame before's, its matrices and its
      lookups.
   shadow_fit_reach tells the CPU, without the cascades, where whatever can cast into the frame's
   view lies.

   World and view space are the client's (render/gfx.h): +y is down, and view depth grows away from
   the camera. The receivers are the ground and the models, which the depth pass draws into two
   buffers of width * height view depths, row by row, at [j * width + i] for the window coordinates
   (i, j), Direct3D 9's pixel centres (render/gfx_gpu.c, MARGIN): `front` holds the nearest receiver
   the pixel shows and `back` the nearest one that is not blended, through which nothing behind it
   shows; 0 where it shows none. The pixel shows the receivers from front to back: front alone where
   back is 0 (blended faces before the sky) or no farther than front. What it shows from the near
   plane to the shadows' reach (ShadowFitFrame.reach) receives shadows; the rest takes no part in the
   fit.

   Floats are rounded as IEEE single precision with every operation rounded on its own (no fused
   multiply-adds), so the results are the same bits on every platform the client builds for and
   from run to run, whatever order the pixels are visited in. The compute passes
   (src/render/shaders/shadow_range.hlsl, shadow_extents.hlsl, shadow_cascades.hlsl, through
   render/shadow_fit_gpu.h) do the same steps in ordinary shader arithmetic
   (tests/test_shadow_fit_gpu.c): the range, the range kept, and given the same split the extents,
   the squares' sizes and the texels their centres lie on are the C's to the bit on every GPU, as
   they depend only on additions, subtractions, multiplications, comparisons and divisions by
   powers of two, which Vulkan rounds exactly, and on the snap, which an exact correction makes
   independent of how the GPU divides; the split and the rest are within the rounding a GPU's
   division and square root may differ by. */

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
    int width, height;  /* the screen's pixels */
    float nearZ, farZ;
    float distance;   /* from the camera to its focus */
};

/* The constants of a frame's passes, which the CPU works out once a frame. */
struct ShadowFitFrame {
    float axis[3][3];         /* the light's axes: two across it, then the way it travels */
    float inverse[12];        /* view to world, a 4x3 as ShadowFitCamera.view */
    /* the pixel (i, j) at view depth d lies along the light's axis a at
       pixel[a][3] + d * ((i - centre[0]) * pixel[a][0] + (j - centre[1]) * pixel[a][1] + pixel[a][2]) */
    float pixel[3][4];
    float centre[2];
    float size[2];            /* the screen's width and height in pixels */
    float nearZ, reach;       /* the view depths that receive shadows */
    float lightNear, lightRange;  /* along the light: the boxes, and a margin either side */
    float lightView[3];       /* towards the light, view space, unit length */
    float bias;               /* the light depth behind the nearest caster within which a receiver
                                 stays lit, in the maps' depth from 0 to 1 */
};

/* The frame's constants for the camera and the light's direction (the way it travels), all but
   the light's depth range, which shadow_fit_bounds adds. The shadows reach four times the camera's
   distance deep, and 400 units more, but no deeper than the far plane. 0 where nothing can be
   fitted: the projection, the screen or the near plane is not positive, the view cannot be
   inverted, or the light has no finite direction. */
int shadow_fit_frame(struct ShadowFitFrame *out, const struct ShadowFitCamera *camera, const float *light);
/* The light's depth range of the frame (lightNear, lightRange, bias): the `count` boxes that hold
   every receiver and caster along the light, and a margin either side. 0, with the frame as it
   was, where no box is finite and not empty. */
int shadow_fit_bounds(struct ShadowFitFrame *frame, const struct ShadowFitBox *boxes, size_t count);

/* the receivers' view depths (above) */
struct ShadowFitDepths {
    const float *front, *back;
    int width, height;
};

/* the nearest and farthest view depth of what the pixels show that receives; nearest > farthest
   where nothing does */
struct ShadowFitRange {
    float nearest, farthest;
};

void shadow_fit_range(struct ShadowFitRange *out, const struct ShadowFitFrame *frame, const struct ShadowFitDepths *depths);

/* The range the frame cuts its slices from (kept), from the range it measured and the one the frame
   before kept (previous; nearest and farthest 0 for none). Where the slices move, every square's
   slice of the view moves with them, so the range stays while it serves:
   - its far end must hold every receiver, or the farthest would lie in no slice: it is kept while
     it lies from the measured farthest to a tenth beyond it, within the reach, and is otherwise
     taken anew, the measured farthest and a hundredth more, or the reach where the reach lies
     within a tenth beyond the farthest, as no receiver lies beyond the reach;
   - its near end only shares the depths out among the slices, as whatever lies nearer than the
     first slice's far depth lies in the first slice: it is kept while the measured nearest lies from
     it to a quarter beyond it, and is otherwise taken anew, the measured nearest. Receivers nearer
     than it take it at once, as they would otherwise crowd into the first slice and grow its square
     frame by frame while the camera walks towards them.
   Kept so, it follows a zoom in steps and holds still while the camera walks or shakes. Where
   nothing receives, the frame before's stays, as it is. */
void shadow_fit_keep(struct ShadowFitRange *kept, const struct ShadowFitRange *measured,
                     const struct ShadowFitRange *previous, const struct ShadowFitFrame *frame);

/* The view depths that bound each cascade's slice: split[0] the nearest, split[c + 1] cascade c's
   far depth, the last a unit beyond the farthest. The slices grow geometrically, each slice's far
   depth the same multiple of its near one. All 0 where nothing receives. */
void shadow_fit_split(float *split, const struct ShadowFitRange *range);

/* where a slice's receivers lie across the light, along the frame's axes 0 and 1; lo > hi where none
   does */
struct ShadowFitExtent {
    float lo[2], hi[2];
};

/* A point belongs to the first slice whose far depth (split[c + 1]) lies beyond it, the cascade the
   receiving shaders look it up in. A pixel's receivers from front to back reach each slice from
   their first point in it to their last, and the light's axes run linearly along them, so those two
   points bound their part of the slice: a slice's far depth ends what lies before it. */
void shadow_fit_extents(struct ShadowFitExtent *out, const struct ShadowFitFrame *frame, const float *split,
                        const struct ShadowFitDepths *depths);

/* The frame's cascades, in the conventions of render/gfx.h's shadows. */
struct ShadowFitCascades {
    /* SHADOW_FIT_CASCADES, or 0 where nothing receives: the receivers then keep the ground's baked
       shadows, as a frame without shadow maps does. The matrices are finite either way. */
    int cascades;
    /* world to each cascade's light space, row vectors: x and y from -1 to 1 across its map, z from
       0 to 1 along the light */
    float world[SHADOW_FIT_CASCADES][16];
    /* the same from view space */
    float fromView[SHADOW_FIT_CASCADES][16];
    /* each cascade's far view depth, the lookup's offset towards the light in view units, the soft
       edge's radius in texture units of its map, and the light depth a texel spans per unit of a
       surface's slant to the light */
    float farDepth[SHADOW_FIT_CASCADES];
    float offset[SHADOW_FIT_CASCADES];
    float blur[SHADOW_FIT_CASCADES];
    float slant[SHADOW_FIT_CASCADES];
    /* each map's square across the light, along the frame's axes 0 and 1; lo > hi for a slice
       that nothing lies in, which no caster reaches */
    float lo[SHADOW_FIT_CASCADES][2], hi[SHADOW_FIT_CASCADES][2];
    /* the half size of each square to remember for the next frame: the square's where its slice
       holds receivers, the frame before's where it holds none */
    float kept[SHADOW_FIT_CASCADES];
};

/* Each slice's square, centred on its receivers and half as wide as the wider of their two
   extents, 5 percent and 4.5 units more for the soft edge and the lookup's offset: rounded up to a
   multiple of 8 units, that is what it needs. Measured the same way, the slice of the view itself
   (the screen's corners from the slice's near depth, the near plane for the first, to its far one)
   is what it can need at most. Where the frame before's size (previous, 0 for none) is at least
   what the square needs and at most the larger of a fifth more and what it can need at most, the
   square keeps it: receivers that come and go within a slice that stays, as they do while the
   camera walks, change nothing. Otherwise it takes anew what they need grown by a fifth where they
   outgrew the frame before's, the top of the band it is kept in, as receivers that spread tend to
   spread on, and by 5 percent where they did not (none kept, or one too large), but no more than
   what it can need at most, rounded up to a multiple of 8. Its centre is the whole number of its
   map's texels nearest the receivers' centre (ties to the greater), so that its texels stay put
   while it keeps its size, the same however a GPU rounds a division. A slice that nothing lies in
   gets a square of 8 units at the light's origin and keeps the frame before's size for the next. */
void shadow_fit_cascades(struct ShadowFitCascades *out, const struct ShadowFitFrame *frame, const float *split,
                         const struct ShadowFitExtent *extents, const float *previous);

/* Where whatever can cast a shadow into the view lies at most: across the light from lo to hi
   (along the frame's axes 0 and 1), around the view from the near plane to the shadows' reach as
   wide as the screen, with a margin for the lookups' offset and soft edge; and along the light
   before `farthest`, the farthest point of that view along it. */
struct ShadowFitReach {
    float lo[2], hi[2];
    float farthest;
};

void shadow_fit_reach(struct ShadowFitReach *out, const struct ShadowFitFrame *frame);

#endif
