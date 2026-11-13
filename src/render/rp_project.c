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

/* custom: the frame's shadows (render/gfx.h): the constants the device fits the cascades to the
   frame's receivers with (render/shadow_fit.h), from the camera's view and the map's light; and
   where a caster can shade the view (renderer_shadow_reach). None without lightmaps (the shadow
   maps take the place of the lightmaps' shadows, and without them the original's ground has none),
   where the map's shadows take nothing away (its editor baked none), where the device draws no
   shadows, or where nothing can be fitted. The box everything that receives and casts lies in
   comes later in the frame (renderer_shadow_bounds), once the static world has posed its models. */
void renderer_shadow_frame(struct Renderer *self, const float *view, float distance)
{
    struct ShadowFitCamera camera;
    int i;

    self->shadowSet = 0;
    if (!g_lightmap || !self->lightSet || !(self->shadowDarkness < 1.0f) || !gfx_shadows_supported())
        return;
    memcpy(camera.view, view, sizeof camera.view);
    camera.proj[0] = self->proj[0];
    camera.proj[1] = self->proj[1];
    camera.centre[0] = (float)self->centreX;
    camera.centre[1] = (float)self->centreY;
    camera.width = self->width;
    camera.height = self->height;
    camera.nearZ = RENDER_NEAR_Z;
    camera.farZ = RENDER_FAR_Z;
    camera.distance = distance;
    if (!shadow_fit_frame(&self->shadow.fit, &camera, self->lightDir))
        return;
    shadow_fit_reach(&self->shadowReach, &self->shadow.fit);
    self->shadow.darkness = self->shadowDarkness;
    for (i = 0; i < 3; i++)
        self->shadow.ambient[i] = self->lightAmbient[i] < 0.0f ? 0.0f : self->lightAmbient[i] > 1.0f ? 1.0f
                                                                                                  : self->lightAmbient[i];
    self->shadowSet = 1;
}

void renderer_shadow_bounds(struct Renderer *self, const struct ShadowFitBox *bounds)
{
    if (self->shadowSet && !shadow_fit_bounds(&self->shadow.fit, bounds, 1))
        self->shadowSet = 0;
}

void renderer_set_camera(struct Renderer *self, const float *view)
{
    memcpy(self->cameraView, view, sizeof self->cameraView);
    self->cameraSet = 1;
}

int renderer_world_quads(const struct Renderer *self, const float *view)
{
    return self->cameraSet && !memcmp(self->cameraView, view, sizeof self->cameraView);
}

int renderer_shadow_reach(const struct Renderer *self, struct SwFrustum *out)
{
    struct SwCascade reach;

    if (!self->shadowSet)
        return 0;
    memcpy(reach.axis, self->shadow.fit.axis, sizeof reach.axis);
    memcpy(reach.lo, self->shadowReach.lo, sizeof reach.lo);
    memcpy(reach.hi, self->shadowReach.hi, sizeof reach.hi);
    /* a caster shades what lies beyond it along the light, so none beyond the view's farthest
       point along it shades the view; before it, the box everything casts from ends the maps */
    reach.depth[0] = -HUGE_VALF;
    reach.depth[1] = self->shadowReach.farthest;
    sw_cascade_planes(&reach, out);
    return 1;
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
