#include "render/rp_pool.h"
#include "render/texture_cache.h"
#include "actor/weather.h"
#include "world/ground.h"
#include "world/camera.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "render/render_api.h"
#include "render/render_options.h"
#include "actor/attr_state.h"

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
    struct TexDrawVertex v;
    float first[3];
    Vec3 p;

    c3d_ground15_corner(self, x, y, ground_water_height(w, (int64_t)x + y - 1), first);
    p.x = first[0];
    p.y = first[1];
    p.z = first[2];
    renderer_project_vertex(g_renderer, &p, view, &v);
    memset(&draw, 0, sizeof draw);
    draw.kind = RP_WORLD_WATER;
    draw.cell = (uint32_t)cell->water;
    draw.colour = colour;
    renderer_add_world_draw(g_renderer, RENDER_ALPHA_CLAMP, &draw, w->texture, v.rhw);
}

/* the blocks that share a cell with `cells`: x0, y0, x1, y1 */
static void gpu_blocks(const struct C3dGround15 *self, const int *cells, int *out)
{
    out[0] = cells[0] < 0 ? 0 : cells[0] / GND_GPU_BLOCK;
    out[1] = cells[1] < 0 ? 0 : cells[1] / GND_GPU_BLOCK;
    out[2] = cells[2] / GND_GPU_BLOCK;
    out[3] = cells[3] / GND_GPU_BLOCK;
    if (out[2] >= self->gpuCols)
        out[2] = self->gpuCols - 1;
    if (out[3] >= self->gpuRows)
        out[3] = self->gpuRows - 1;
}

/* Queues the blocks as one shadow caster each, once a frame; the ground casts without its
   textures. */
void c3d_ground15_cast(struct C3dGround15 *self, const int *cells)
{
    int frame = g_renderer->frame, b4[4], bx, by;
    struct RPWorldDraw draw;

    if (!self->gpuBlocks)
        return;
    memset(&draw, 0, sizeof draw);
    draw.kind = RP_WORLD_CAST;
    draw.textureArray = SW_NO_ARRAY;
    gpu_blocks(self, cells, b4);
    for (by = b4[1]; by <= b4[3]; by++)
        for (bx = b4[0]; bx <= b4[2]; bx++) {
            struct GndGpuBlock *b = &self->gpuBlocks[by * self->gpuCols + bx];

            if (b->castFrame == frame || !b->indexCount)
                continue;
            b->castFrame = frame;
            draw.firstIndex = b->firstIndex;
            draw.count = b->indexCount;
            draw.baseVertex = b->baseVertex;
            draw.ids = b->ids;
            renderer_add_world_draw(g_renderer, RENDER_SHADOW_CASTERS, &draw, NULL, 0.0f);
        }
}

void c3d_ground15_leaf(struct C3dGround15 *self, const float *view, const int *cells)
{
    DWORD colour;
    int x, y;

    if (g_renderer->shadowPending)
        c3d_ground15_cast(self, cells);
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

static void attr_quad(struct C3dGround15 *self, const float *view, int ax, int ay, DWORD colour,
                      struct Texture *texture, const float *uv, float bias)
{
    float quad[12] = {0};
    struct RPQuad *q;
    float su, sv;
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

    q = &renderer_new_prim(g_renderer)->base;
    for (i = 0; i < 4; i++) {
        {
            Vec3 world_point = {quad[i * 3], quad[i * 3 + 1], quad[i * 3 + 2]};
            renderer_project_vertex(g_renderer, &world_point, view, (struct TexDrawVertex *)q->verts + i);
        }
        ((struct TexDrawVertex *)q->verts)[i].z -= bias;
    }
    su = (float)(DWORD)texture->reqWidth / (float)(DWORD)texture->base.width;
    sv = (float)(DWORD)texture->reqHeight / (float)(DWORD)texture->base.height;
    ((struct TexDrawVertex *)q->verts)[0].u = uv[0] * su;
    ((struct TexDrawVertex *)q->verts)[0].v = uv[1] * sv;
    ((struct TexDrawVertex *)q->verts)[1].u = uv[2] * su;
    ((struct TexDrawVertex *)q->verts)[1].v = uv[3] * sv;
    ((struct TexDrawVertex *)q->verts)[2].u = uv[4] * su;
    ((struct TexDrawVertex *)q->verts)[2].v = uv[5] * sv;
    ((struct TexDrawVertex *)q->verts)[3].u = uv[6] * su;
    ((struct TexDrawVertex *)q->verts)[3].v = uv[7] * sv;
    q->srcBlend = 5;
    q->destBlend = 6;
    q->texture = texture;
    for (i = 0; i < 4; i++)
        ((struct TexDrawVertex *)q->verts)[i].diffuse = colour;
    renderer_add_prim(g_renderer, q, 1);
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
    struct RPQuad *q;
    int i;

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

    q = &renderer_new_prim(g_renderer)->base;
    for (i = 0; i < 4; i++)
        {
            Vec3 world_point = {quad[i * 3], quad[i * 3 + 1], quad[i * 3 + 2]};
            renderer_project_vertex(g_renderer, &world_point, view, (struct TexDrawVertex *)q->verts + i);
        }
    ((struct TexDrawVertex *)q->verts)[0].u = 0.0f;
    ((struct TexDrawVertex *)q->verts)[0].v = 0.0f;
    ((struct TexDrawVertex *)q->verts)[1].u = 1.0f;
    ((struct TexDrawVertex *)q->verts)[1].v = 0.0f;
    ((struct TexDrawVertex *)q->verts)[2].u = 0.0f;
    ((struct TexDrawVertex *)q->verts)[2].v = 1.0f;
    ((struct TexDrawVertex *)q->verts)[3].u = 1.0f;
    ((struct TexDrawVertex *)q->verts)[3].v = 1.0f;
    q->srcBlend = 5;
    q->destBlend = 6;
    q->texture = texture;
    for (i = 0; i < 4; i++)
        ((struct TexDrawVertex *)q->verts)[i].diffuse = colour;
    renderer_add_prim(g_renderer, q, 1);
}

void c3d_ground15_render_attr_tile(struct C3dGround15 *self, const float *view,
                                           int ax, int ay, DWORD colour)
{
    struct Texture *grid = texture_cache_get(&g_textureCache, "grid.tga", 0, 0);

    c3d_ground15_draw_attr_quad(self, view, ax, ay, colour, grid, g_naviCellUVs);
}
