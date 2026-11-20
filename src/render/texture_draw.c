#include "render/render_api.h"
#include "render/cdc.h"
#include "render/screen_prim.h"

#define TEXDRAW_LIST SCREEN_LIST
#define TEXDRAW_Z    1e-5f
#define TEXDRAW_RHW  0.99999f
/* custom: the frame's last list (RENDER_BLENDED_OVERLAY, by its rhw, which the list sorts by, after
   everything queued there so far, the mouse cursor's sprite included) */
#define ON_TOP_LIST SCREEN_LIST_FILL
#define ON_TOP_RHW  1.0f

static float size_to_float(unsigned u)
{
    return (float)(double)u;
}

static void set_vertex(struct TexDrawVertex *v, float x, float y, DWORD colour, float u, float tv, float rhw)
{
    v->x = x;
    v->y = y;
    v->z = TEXDRAW_Z;
    v->rhw = rhw;
    v->diffuse = colour;
    v->u = u;
    v->v = tv;
}

static void submit_to(struct Texture *tex, int x0, int y0, int x1, int y1, DWORD colour,
                      float u0, float v0, float u1, float v1, int list, float rhw)
{
    struct TexDrawPrim *p = renderer_new_prim(g_renderer);
    float fx0 = (float)x0, fy0 = (float)y0, fx1 = (float)x1, fy1 = (float)y1;

    p->base.texture = tex;
    set_vertex(&p->quad[0], fx0, fy0, colour, u0, v0, rhw);
    set_vertex(&p->quad[1], fx1, fy0, colour, u1, v0, rhw);
    set_vertex(&p->quad[2], fx0, fy1, colour, u0, v1, rhw);
    set_vertex(&p->quad[3], fx1, fy1, colour, u1, v1, rhw);
    renderer_add_prim(g_renderer, &p->base, list);
}

static void submit(struct Texture *tex, int x0, int y0, int x1, int y1, DWORD colour,
                   float u0, float v0, float u1, float v1)
{
    submit_to(tex, x0, y0, x1, y1, colour, u0, v0, u1, v1, TEXDRAW_LIST, TEXDRAW_RHW);
}

static void draw_surface(struct Texture *self, int x, int y, int w, int h, DWORD colour, int list, float rhw)
{
    float fw = size_to_float((unsigned)self->base.width);
    float fh = size_to_float((unsigned)self->base.height);
    float u0 = TEXEL_CENTRE / fw, v0 = TEXEL_CENTRE / fh;
    float v1 = ((float)h + TEXEL_CENTRE) / fh;
    float u1 = ((float)w + TEXEL_CENTRE) / fw;

    submit_to(self, x, y, x + w, y + h, colour, u0, v0, u1, v1, list, rhw);
}

void texture_draw_surface(struct Texture *self, int x, int y, int w, int h, DWORD colour)
{
    draw_surface(self, x, y, w, h, colour, TEXDRAW_LIST, TEXDRAW_RHW);
}

void texture_draw_surface_on_top(struct Texture *self, int x, int y, int w, int h, DWORD colour)
{
    draw_surface(self, x, y, w, h, colour, ON_TOP_LIST, ON_TOP_RHW);
}

static void stretch(struct Texture *self, int x, int y, int w, int h, DWORD colour)
{
    float fw = size_to_float((unsigned)self->base.width);
    float u0 = TEXEL_CENTRE / fw;
    float fh = size_to_float((unsigned)self->base.height);
    float v0 = TEXEL_CENTRE / fh;
    float u1 = (size_to_float((unsigned)self->reqWidth) + TEXEL_CENTRE) / fw;
    float v1 = (size_to_float((unsigned)self->reqHeight) + TEXEL_CENTRE) / fh;

    submit(self, x, y, x + w, y + h, colour, u0, v0, u1, v1);
}

void texture_draw_surface_stretch(struct Texture *self, int x, int y, int w, int h)
{
    stretch(self, x, y, w, h, 0xffffffff);
}
