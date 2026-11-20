#ifndef TESTS_FIXTURES_SHADOW_BOX_FIT_H
#define TESTS_FIXTURES_SHADOW_BOX_FIT_H

#include "render/shadow_fit.h"
#include <stddef.h>

/* The CPU's shadow fit to boxes that the GPU's fit to the frame's pixels replaced
   (render/rp_project.c, renderer_shadow_frame, as the client had it at 5ea7b76b), for
   tests/test_shadow_fit.c to compare texel sizes and resizes with. */

struct BoxFit {
    int cascades;
    float fromView[SHADOW_FIT_CASCADES][16];
    float farDepth[SHADOW_FIT_CASCADES], offset[SHADOW_FIT_CASCADES], blur[SHADOW_FIT_CASCADES];
    float slant[SHADOW_FIT_CASCADES];
    float lightView[3], bias;
    /* where a caster can lie: across the light (axis 0 and 1) in a cascade's square, along it
       (axis 2) from depth[0] to depth[1] */
    float axis[3][3];
    float lo[SHADOW_FIT_CASCADES][2], hi[SHADOW_FIT_CASCADES][2];
    float depth[2];
};

/* The cascades fitted to the parts of the boxes the camera's view of a screen width by height
   pixels sees, as renderer_shadow_frame fitted them. 0 where it drew no shadows. */
int box_fit(struct BoxFit *out, const struct ShadowFitCamera *camera, int width, int height, const float *light,
            const struct ShadowFitBox *boxes, size_t count);

#endif
