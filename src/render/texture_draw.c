#include "render/render_api.h"
#include "render/cdc.h"
#include "render/screen_prim.h"

#define TEXDRAW_LIST SCREEN_LIST
#define TEXDRAW_Z    1e-5f
#define TEXDRAW_RHW  0.99999f

static float size_to_float(unsigned u)
{
    return (float)(double)u;
}

static void set_vertex(struct TexDrawVertex *v, float x, float y, DWORD colour, float u, float tv)
{
    v->x = x;
    v->y = y;
    v->z = TEXDRAW_Z;
    v->rhw = TEXDRAW_RHW;
    v->diffuse = colour;
    v->specular = 0xffffffff;
    v->u = u;
    v->v = tv;
}

static void submit(struct Texture *tex, int x0, int y0, int x1, int y1, DWORD colour,
                   float u0, float v0, float u1, float v1)
{
    struct TexDrawPrim *p = renderer_new_prim(g_renderer);
    float fx0 = (float)x0, fy0 = (float)y0, fx1 = (float)x1, fy1 = (float)y1;

    p->base.texture = tex;
    set_vertex(&p->quad[0], fx0, fy0, colour, u0, v0);
    set_vertex(&p->quad[1], fx1, fy0, colour, u1, v0);
    set_vertex(&p->quad[2], fx0, fy1, colour, u0, v1);
    set_vertex(&p->quad[3], fx1, fy1, colour, u1, v1);
    renderer_add_prim(g_renderer, &p->base, TEXDRAW_LIST);
}

void texture_draw_surface(struct Texture *self, int x, int y, int w, int h, DWORD colour)
{
    float fw = size_to_float((unsigned)self->base.width);
    float fh = size_to_float((unsigned)self->base.height);
    float u0 = g_texelOffsetU / fw, v0 = g_texelOffsetV / fh;
    float v1 = ((float)h + g_texelOffsetV) / fh;
    float u1 = ((float)w + g_texelOffsetU) / fw;

    submit(self, x, y, x + w, y + h, colour, u0, v0, u1, v1);
}

static void stretch(struct Texture *self, int x, int y, int w, int h, DWORD colour)
{
    float fw = size_to_float((unsigned)self->base.width);
    float u0 = g_texelOffsetU / fw;
    float fh = size_to_float((unsigned)self->base.height);
    float v0 = g_texelOffsetV / fh;
    float u1 = (size_to_float((unsigned)self->reqWidth) + g_texelOffsetU) / fw;
    float v1 = (size_to_float((unsigned)self->reqHeight) + g_texelOffsetV) / fh;

    submit(self, x, y, x + w, y + h, colour, u0, v0, u1, v1);
}

void texture_draw_surface_stretch(struct Texture *self, int x, int y, int w, int h)
{
    stretch(self, x, y, w, h, 0xffffffff);
}
