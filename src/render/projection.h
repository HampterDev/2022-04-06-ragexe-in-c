#ifndef RENDER_PROJECTION_H
#define RENDER_PROJECTION_H

#include "render/math3d.h"

struct Renderer;
struct TexDrawVertex;
struct RPTwoTexVertex;

void renderer_project_point(struct Renderer *renderer, const Vec3 *point,
    const float *view, float *x, float *y, float *rhw);
void renderer_project_centred(struct Renderer *renderer, const Vec3 *point,
    const float *view, float *x, float *y, float *rhw, float width, float height);

void renderer_project_vertex(struct Renderer *renderer, const Vec3 *point,
    const float *view, struct TexDrawVertex *vertex);
void renderer_project_multitex_vertex(struct Renderer *renderer, const Vec3 *point,
    const float *view, struct RPTwoTexVertex *vertex);

float renderer_project_depth(float rhw);
/* custom: the projection of renderer_project_vertex and the vertex fog of rp_vertex_fog_alpha for
   the view matrix `view`, for static meshes the device projects (render/gfx.h) */
struct GfxWorldView;
void renderer_world_view(const struct Renderer *renderer, const float *view, struct GfxWorldView *out);
float renderer_screen_scale(struct Renderer *renderer, float rhw);
float renderer_scale_x(struct Renderer *renderer);

void renderer_cursor_ray(struct Renderer *renderer, int x, int y, const Vec3 *eye,
    const float *inverse_view, struct Ray3 *ray);

#endif
