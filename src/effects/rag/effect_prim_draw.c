#include "render/projection.h"
#include "render/quads.h"
#include "render/rp_pool.h"
#include <string.h>
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"

#define PRIM_POOL_LIMIT 0x1fe

struct TexDrawVertex *prim_project_vertex(struct TexDrawVertex *out, const float *matrix, Vec3 pos)
{
    out->x = 0.0f;
    out->y = 0.0f;
    out->z = 0.0f;
    out->rhw = 0.0f;
    out->diffuse = 0;
    out->u = 0.0f;
    out->v = 0.0f;
    renderer_project_vertex(g_renderer, &pos, matrix, out);
    return out;
}

/* custom: how the device takes a set of corners onto the screen (render/quads.h, struct
   GfxCorners): its rhw and depth rules, and for an upright quad the point its corners stand around
   and their lift */
struct WorldRule {
    int rhw, depth;
    float rhwValue, depthValue;
    int upright;
    Vec3 centre;
    float lift;
};

static const struct WorldRule g_projected = {GFX_RHW_PROJECTED, GFX_DEPTH_PROJECTED, 0.0f, 0.0f, 0, {0.0f, 0.0f, 0.0f},
                                             0.0f};

/* custom: a new set of the frame's, holding no corner yet, for corners in `colour` through `view` by
   the rule */
static struct GfxCornerSet *new_corner_set(const float *view, uint32_t colour, const struct WorldRule *rule)
{
    struct GfxCornerSet *set = renderer_new_corner_set(g_renderer);
    struct GfxCorners *c = &set->corners;

    memcpy(c->view, view, sizeof c->view);
    c->proj[0] = g_renderer->proj[0];
    c->proj[1] = g_renderer->proj[1];
    c->centre[0] = (float)g_renderer->centreX;
    c->centre[1] = (float)g_renderer->centreY;
    c->rhw = rule->rhw;
    c->depth = rule->depth;
    c->rhwValue = rule->rhwValue;
    c->depthValue = rule->depthValue;
    c->colour = colour;
    if (rule->upright) {
        c->upright = 1;
        c->uprightPoint[0] = rule->centre.x;
        c->uprightPoint[1] = rule->centre.y;
        c->uprightPoint[2] = rule->centre.z;
        c->lift = rule->lift;
    }
    return set;
}

/* custom: the corner's place in the set, where the set has it already (bit for bit) */
static int set_corner(const struct GfxCorners *c, const struct PrimCorner *corner)
{
    int k;

    for (k = 0; k < c->count; k++)
        if (!memcmp(c->point[k], corner->pos, sizeof corner->pos) && !memcmp(&c->u[k], &corner->u, sizeof corner->u)
            && !memcmp(&c->v[k], &corner->v, sizeof corner->v))
            return k;
    return -1;
}

/* custom: the effect's triangles tri[0] to tri[count - 1] (their corners in the world, in their
   colour, with their textures and blending) as instances of the frame, in that order, the corners
   they have in common put up once: a set of up to GFX_CORNERS_MAX corners, which the triangles name
   (render/quads.h, GFX_QUAD_CORNERS), until one does not fit or is of another colour. The CPU
   finds the corners' rhw alone, by the rule, for the lists' key (the rhw of a triangle's first two
   corners, as of its vertices before) and whether a triangle lies flat on the screen. In an
   upright quad the second triangle's corner 2 stands on the right, the first's on the left. */
static void queue_world_triangles(const struct PrimTriangle *tri, int count, const float *view, int flags,
                                  const struct WorldRule *rule)
{
    struct GfxCornerSet *set = NULL;
    float rhw[GFX_CORNERS_MAX];
    int t, k;

    for (t = 0; t < count; t++) {
        struct TexDrawPrim *prim;
        struct GfxQuad *q;
        float own[3];
        int missing = 0;

        for (k = 0; k < 3 && set; k++)
            missing += set_corner(&set->corners, &tri[t].world.corner[k]) < 0;
        if (!set || set->corners.colour != tri[t].world.colour || set->corners.count + missing > GFX_CORNERS_MAX)
            set = new_corner_set(view, tri[t].world.colour, rule);
        prim = renderer_new_triangle_instance(g_renderer);
        q = &prim->instance;
        q->kind = GFX_QUAD_CORNERS;
        q->corners.set = set;
        q->corners.right = rule->upright && t == 1;
        for (k = 0; k < 3; k++) {
            const struct PrimCorner *corner = &tri[t].world.corner[k];
            struct GfxCorners *c = &set->corners;
            int at = set_corner(c, corner);

            if (at < 0) {
                Vec3 p;

                at = c->count++;
                memcpy(c->point[at], corner->pos, sizeof corner->pos);
                c->u[at] = corner->u;
                c->v[at] = corner->v;
                p.x = corner->pos[0];
                p.y = corner->pos[1];
                p.z = corner->pos[2];
                if (rule->rhw == GFX_RHW_VALUE)
                    rhw[at] = rule->rhwValue;
                else if (rule->rhw == GFX_RHW_ADD)
                    rhw[at] = renderer_project_rhw(&p, view) + rule->rhwValue;
                else
                    rhw[at] = renderer_project_rhw(&p, view);
            }
            q->corners.corner[k] = (unsigned char)at;
            own[k] = rhw[at];
        }
        q->corners.flat = quad_rhw_flat(own, 3);
        prim->base.texture = tri[t].texture;
        prim->base.stageMode = 0;
        prim->base.srcBlend = (uint32_t)tri[t].src_blend;
        prim->base.destBlend = (uint32_t)tri[t].dest_blend;
        prim->base.rhw[0] = own[0];
        prim->base.rhw[1] = own[1];
        renderer_add_prim(g_renderer, &prim->base, flags);
    }
}

void prim_add_world_triangles(const struct PrimTriangle *tri, int count, const float *view, int flags)
{
    queue_world_triangles(tri, count, view, flags, &g_projected);
}

static float upright_lift(const struct EffectPrim *self)
{
    return (float)(-30 * g_renderer->height / 480) * self->base.scale;
}

/* custom: the original moved the projected vertices of a quad's two triangles by its mode, which
   the device now does as the rules say: PRIM_QUAD_DEPTH_BIAS adds 4.5e-5 to the rhw, and z is the
   depth of that rhw; PRIM_QUAD_IN_FRONT puts every corner in front of everything (z 1e-9, rhw 1);
   PRIM_QUAD_UPRIGHT does that too, and stands the triangles up on the screen around the
   primitive's point, lifted, each as wide as its first two corners lie apart (the second's corner
   2 on the right, the first's on the left); other modes leave them as projected. */
static void queue_quad_triangles(struct EffectPrim *self, const struct PrimTriangle *tri, const float *matrix,
                                 unsigned char mode)
{
    struct WorldRule rule = g_projected;

    if (mode == PRIM_QUAD_DEPTH_BIAS) {
        rule.rhw = GFX_RHW_ADD;
        rule.rhwValue = 4.5e-5f;
    } else if (mode == PRIM_QUAD_IN_FRONT || mode == PRIM_QUAD_UPRIGHT) {
        rule.rhw = GFX_RHW_VALUE;
        rule.rhwValue = 1.0f;
        rule.depth = GFX_DEPTH_VALUE;
        rule.depthValue = 1e-9f;
        if (mode == PRIM_QUAD_UPRIGHT) {
            rule.upright = 1;
            rule.centre = self->base.pos;
            rule.lift = upright_lift(self);
        }
    }
    queue_world_triangles(tri, 2, matrix, self->list_flags, &rule);
}

void prim_draw_quad(struct EffectPrim *self, int *count, short alpha, Vec3 p0, Vec3 p1,
                               Vec3 p2, Vec3 p3, const float *matrix, short blend, char texture,
                               unsigned char in_world, unsigned char r, unsigned char g, unsigned char b,
                               float v_p2p3, float v_p0p1, float u_p0p3, float u_p1p2, unsigned char mode)
{
    const struct RagEffect *master = self->master;
    int dest = blend != PRIM_BLEND_ADDITIVE ? DRAW_BLEND_INVSRCALPHA : DRAW_BLEND_ONE;
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
        tri->src_blend = DRAW_BLEND_ONE;
        tri->dest_blend = DRAW_BLEND_ZERO;
    } else {
        tri->src_blend = DRAW_BLEND_SRCALPHA;
        tri->dest_blend = blend == PRIM_BLEND_ALPHA ? DRAW_BLEND_INVSRCALPHA : dest;
    }
    prim_set_corner(&tri->world.corner[0], p0, u_p0p3, v_p0p1);
    prim_set_corner(&tri->world.corner[1], p1, u_p1p2, v_p0p1);
    prim_set_corner(&tri->world.corner[2], p2, u_p1p2, v_p2p3);
    tri->world.colour = argb;
    tri->texture = self->textures[texture];
    ++*count;

    tri = &self->triangles[*count];
    if (blend == PRIM_BLEND_OPAQUE) {
        tri->src_blend = DRAW_BLEND_ONE;
        tri->dest_blend = DRAW_BLEND_ZERO;
    } else {
        tri->src_blend = DRAW_BLEND_SRCALPHA;
        tri->dest_blend = blend == PRIM_BLEND_ALPHA ? DRAW_BLEND_INVSRCALPHA : dest;
    }
    prim_set_corner(&tri->world.corner[0], p3, u_p0p3, v_p2p3);
    prim_set_corner(&tri->world.corner[1], p2, u_p1p2, v_p2p3);
    prim_set_corner(&tri->world.corner[2], p0, u_p0p3, v_p0p1);
    tri->world.colour = argb;
    tri->texture = self->textures[texture];
    queue_quad_triangles(self, tri - 1, matrix, mode);
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
    tri->src_blend = DRAW_BLEND_SRCALPHA;
    tri->dest_blend = dest;
    prim_set_corner(&tri->world.corner[0], p0, u0, look.v);
    prim_set_corner(&tri->world.corner[1], p1, u1, look.v);
    prim_set_corner(&tri->world.corner[2], p2, u1, 0.0f);
    tri->world.colour = argb;
    tri->texture = self->textures[texture];
    ++*count;

    tri = &self->triangles[*count];
    tri->src_blend = DRAW_BLEND_SRCALPHA;
    tri->dest_blend = dest;
    prim_set_corner(&tri->world.corner[0], p3, u0, 0.0f);
    prim_set_corner(&tri->world.corner[1], p2, u1, 0.0f);
    prim_set_corner(&tri->world.corner[2], p0, u0, look.v);
    tri->world.colour = argb;
    tri->texture = self->textures[texture];
    prim_add_world_triangles(tri - 1, 2, matrix, self->list_flags);
    ++*count;
}
