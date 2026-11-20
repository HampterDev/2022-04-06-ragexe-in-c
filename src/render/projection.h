#ifndef RENDER_PROJECTION_H
#define RENDER_PROJECTION_H

#include "render/math3d.h"

struct Renderer;
struct TexDrawVertex;

void renderer_project_point(struct Renderer *renderer, const Vec3 *point,
    const float *view, float *x, float *y, float *rhw);
void renderer_project_centred(struct Renderer *renderer, const Vec3 *point,
    const float *view, float *x, float *y, float *rhw, float width, float height);

void renderer_project_vertex(struct Renderer *renderer, const Vec3 *point,
    const float *view, struct TexDrawVertex *vertex);
/* custom: the rhw renderer_project_vertex gives a point, to the bit, and the point in view space as
   it computes it on the way (x, y, depth), whose 1 / depth is that rhw */
float renderer_project_rhw(const Vec3 *point, const float *view);
void renderer_project_view(const Vec3 *point, const float *view, float *out);

float renderer_project_depth(float rhw);
/* custom: the projection of renderer_project_vertex for the view matrix `view`, for what the device
   projects itself (render/gfx.h) */
struct GfxWorldView;
void renderer_world_view(const struct Renderer *renderer, const float *view, struct GfxWorldView *out);
float renderer_screen_scale(struct Renderer *renderer, float rhw);
float renderer_scale_x(struct Renderer *renderer);

void renderer_cursor_ray(struct Renderer *renderer, int x, int y, const Vec3 *eye,
    const float *inverse_view, struct Ray3 *ray);

#endif
