#include "render/render_api.h"
#include "render/renderer.h"
#include "render/math3d.h"
#include "render/rp_pool.h"
#include "render/render_options.h"
#include <math.h>
#include <string.h>

DWORD rp_fog_blend(DWORD argb)
{
    DWORD fog = g_fogColor;
    float f = (float)(int)(fog >> 24) / 255.0f;
    float keep = 1.0f - f;
    BYTE *c = (BYTE *)&argb;

    c[2] = (BYTE)(int)((float)(int)c[2] * keep + (float)(int)((fog >> 16) & 0xff) * f);
    c[1] = (BYTE)(int)((float)(int)((fog >> 8) & 0xff) * f + (float)(int)c[1] * keep);
    c[0] = (BYTE)(int)((float)(int)c[0] * keep + (float)(int)(fog & 0xff) * f);
    return argb;
}

float renderer_screen_scale(struct Renderer *self, float rhw)
{
    return (float)((double)self->proj[0] * rhw / 7.0);
}

float renderer_scale_x(struct Renderer *self)
{
    return self->scaleX;
}

float sprite_row_rhw(float depth, float shift)
{
    float rhw;

    if (!(depth > 0.0f))
        return 1.0f / (depth + shift);
    rhw = (depth - shift) / (depth * depth);
    return rhw > 1.0f / RENDER_FAR_Z ? rhw : 1.0f / RENDER_FAR_Z;
}

float g_viewGroundUp[3] = {0.0f, 1.0f, 0.0f};

/* how far (a share of the depth) a flat sprite is moved towards the camera so that it stays above
   the ground's own triangles; an upright sprite's feet are lifted by twice that, to stay above it */
#define FLAT_DEPTH_SLACK 0.001f

static float ground_row_slope(const float *up, const float *pos)
{
    float height = up[0] * pos[0] + up[1] * pos[1] + up[2] * pos[2];

    if (height == 0.0f || !g_renderer || g_renderer->proj[1] == 0.0f)
        return 0.0f;
    return up[1] / (g_renderer->proj[1] * height);
}

float sprite_flat_row_rhw(const float *up, const float *pos, float rows)
{
    float rhw;

    if (!(pos[2] > 0.0f))
        return 1.0f / pos[2];
    rhw = (1.0f + FLAT_DEPTH_SLACK) / pos[2] + rows * ground_row_slope(up, pos);
    return rhw > 1.0f / RENDER_FAR_Z ? rhw : 1.0f / RENDER_FAR_Z;
}

float sprite_ground_lift(const float *up, const float *pos, float step, float rows)
{
    float slope;

    if (!(pos[2] > 0.0f))
        return 0.0f;
    slope = ground_row_slope(up, pos);
    return ((slope > 0.0f ? slope : 0.0f) + step / (pos[2] * pos[2])) * rows
           + 2.0f * FLAT_DEPTH_SLACK / pos[2];
}

float sprite_ground_squash(const float *up, const float *pos)
{
    float lu = sqrtf(up[0] * up[0] + up[1] * up[1] + up[2] * up[2]);
    float lp = sqrtf(pos[0] * pos[0] + pos[1] * pos[1] + pos[2] * pos[2]);
    float s;

    if (lu == 0.0f || lp == 0.0f)
        return 1.0f;
    s = fabsf(up[0] * pos[0] + up[1] * pos[1] + up[2] * pos[2]) / (lu * lp) / 0.70710678f;
    return s < 1.5f ? s : 1.5f;
}

void renderer_world_view(const struct Renderer *self, const float *view, struct GfxWorldView *out)
{
    memcpy(out->view, view, sizeof out->view);
    out->proj[0] = self->proj[0];
    out->proj[1] = self->proj[1];
    out->centre[0] = (float)self->centreX;
    out->centre[1] = (float)self->centreY;
    out->nearZ = RENDER_NEAR_Z;
    out->depthScale = RENDER_DEPTH_SCALE;
}

/* custom: the cascades of the meshes' shadow maps (render/gfx.h). The ground and the models
   receive shadows, and the world hands the frame the boxes they lie in (renderer_shadow_frame).
   The view is cut into GFX_SHADOW_CASCADES slices of view depth from the nearest to the farthest
   of its points inside those boxes, no further than four times the camera's distance and the
   view's far plane (a cap of 8,000 left everything out of reach, and the frame without shadows,
   when the camera looked down steeply from beyond about 8,000). The slices grow with the depth
   (SHADOW_SPLIT between even and geometric steps). Each cascade's map covers its slice's part of
   the boxes as the light sees it: the light map is orthographic, so a caster that shades
   something in the slice lands on the same place of the map, however far off screen it stands,
   and along the light the maps keep every box, SHADOW_MARGIN beyond. The receivers were once
   taken to lie in a band from 300 units above the camera's focus to 60 below it: hills and
   models taller than that, and from a flat camera far off even the focus, fell outside every
   cascade and were drawn without shadows. A cascade's square is rounded up and its centre kept to
   whole texels so that its texels stay put while the camera moves and shadow edges do not
   crawl. */
#define SHADOW_SPLIT 0.7f
#define SHADOW_MARGIN 8.0f
/* a pixel stays lit up to SHADOW_BIAS units behind the nearest caster, and is looked up from a
   point moved SHADOW_OFFSET texels off its surface towards the light (times the sine of the angle
   between them), which keeps a surface at a slant to the light from shadowing itself. Models lay
   decorations a unit or two over their surfaces (Prontera's flower beds put the flowers over the
   grass); at 1 unit those shaded the surface under them. Where a model lies that close over the
   ground, the ground keeps its baked shadow instead (render/gfx.h): Prontera's flower beds lay
   their grass 0.3 units and their flowers 0.2 to 2.4 units over the plaza, which the editor baked
   in shadow, and the shadow maps lit the plaza between the flowers, in bands where the
   comparison changed from texel to texel. The bias grows by a texel's width times the tangent of
   the surface's slant to the light, what the depth of one texel spans over it. */
#define SHADOW_BIAS 4.0f
#define SHADOW_OFFSET 1.5f
/* shadow edges fade over twice SHADOW_SOFTNESS units, about as far as the one coarse shadow map
   first drawn here blurred them (0.8 units a texel at the default zoom); at 1.5 the edges spread
   over three units and the shadows of thin casters, such as a lamp post's, faded away */
#define SHADOW_SOFTNESS 0.5f

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

static float dot3(const float *a, const float *b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

/* a slice of the view from one view depth to another: its corners, the corner rays' at the
   nearer depth and then at the farther (bit 1 across, bit 2 down, as the rays), and the planes
   that bound it, inside where n . p + n[3] >= 0; world coordinates */
struct ViewSlice {
    float corner[8][3];
    float plane[6][4];
};

/* the points of the view found inside the boxes: their view depths and where they lie across the
   light (axes r and u) */
struct ShadowFit {
    const float *view, *r, *u;
    float nearest, farthest, lo[2], hi[2];
    int points;
};

static void view_slice(struct ViewSlice *s, const float *view, const float *eye, const float (*ray)[3], float d0,
                       float d1)
{
    static const unsigned char sides[4][2] = {{0, 1}, {2, 3}, {0, 2}, {1, 3}};
    float middle[3];
    int i, k;

    for (k = 0; k < 8; k++)
        for (i = 0; i < 3; i++)
            s->corner[k][i] = eye[i] + ray[k & 3][i] * (k < 4 ? d0 : d1);
    for (i = 0; i < 3; i++) {
        s->plane[0][i] = view[2 + 3 * i];
        s->plane[1][i] = -view[2 + 3 * i];
        middle[i] = ray[0][i] + ray[1][i] + ray[2][i] + ray[3][i];
    }
    s->plane[0][3] = view[11] - d0;
    s->plane[1][3] = d1 - view[11];
    /* the four sides through the eye, each turned towards the view's middle */
    for (k = 0; k < 4; k++) {
        const float *a = ray[sides[k][0]], *b = ray[sides[k][1]];
        float *n = s->plane[2 + k], sign;

        n[0] = a[1] * b[2] - a[2] * b[1];
        n[1] = a[2] * b[0] - a[0] * b[2];
        n[2] = a[0] * b[1] - a[1] * b[0];
        sign = dot3(n, middle) < 0.0f ? -1.0f : 1.0f;
        for (i = 0; i < 3; i++)
            n[i] *= sign;
        n[3] = -dot3(n, eye);
    }
}

static void fit_reset(struct ShadowFit *fit)
{
    fit->nearest = fit->lo[0] = fit->lo[1] = 1e30f;
    fit->farthest = fit->hi[0] = fit->hi[1] = -1e30f;
    fit->points = 0;
}

/* the points from centre - half to centre + half (half 0 for a single point): the depths and the
   light's axes are linear, so their extremes over a box lie half's reach either side of its centre */
static void fit_extent(struct ShadowFit *fit, const float *centre, const float *half)
{
    const float *m = fit->view, *r = fit->r, *u = fit->u;
    float depth = m[2] * centre[0] + m[5] * centre[1] + m[8] * centre[2] + m[11], x = dot3(centre, r), y = dot3(centre, u);
    float dd = fabsf(m[2]) * half[0] + fabsf(m[5]) * half[1] + fabsf(m[8]) * half[2];
    float dx = fabsf(r[0]) * half[0] + fabsf(r[1]) * half[1] + fabsf(r[2]) * half[2];
    float dy = fabsf(u[0]) * half[0] + fabsf(u[1]) * half[1] + fabsf(u[2]) * half[2];

    fit->nearest = depth - dd < fit->nearest ? depth - dd : fit->nearest;
    fit->farthest = depth + dd > fit->farthest ? depth + dd : fit->farthest;
    fit->lo[0] = x - dx < fit->lo[0] ? x - dx : fit->lo[0];
    fit->hi[0] = x + dx > fit->hi[0] ? x + dx : fit->hi[0];
    fit->lo[1] = y - dy < fit->lo[1] ? y - dy : fit->lo[1];
    fit->hi[1] = y + dy > fit->hi[1] ? y + dy : fit->hi[1];
    fit->points++;
}

static void fit_point(struct ShadowFit *fit, const float *p)
{
    static const float none[3] = {0.0f, 0.0f, 0.0f};

    fit_extent(fit, p, none);
}

/* the ends of the part of the segment from a to b inside all six planes */
static void fit_segment(struct ShadowFit *fit, const float *a, const float *b, const float (*plane)[4])
{
    float t0 = 0.0f, t1 = 1.0f, p[3];
    int i, k;

    for (k = 0; k < 6; k++) {
        float fa = dot3(plane[k], a) + plane[k][3], fb = dot3(plane[k], b) + plane[k][3];

        if (fa < 0.0f && fb < 0.0f)
            return;
        if (fa < 0.0f)
            t0 = fa / (fa - fb) > t0 ? fa / (fa - fb) : t0;
        else if (fb < 0.0f)
            t1 = fa / (fa - fb) < t1 ? fa / (fa - fb) : t1;
    }
    if (t0 > t1)
        return;
    for (k = 0; k < 2; k++) {
        for (i = 0; i < 3; i++)
            p[i] = a[i] + (b[i] - a[i]) * (k ? t1 : t0);
        fit_point(fit, p);
    }
}

/* a box's centre and half its size, or 0 where it is empty or not finite */
static int box_extent(const struct ShadowBox *box, float *centre, float *half)
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

/* the corners of the part of a box inside the slice. Both are convex, so they are the box's
   corners inside the slice, the slice's inside the box, and the points where the edges of either
   cross the faces of the other: the ends of each edge's part inside the other. The slice's edges
   lie in its four sides, so a box within those adds nothing through them. */
static void fit_box(struct ShadowFit *fit, const struct ViewSlice *s, const struct ShadowBox *box)
{
    float plane[6][4] = {{0}}, corner[8][3], centre[3], half[3];
    int i, k, inside = 1, within = 1;

    if (!box_extent(box, centre, half))
        return;
    for (k = 0; k < 6; k++) {
        const float *n = s->plane[k];
        float mid = dot3(n, centre) + n[3], reach = fabsf(n[0]) * half[0] + fabsf(n[1]) * half[1] + fabsf(n[2]) * half[2];

        if (mid + reach < 0.0f)
            return;
        if (mid - reach < 0.0f) {
            inside = 0;
            within = within && k < 2;
        }
    }
    if (inside) {
        fit_extent(fit, centre, half);
        return;
    }
    for (k = 0; k < 8; k++)
        for (i = 0; i < 3; i++)
            corner[k][i] = k & (1 << i) ? box->max[i] : box->min[i];
    for (i = 0; i < 3; i++) {
        plane[2 * i][i] = 1.0f;
        plane[2 * i][3] = -box->min[i];
        plane[2 * i + 1][i] = -1.0f;
        plane[2 * i + 1][3] = box->max[i];
    }
    for (k = 0; k < 8; k++)
        for (i = 1; i < 8; i <<= 1)
            if (!(k & i)) {
                fit_segment(fit, corner[k], corner[k | i], s->plane);
                if (!within)
                    fit_segment(fit, s->corner[k], s->corner[k | i], plane);
            }
}

void renderer_shadow_frame(struct Renderer *self, const float *view, float distance, const struct ShadowBox *boxes,
                           size_t count)
{
    struct GfxShadow *shadow = &self->shadow;
    struct ShadowFit fit;
    struct ViewSlice slice;
    float inv[12], f[3], up[3] = {0, 0, 0}, r[3], u[3], eye[3], ray[4][3], centre[3], half[3];
    float len, nearest, farthest, maxDepth, lightNear = 1e30f, lightFar = -1e30f, range;
    float split[GFX_SHADOW_CASCADES + 1];
    size_t b;
    int i, j, k, c;

    self->shadowPending = 0;
    /* the shadow maps take the place of the lightmaps' shadows: without lightmaps the ground has
       none, as in the original, and nothing casts */
    if (!g_lightmap || !self->lightSet || !(self->shadowDarkness < 1.0f) || !gfx_shadows_supported()
        || !invert_4x3(view, inv))
        return;
    len = sqrtf(dot3(self->lightDir, self->lightDir));
    if (!(len > 0.0f))
        return;
    /* the map's light vector points the way the light travels: Prontera's is (0.5, 0.707, 0.5),
       down (+y) onto the map; taken the other way, the shadow map looked up from under it. The
       shadows then fall where the map's editor baked them into the lightmaps: on the ground of
       yuno_fild01's south gate and two Prontera scenes, the baked and the drawn shadows overlap
       0.49, 0.72 and 0.37 (shared over either), against 0.09 to 0.35 with x, z or both mirrored. */
    for (i = 0; i < 3; i++)
        f[i] = self->lightDir[i] / len;
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
        return;
    lightNear -= SHADOW_MARGIN;
    range = lightFar + SHADOW_MARGIN - lightNear;

    /* the view's four corner rays in the world, per unit of view depth, and its depths in the boxes */
    maxDepth = distance * 4.0f + 400.0f;
    maxDepth = maxDepth < RENDER_FAR_Z ? maxDepth : RENDER_FAR_Z;
    for (i = 0; i < 3; i++)
        eye[i] = inv[9 + i];
    for (k = 0; k < 4; k++) {
        float dx = ((k & 1 ? (float)self->width : 0.0f) - (float)self->centreX) / self->proj[0];
        float dy = ((k & 2 ? (float)self->height : 0.0f) - (float)self->centreY) / self->proj[1];

        for (i = 0; i < 3; i++)
            ray[k][i] = dx * inv[i] + dy * inv[3 + i] + inv[6 + i];
    }
    fit.view = view;
    fit.r = r;
    fit.u = u;
    fit_reset(&fit);
    view_slice(&slice, view, eye, ray, RENDER_NEAR_Z, maxDepth);
    for (b = 0; b < count; b++)
        fit_box(&fit, &slice, &boxes[b]);
    if (!fit.points)
        return;
    nearest = fit.nearest > RENDER_NEAR_Z ? fit.nearest : RENDER_NEAR_Z;
    farthest = fit.farthest < maxDepth ? fit.farthest : maxDepth;
    /* a unit beyond: the shaders look a pixel up in the first cascade whose far depth lies beyond
       it, so that the farthest point itself has one */
    farthest = (farthest > nearest ? farthest : nearest) + 1.0f;
    for (c = 0; c <= GFX_SHADOW_CASCADES; c++) {
        float t = (float)c / GFX_SHADOW_CASCADES;
        split[c] = SHADOW_SPLIT * nearest * powf(farthest / nearest, t) + (1.0f - SHADOW_SPLIT) * (nearest + (farthest - nearest) * t);
    }
    split[0] = nearest;
    split[GFX_SHADOW_CASCADES] = farthest;

    shadow->cascades = GFX_SHADOW_CASCADES;
    for (c = 0; c < GFX_SHADOW_CASCADES; c++) {
        float square[2] = {0.0f, 0.0f}, radius = 8.0f, texel, w[4][4];

        fit_reset(&fit);
        view_slice(&slice, view, eye, ray, split[c], split[c + 1]);
        for (b = 0; b < count; b++)
            fit_box(&fit, &slice, &boxes[b]);
        /* a margin for the soft edge and the offset, then up to a multiple of 8 units. A slice with
           nothing in it (between the top of a cliff and its foot) takes no casters. */
        if (fit.points) {
            square[0] = (fit.lo[0] + fit.hi[0]) * 0.5f;
            square[1] = (fit.lo[1] + fit.hi[1]) * 0.5f;
            radius = (fit.hi[0] - fit.lo[0] > fit.hi[1] - fit.lo[1] ? fit.hi[0] - fit.lo[0] : fit.hi[1] - fit.lo[1]) * 0.5f;
            radius = ceilf((radius * 1.05f + 4.0f + SHADOW_SOFTNESS) / 8.0f) * 8.0f;
        }
        texel = 2.0f * radius / GFX_SHADOW_SIZE;
        square[0] = floorf(square[0] / texel + 0.5f) * texel;
        square[1] = floorf(square[1] / texel + 0.5f) * texel;
        for (i = 0; i < 2; i++) {
            self->shadowReach.lo[c][i] = fit.points ? square[i] - radius : 1e30f;
            self->shadowReach.hi[c][i] = fit.points ? square[i] + radius : -1e30f;
        }
        for (i = 0; i < 3; i++) {
            w[i][0] = r[i] / radius;
            w[i][1] = u[i] / radius;
            w[i][2] = f[i] / range;
            w[i][3] = 0.0f;
        }
        w[3][0] = -square[0] / radius;
        w[3][1] = -square[1] / radius;
        w[3][2] = -lightNear / range;
        w[3][3] = 1.0f;
        /* view to world (the inverse view, a 4x4 with the last column 0 0 0 1), then to light */
        for (i = 0; i < 4; i++)
            for (j = 0; j < 4; j++) {
                float sum = i == 3 ? w[3][j] : 0.0f;
                for (k = 0; k < 3; k++)
                    sum += inv[i * 3 + k] * w[k][j];
                shadow->fromView[c][i * 4 + j] = sum;
            }
        shadow->farDepth[c] = split[c + 1];
        shadow->offset[c] = SHADOW_OFFSET * texel;
        shadow->slant[c] = (SHADOW_SOFTNESS + texel) / range;
        shadow->blur[c] = SHADOW_SOFTNESS / (2.0f * radius);
    }
    self->shadowReach.depth[0] = lightNear;
    self->shadowReach.depth[1] = lightNear + range;
    for (i = 0; i < 3; i++) {
        self->shadowReach.axis[0][i] = r[i];
        self->shadowReach.axis[1][i] = u[i];
        self->shadowReach.axis[2][i] = f[i];
    }
    /* towards the light in view space */
    for (i = 0; i < 3; i++)
        shadow->lightView[i] = -(f[0] * view[i] + f[1] * view[3 + i] + f[2] * view[6 + i]);
    len = sqrtf(dot3(shadow->lightView, shadow->lightView));
    if (len > 0.0f)
        for (i = 0; i < 3; i++)
            shadow->lightView[i] /= len;
    shadow->bias = SHADOW_BIAS / range;
    shadow->darkness = self->shadowDarkness;
    for (i = 0; i < 3; i++)
        shadow->ambient[i] = self->lightAmbient[i] < 0.0f ? 0.0f : self->lightAmbient[i] > 1.0f ? 1.0f
                                                                                             : self->lightAmbient[i];
    self->shadowPending = 1;
}

void renderer_set_camera(struct Renderer *self, const float *view)
{
    memcpy(self->cameraView, view, sizeof self->cameraView);
    self->cameraSet = 1;
}

/* the box's extent along each of the light's axes against a cascade's square and the depth kept:
   a caster casts into a cascade wherever it lies along the light, the map being orthographic */
int renderer_shadow_reaches(const struct Renderer *self, const float *centre, const float *half, const float (*axes)[3])
{
    static const float world[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    const struct ShadowReach *reach = &self->shadowReach;
    float mid[3], ext[3];
    int a, b, c;

    if (!self->shadowPending)
        return 0;
    if (!axes)
        axes = world;
    for (a = 0; a < 3; a++) {
        mid[a] = dot3(centre, reach->axis[a]);
        ext[a] = 0.0f;
        for (b = 0; b < 3; b++)
            ext[a] += half[b] * fabsf(dot3(axes[b], reach->axis[a]));
    }
    if (mid[2] + ext[2] < reach->depth[0] || mid[2] - ext[2] > reach->depth[1])
        return 0;
    for (c = 0; c < self->shadow.cascades; c++)
        if (mid[0] + ext[0] >= reach->lo[c][0] && mid[0] - ext[0] <= reach->hi[c][0]
            && mid[1] + ext[1] >= reach->lo[c][1] && mid[1] - ext[1] <= reach->hi[c][1])
            return 1;
    return 0;
}

float renderer_project_depth(float rhw)
{
    return (1.0f / rhw - RENDER_NEAR_Z) * RENDER_DEPTH_SCALE * rhw;
}

static float view_z(const Vec3 *v, const float *m)
{
    return m[2] * v->x + m[5] * v->y + m[8] * v->z + m[11];
}

static float depth_rhw(const Vec3 *v, const float *m)
{
    return 1.0f / view_z(v, m);
}

static float view_x(const Vec3 *v, const float *m)
{
    return m[3] * v->y + v->x * m[0] + m[6] * v->z + m[9];
}

static float view_y(const Vec3 *v, const float *m)
{
    return m[4] * v->y + m[1] * v->x + m[7] * v->z + m[10];
}

void renderer_project_point(struct Renderer *self, const Vec3 *v, const float *m,
                                        float *sx, float *sy, float *rhw)
{
    float r;

    r = depth_rhw(v, m);
    *rhw = r;
    *sx = view_x(v, m) * r * self->proj[0] + (float)self->halfW;
    *sy = view_y(v, m) * *rhw * self->proj[1] + (float)self->halfH;
}

void renderer_project_centred(struct Renderer *self, const Vec3 *v, const float *m,
                                        float *sx, float *sy, float *rhw, float width, float height)
{
    float r;

    r = depth_rhw(v, m);
    *rhw = r;
    *sx = view_x(v, m) * r * self->proj[0] + width * 0.5f;
    *sy = view_y(v, m) * *rhw * self->proj[1] + height * 0.5f;
}

struct ScreenPoint { float x, y, z, rhw; };

static struct ScreenPoint project_vertex(const struct Renderer *self, const Vec3 *v, const float *m)
{
    float r = depth_rhw(v, m);
    float x, y;
    struct ScreenPoint out;

    out.rhw = r;
    x = view_x(v, m) * r * self->proj[0] + (float)self->centreX;
    out.x = x;
    y = view_y(v, m) * r * self->proj[1] + (float)self->centreY;
    out.z = (1.0f / r - RENDER_NEAR_Z) * RENDER_DEPTH_SCALE * r;
    out.y = y;
    return out;
}

void renderer_project_vertex(struct Renderer *self, const Vec3 *v, const float *m,
                              struct TexDrawVertex *out)
{
    struct ScreenPoint point = project_vertex(self, v, m);
    out->rhw = point.rhw;
    out->x = point.x;
    out->z = point.z;
    out->y = point.y;
}

float renderer_project_rhw(const Vec3 *v, const float *m)
{
    return depth_rhw(v, m);
}

void renderer_project_view(const Vec3 *v, const float *m, float *out)
{
    out[0] = view_x(v, m);
    out[1] = view_y(v, m);
    out[2] = view_z(v, m);
}

void renderer_cursor_ray(struct Renderer *self, int x, int y, const Vec3 *eye, const float *m,
                                    struct Ray3 *out)
{
    float nx = (float)x / (float)self->centreX - 1.0f;
    float ny = 1.0f - (float)y / (float)self->centreY;
    float dx = (float)self->centreX * nx / self->proj[0];
    float dy = -((float)self->centreY * ny / self->proj[1]);
    float inv;

    out->origin.x = eye->x;
    out->origin.y = eye->y;
    out->origin.z = eye->z;
    out->dir.x = dy * m[3] + dx * m[0] + m[6] + 0.0f;
    out->dir.y = dy * m[4] + dx * m[1] + m[7] + 0.0f;
    out->dir.z = dy * m[5] + dx * m[2] + m[8] + 0.0f;
    inv = (float)(1.0 / (double)float_sqrt(out->dir.y * out->dir.y + out->dir.x * out->dir.x + out->dir.z * out->dir.z));
    out->dir.x *= inv;
    out->dir.y *= inv;
    out->dir.z *= inv;
    out->length = 0.0f;
}
