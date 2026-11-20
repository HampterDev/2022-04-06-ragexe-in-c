#include "render/projection.h"
#include "render/quads.h"
#include "render/rp_pool.h"
#include "render/texture_cache.h"
#include "actors/weather.h"
#include "world/ground.h"
#include "world/camera.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "render/render_api.h"
#include "render/render_options.h"
#include "actors/attr_state.h"

/* custom: the ground's water as the static world's (render/gfx.h, gfx_world_water): the original
   drew a quad over each cell of the frame whose ground the wave stood below, its corners raised by
   the wave as it stood at the cell's diagonal and the ones on either side, projected on the CPU
   each frame. The quads are now built on the GPU from the cells placed once (water.hlsl), the same
   heights and the same tests, and the frame queues each cell it shows where the original queued
   its quad, keyed as that was by the rhw of its first corner, so that it is drawn among the
   sprites and the alpha-blended models in the same order. A cell whose quad the GPU then leaves
   out draws nothing there. */

/* the colour of the frame's water quads */
static DWORD water_colour(const struct C3dGround15 *self)
{
    DWORD c;

    if (g_playerBlind != 0 || self->waterType != 4)
        return 0x90ffffff;
    c = (BYTE)(int)(g_weather.tint[0] * 255.0f);
    c = (c + 0xff00) << 8;
    c += (BYTE)(int)(g_weather.tint[1] * 255.0f);
    c <<= 8;
    c += (BYTE)(int)(g_weather.tint[2] * 255.0f);
    return c;
}

/* Queues a cell's water where the original queued its quad, among the alpha-blended quads (the
   flags 1 of renderer_add_prim), keyed by the rhw of its first corner as the wave raises it. */
static void queue_water(struct C3dGround15 *self, int x, int y, struct GndCell *cell, const float *view, DWORD colour)
{
    const struct GndWater *w = &self->waters[c3d_ground15_water_block(self, x, y)];
    struct RPWorldDraw draw;
    float first[3];
    Vec3 p;

    c3d_ground15_corner(self, x, y, ground_water_height(w, (int64_t)x + y - 1), first);
    p.x = first[0];
    p.y = first[1];
    p.z = first[2];
    memset(&draw, 0, sizeof draw);
    draw.kind = RP_WORLD_WATER;
    draw.cell = (uint32_t)cell->water;
    draw.colour = colour;
    renderer_add_world_draw(g_renderer, RENDER_ALPHA_CLAMP, &draw, w->texture, renderer_project_rhw(&p, view));
}

void c3d_ground15_leaf(struct C3dGround15 *self, const float *view, const int *cells)
{
    DWORD colour;
    int x, y;

    if (!self->waters)
        return;
    colour = water_colour(self);
    for (y = cells[1]; y <= cells[3]; y++)
        for (x = cells[0]; x <= cells[2]; x++) {
            struct GndCell *cell = GND_CELL(self, x, y);

            if (cell->lastFrame == g_renderer->frame)
                continue;
            cell->lastFrame = g_renderer->frame;
            if (cell->water >= 0)
                queue_water(self, x, y, cell, view, colour);
        }
}

/* custom: a quad over the ground's cells, its corners in the world (x, y and z of each in `quad`),
   as an instance whose corners the device projects through the view as renderer_project_vertex
   projected them (render/gfx.h, GFX_QUAD_WORLD), with the texture coordinates of each corner in
   uv, in `colour`, blended (5, 6) among the alpha-blended quads (the flags 1 of renderer_add_prim)
   and keyed by the rhw of its corners, as its vertices were. The CPU finds the rhw, for the key and
   whether the quad lies flat on the screen, and the z of each corner less `bias`, which the depth
   test compares with the ground by its bits (GFX_DEPTH_CORNERS). */
static void queue_cell_quad(const float *quad, const float *view, DWORD colour, struct Texture *texture,
                            const float *uv, float bias)
{
    struct TexDrawPrim *prim = renderer_new_prim_instance(g_renderer);
    struct GfxQuad *q = &prim->instance;
    float rhw[4];
    int i;

    q->kind = GFX_QUAD_WORLD;
    for (i = 0; i < 4; i++) {
        Vec3 p;

        p.x = quad[i * 3];
        p.y = quad[i * 3 + 1];
        p.z = quad[i * 3 + 2];
        memcpy(q->world.point[i], &quad[i * 3], sizeof q->world.point[i]);
        rhw[i] = renderer_project_rhw(&p, view);
        q->world.u[i] = uv[i * 2];
        q->world.v[i] = uv[i * 2 + 1];
    }
    memcpy(q->world.view, view, sizeof q->world.view);
    q->world.proj[0] = g_renderer->proj[0];
    q->world.proj[1] = g_renderer->proj[1];
    q->world.centre[0] = (float)g_renderer->centreX;
    q->world.centre[1] = (float)g_renderer->centreY;
    q->world.rhw = GFX_RHW_PROJECTED;
    q->world.depth = GFX_DEPTH_CORNERS;
    q->world.depthValue = 0.0f;
    for (i = 0; i < 4; i++)
        q->world.z[i] = bias != 0.0f ? renderer_project_depth(rhw[i]) + -bias : renderer_project_depth(rhw[i]);
    q->world.colour = colour;
    q->world.flat = quad_rhw_flat(rhw, 4);
    prim->base.srcBlend = 5;
    prim->base.destBlend = 6;
    prim->base.texture = texture;
    prim->base.rhw[0] = rhw[0];
    prim->base.rhw[1] = rhw[1];
    renderer_add_prim(g_renderer, &prim->base, 1);
}

static void attr_quad(struct C3dGround15 *self, const float *view, int ax, int ay, DWORD colour,
                      struct Texture *texture, const float *uv, float bias)
{
    float quad[12] = {0};
    float su, sv, scaled[8];
    int i;

    if (!c3d_ground15_get_attr_quad(self, ax, ay, quad))
        return;

    for (i = 0; i < 4; i++) {
        float *p = &quad[i * 3];
        float x = p[0], z;

        if (ax & 1) {
            if (quad[3] > x)
                p[0] = (quad[3] - x) * 0.5f + x;
        } else {
            if (x > quad[0])
                p[0] = x - (x - quad[0]) * 0.5f;
        }
        z = p[2];
        if (ay & 1) {
            if (quad[8] > z)
                p[2] = (quad[8] - z) * 0.5f + z;
        } else {
            if (z > quad[2])
                p[2] = z - (z - quad[2]) * 0.5f;
        }
    }

    su = (float)(DWORD)texture->reqWidth / (float)(DWORD)texture->base.width;
    sv = (float)(DWORD)texture->reqHeight / (float)(DWORD)texture->base.height;
    for (i = 0; i < 4; i++) {
        scaled[i * 2] = uv[i * 2] * su;
        scaled[i * 2 + 1] = uv[i * 2 + 1] * sv;
    }
    queue_cell_quad(quad, view, colour, texture, scaled, bias);
}

void c3d_ground15_draw_attr_quad(struct C3dGround15 *self, const float *view, int ax,
                                         int ay, DWORD colour, struct Texture *texture, const float *uv)
{
    attr_quad(self, view, ax, ay, colour, texture, uv, 3.0517578e-5f);
}

void c3d_ground15_draw_attr_quad_raised(struct C3dGround15 *self, const float *view,
                                               int ax, int ay, DWORD colour, struct Texture *texture,
                                               const float *uv)
{
    attr_quad(self, view, ax, ay, colour, texture, uv, 0.10003052f);
}

/* the navigation marker's texture coordinates, corner by corner */
static const float k_marker_uv[8] = {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};

void c3d_ground15_draw_attr_marker(struct C3dGround15 *self, const float *view,
                                           int ax, int ay, DWORD colour, struct Texture *texture)
{
    int cx = (int)((float)ax * 0.5f);
    int cy = (int)((float)ay * 0.5f);
    int ayOdd = ay % 2;
    int width = self->width;
    int halfW = (int)((float)width * 0.5f);
    int halfH = (int)((float)self->height * 0.5f);
    float half = self->zoom * 0.5f;
    float quarter = half * 0.5f;
    const struct AttrCell *c;
    float quad[12], offX, offZ, x0, z0, left, right, front, back;

    if (ax < 0 || cx >= width || ay < 0 || cy >= self->height)
        return;
    c = c3d_attr_get_cell(self->attr, ax, ay);
    if (!c)
        return;
    offX = ax % 2 == 1 ? half : 0.0f;
    offZ = 0.0f;
    x0 = (float)(cx - halfW) * self->zoom + offX;
    if (ayOdd == 1)
        offZ = half;
    z0 = (float)(cy - halfH) * self->zoom + offZ;
    left = x0 - quarter;
    right = x0 + half + quarter;
    front = z0 - quarter;
    back = z0 + half + quarter;
    quad[0] = left;
    quad[1] = c->height[0];
    quad[2] = front;
    quad[3] = right;
    quad[4] = c->height[1];
    quad[5] = front;
    quad[6] = left;
    quad[7] = c->height[2];
    quad[8] = back;
    quad[9] = right;
    quad[10] = c->height[3];
    quad[11] = back;

    queue_cell_quad(quad, view, colour, texture, k_marker_uv, 0.0f);
}

void c3d_ground15_render_attr_tile(struct C3dGround15 *self, const float *view,
                                           int ax, int ay, DWORD colour)
{
    struct Texture *grid = texture_cache_get(&g_textureCache, "grid.tga", 0, 0);

    c3d_ground15_draw_attr_quad(self, view, ax, ay, colour, grid, g_naviCellUVs);
}
