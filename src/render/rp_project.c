#include "render/render_api.h"
#include "render/renderer.h"
#include "render/math3d.h"
#include "render/rp_pool.h"
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

void renderer_world_view(const struct Renderer *self, const float *view, const float *scale,
                         struct GfxWorldView *out)
{
    float range = g_fogFar - g_fogNear;
    int i;

    memcpy(out->view, view, sizeof out->view);
    for (i = 0; i < 3; i++)
        out->colourScale[i] = scale ? scale[i] : 1.0f;
    out->proj[0] = self->proj[0];
    out->proj[1] = self->proj[1];
    out->centre[0] = (float)self->centreX;
    out->centre[1] = (float)self->centreY;
    out->nearZ = RENDER_NEAR_Z;
    out->depthScale = RENDER_DEPTH_SCALE;
    /* rp_vertex_fog_alpha: 1 - ((vz - 10) / 1490 - near) / (far - near), or without the second
       step when far equals near */
    if (range != 0.0f) {
        out->fogStart = g_fogNear * 1490.0f + 10.0f;
        out->fogEnd = g_fogFar * 1490.0f + 10.0f;
    } else {
        out->fogStart = 10.0f;
        out->fogEnd = 1500.0f;
    }
}

/* custom: the cascades of the models' shadow maps (render/gfx.h). The models that receive shadow
   lie in a band from SHADOW_ABOVE units above the camera's focus to SHADOW_BELOW below it (+y is
   down), so the view is cut into GFX_SHADOW_CASCADES slices of view depth between where its four
   corner rays enter and leave that band, no further than four times the camera's distance. The
   slices grow with the depth (SHADOW_SPLIT between even and geometric steps). Each cascade's map
   covers its slice's part of the band as the light sees it: the light map is orthographic, so a
   caster that shades something in the slice lands on the same place of the map, however far off
   screen it stands; SHADOW_DEPTH either way along the light is kept. A cascade's square is rounded
   up and its centre kept to whole texels so that its texels stay put while the camera moves and
   shadow edges do not crawl. */
#define SHADOW_ABOVE 300.0f
#define SHADOW_BELOW 60.0f
#define SHADOW_DEPTH 3000.0f
#define SHADOW_SPLIT 0.7f
/* a pixel stays lit up to SHADOW_BIAS units behind the nearest caster, and is looked up from a
   point moved SHADOW_OFFSET texels off its surface towards the light (times the sine of the angle
   between them), which keeps a surface at a slant to the light from shadowing itself. Models lay
   decorations a unit or two over their surfaces (Prontera's flower beds put the flowers over the
   grass); at 1 unit those shaded the surface under them. */
#define SHADOW_BIAS 4.0f
#define SHADOW_OFFSET 1.5f
/* shadow edges fade over twice SHADOW_SOFTNESS units, about as far as the one coarse shadow map
   first drawn here blurred them (0.8 units a texel at the default zoom); at 1.5 the edges spread
   over three units and the shadows of thin casters, such as a lamp post's, faded away */
#define SHADOW_SOFTNESS 0.5f
/* how dark a shadow is: the lightmaps the map's editor baked hold 128 in shadow and 255 in the
   sun (the GNDs of yuno_fild01, prontera and iz_int03: some 45 percent of texels each, the rest
   on the edges between), and the ground draws its colour times that intensity */
#define SHADOW_DARKNESS (128.0f / 255.0f)

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

void renderer_shadow_frame(struct Renderer *self, const float *view, const float *focus, float distance)
{
    struct GfxShadow *shadow = &self->shadow;
    float inv[12], f[3], up[3] = {0, 0, 0}, r[3], u[3], eye[3], ray[4][3], enter[4], leave[4];
    float top = focus[1] - SHADOW_ABOVE, bottom = focus[1] + SHADOW_BELOW, len, nearest, farthest, maxDepth;
    float split[GFX_SHADOW_CASCADES + 1];
    int i, j, k, c, any = 0;

    self->shadowPending = 0;
    if (!self->lightSet || !invert_4x3(view, inv))
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

    /* the view's four corner rays in the world, per unit of view depth, and where they cross the band */
    maxDepth = distance * 4.0f + 400.0f;
    maxDepth = maxDepth < 8000.0f ? maxDepth : 8000.0f;
    for (i = 0; i < 3; i++)
        eye[i] = inv[9 + i];
    nearest = maxDepth;
    farthest = RENDER_NEAR_Z;
    for (k = 0; k < 4; k++) {
        float dx = ((k & 1 ? (float)self->width : 0.0f) - (float)self->centreX) / self->proj[0];
        float dy = ((k & 2 ? (float)self->height : 0.0f) - (float)self->centreY) / self->proj[1];

        for (i = 0; i < 3; i++)
            ray[k][i] = dx * inv[i] + dy * inv[3 + i] + inv[6 + i];
        enter[k] = RENDER_NEAR_Z;
        leave[k] = maxDepth;
        if (ray[k][1] != 0.0f) {
            float a = (top - eye[1]) / ray[k][1], b = (bottom - eye[1]) / ray[k][1];
            if ((a < b ? a : b) > enter[k])
                enter[k] = a < b ? a : b;
            if ((a > b ? a : b) < leave[k])
                leave[k] = a > b ? a : b;
        } else if (eye[1] < top || eye[1] > bottom) {
            leave[k] = enter[k];
        }
        if (enter[k] < leave[k]) {
            nearest = enter[k] < nearest ? enter[k] : nearest;
            farthest = leave[k] > farthest ? leave[k] : farthest;
            any = 1;
        }
    }
    if (!any)
        return;
    for (c = 0; c <= GFX_SHADOW_CASCADES; c++) {
        float t = (float)c / GFX_SHADOW_CASCADES;
        split[c] = SHADOW_SPLIT * nearest * powf(farthest / nearest, t) + (1.0f - SHADOW_SPLIT) * (nearest + (farthest - nearest) * t);
    }
    split[0] = nearest;
    split[GFX_SHADOW_CASCADES] = farthest;

    shadow->cascades = GFX_SHADOW_CASCADES;
    for (c = 0; c < GFX_SHADOW_CASCADES; c++) {
        float lo[2] = {1e30f, 1e30f}, hi[2] = {-1e30f, -1e30f}, centre[2], radius = 0.0f, texel, depth, w[4][4];
        int points = 0;

        /* the slice's part of each corner ray inside the band, or the whole slice where none is */
        for (j = 0; j < 2; j++)
            for (k = 0; k < 4; k++) {
                float a = split[c] > enter[k] ? split[c] : enter[k];
                float b = split[c + 1] < leave[k] ? split[c + 1] : leave[k];
                float t = j ? b : a, p[3], x, y;

                if (a > b)
                    continue;
                for (i = 0; i < 3; i++)
                    p[i] = eye[i] + ray[k][i] * t;
                x = dot3(p, r);
                y = dot3(p, u);
                lo[0] = x < lo[0] ? x : lo[0];
                hi[0] = x > hi[0] ? x : hi[0];
                lo[1] = y < lo[1] ? y : lo[1];
                hi[1] = y > hi[1] ? y : hi[1];
                points++;
            }
        if (!points)
            for (j = 0; j < 2; j++)
                for (k = 0; k < 4; k++) {
                    float p[3], x, y;
                    for (i = 0; i < 3; i++)
                        p[i] = eye[i] + ray[k][i] * split[c + j];
                    x = dot3(p, r);
                    y = dot3(p, u);
                    lo[0] = x < lo[0] ? x : lo[0];
                    hi[0] = x > hi[0] ? x : hi[0];
                    lo[1] = y < lo[1] ? y : lo[1];
                    hi[1] = y > hi[1] ? y : hi[1];
                }
        centre[0] = (lo[0] + hi[0]) * 0.5f;
        centre[1] = (lo[1] + hi[1]) * 0.5f;
        radius = (hi[0] - lo[0] > hi[1] - lo[1] ? hi[0] - lo[0] : hi[1] - lo[1]) * 0.5f;
        /* a margin for the soft edge and the offset, then up to a multiple of 8 units */
        radius = ceilf((radius * 1.05f + 4.0f + SHADOW_SOFTNESS) / 8.0f) * 8.0f;
        texel = 2.0f * radius / GFX_SHADOW_SIZE;
        centre[0] = floorf(centre[0] / texel + 0.5f) * texel;
        centre[1] = floorf(centre[1] / texel + 0.5f) * texel;
        depth = dot3(focus, f);
        for (i = 0; i < 3; i++) {
            w[i][0] = r[i] / radius;
            w[i][1] = u[i] / radius;
            w[i][2] = f[i] / (2.0f * SHADOW_DEPTH);
            w[i][3] = 0.0f;
        }
        w[3][0] = -centre[0] / radius;
        w[3][1] = -centre[1] / radius;
        w[3][2] = 0.5f - depth / (2.0f * SHADOW_DEPTH);
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
        shadow->blur[c] = SHADOW_SOFTNESS / (2.0f * radius);
    }
    /* towards the light in view space */
    for (i = 0; i < 3; i++)
        shadow->lightView[i] = -(f[0] * view[i] + f[1] * view[3 + i] + f[2] * view[6 + i]);
    len = sqrtf(dot3(shadow->lightView, shadow->lightView));
    if (len > 0.0f)
        for (i = 0; i < 3; i++)
            shadow->lightView[i] /= len;
    shadow->bias = SHADOW_BIAS / (2.0f * SHADOW_DEPTH);
    shadow->slant = SHADOW_SOFTNESS / (2.0f * SHADOW_DEPTH);
    shadow->darkness = SHADOW_DARKNESS;
    self->shadowPending = 1;
}

float renderer_project_depth(float rhw)
{
    return (1.0f / rhw - RENDER_NEAR_Z) * RENDER_DEPTH_SCALE * rhw;
}

static float depth_rhw(const Vec3 *v, const float *m)
{
    return 1.0f / (m[2] * v->x + m[5] * v->y + m[8] * v->z + m[11]);
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

void renderer_project_multitex_vertex(struct Renderer *self, const Vec3 *v,
                                      const float *m, struct RPTwoTexVertex *out)
{
    struct ScreenPoint point = project_vertex(self, v, m);
    out->rhw = point.rhw;
    out->x = point.x;
    out->z = point.z;
    out->y = point.y;
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
