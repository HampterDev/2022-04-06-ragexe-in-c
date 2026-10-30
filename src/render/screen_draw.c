#include "render/projection.h"
#include "render/render_api.h"
#include "render/texture_globals.h"
#include "resources/attr_queries.h"
#include "world/mode_manager.h"
#include <math.h>
#include "render/math3d.h"
#include "render/screen_prim.h"
#include "render/screen_draw.h"
#include "std/std.h"
#include "game/context.h"

__declspec(align(16)) const struct TextureUV g_fullUV = { 0.0f, 0.0f, 1.0f, 1.0f };

#define BOX_Z   1e-6f
#define BOX_RHW 0.999999f
#define LINE_Z  0.8f
#define TWO_PI  6.283184f
#define HALF_PI 1.570796f

static float size_to_float(unsigned u)
{
    return (float)(double)u;
}

static void box_vertex(struct TexDrawVertex *v, float x, float y, float z, float rhw, DWORD colour,
                       float u, float tv)
{
    v->x = x;
    v->y = y;
    v->z = z;
    v->rhw = rhw;
    v->diffuse = colour;
    v->u = u;
    v->v = tv;
}

static struct TexDrawPrim *box(int x, int y, int w, int h, DWORD colour, struct Texture *tex, float rhw,
                               float u0, float v0, float u1, float v1)
{
    struct TexDrawPrim *p = renderer_new_prim(g_renderer);
    float x0 = (float)x, y0 = (float)y, x1 = (float)(x + w), y1 = (float)(y + h);

    p->base.texture = tex;
    box_vertex(&p->quad[0], x0, y0, BOX_Z, rhw, colour, u0, v0);
    box_vertex(&p->quad[1], x1, y0, BOX_Z, rhw, colour, u1, v0);
    box_vertex(&p->quad[2], x0, y1, BOX_Z, rhw, colour, u0, v1);
    box_vertex(&p->quad[3], x1, y1, BOX_Z, rhw, colour, u1, v1);
    return p;
}

void screen_quad_points(float ax, float ay, float bx, float by, float cx, float cy, float dx, float dy,
                                struct Texture *tex, DWORD colour)
{
    struct TexDrawPrim *p = renderer_new_prim(g_renderer);
    struct TexDrawVertex *v = p->base.verts;

    v[0].x = ax;
    v[0].diffuse = colour;
    v[0].y = ay;
    v[0].z = LINE_Z;
    v[0].rhw = 1.0f;
    v[0].u = 0.0f;
    v[0].v = 0.0f;
    v[1].x = bx;
    v[1].diffuse = colour;
    v[1].y = by;
    v[1].z = LINE_Z;
    v[1].rhw = 1.0f;
    v[1].u = 1.0f;
    v[1].v = 0.0f;
    v[2].x = cx;
    v[2].diffuse = colour;
    v[2].y = cy;
    v[2].z = LINE_Z;
    v[2].rhw = 1.0f;
    v[2].u = 0.0f;
    v[2].v = 1.0f;
    v[3].x = dx;
    v[3].diffuse = colour;
    v[3].y = dy;
    v[3].z = LINE_Z;
    v[3].rhw = 1.0f;
    v[3].u = 1.0f;
    v[3].v = 1.0f;
    p->base.texture = tex;
    p->base.srcBlend = 2;
    p->base.destBlend = 4;
    renderer_add_prim(g_renderer, &p->base, SCREEN_LIST);
}

static float line_normal_angle(float x0, float y0, float x1, float y1, float *nx, float *ny)
{
    float dy = y1 - y0, dx = x1 - x0;
    float inv = (float)(1.0 / (double)float_sqrt(dy * dy + dx * dx));
    float a;

    *ny = inv * dy;
    *nx = inv * dx;
    a = (float)acos((double)*nx);
    if (*ny < 0.0f)
        a = TWO_PI - a;
    return a + HALF_PI;
}

void screen_thick_line(float x0, float y0, float x1, float y1, int width, struct Texture *tex,
                               DWORD colour)
{
    float len = float_sqrt((y1 - y0) * (y1 - y0) + (x1 - x0) * (x1 - x0));
    float nx, ny, a, w, ox, oy;

    if (len == 0.0f)
        return;
    a = line_normal_angle(x0, y0, x1, y1, &nx, &ny);
    w = (float)width;
    ox = float_cos(a) * w;
    oy = float_sin(a) * w;
    screen_quad_points(x0 - ox, y0 - oy, ox + x0, oy + y0, x1 - ox, y1 - oy, ox + x1, oy + y1, tex, colour);
}

void draw_box_screen(int x, int y, int w, int h, DWORD colour)
{
    if (!(colour & 0xff000000))
        return;
    renderer_add_prim(g_renderer, &(box(x, y, w, h, colour, &g_whiteTexture, BOX_RHW, 0, 0, 0, 0))->base, SCREEN_LIST);
}

void screen_fill_rect(int x, int y, int w, int h, DWORD colour)
{
    struct TexDrawPrim *p;

    if (!(colour & 0xff000000))
        return;
    p = box(x, y, w, h, colour, &g_whiteTexture, BOX_Z, 0, 0, 0, 0);
    p->base.srcBlend = 5;
    p->base.destBlend = 6;
    renderer_add_prim(g_renderer, &p->base, SCREEN_LIST_FILL);
}

void screen_box_textured(int x, int y, int w, int h, DWORD colour, struct Texture *tex)
{
    if (!(colour & 0xff000000))
        return;
    renderer_add_prim(g_renderer,
                      &(box(x, y, w, h, colour, tex ? tex : &g_whiteTexture, BOX_RHW, 0.0f, 0.0f, 1.0f, 1.0f))->base,
                      SCREEN_LIST);
}

void renderer_queue_texture_quad(float x, float y, float w, float h, DWORD colour, struct Texture *tex)
{
    struct TexDrawPrim *p;
    float x1, y1;

    if (!(colour & 0xff000000))
        return;
    p = renderer_new_prim(g_renderer);
    p->base.texture = tex ? tex : &g_whiteTexture;
    x1 = x + w;
    y1 = y + h;
    p->quad[0].x = x;
    p->quad[0].y = y;
    p->quad[0].u = g_texelOffsetU / (float)(int)tex->base.width;
    p->quad[0].v = g_texelOffsetV / (float)(int)tex->base.height;
    p->quad[1].x = x1;
    p->quad[1].y = y;
    p->quad[1].u = (size_to_float((unsigned)tex->reqWidth) + g_texelOffsetU) / (float)(int)tex->base.width;
    p->quad[1].v = g_texelOffsetV / (float)(int)tex->base.height;
    p->quad[2].x = x;
    p->quad[2].y = y1;
    p->quad[2].u = g_texelOffsetU / (float)(int)tex->base.width;
    p->quad[2].v = (size_to_float((unsigned)tex->reqHeight) + g_texelOffsetV) / (float)(int)tex->base.height;
    p->quad[3].x = x1;
    p->quad[3].y = y1;
    p->quad[3].u = (size_to_float((unsigned)tex->reqWidth) + g_texelOffsetU) / (float)(int)tex->base.width;
    p->quad[3].v = (size_to_float((unsigned)tex->reqHeight) + g_texelOffsetV) / (float)(int)tex->base.height;
    p->quad[0].z = BOX_Z;
    p->quad[0].rhw = BOX_RHW;
    p->quad[0].diffuse = colour;
    p->quad[1].z = BOX_Z;
    p->quad[1].rhw = BOX_RHW;
    p->quad[1].diffuse = colour;
    p->quad[2].z = BOX_Z;
    p->quad[2].rhw = BOX_RHW;
    p->quad[2].diffuse = colour;
    p->quad[3].z = BOX_Z;
    p->quad[3].rhw = BOX_RHW;
    p->quad[3].diffuse = colour;
    renderer_add_prim(g_renderer, &p->base, SCREEN_LIST);
}
