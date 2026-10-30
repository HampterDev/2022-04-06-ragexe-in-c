#include "render/projection.h"
#include "render/rp_pool.h"
#include <math.h>
#include <string.h>
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"

#define PRIM_POOL_LIMIT 0x1fe

#define D3DBLEND_ZERO        1
#define D3DBLEND_ONE         2
#define D3DBLEND_SRCALPHA    5
#define D3DBLEND_INVSRCALPHA 6

struct TexDrawVertex *prim_project_vertex(struct TexDrawVertex *out, const float *matrix, Vec3 pos)
{
    out->x = 0.0f;
    out->y = 0.0f;
    out->z = 0.0f;
    out->rhw = 0.0f;
    out->diffuse = 0;
    out->specular = 0xff000000;
    out->u = 0.0f;
    out->v = 0.0f;
    renderer_project_vertex(g_renderer, &pos, matrix, out);
    return out;
}

static void set_vertex(struct TexDrawVertex *vx, const float *matrix, Vec3 pos, unsigned argb, float u, float v)
{
    struct TexDrawVertex projected;

    *vx = *prim_project_vertex(&projected, matrix, pos);
    vx->diffuse = argb;
    vx->u = u;
    vx->v = v;
}

static float upright_lift(const struct EffectPrim *self)
{
    return (float)(-30 * g_renderer->height / 480) * self->base.scale;
}

static void quad_depth(struct EffectPrim *self, struct PrimTriangle *tri, const float *matrix, unsigned char mode,
                       int second)
{
    int i;

    if (mode == PRIM_QUAD_DEPTH_BIAS) {
        for (i = 0; i < 3; i++) {
            tri->v[i].rhw = tri->v[i].rhw + 4.5e-5f;
            tri->v[i].z = renderer_project_depth(tri->v[i].rhw);
        }
        return;
    }
    if (mode == PRIM_QUAD_UPRIGHT) {
        struct TexDrawVertex centre;
        float half_width;

        prim_project_vertex(&centre, matrix, self->base.pos);
        half_width = (float)fabs((double)(tri->v[0].x - tri->v[1].x)) * 0.5f;
        tri->v[0].x = centre.x + half_width;
        tri->v[1].x = centre.x - half_width;
        tri->v[2].x = second ? centre.x + half_width : centre.x - half_width;
        for (i = 0; i < 3; i++)
            tri->v[i].y = upright_lift(self) + tri->v[i].y;
        mode = PRIM_QUAD_IN_FRONT;
    }
    if (mode == PRIM_QUAD_IN_FRONT) {
        for (i = 0; i < 3; i++) {
            tri->v[i].z = 1e-9f;
            tri->v[i].rhw = 1.0f;
        }
    }
}

void prim_draw_quad(struct EffectPrim *self, int *count, short alpha, Vec3 p0, Vec3 p1,
                               Vec3 p2, Vec3 p3, const float *matrix, short blend, char texture,
                               unsigned char in_world, unsigned char r, unsigned char g, unsigned char b,
                               float v_p2p3, float v_p0p1, float u_p0p3, float u_p1p2, unsigned char mode)
{
    const struct RagEffect *master = self->master;
    int dest = blend != PRIM_BLEND_ADDITIVE ? D3DBLEND_INVSRCALPHA : D3DBLEND_ONE;
    int red = r, green = g, blue = b;
    unsigned argb;
    struct PrimTriangle *tri;

    if (master->color_r != -1)
        red = (unsigned char)master->color_r;
    if (master->color_g != -1)
        green = (unsigned char)master->color_g;
    if (master->color_b != -1)
        blue = (unsigned char)master->color_b;
    if (master->dest_blend != -1)
        dest = master->dest_blend;
    if (*count >= PRIM_POOL_LIMIT)
        return;

    argb = (unsigned)(int)alpha;
    argb = (argb << 8) + (unsigned char)red;
    argb = (argb << 8) + (unsigned char)green;
    argb = (argb << 8) + (unsigned char)blue;

    if (!in_world) {
        p0.x = self->base.pos.x + p0.x;
        p0.y = self->base.pos.y + p0.y;
        p0.z = self->base.pos.z + p0.z;
        p1.x = self->base.pos.x + p1.x;
        p1.y = self->base.pos.y + p1.y;
        p1.z = self->base.pos.z + p1.z;
        p2.x = self->base.pos.x + p2.x;
        p2.y = self->base.pos.y + p2.y;
        p2.z = self->base.pos.z + p2.z;
        p3.x = self->base.pos.x + p3.x;
        p3.y = self->base.pos.y + p3.y;
        p3.z = self->base.pos.z + p3.z;
    }

    tri = &self->triangles[*count];
    if (blend == PRIM_BLEND_OPAQUE) {
        tri->src_blend = D3DBLEND_ONE;
        tri->dest_blend = D3DBLEND_ZERO;
    } else {
        tri->src_blend = D3DBLEND_SRCALPHA;
        tri->dest_blend = blend == PRIM_BLEND_ALPHA ? D3DBLEND_INVSRCALPHA : dest;
    }
    set_vertex(&tri->v[0], matrix, p0, argb, u_p0p3, v_p0p1);
    set_vertex(&tri->v[1], matrix, p1, argb, u_p1p2, v_p0p1);
    set_vertex(&tri->v[2], matrix, p2, argb, u_p1p2, v_p2p3);
    tri->texture = self->textures[texture];
    quad_depth(self, tri, matrix, mode, 0);
    renderer_add_triangle(g_renderer, tri, self->list_flags);
    ++*count;

    tri = &self->triangles[*count];
    if (blend == PRIM_BLEND_OPAQUE) {
        tri->src_blend = D3DBLEND_ONE;
        tri->dest_blend = D3DBLEND_ZERO;
    } else {
        tri->src_blend = D3DBLEND_SRCALPHA;
        tri->dest_blend = blend == PRIM_BLEND_ALPHA ? D3DBLEND_INVSRCALPHA : dest;
    }
    set_vertex(&tri->v[0], matrix, p3, argb, u_p0p3, v_p2p3);
    set_vertex(&tri->v[1], matrix, p2, argb, u_p1p2, v_p2p3);
    set_vertex(&tri->v[2], matrix, p0, argb, u_p0p3, v_p0p1);
    tri->texture = self->textures[texture];
    quad_depth(self, tri, matrix, mode, 1);
    renderer_add_triangle(g_renderer, tri, self->list_flags);
    ++*count;
}

struct StripStyle {
    unsigned char r, g, b;
    int dest;
    int alpha;
    float v;
};

static void strip_style(int style, struct StripStyle *out)
{
    out->r = out->g = out->b = 0xff;
    out->dest = 2;
    out->alpha = -1;
    out->v = 1.0f;

#define STYLE(rr, gg, bb, dd) (out->r = (rr), out->g = (gg), out->b = (bb), out->dest = (dd))
    switch (style) {
    case 0:   STYLE(0xff, 0xff, 0xff, 6); break;
    case 1:   STYLE(0xff, 0xaf, 0xaf, 2); break;
    case 2:   STYLE(0xc3, 0xc3, 0xff, 6); break;
    case 3:   STYLE(0xff, 0xff, 0xff, 6); out->alpha = 0x50; break;
    case 4:   STYLE(0x64, 0x64, 0xff, 2); break;
    case 6:   STYLE(0xff, 0x64, 0x64, 2); break;
    case 7:   STYLE(0x64, 0xff, 0x64, 2); break;
    case 8:   STYLE(0xff, 0xff, 0xaa, 2); break;
    case 9:   STYLE(0x8c, 0xd2, 0x8c, 2); break;
    case 10:  STYLE(0xff, 0x59, 0xb6, 2); break;
    case 11:  STYLE(0xaa, 0xaa, 0xff, 2); break;
    case 12:  STYLE(0x32, 0x32, 0x32, 6); break;
    case 13:  STYLE(0xff, 0x32, 0xff, 6); break;
    case 14:  STYLE(0x78, 0x78, 0xff, 6); break;
    case 15:  STYLE(0x0a, 0xff, 0x0a, 2); break;
    case 16:  STYLE(0x14, 0xff, 0x14, 2); break;
    case 17:  STYLE(0x00, 0x00, 0xff, 2); break;
    case 18:  STYLE(0xff, 0x00, 0x00, 2); break;
    case 19:  STYLE(0x00, 0xff, 0x00, 2); break;
    case 20:  STYLE(0xff, 0xff, 0x00, 2); break;
    case 21:  STYLE(0xff, 0x00, 0xff, 2); break;
    case 22:  STYLE(0x00, 0xff, 0xff, 2); break;
    case 23:  STYLE(0x29, 0x5e, 0xbc, 6); break;
    case 24:  STYLE(0x0a, 0x42, 0x72, 2); break;
    case 25:  STYLE(0x23, 0x92, 0xb9, 2); break;
    case 26:  STYLE(0xa4, 0xcd, 0xfe, 2); break;
    case 27:  STYLE(0x54, 0xa9, 0xcd, 2); break;
    case 28:  STYLE(0x6c, 0x1a, 0xff, 2); break;
    case 29:  STYLE(0xb1, 0xff, 0x3a, 2); break;
    case 30:  STYLE(0x55, 0x1e, 0xb9, 2); break;
    case 31:  STYLE(0xf4, 0xbb, 0x7c, 2); break;
    case 32:  STYLE(0x00, 0x00, 0x00, 6); break;
    case 40:  STYLE(0x46, 0x46, 0xff, 2); break;
    case 41:  STYLE(0x1e, 0x1e, 0xff, 2); break;
    case 42:  STYLE(0x2b, 0x28, 0x2e, 6); break;
    case 43:  STYLE(0x7d, 0xb6, 0xa0, 6); break;
    case 44:  STYLE(0xb4, 0x37, 0x1e, 6); break;
    case 45:  STYLE(0x41, 0x8d, 0xe7, 2); break;
    case 46:  STYLE(0xa4, 0x2f, 0x1d, 2); break;
    case 47:  STYLE(0xac, 0x9a, 0x38, 2); break;
    case 48:  STYLE(0x50, 0x14, 0x14, 2); break;
    case 49:  STYLE(0xc8, 0x82, 0x54, 2); break;
    case 50:  STYLE(0xff, 0xc6, 0x5b, 2); break;
    case 100: STYLE(0xfa, 0xfa, 0xff, 6); out->v = 0.9f; break;
    default:  break;
    }
#undef STYLE
}

void prim_draw_strip(struct EffectPrim *self, int *count, int segment, short alpha, Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3, const float *matrix, char texture)
{
    const struct RagEffect *master;
    struct StripStyle look;
    struct PrimTriangle *tri;
    int alpha_used, style, red, green, blue, dest;
    unsigned argb;
    float u0, u1;

    if (*count >= PRIM_POOL_LIMIT)
        return;
    style = (int)self->scale;
    strip_style((unsigned)style > 100 ? -1 : style, &look);
    alpha_used = look.alpha != -1 ? look.alpha : alpha;

    master = self->master;
    red = look.r;
    green = look.g;
    blue = look.b;
    if (master->color_r != -1)
        red = (unsigned char)master->color_r;
    if (master->color_g != -1)
        green = (unsigned char)master->color_g;
    if (master->color_b != -1)
        blue = (unsigned char)master->color_b;
    dest = master->dest_blend != -1 ? master->dest_blend : look.dest;
    argb = (unsigned)alpha_used;
    argb = (argb << 8) + (unsigned char)red;
    argb = (argb << 8) + (unsigned char)green;
    argb = (argb << 8) + (unsigned char)blue;

    if (segment < 100) {
        u0 = (float)segment / 21.0f;
        if (segment == 0)
            u0 = 0.0f;
        u1 = (float)(segment + 1) / 21.0f;
    } else if (segment == 100) {
        u0 = 0.35f;
        u1 = 0.45f;
    } else if (segment == 101) {
        u0 = 0.45f;
        u1 = 0.55f;
    } else {
        u0 = 0.55f;
        u1 = 0.65f;
    }

    p0.x = self->base.pos.x + p0.x;
    p0.y = self->base.pos.y + p0.y;
    p0.z = self->base.pos.z + p0.z;
    p1.x = self->base.pos.x + p1.x;
    p1.y = self->base.pos.y + p1.y;
    p1.z = self->base.pos.z + p1.z;
    p2.x = self->base.pos.x + p2.x;
    p2.y = self->base.pos.y + p2.y;
    p2.z = self->base.pos.z + p2.z;
    p3.x = self->base.pos.x + p3.x;
    p3.y = self->base.pos.y + p3.y;
    p3.z = self->base.pos.z + p3.z;

    tri = &self->triangles[*count];
    tri->src_blend = D3DBLEND_SRCALPHA;
    tri->dest_blend = dest;
    set_vertex(&tri->v[0], matrix, p0, argb, u0, look.v);
    set_vertex(&tri->v[1], matrix, p1, argb, u1, look.v);
    set_vertex(&tri->v[2], matrix, p2, argb, u1, 0.0f);
    tri->texture = self->textures[texture];
    renderer_add_triangle(g_renderer, tri, self->list_flags);
    ++*count;

    tri = &self->triangles[*count];
    tri->src_blend = D3DBLEND_SRCALPHA;
    tri->dest_blend = dest;
    set_vertex(&tri->v[0], matrix, p3, argb, u0, 0.0f);
    set_vertex(&tri->v[1], matrix, p2, argb, u1, 0.0f);
    set_vertex(&tri->v[2], matrix, p0, argb, u0, look.v);
    tri->texture = self->textures[texture];
    renderer_add_triangle(g_renderer, tri, self->list_flags);
    ++*count;
}
