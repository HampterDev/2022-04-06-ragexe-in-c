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

/* custom: whether a projected quad has a corner on or behind the camera's own plane. The original
   drew its corners as pre-transformed vertices, where such a corner flips to the far side of the
   screen; its camera stays high above the ground and never meets such a cell, but with the
   unlocked tilt (game/camera.h) the eye comes down to a few units above it. Those cells lie below
   the bottom of the view, so their water is left out. */
static int behind_eye(const struct TexDrawVertex *v)
{
    return !(v[0].rhw > 0.0f) || !(v[1].rhw > 0.0f) || !(v[2].rhw > 0.0f) || !(v[3].rhw > 0.0f);
}

/* A cell's water quad, or NULL where none is drawn. custom: its corners stay in world coordinates
   for the device to project (RPQuad.world), the projected ones deciding the rest. */
static struct RPQuad *water_quad(struct C3dGround15 *self, int x, int y, struct GndCell *cell, const float *view,
                                 const float *tint, float h, float hPrev, float hNext)
{
    const struct GndSurface *top = cell->top;
    struct TexDrawVertex v[4];
    struct RPQuad *q;
    float u0, u1, v0, v1;
    int i;

    memset(v, 0, sizeof(v));
    for (i = 0; i < 4; i++)
        v[i].specular = 0xff000000;
    if (!(top->v[0].pos[1] > h || top->v[1].pos[1] > h || top->v[2].pos[1] > h || top->v[3].pos[1] > h))
        return NULL;
    cell->water[0][1] = hPrev;
    cell->water[1][1] = h;
    cell->water[2][1] = h;
    cell->water[3][1] = hNext;
    for (i = 0; i < 4; i++)
        {
            Vec3 world_point = {cell->water[i][0], cell->water[i][1], cell->water[i][2]};
            renderer_project_vertex(g_renderer, &world_point, view, &v[i]);
        }
    if (behind_eye(v))
        return NULL;
    if (!((v[2].x - v[0].x) * (v[1].y - v[0].y) > (v[1].x - v[0].x) * (v[2].y - v[0].y)))
        return NULL;

    q = &renderer_new_prim(g_renderer)->base;
    for (i = 0; i < 4; i++) {
        v[i].x = cell->water[i][0];
        v[i].y = cell->water[i][1];
        v[i].z = cell->water[i][2];
        rp_quad_set_vertex_pos(q, i, &v[i]);
    }
    q->world = 1;
    if (g_playerBlind == 0 && self->waterType == 4) {
        DWORD c = (BYTE)(int)(tint[0] * 255.0f);
        c = (c + 0xff00) << 8;
        c += (BYTE)(int)(tint[1] * 255.0f);
        c <<= 8;
        c += (BYTE)(int)(tint[2] * 255.0f);
        ((struct TexDrawVertex *)q->verts)[0].diffuse = c;
        ((struct TexDrawVertex *)q->verts)[1].diffuse = ((struct TexDrawVertex *)q->verts)[0].diffuse;
        ((struct TexDrawVertex *)q->verts)[2].diffuse = ((struct TexDrawVertex *)q->verts)[0].diffuse;
        ((struct TexDrawVertex *)q->verts)[3].diffuse = ((struct TexDrawVertex *)q->verts)[0].diffuse;
    } else {
        for (i = 0; i < 4; i++)
            ((struct TexDrawVertex *)q->verts)[i].diffuse = 0x90ffffff;
    }
    u0 = (float)((unsigned)x % 4) * 0.25f;
    u1 = ((float)((unsigned)x % 4) + 1.0f) * 0.25f;
    v0 = (float)((unsigned)y % 4) * 0.25f;
    v1 = ((float)((unsigned)y % 4) + 1.0f) * 0.25f;
    ((struct TexDrawVertex *)q->verts)[0].u = u0;
    ((struct TexDrawVertex *)q->verts)[0].v = v0;
    ((struct TexDrawVertex *)q->verts)[1].u = u1;
    ((struct TexDrawVertex *)q->verts)[1].v = v0;
    ((struct TexDrawVertex *)q->verts)[2].u = u0;
    ((struct TexDrawVertex *)q->verts)[2].v = v1;
    ((struct TexDrawVertex *)q->verts)[3].u = u1;
    ((struct TexDrawVertex *)q->verts)[3].v = v1;
    return q;
}

static void c3d_ground15_draw_water(struct C3dGround15 *self, int x, int y,
                                      struct GndCell *cell, const float *view, const float *tint)
{
    float h, hPrev, hNext;
    int64_t diagonal;
    int block;
    const struct GndWater *w;
    struct RPQuad *q;

    if (!self->waters)
        return;
    block = self->waterPerBlock ? c3d_ground15_get_block_index(self, x, y) : 0;
    if (block < 0 || (size_t)block >= self->waterCount)
        return;
    w = self->waters + block;
    diagonal = (int64_t)x + y;
    h = ground_water_height(w, diagonal);
    hPrev = ground_water_height(w, diagonal - 1);
    hNext = ground_water_height(w, diagonal + 1);
    q = water_quad(self, x, y, cell, view, tint, h, hPrev, hNext);
    if (!q)
        return;
    q->texture = (self->waters)[block].texture;
    renderer_add_prim(g_renderer, q, 1);
}

/* custom: the ground as static meshes the device projects (render/gfx.h, world/ground.h), in place
   of the original's quads, one a surface, which the CPU projected each frame. A block's mesh holds
   four vertices and two triangles per surface, the corners and their order as the original gave
   them to its quads (a strip, 0 1 2 then 2 1 3), so the device draws the same triangles with the
   same colours, texture coordinates and lightmap coordinates, and scales the colours by the
   weather colour as the original did. With lightmaps off the draws bind none and stage 1 stays
   off (render/crenderer.c), so the ground is its texture times its colour. */

struct GpuSurface {
    struct GndSurface *surface;
    uintptr_t key[2];
};

static int gpu_surface_less(const void *a, const void *b)
{
    const struct GpuSurface *x = a, *y = b;
    int i;

    for (i = 0; i < 2; i++)
        if (x->key[i] != y->key[i])
            return x->key[i] < y->key[i] ? -1 : 1;
    return 0;
}

static int gpu_add(struct GpuSurface *list, int n, struct GndSurface *s)
{
    if (!s)
        return n;
    list[n].surface = s;
    list[n].key[0] = (uintptr_t)s->texture;
    list[n].key[1] = (uintptr_t)s->lightmapPage;
    return n + 1;
}

static int gpu_block_build(struct C3dGround15 *self, struct GndGpuBlock *b, int bx, int by,
                           struct GpuSurface *list, struct GfxMeshVertex *verts, WORD *indices)
{
    int x0 = bx * GND_GPU_BLOCK, y0 = by * GND_GPU_BLOCK, x, y, n = 0, i, k;
    int x1 = x0 + GND_GPU_BLOCK < self->width ? x0 + GND_GPU_BLOCK : self->width;
    int y1 = y0 + GND_GPU_BLOCK < self->height ? y0 + GND_GPU_BLOCK : self->height;

    b->lastFrame = b->castFrame = -1;
    for (y = y0; y < y1; y++)
        for (x = x0; x < x1; x++) {
            struct GndCell *cell = GND_CELL(self, x, y);

            n = gpu_add(list, n, cell->top);
            n = gpu_add(list, n, cell->frontWall);
            n = gpu_add(list, n, cell->rightWall);
        }
    if (!n)
        return 1;
    qsort(list, (size_t)n, sizeof *list, gpu_surface_less);
    b->draws = calloc((size_t)n, sizeof *b->draws);
    if (!b->draws)
        return 0;
    for (i = 0; i < n; i++) {
        const struct GndSurface *s = list[i].surface;
        struct GndGpuDraw *d = b->drawCount ? &b->draws[b->drawCount - 1] : NULL;

        const float *p0 = s->v[0].pos, *p1 = s->v[1].pos, *p2 = s->v[2].pos;
        float e1[3] = {p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]};
        float e2[3] = {p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]};
        float n[3] = {e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0]};
        float len = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);

        for (k = 0; k < 3 && len > 0.0f; k++)
            n[k] /= len;
        for (k = 0; k < 4; k++) {
            struct GfxMeshVertex *v = &verts[i * 4 + k];

            v->x = s->v[k].pos[0];
            v->y = s->v[k].pos[1];
            v->z = s->v[k].pos[2];
            v->nx = n[0];
            v->ny = n[1];
            v->nz = n[2];
            v->diffuse = v->colour2 = s->v[k].colour;
            v->u0 = s->v[k].u;
            v->v0 = s->v[k].v;
            v->u1 = s->v[k].lu;
            v->v1 = s->v[k].lv;
        }
        indices[i * 6] = (WORD)(i * 4);
        indices[i * 6 + 1] = (WORD)(i * 4 + 1);
        indices[i * 6 + 2] = (WORD)(i * 4 + 2);
        indices[i * 6 + 3] = (WORD)(i * 4 + 2);
        indices[i * 6 + 4] = (WORD)(i * 4 + 1);
        indices[i * 6 + 5] = (WORD)(i * 4 + 3);
        if (!d || i == 0 || gpu_surface_less(&list[i - 1], &list[i])) {
            d = &b->draws[b->drawCount++];
            d->texture = s->texture;
            d->lightmap = s->lightmapPage;
            d->first = i * 6;
        }
        d->count += 6;
    }
    b->indexCount = n * 6;
    b->mesh = gfx_mesh_create(verts, n * 4, indices, n * 6);
    return b->mesh != NULL;
}

static int gpu_build(struct C3dGround15 *self)
{
    enum { MAX_SURFACES = GND_GPU_BLOCK * GND_GPU_BLOCK * 3 };
    struct GpuSurface *list = malloc(MAX_SURFACES * sizeof *list);
    struct GfxMeshVertex *verts = malloc(MAX_SURFACES * 4 * sizeof *verts);
    WORD *indices = malloc(MAX_SURFACES * 6 * sizeof *indices);
    int bx, by, ok = list && verts && indices && self->width > 0 && self->height > 0;

    if (ok) {
        self->gpuCols = (self->width + GND_GPU_BLOCK - 1) / GND_GPU_BLOCK;
        self->gpuRows = (self->height + GND_GPU_BLOCK - 1) / GND_GPU_BLOCK;
        self->gpuBlocks = calloc((size_t)self->gpuCols * (size_t)self->gpuRows, sizeof *self->gpuBlocks);
        ok = self->gpuBlocks != NULL;
    }
    for (by = 0; ok && by < self->gpuRows; by++)
        for (bx = 0; ok && bx < self->gpuCols; bx++)
            ok = gpu_block_build(self, &self->gpuBlocks[by * self->gpuCols + bx], bx, by, list, verts, indices);
    free(list);
    free(verts);
    free(indices);
    if (!ok) {
        ground_gpu_free(self);
        self->gpuState = -1;
        return 0;
    }
    self->gpuState = 1;
    return 1;
}

/* whether the blocks are there, built on the first call; where they cannot be, the ground is not
   drawn */
static int gpu_ready(struct C3dGround15 *self)
{
    if (self->gpuState == 0)
        gpu_build(self);
    return self->gpuState == 1;
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

/* Queues the blocks as one shadow caster each, once a frame. */
static void gpu_cast(struct C3dGround15 *self, const float *view, const int *cells)
{
    int frame = g_renderer->frame, b4[4], bx, by;
    struct RPMeshDraw draw;

    memset(&draw, 0, sizeof draw);
    memcpy(draw.view, view, sizeof draw.view);
    draw.draw.kind = GFX_MESH_GROUND;
    draw.draw.colour[0] = draw.draw.colour[1] = draw.draw.colour[2] = draw.draw.colour[3] = 1.0f;
    draw.draw.uv[0] = draw.draw.uv[3] = 1.0f;
    gpu_blocks(self, cells, b4);
    for (by = b4[1]; by <= b4[3]; by++)
        for (bx = b4[0]; bx <= b4[2]; bx++) {
            struct GndGpuBlock *b = &self->gpuBlocks[by * self->gpuCols + bx];

            if (b->castFrame == frame || !b->indexCount)
                continue;
            b->castFrame = frame;
            draw.mesh = b->mesh;
            draw.draw.count = b->indexCount;
            renderer_add_mesh_draw(g_renderer, RENDER_SHADOW_CASTERS, &draw, NULL, NULL, 0.0f);
        }
}

/* Queues the blocks that share a cell with `cells`, each once a frame; the device clips what lies
   outside the view. */
static void gpu_render(struct C3dGround15 *self, const float *view, const int *cells)
{
    int frame = g_renderer->frame, b4[4], bx, by, i;
    struct RPMeshDraw draw;

    memset(&draw, 0, sizeof draw);
    memcpy(draw.view, view, sizeof draw.view);
    draw.draw.kind = GFX_MESH_GROUND;
    for (i = 0; i < 3; i++)
        draw.draw.colour[i] = g_weather.color[i];
    draw.draw.colour[3] = 1.0f;
    draw.draw.uv[0] = draw.draw.uv[3] = 1.0f;

    gpu_blocks(self, cells, b4);
    for (by = b4[1]; by <= b4[3]; by++)
        for (bx = b4[0]; bx <= b4[2]; bx++) {
            struct GndGpuBlock *b = &self->gpuBlocks[by * self->gpuCols + bx];

            if (b->lastFrame == frame)
                continue;
            b->lastFrame = frame;
            draw.mesh = b->mesh;
            for (i = 0; i < b->drawCount; i++) {
                draw.draw.first = b->draws[i].first;
                draw.draw.count = b->draws[i].count;
                renderer_add_mesh_draw(g_renderer, RENDER_GROUND_MESH, &draw, b->draws[i].texture,
                                       g_lightmap ? b->draws[i].lightmap : NULL, 0.0f);
            }
        }
}

void c3d_ground15_cast(struct C3dGround15 *self, const float *view, const int *cells)
{
    if (gpu_ready(self))
        gpu_cast(self, view, cells);
}

void c3d_ground15_render(struct C3dGround15 *self, const float *view, const int *cells)
{
    float tint[3];
    int x, y;

    if (gpu_ready(self)) {
        gpu_render(self, view, cells);
        if (g_renderer->shadowPending)
            gpu_cast(self, view, cells);
    }
    if (!self->waters)
        return;
    tint[0] = g_weather.tint[0];
    tint[1] = g_weather.tint[1];
    tint[2] = g_weather.tint[2];
    for (y = cells[1]; y <= cells[3]; y++)
        for (x = cells[0]; x <= cells[2]; x++) {
            struct GndCell *cell = GND_CELL(self, x, y);

            if (cell->lastFrame == g_renderer->frame)
                continue;
            cell->lastFrame = g_renderer->frame;
            if (cell->top)
                c3d_ground15_draw_water(self, x, y, cell, view, tint);
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
