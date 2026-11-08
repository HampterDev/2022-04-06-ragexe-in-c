#include "render/rp_pool.h"
#include "render/texture_cache.h"
#include "actor/weather.h"
#include "world/ground.h"
#include "world/camera.h"

#include <math.h>
#include <string.h>
#include "render/render_api.h"
#include "render/render_options.h"
#include "actor/attr_state.h"

static int off_screen(const struct TexDrawVertex *v)
{
    struct Renderer *r = g_renderer;
    float lo01, lo23, hi01, hi23, lo, hi;

    lo01 = v[0].x < v[1].x ? v[0].x : v[1].x;
    lo23 = v[2].x < v[3].x ? v[2].x : v[3].x;
    lo = lo23 > lo01 ? lo01 : lo23;
    if (lo >= (float)r->width)
        return 1;
    hi01 = v[0].x > v[1].x ? v[0].x : v[1].x;
    hi23 = v[2].x > v[3].x ? v[2].x : v[3].x;
    hi = hi01 > hi23 ? hi01 : hi23;
    if (0.0f > hi)
        return 1;
    lo01 = v[0].y < v[1].y ? v[0].y : v[1].y;
    lo23 = v[2].y < v[3].y ? v[2].y : v[3].y;
    lo = lo23 > lo01 ? lo01 : lo23;
    if (lo >= (float)r->height)
        return 1;
    hi01 = v[0].y > v[1].y ? v[0].y : v[1].y;
    hi23 = v[2].y > v[3].y ? v[2].y : v[3].y;
    hi = hi01 > hi23 ? hi01 : hi23;
    if (0.0f > hi)
        return 1;
    return 0;
}

/* custom: whether a projected quad has a corner on or behind the camera's own plane. Its corners
   are drawn as pre-transformed vertices, so such a corner flips to the far side of the screen and
   the quad covers the view with its texture sampled at one point (flat white, grey or olive ground,
   or black blocks). The original's camera stays high above the ground and never meets such a
   cell; with the unlocked tilt (game/camera.h) the eye comes down to a few units above it, and the
   cells under and behind it reach the camera plane. Those cells lie below the bottom of the view,
   so they are left out. */
static int behind_eye(const struct TexDrawVertex *v)
{
    return !(v[0].rhw > 0.0f) || !(v[1].rhw > 0.0f) || !(v[2].rhw > 0.0f) || !(v[3].rhw > 0.0f);
}

static void c3d_ground15_draw_surface(struct C3dGround15 *self, const float *view,
                                        struct GndSurface *s, char clip)
{
    struct TexDrawVertex v[4];
    float lm[4][2];
    float scale[3];
    struct RPQuad *q;
    int i;

    memset(v, 0, sizeof(v));
    for (i = 0; i < 4; i++)
        v[i].specular = 0xff000000;
    scale[0] = g_weather.color[0];
    scale[1] = g_weather.color[1];
    scale[2] = g_weather.color[2];
    for (i = 0; i < 4; i++)
        {
            Vec3 world_point = {s->v[i].pos[0], s->v[i].pos[1], s->v[i].pos[2]};
            renderer_project_vertex(g_renderer, &world_point, view, &v[i]);
        }
    if (behind_eye(v))
        return;
    if (clip && off_screen(v))
        return;

    if (g_playerBlind || !s->halfAlpha) {
        for (i = 0; i < 4; i++) {
            DWORD c;
            BYTE *b = (BYTE *)&c;

            v[i].u = s->v[i].u;
            v[i].v = s->v[i].v;
            lm[i][0] = s->v[i].lu;
            lm[i][1] = s->v[i].lv;
            c = s->v[i].colour;
            b[2] = (BYTE)(int)((float)(BYTE)(c >> 16) * scale[0]);
            b[1] = (BYTE)(int)((float)b[1] * scale[1]);
            b[0] = (BYTE)(int)((float)b[0] * scale[2]);
            v[i].diffuse = c;
        }
    } else {
        for (i = 0; i < 4; i++) {
            v[i].diffuse = s->v[i].colour;
            v[i].u = s->v[i].u;
            v[i].v = s->v[i].v;
            lm[i][0] = s->v[i].lu;
            lm[i][1] = s->v[i].lv;
            ((BYTE *)&v[i].diffuse)[3] = 0x80;
        }
    }

    if (g_waterBumpSupported) {
        if (g_lightmap) {
            struct RPTwoTex *t = &renderer_new_two_tex_quad(g_renderer)->prim;
            struct RPTwoTexVertex *vertices = t->verts;

            for (i = 0; i < 4; i++) {
                memcpy(&vertices[i], &v[i], sizeof(v[i]));
                vertices[i].u1 = lm[i][0];
                vertices[i].v1 = lm[i][1];
            }
            t->tex0 = s->texture;
            t->tex1 = s->lightmapPage;
            renderer_add_two_tex_prim(g_renderer, t, 0);
            return;
        }
    } else if (g_lightmap) {
        q = &renderer_new_prim(g_renderer)->base;
        memcpy(q->verts, v, sizeof(v));
        for (i = 0; i < 4; i++) {
            ((struct TexDrawVertex *)q->verts)[i].u = lm[i][0];
            ((struct TexDrawVertex *)q->verts)[i].v = lm[i][1];
        }
        q->texture = s->lightmapPage;
        renderer_add_prim(g_renderer, q, 0x1000);
    }
    q = &renderer_new_prim(g_renderer)->base;
    memcpy(q->verts, v, sizeof(v));
    q->texture = s->texture;
    renderer_add_prim(g_renderer, q, 0x800);
}

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
    for (i = 0; i < 4; i++)
        rp_quad_set_vertex_pos(q, i, &v[i]);
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

void c3d_ground15_render(struct C3dGround15 *self, const float *view,
                                   const int *cells, char clip)
{
    float tint[3];
    int x, y;

    tint[0] = g_weather.tint[0];
    tint[1] = g_weather.tint[1];
    tint[2] = g_weather.tint[2];
    for (y = cells[1]; y <= cells[3]; y++)
        for (x = cells[0]; x <= cells[2]; x++) {
            struct GndCell *cell = GND_CELL(self, x, y);
            int frame = g_renderer->frame;

            if (cell->lastFrame == frame)
                continue;
            cell->lastFrame = frame;
            if (cell->top) {
                c3d_ground15_draw_water(self, x, y, cell, view, tint);
                c3d_ground15_draw_surface(self, view, cell->top, clip);
            }
            if (cell->frontWall)
                c3d_ground15_draw_surface(self, view, cell->frontWall, clip);
            if (cell->rightWall)
                c3d_ground15_draw_surface(self, view, cell->rightWall, clip);
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
