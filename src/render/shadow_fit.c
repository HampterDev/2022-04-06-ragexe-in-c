#include "render/shadow_fit.h"
#include <math.h>
#include <string.h>

/* Clang fuses a * b + c within an expression unless told not to (GCC and MSVC do not in C11);
   fused, the results would differ in their last bits from platform to platform */
#if defined(__clang__)
#pragma STDC FP_CONTRACT OFF
#endif

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
/* the range the slices are cut from is kept while its far end lies from the measured farthest to
   FAR_KEEP times it, and is taken anew FAR_AHEAD times the measured farthest, or the reach where
   that lies within FAR_KEEP times it; its near end while the measured nearest lies from it to
   NEAR_ABOVE times it (shadow_fit_keep) */
#define FAR_KEEP 1.1f
#define FAR_AHEAD 1.01f
#define NEAR_ABOVE 1.25f
/* a square is kept while it is at most SQUARE_KEEP times what it needs, or no larger than its slice
   of the view; taken anew it is SQUARE_KEEP times what it needs where it grows, SQUARE_AHEAD
   otherwise (shadow_fit_cascades) */
#define SQUARE_AHEAD 1.05f
#define SQUARE_KEEP 1.2f

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

int shadow_fit_frame(struct ShadowFitFrame *out, const struct ShadowFitCamera *camera, const float *light)
{
    const float *view = camera->view, *inv = out->inverse;
    float f[3], up[3] = {0.0f, 0.0f, 0.0f}, r[3], u[3];
    float len;
    int i, k, a;

    memset(out, 0, sizeof *out);
    if (!(camera->proj[0] > 0.0f && camera->proj[1] > 0.0f && camera->nearZ > 0.0f) || camera->width <= 0
        || camera->height <= 0 || !invert_4x3(view, out->inverse))
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
    for (i = 0; i < 3; i++) {
        out->axis[0][i] = r[i];
        out->axis[1][i] = u[i];
        out->axis[2][i] = f[i];
    }
    /* a pixel's point in the world is the eye (inverse row 3) plus its view depth times its ray:
       inverse rows 0 and 1 by its offset from the centre over proj, and row 2 */
    for (a = 0; a < 3; a++) {
        const float *axis = out->axis[a];

        out->pixel[a][0] = dot3(axis, &inv[0]) / camera->proj[0];
        out->pixel[a][1] = dot3(axis, &inv[3]) / camera->proj[1];
        out->pixel[a][2] = dot3(axis, &inv[6]);
        out->pixel[a][3] = dot3(axis, &inv[9]);
    }
    out->centre[0] = camera->centre[0];
    out->centre[1] = camera->centre[1];
    out->size[0] = (float)camera->width;
    out->size[1] = (float)camera->height;
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

int shadow_fit_bounds(struct ShadowFitFrame *frame, const struct ShadowFitBox *boxes, size_t count)
{
    const float *f = frame->axis[2];
    float centre[3], half[3], lightNear = 1e30f, lightFar = -1e30f;
    size_t b;

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
    frame->lightNear = lightNear;
    frame->lightRange = lightFar + SHADOW_MARGIN - lightNear;
    frame->bias = SHADOW_BIAS / frame->lightRange;
    return 1;
}

/* What the pixel at p shows that receives, from lo to hi: its receivers from front to back, within
   the near plane and the reach. 0 where it shows none there. */
static int shown(const struct ShadowFitFrame *frame, const struct ShadowFitDepths *depths, size_t p, float *lo,
                 float *hi)
{
    float front = depths->front[p], back = depths->back[p];

    if (!(front > 0.0f))
        return 0;
    back = back > front ? back : front;
    *lo = front > frame->nearZ ? front : frame->nearZ;
    *hi = back < frame->reach ? back : frame->reach;
    return *lo <= *hi;
}

static size_t pixels(const struct ShadowFitDepths *depths)
{
    return depths->width > 0 && depths->height > 0 ? (size_t)depths->width * (size_t)depths->height : 0;
}

void shadow_fit_range(struct ShadowFitRange *out, const struct ShadowFitFrame *frame, const struct ShadowFitDepths *depths)
{
    size_t n = pixels(depths), p;
    float lo, hi;

    out->nearest = HUGE_VALF;
    out->farthest = -HUGE_VALF;
    for (p = 0; p < n; p++) {
        if (!shown(frame, depths, p, &lo, &hi))
            continue;
        out->nearest = lo < out->nearest ? lo : out->nearest;
        out->farthest = hi > out->farthest ? hi : out->farthest;
    }
}

void shadow_fit_keep(struct ShadowFitRange *kept, const struct ShadowFitRange *measured,
                     const struct ShadowFitRange *previous, const struct ShadowFitFrame *frame)
{
    float nearest = previous->nearest, farthest = previous->farthest;
    float m0 = measured->nearest, m1 = measured->farthest;

    *kept = *previous;
    if (!(m0 <= m1 && m0 > 0.0f))
        return;
    /* a range of none (0), or one not finite, is taken anew: no comparison holds for it */
    if (!(farthest >= m1 && farthest <= m1 * FAR_KEEP && farthest <= frame->reach)) {
        /* where the reach lies within the band it is kept in, the reach: no receiver lies beyond
           it, so the far end need not grow again */
        farthest = frame->reach <= m1 * FAR_KEEP ? frame->reach : m1 * FAR_AHEAD;
        farthest = farthest > m1 ? farthest : m1;
    }
    /* kept, it lies no farther than the measured nearest, so before the far end */
    if (!(m0 >= nearest && m0 <= nearest * NEAR_ABOVE))
        nearest = m0;
    kept->nearest = nearest;
    kept->farthest = farthest;
}

_Static_assert(SHADOW_FIT_CASCADES == 4, "shadow_fit_split takes the quarter powers by square roots");

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
    /* geometric: (farthest / nearest) to the quarter powers by square roots, which every C library
       rounds exactly, where powf differs from one to another (a GPU's square root and division
       may differ by a few units in the last place: render/shadow_fit.h) */
    power[4] = farthest / nearest;
    power[2] = sqrtf(power[4]);
    power[1] = sqrtf(power[2]);
    power[3] = power[1] * power[2];
    split[0] = nearest;
    for (c = 1; c < SHADOW_FIT_CASCADES; c++)
        split[c] = nearest * power[c];
    split[SHADOW_FIT_CASCADES] = farthest;
}

/* Where the pixel (i, j) at view depth d lies along the light's axis a. A zero is made +0, so that
   which of two equal values is the least or the greatest does not depend on their order. */
static float pixel_across(const struct ShadowFitFrame *frame, int a, int i, int j, float d)
{
    const float *k = frame->pixel[a];
    float t = ((float)i - frame->centre[0]) * k[0] + ((float)j - frame->centre[1]) * k[1] + k[2];
    float x = k[3] + d * t;

    return x == 0.0f ? 0.0f : x;
}

/* the slice of a view depth: the first whose far depth lies beyond it, SHADOW_FIT_CASCADES for none */
static int slice_of(const float *split, float d)
{
    int c;

    for (c = 0; c < SHADOW_FIT_CASCADES && !(d < split[c + 1]); c++)
        ;
    return c;
}

static void extend(struct ShadowFitExtent *e, const struct ShadowFitFrame *frame, int i, int j, float d)
{
    int a;

    for (a = 0; a < 2; a++) {
        float x = pixel_across(frame, a, i, j, d);

        e->lo[a] = x < e->lo[a] ? x : e->lo[a];
        e->hi[a] = x > e->hi[a] ? x : e->hi[a];
    }
}

void shadow_fit_extents(struct ShadowFitExtent *out, const struct ShadowFitFrame *frame, const float *split,
                        const struct ShadowFitDepths *depths)
{
    int c, i, j, a, first, last;
    float lo, hi;

    for (c = 0; c < SHADOW_FIT_CASCADES; c++)
        for (a = 0; a < 2; a++) {
            out[c].lo[a] = HUGE_VALF;
            out[c].hi[a] = -HUGE_VALF;
        }
    for (j = 0; j < depths->height; j++)
        for (i = 0; i < depths->width; i++) {
            if (!shown(frame, depths, (size_t)j * (size_t)depths->width + (size_t)i, &lo, &hi))
                continue;
            first = slice_of(split, lo);
            last = slice_of(split, hi);
            last = last < SHADOW_FIT_CASCADES ? last : SHADOW_FIT_CASCADES - 1;
            for (c = first; c <= last; c++) {
                extend(&out[c], frame, i, j, c == first ? lo : split[c]);
                extend(&out[c], frame, i, j, c == last ? hi : split[c + 1]);
            }
        }
}

/* Half a square across the light that holds a slice's receivers spread over `wide` units (the wider
   of their two extents): a margin for the soft edge and the lookup's offset, then up to a multiple
   of 8 units. */
static float square_need(float wide)
{
    return wide * 0.5f * 1.05f + 4.0f + SHADOW_SOFTNESS;
}

/* The wider of the two extents across the light of the view from view depth d0 to d1: the screen's
   corners at both, as the axes run linearly along the view's edges and its depth. */
static float view_wide(const struct ShadowFitFrame *frame, float d0, float d1)
{
    float lo[2] = {HUGE_VALF, HUGE_VALF}, hi[2] = {-HUGE_VALF, -HUGE_VALF};
    int k, a;

    for (k = 0; k < 8; k++) {
        int i = k & 1 ? (int)frame->size[0] - 1 : 0, j = k & 2 ? (int)frame->size[1] - 1 : 0;

        for (a = 0; a < 2; a++) {
            float x = pixel_across(frame, a, i, j, k & 4 ? d1 : d0);

            lo[a] = x < lo[a] ? x : lo[a];
            hi[a] = x > hi[a] ? x : hi[a];
        }
    }
    return hi[0] - lo[0] > hi[1] - lo[1] ? hi[0] - lo[0] : hi[1] - lo[1];
}

/* The whole number of texels nearest x (ties to the greater): floor(x / texel + 0.5), made exact by
   a correction with exact products, so that it does not depend on how the division rounds (a GPU
   may round it either way). texel is a multiple of 8 units over 1024, k / 128 for a whole k, so
   (n -+ 0.5) * texel = (2n -+ 1) * k / 256 is exact while |x| < 2^16 units. */
static float whole_texels(float x, float texel)
{
    float n = floorf(x / texel + 0.5f);

    if ((n - 0.5f) * texel > x)
        n -= 1.0f;
    else if ((n + 0.5f) * texel <= x)
        n += 1.0f;
    return n;
}

void shadow_fit_cascades(struct ShadowFitCascades *out, const struct ShadowFitFrame *frame, const float *split,
                         const struct ShadowFitExtent *extents, const float *previous)
{
    const float *inv = frame->inverse, *r = frame->axis[0], *u = frame->axis[1], *f = frame->axis[2];
    float range = frame->lightRange;
    int c, i, j, k;

    out->cascades = 0;
    for (c = 0; c < SHADOW_FIT_CASCADES; c++) {
        const struct ShadowFitExtent *e = &extents[c];
        float square[2] = {0.0f, 0.0f}, radius = 8.0f, texel, w[4][4];
        int used = e->lo[0] <= e->hi[0] && e->lo[1] <= e->hi[1];

        out->kept[c] = previous[c];
        if (used) {
            float need, base, most, keep;

            out->cascades = SHADOW_FIT_CASCADES;
            square[0] = (e->lo[0] + e->hi[0]) * 0.5f;
            square[1] = (e->lo[1] + e->hi[1]) * 0.5f;
            need = square_need(e->hi[0] - e->lo[0] > e->hi[1] - e->lo[1] ? e->hi[0] - e->lo[0] : e->hi[1] - e->lo[1]);
            base = ceilf(need / 8.0f) * 8.0f;
            /* the most it can need: its slice of the view, the first from the near plane */
            most = ceilf(square_need(view_wide(frame, c ? split[c] : frame->nearZ, split[c + 1])) / 8.0f) * 8.0f;
            keep = base * SQUARE_KEEP > most ? base * SQUARE_KEEP : most;
            if (previous[c] >= base && previous[c] <= keep) {
                radius = previous[c];
            } else {
                /* one its receivers outgrew grows to the top of the band it is kept in, as receivers
                   that spread tend to spread on; one taken anew otherwise (none kept, or one too
                   large) a little ahead of what they need */
                float ahead = previous[c] > 0.0f && previous[c] < base ? SQUARE_KEEP : SQUARE_AHEAD;

                radius = ceilf(need * ahead / 8.0f) * 8.0f;
                radius = radius < most ? radius : most;
                radius = radius > base ? radius : base;
            }
            out->kept[c] = radius;
        }
        /* the centre on whole texels, so that the texels stay put while the camera moves */
        texel = 2.0f * radius / SHADOW_FIT_SIZE;
        for (i = 0; i < 2; i++) {
            square[i] = whole_texels(square[i], texel) * texel;
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

void shadow_fit_reach(struct ShadowFitReach *out, const struct ShadowFitFrame *frame)
{
    float margin;
    int k, a;

    out->lo[0] = out->lo[1] = HUGE_VALF;
    out->hi[0] = out->hi[1] = out->farthest = -HUGE_VALF;
    /* the view's corners from the near plane to the reach: the axes run linearly along the view's
       edges, so the view lies within its corners' extremes */
    for (k = 0; k < 8; k++) {
        int i = k & 1 ? (int)frame->size[0] : 0, j = k & 2 ? (int)frame->size[1] : 0;
        float d = k & 4 ? frame->reach : frame->nearZ, x[3];

        for (a = 0; a < 3; a++)
            x[a] = pixel_across(frame, a, i, j, d);
        for (a = 0; a < 2; a++) {
            out->lo[a] = x[a] < out->lo[a] ? x[a] : out->lo[a];
            out->hi[a] = x[a] > out->hi[a] ? x[a] : out->hi[a];
        }
        out->farthest = x[2] > out->farthest ? x[2] : out->farthest;
    }
    /* A receiver is looked up from a point moved off it by 1.5 texels, over a disc of half a unit.
       A square's half size, kept up to a fifth larger than it needs or no larger than its slice of
       the view, is at most 0.63 times the view's extent across the light and 15 units more, so 1.5
       texels are under a thousandth of that extent and a few hundredths of a unit: a 512th of it
       and 2 units more hold both. */
    margin = (out->hi[0] - out->lo[0] > out->hi[1] - out->lo[1] ? out->hi[0] - out->lo[0] : out->hi[1] - out->lo[1])
                 / 512.0f
             + 2.0f;
    for (a = 0; a < 2; a++) {
        out->lo[a] -= margin;
        out->hi[a] += margin;
    }
}
