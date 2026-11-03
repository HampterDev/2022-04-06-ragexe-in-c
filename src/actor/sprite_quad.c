#include "actor/render_ops.h"
#include "render/math3d.h"
#include "render/rp_pool.h"
#include "render/sprite_atlas.h"
#include "render/texture_types.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include "game/context.h"
#include "actor/actor.h"
#include "actor/sprite_render.h"
#include "world/world_imports.h"
#include "actor/game_actor.h"
#include "actor/sprite_quad.h"
#include "render/render_options.h"
#include "ui/render/graphic_setting.h"

float g_spriteLayerDepthStep = 9.00000032e-06f;

float g_spriteScaleDepthBias = 3.60000013e-05f;

unsigned char render_object_clip_to_quad(struct RenderObject *self, int index, int layer,
                                                  float scale, const struct SprClip *clip, float x, float y,
                                                  const float *view, float shift, float *depth_a, float *depth_b,
                                                  const struct SprImg *img, struct SqQuad *quad, int *rect,
                                                  unsigned *colour, int dx, int dy, unsigned char fixed_step)
{
    if (!img || !clip)
        return 0;
    float zbias = self->scale * g_spriteScaleDepthBias;
    double y_d = (double)y, x_d = (double)x;
    double sy = (double)(clip->zoomY * scale * self->zoom);
    double sx;
    float top, bottom, x0, x1, db, mr, mg, mb;
    int flags;
    unsigned c;

    top = (float)((double)(clip->y + dy) * sy + y_d);
    bottom = (float)((double)((img->heightHalved + 1) * img->height + clip->y + dy) * sy + y_d + 1.0);
    sx = (double)(clip->zoomX * scale * self->zoom);
    if (clip->flags & 1) {
        x0 = (float)((double)((img->widthHalved + 1) * img->width + clip->x + dx) * sx + x_d);
        x1 = (float)((double)(dx + clip->x) * sx + x_d + 1.0);
        if (top >= (float)g_renderer->height || 0.0f >= bottom || x1 >= (float)g_renderer->width || 0.0f >= x0)
            return 0;
    } else {
        x0 = (float)((double)(clip->x + dx) * sx + x_d);
        x1 = (float)((double)((img->widthHalved + 1) * img->width + clip->x + dx) * sx + x_d + 1.0);
        if (top >= (float)g_renderer->height || 0.0f >= bottom || x0 >= (float)g_renderer->width || 0.0f >= x1)
            return 0;
    }

    flags = self->overlay_flags;
    if (flags & 0x80) {
        short t = (short)self->overlay_step;
        float k, d;

        if (t <= 0x12)
            k = g_sinTable[t * 5] * 0.3f;
        else if (t <= 0x14)
            k = 0.3f;
        else if (t <= 0x17)
            k = (float)(0x17 - t) * 0.1f;
        else
            k = 0.0f;
        d = (bottom - top) * k;
        top = top - d;
        bottom = bottom - d;
    }
    if (flags & 0x20) {
        short t = (short)self->overlay_step;
        float f = 1.0f;

        if (t <= 3) {
            f = (100.0f - (float)t * 20.0f) * 0.01f;
        } else if (t <= 0x21) {
            int r = rand() % 6;

            f = 0.4f - (float)(r < 0 ? 0 : r) * 0.01f;
        }
        top = top + (bottom - top) * (1.0f - f);
    }

    if (self->fixed_depth) {
        float eps = g_spriteLayerDepthStep;
        float fi = (float)index;

        *depth_a = fi * eps + 0.1f - eps * 20.0f + (float)self->depth_layer * eps;
        eps = g_spriteLayerDepthStep;
        db = fi * eps + 0.1f - eps * 20.0f + (float)self->depth_layer * eps;
    } else {
        int n = self->depth_layer + 3 + layer;
        float eb = bottom - y, et = top - y;

        if (self->slant_depth) {
            eb = eb * g_depthPerPixelCot;
            *depth_a = 1.0f / (view[2] - eb) + (float)n * g_spriteLayerDepthStep + zbias;
            et = et * g_depthPerPixelCot;
            db = 1.0f / (view[2] - et);
        } else {
            float term = (float)n * g_spriteLayerDepthStep;

            if (fixed_step) {
                eb = eb * 0.06f;
                et = et * 0.06f;
                *depth_a = 1.0f / (eb + view[2]) + term + zbias;
            } else {
                eb = eb * g_spriteRowDepthStep;
                *depth_a = 1.0f / (eb + view[2]) + term + zbias;
                et = et * g_spriteRowDepthStep;
            }
            db = 1.0f / (et + view[2]);
        }
        db = db + (float)(self->depth_layer + 3 + layer) * g_spriteLayerDepthStep;
        db = db + zbias;
    }

    *depth_b = db;
    quad->x0 = x0 + shift;
    quad->top = top;
    quad->bottom = bottom;
    quad->x1 = x1 + shift;
    quad->depth_a = *depth_a;
    quad->depth_b = *depth_b;
    quad->angle = (float)(clip->angle + self->spr_angle);
    rect[0] = (int)x0;
    rect[1] = (int)top;
    rect[2] = (int)x1;
    rect[3] = (int)bottom;

    mr = 1.0f;
    mg = 1.0f;
    mb = 1.0f;
    if (self->cell_lit == 1 && g_lightmap != 0) {
        struct C3dGround15 *ground = game_current_ground();
        int cx, cy;

        if (ground && game_world_to_cell(self->pos.x, self->pos.z, &cx, &cy))
            c3d_ground15_get_vertex_colour(ground, cx, cy, &mr, &mg, &mb);
    }

    {
        const unsigned char *argb = (const unsigned char *)&self->argb;
        float fa = (float)argb[3];
        float fr = (float)argb[2] * mr;
        float fg = (float)argb[1] * mg;
        float fb = (float)argb[0] * mb;

        if (self->hit_flash && self->hit_argb != 0xffff0000u) {
            *colour = self->hit_argb;
            c = self->hit_argb;
        } else {
            unsigned char *out = (unsigned char *)colour;

            out[3] = (unsigned char)(int)((float)clip->color[3] * fa / 255.0f);
            out[2] = (unsigned char)(int)((float)clip->color[0] * fr / 255.0f);
            out[1] = (unsigned char)(int)((float)clip->color[1] * fg / 255.0f);
            out[0] = (unsigned char)(int)((float)clip->color[2] * fb / 255.0f);
            c = *colour;
        }
        quad->color = c;
    }

    if (self->overlay_flags & 0x40) {
        short t = (short)self->overlay_step;

        if (t <= 0x24) {
            float angle = 180.0f - g_sinTable[90 + t * 5] * 180.0f;

            quad->angle = angle;
            if (angle >= 360.0f)
                quad->angle = angle - 360.0f;
            return 1;
        }
        quad->angle = 0.0f;
    }
    return 1;
}

void render_object_submit_spr_quad(struct RenderObject *self, struct SqQuad *quad,
                                             const void *palette, int flags, float x, float y)
{
    if (!quad->image)
        return;
    struct SpriteRegion uv = { 0 };
    struct SprImg *img = quad->image;
    struct Texture *tex = img->texture;
    struct TexDrawPrim *prim;
    struct TexDrawVertex *v;
    float z_a, z_b, last_y;
    int list;

    if (!tex) {
        tex = sprite_atlas_resolve(g_renderer, img, palette, &uv);
    } else {
        uv.u = 0.0f;
        uv.v = 0.0f;
        uv.u1 = (float)(unsigned)tex->reqWidth / (float)(unsigned)tex->base.width;
        uv.v1 = (float)(unsigned)tex->reqHeight / (float)(unsigned)tex->base.height;
    }
    if (!tex)
        return;
    z_a = renderer_project_depth(quad->depth_a);
    z_b = renderer_project_depth(quad->depth_b);
    prim = renderer_new_prim(g_renderer);
    prim->base.texture = tex;
    v = prim->base.verts;
    if (quad->angle != 0.0f) {
        float c = get_cos_deg((int)quad->angle);
        float s = get_sin_deg((int)quad->angle);
        float hy = (quad->bottom - quad->top) * 0.5f;
        float hx = (quad->x1 - quad->x0) * 0.5f;
        float cy = quad->top + hy + y;
        float cx = quad->x0 + hx + x;
        float hy_s = hy * s, hx_c = hx * c, hx_s = hx * s, hy_c = hy * c;

        v[0].x = -hx_c - -hy_s + cx + 0.5f;
        v[1].y = -hy_c + hx_s + cy + 0.5f;
        v[0].y = -hy_c + -hx_s + cy + 0.5f;
        v[2].x = -hx_c - hy_s + cx + 0.5f;
        v[1].x = hx_c - -hy_s + cx + 0.5f;
        v[3].x = hx_c - hy_s + cx + 0.5f;
        v[2].y = hy_c + -hx_s + cy + 0.5f;
        last_y = hy_c + hx_s + cy;
    } else {
        v[0].x = quad->x0 + 0.5f;
        v[0].y = quad->top + 0.5f;
        v[1].x = quad->x1 + 0.5f;
        v[1].y = quad->top + 0.5f;
        v[2].x = quad->x0 + 0.5f;
        v[2].y = quad->bottom + 0.5f;
        v[3].x = quad->x1 + 0.5f;
        last_y = quad->bottom;
    }
    v[3].y = last_y + 0.5f;
    if (*(const int *)palette == 0xd77b)
        uv.u1 = uv.u1 - 0.002f;

    v[0].rhw = quad->depth_b;
    v[0].z = z_b;
    v[0].u = uv.u;
    v[0].v = uv.v;
    v[0].diffuse = quad->color;
    v[1].rhw = quad->depth_b;
    v[1].z = z_b;
    v[1].u = uv.u1;
    v[1].v = uv.v;
    v[1].diffuse = quad->color;
    v[2].rhw = quad->depth_a;
    v[2].z = z_a;
    v[2].u = uv.u;
    v[2].v = uv.v1;
    v[2].diffuse = quad->color;
    v[3].rhw = quad->depth_a;
    v[3].z = z_a;
    v[3].u = uv.u1;
    v[3].v = uv.v1;
    v[3].diffuse = quad->color;

    prim->base.srcBlend = 5;
    prim->base.destBlend = (flags != 5) * 4 + 2;
    list = flags | self->prim_list;
    prim->base.alpha_mode = g_renderer->twoPassAlpha != 1;
    renderer_add_prim(g_renderer, &prim->base, list);
}
