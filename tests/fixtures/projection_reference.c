#include "render/renderer.h"
#include "render/render_api.h"
#include "render/primitive_types.h"
#include "projection_reference.h"

float reference_CRenderer_ScreenScaleAtRhw(struct Renderer *self, float rhw)
{
    return (float)((double)self->proj[0] * rhw / 7.0);
}

float reference_CRenderer_GetScaleX(struct Renderer *self)
{
    return self->scaleX;
}

/* The original maps depth over 10..1500 (a factor of 1.0067114); custom draws to RENDER_FAR_Z. */
float reference_Renderer_ProjectZ(float rhw)
{
    return (1.0f / rhw - RENDER_NEAR_Z) * RENDER_DEPTH_SCALE * rhw;
}

static float reference_depth_rhw(const float *v, const float *m)
{
    return 1.0f / (m[2] * v[0] + m[5] * v[1] + m[8] * v[2] + m[11]);
}

static float reference_view_x(const float *v, const float *m)
{
    return m[3] * v[1] + v[0] * m[0] + m[6] * v[2] + m[9];
}

static float reference_view_y(const float *v, const float *m)
{
    return m[4] * v[1] + m[1] * v[0] + m[7] * v[2] + m[10];
}

void reference_CRenderer_ProjectPoint(struct Renderer *self, const float *v, const float *m,
                                        float *sx, float *sy, float *rhw)
{
    float r;

    r = reference_depth_rhw(v, m);
    *rhw = r;
    *sx = reference_view_x(v, m) * r * self->proj[0] + (float)self->halfW;
    *sy = reference_view_y(v, m) * *rhw * self->proj[1] + (float)self->halfH;
}

void reference_Renderer_ProjectCentred(struct Renderer *self, const float *v, const float *m,
                                        float *sx, float *sy, float *rhw, float width, float height)
{
    float r;

    r = reference_depth_rhw(v, m);
    *rhw = r;
    *sx = reference_view_x(v, m) * r * self->proj[0] + width * 0.5f;
    *sy = reference_view_y(v, m) * *rhw * self->proj[1] + height * 0.5f;
}

static void reference_project_vertex(const struct Renderer *self, const float *v, const float *m, float *out)
{
    float r = reference_depth_rhw(v, m);
    float x, y;

    out[3] = r;
    x = reference_view_x(v, m) * r * self->proj[0] + (float)self->centreX;
    out[0] = x;
    y = reference_view_y(v, m) * r * self->proj[1] + (float)self->centreY;
    out[2] = (1.0f / r - RENDER_NEAR_Z) * RENDER_DEPTH_SCALE * r;
    out[1] = y;
}

void reference_CRenderer_ProjectVertex(struct Renderer *self, const float *v, const float *m,
                                         struct TexDrawVertex *out)
{
    reference_project_vertex(self, v, m, &out->x);
}

void reference_CRenderer_ProjectMultiTexVertex(struct Renderer *self, const float *v,
                                                 const float *m, float *out)
{
    reference_project_vertex(self, v, m, out);
}

void reference_CRenderer_CursorRay(struct Renderer *self, int x, int y, const float *eye, const float *m,
                                    float *out)
{
    float nx = (float)x / (float)self->centreX - 1.0f;
    float ny = 1.0f - (float)y / (float)self->centreY;
    float dx = (float)self->centreX * nx / self->proj[0];
    float dy = -((float)self->centreY * ny / self->proj[1]);
    float inv;

    out[0] = eye[0];
    out[1] = eye[1];
    out[2] = eye[2];
    out[3] = dy * m[3] + dx * m[0] + m[6] + 0.0f;
    out[4] = dy * m[4] + dx * m[1] + m[7] + 0.0f;
    out[5] = dy * m[5] + dx * m[2] + m[8] + 0.0f;
    inv = (float)(1.0 / (double)float_sqrt(out[4] * out[4] + out[3] * out[3] + out[5] * out[5]));
    out[3] *= inv;
    out[4] *= inv;
    out[5] *= inv;
}
