#include "render/shadow_fit.h"
#include <math.h>
#include <string.h>

/* Clang fuses a * b + c within an expression unless told not to (GCC and MSVC do not in C11);
   fused, the results would differ in their last bits from platform to platform */
#if defined(__clang__)
#pragma STDC FP_CONTRACT OFF
#endif

/* the slices grow with depth, SHADOW_SPLIT of the way from even steps to geometric ones */
#define SHADOW_SPLIT 0.7f
/* along the light the maps keep the boxes and this many units either side */
#define SHADOW_MARGIN 8.0f
/* a pixel stays lit up to SHADOW_BIAS units behind the nearest caster, and is looked up from a
   point moved SHADOW_OFFSET texels off its surface towards the light (times the sine of the angle
   between them), which keeps a surface at a slant to the light from shadowing itself. Models lay
   decorations a unit or two over their surfaces (Prontera's flower beds put the flowers over the
   grass); at 1 unit those shaded the surface under them. The bias grows by a texel's width times
   the tangent of the surface's slant to the light, what the depth of one texel spans over it. */
#define SHADOW_BIAS 4.0f
#define SHADOW_OFFSET 1.5f
/* shadow edges fade over twice SHADOW_SOFTNESS units; at 1.5 the shadows of thin casters, such as
   a lamp post's, faded away */
#define SHADOW_SOFTNESS 0.5f
/* the shadows reach REACH_SCALE times the camera's distance deep, and REACH_EXTRA units more */
#define REACH_SCALE 4.0f
#define REACH_EXTRA 400.0f

static float dot3(const float *a, const float *b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static int invert_4x3(const float *m, float *out)
{
    float det = m[0] * (m[4] * m[8] - m[5] * m[7]) - m[1] * (m[3] * m[8] - m[5] * m[6])
                + m[2] * (m[3] * m[7] - m[4] * m[6]);
    float inv;
    int i;

    if (!(det > 1e-12f || det < -1e-12f))
        return 0;
    inv = 1.0f / det;
    out[0] = (m[4] * m[8] - m[5] * m[7]) * inv;
    out[1] = (m[2] * m[7] - m[1] * m[8]) * inv;
    out[2] = (m[1] * m[5] - m[2] * m[4]) * inv;
    out[3] = (m[5] * m[6] - m[3] * m[8]) * inv;
    out[4] = (m[0] * m[8] - m[2] * m[6]) * inv;
    out[5] = (m[2] * m[3] - m[0] * m[5]) * inv;
    out[6] = (m[3] * m[7] - m[4] * m[6]) * inv;
    out[7] = (m[1] * m[6] - m[0] * m[7]) * inv;
    out[8] = (m[0] * m[4] - m[1] * m[3]) * inv;
    for (i = 0; i < 3; i++)
        out[9 + i] = -(m[9] * out[i] + m[10] * out[3 + i] + m[11] * out[6 + i]);
    return 1;
}

static int is_finite(float v)
{
    return v - v == 0.0f;
}

/* a box's centre and half its size, or 0 where it is empty or not finite */
static int box_extent(const struct ShadowFitBox *box, float *centre, float *half)
{
    int i;

    for (i = 0; i < 3; i++) {
        if (!(box->min[i] > -1e30f && box->max[i] < 1e30f && box->min[i] <= box->max[i]))
            return 0;
        centre[i] = (box->min[i] + box->max[i]) * 0.5f;
        half[i] = (box->max[i] - box->min[i]) * 0.5f;
    }
    return 1;
}

int shadow_fit_frame(struct ShadowFitFrame *out, const struct ShadowFitCamera *camera, const float *light,
                     const struct ShadowFitBox *boxes, size_t count)
{
    const float *view = camera->view, *inv = out->inverse;
    float f[3], up[3] = {0.0f, 0.0f, 0.0f}, r[3], u[3], centre[3], half[3];
    float len, lightNear = 1e30f, lightFar = -1e30f;
    size_t b;
    int i, k, a;

    memset(out, 0, sizeof *out);
    if (!(camera->proj[0] > 0.0f && camera->proj[1] > 0.0f && camera->nearZ > 0.0f)
        || !invert_4x3(view, out->inverse))
        return 0;
    /* the map's light vector points the way the light travels, down (+y) onto the map */
    len = sqrtf(dot3(light, light));
    if (!(len > 0.0f && is_finite(len)))
        return 0;
    for (i = 0; i < 3; i++)
        f[i] = light[i] / len;
    /* across the light: the world axis the light runs least along, crossed with it */
    k = fabsf(f[0]) <= fabsf(f[1]) ? (fabsf(f[0]) <= fabsf(f[2]) ? 0 : 2) : (fabsf(f[1]) <= fabsf(f[2]) ? 1 : 2);
    up[k] = 1.0f;
    r[0] = up[1] * f[2] - up[2] * f[1];
    r[1] = up[2] * f[0] - up[0] * f[2];
    r[2] = up[0] * f[1] - up[1] * f[0];
    len = sqrtf(dot3(r, r));
    for (i = 0; i < 3; i++)
        r[i] /= len;
    u[0] = f[1] * r[2] - f[2] * r[1];
    u[1] = f[2] * r[0] - f[0] * r[2];
    u[2] = f[0] * r[1] - f[1] * r[0];

    /* along the light the maps keep every box: the casters stand in them too */
    for (b = 0; b < count; b++) {
        float mid, reach;

        if (!box_extent(&boxes[b], centre, half))
            continue;
        mid = dot3(centre, f);
        reach = fabsf(f[0]) * half[0] + fabsf(f[1]) * half[1] + fabsf(f[2]) * half[2];
        lightNear = mid - reach < lightNear ? mid - reach : lightNear;
        lightFar = mid + reach > lightFar ? mid + reach : lightFar;
    }
    if (lightNear > lightFar)
        return 0;
    lightNear -= SHADOW_MARGIN;
    out->lightNear = lightNear;
    out->lightRange = lightFar + SHADOW_MARGIN - lightNear;
    out->bias = SHADOW_BIAS / out->lightRange;

    for (i = 0; i < 3; i++) {
        out->axis[0][i] = r[i];
        out->axis[1][i] = u[i];
        out->axis[2][i] = f[i];
    }
    /* a pixel's point in the world is the eye (inverse row 3) plus its view depth times its ray:
       inverse rows 0 and 1 by its offset from the centre over proj, and row 2 */
    for (a = 0; a < 2; a++) {
        const float *axis = out->axis[a];

        out->pixel[a][0] = dot3(axis, &inv[0]) / camera->proj[0];
        out->pixel[a][1] = dot3(axis, &inv[3]) / camera->proj[1];
        out->pixel[a][2] = dot3(axis, &inv[6]);
        out->pixel[a][3] = dot3(axis, &inv[9]);
    }
    out->centre[0] = camera->centre[0];
    out->centre[1] = camera->centre[1];
    out->nearZ = camera->nearZ;
    out->reach = camera->distance * REACH_SCALE + REACH_EXTRA;
    out->reach = out->reach < camera->farZ ? out->reach : camera->farZ;

    /* towards the light in view space */
    for (i = 0; i < 3; i++)
        out->lightView[i] = -(f[0] * view[i] + f[1] * view[3 + i] + f[2] * view[6 + i]);
    len = sqrtf(dot3(out->lightView, out->lightView));
    if (len > 0.0f)
        for (i = 0; i < 3; i++)
            out->lightView[i] /= len;
    return 1;
}

static int receives(const struct ShadowFitFrame *frame, float depth)
{
    return depth >= frame->nearZ && depth <= frame->reach;
}

void shadow_fit_range(struct ShadowFitRange *out, const struct ShadowFitFrame *frame, const float *depth, int width,
                      int height)
{
    size_t n = width > 0 && height > 0 ? (size_t)width * (size_t)height : 0, p;

    out->nearest = HUGE_VALF;
    out->farthest = -HUGE_VALF;
    for (p = 0; p < n; p++) {
        float d = depth[p];

        if (!receives(frame, d))
            continue;
        out->nearest = d < out->nearest ? d : out->nearest;
        out->farthest = d > out->farthest ? d : out->farthest;
    }
}

void shadow_fit_split(float *split, const struct ShadowFitRange *range)
{
    float nearest = range->nearest, farthest, power[SHADOW_FIT_CASCADES + 1];
    int c;

    if (!(nearest <= range->farthest && nearest > 0.0f)) {
        memset(split, 0, (SHADOW_FIT_CASCADES + 1) * sizeof *split);
        return;
    }
    /* a unit beyond: the shaders look a pixel up in the first cascade whose far depth lies beyond
       it, so that the farthest pixel itself has one */
    farthest = range->farthest + 1.0f;
    /* (farthest / nearest) to the quarter powers by square roots, which are rounded exactly
       everywhere, where powf differs between C libraries and on the GPU */
    power[0] = 1.0f;
    power[4] = farthest / nearest;
    power[2] = sqrtf(power[4]);
    power[1] = sqrtf(power[2]);
    power[3] = power[1] * power[2];
    for (c = 0; c <= SHADOW_FIT_CASCADES; c++) {
        float t = (float)c / SHADOW_FIT_CASCADES;
        split[c] = SHADOW_SPLIT * nearest * power[c] + (1.0f - SHADOW_SPLIT) * (nearest + (farthest - nearest) * t);
    }
    split[0] = nearest;
    split[SHADOW_FIT_CASCADES] = farthest;
}

/* Where the pixel (i, j) at view depth d lies across the light. A zero is made +0, so that which
   of two equal values is the least or the greatest does not depend on their order. */
static float pixel_across(const struct ShadowFitFrame *frame, int a, int i, int j, float d)
{
    const float *k = frame->pixel[a];
    float t = ((float)i - frame->centre[0]) * k[0] + ((float)j - frame->centre[1]) * k[1] + k[2];
    float x = k[3] + d * t;

    return x == 0.0f ? 0.0f : x;
}

void shadow_fit_extents(struct ShadowFitExtent *out, const struct ShadowFitFrame *frame, const float *split,
                        const float *depth, int width, int height)
{
    int c, i, j, a;

    for (c = 0; c < SHADOW_FIT_CASCADES; c++)
        for (a = 0; a < 2; a++) {
            out[c].lo[a] = HUGE_VALF;
            out[c].hi[a] = -HUGE_VALF;
        }
    for (j = 0; j < height; j++)
        for (i = 0; i < width; i++) {
            float d = depth[(size_t)j * (size_t)width + (size_t)i];

            if (!receives(frame, d))
                continue;
            for (c = 0; c < SHADOW_FIT_CASCADES && !(d < split[c + 1]); c++)
                ;
            if (c == SHADOW_FIT_CASCADES)
                continue;
            for (a = 0; a < 2; a++) {
                float x = pixel_across(frame, a, i, j, d);

                out[c].lo[a] = x < out[c].lo[a] ? x : out[c].lo[a];
                out[c].hi[a] = x > out[c].hi[a] ? x : out[c].hi[a];
            }
        }
}

void shadow_fit_cascades(struct ShadowFitCascades *out, const struct ShadowFitFrame *frame, const float *split,
                         const struct ShadowFitExtent *extents)
{
    const float *inv = frame->inverse, *r = frame->axis[0], *u = frame->axis[1], *f = frame->axis[2];
    float range = frame->lightRange;
    int c, i, j, k;

    out->cascades = 0;
    for (c = 0; c < SHADOW_FIT_CASCADES; c++) {
        const struct ShadowFitExtent *e = &extents[c];
        float square[2] = {0.0f, 0.0f}, radius = 8.0f, texel, w[4][4];
        int used = e->lo[0] <= e->hi[0] && e->lo[1] <= e->hi[1];

        /* a margin for the soft edge and the offset, then up to a multiple of 8 units */
        if (used) {
            out->cascades = SHADOW_FIT_CASCADES;
            square[0] = (e->lo[0] + e->hi[0]) * 0.5f;
            square[1] = (e->lo[1] + e->hi[1]) * 0.5f;
            radius = (e->hi[0] - e->lo[0] > e->hi[1] - e->lo[1] ? e->hi[0] - e->lo[0] : e->hi[1] - e->lo[1]) * 0.5f;
            radius = ceilf((radius * 1.05f + 4.0f + SHADOW_SOFTNESS) / 8.0f) * 8.0f;
        }
        /* the centre on whole texels, so that the texels stay put while the camera moves */
        texel = 2.0f * radius / SHADOW_FIT_SIZE;
        square[0] = floorf(square[0] / texel + 0.5f) * texel;
        square[1] = floorf(square[1] / texel + 0.5f) * texel;
        for (i = 0; i < 2; i++) {
            out->lo[c][i] = used ? square[i] - radius : 1e30f;
            out->hi[c][i] = used ? square[i] + radius : -1e30f;
        }
        for (i = 0; i < 3; i++) {
            w[i][0] = r[i] / radius;
            w[i][1] = u[i] / radius;
            w[i][2] = f[i] / range;
            w[i][3] = 0.0f;
        }
        w[3][0] = -square[0] / radius;
        w[3][1] = -square[1] / radius;
        w[3][2] = -frame->lightNear / range;
        w[3][3] = 1.0f;
        memcpy(out->world[c], w, sizeof out->world[c]);
        /* view to world (the inverse view, a 4x4 with the last column 0 0 0 1), then to light */
        for (i = 0; i < 4; i++)
            for (j = 0; j < 4; j++) {
                float sum = i == 3 ? w[3][j] : 0.0f;
                for (k = 0; k < 3; k++)
                    sum += inv[i * 3 + k] * w[k][j];
                out->fromView[c][i * 4 + j] = sum;
            }
        out->farDepth[c] = split[c + 1];
        out->offset[c] = SHADOW_OFFSET * texel;
        out->slant[c] = (SHADOW_SOFTNESS + texel) / range;
        out->blur[c] = SHADOW_SOFTNESS / (2.0f * radius);
    }
}

int shadow_fit_reaches(const struct ShadowFitFrame *frame, const struct ShadowFitCascades *fit, int cascade,
                       const float *centre, const float *half, const float (*axes)[3])
{
    static const float world[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    float mid[3], ext[3];
    int a, b;

    if (cascade < 0 || cascade >= fit->cascades)
        return 0;
    if (!axes)
        axes = world;
    for (a = 0; a < 3; a++) {
        mid[a] = dot3(centre, frame->axis[a]);
        ext[a] = 0.0f;
        for (b = 0; b < 3; b++)
            ext[a] += half[b] * fabsf(dot3(axes[b], frame->axis[a]));
    }
    if (mid[2] + ext[2] < frame->lightNear || mid[2] - ext[2] > frame->lightNear + frame->lightRange)
        return 0;
    return mid[0] + ext[0] >= fit->lo[cascade][0] && mid[0] - ext[0] <= fit->hi[cascade][0]
           && mid[1] + ext[1] >= fit->lo[cascade][1] && mid[1] - ext[1] <= fit->hi[cascade][1];
}
