#include "render/rp_pool.h"
#include "render/sprite_atlas.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include "fileio/resource_api.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/effect_math.h"
#include "render/render_api.h"
#include "game/context.h"

#define CLIP_MIRRORED 1
#define SPR_PALETTE(spr) (((struct SprRes *)(spr))->palette)

#define LIST_WORLD 5

static float degrees_to_radians(float deg)
{
    return deg / g_halfTurnDegrees * g_pi;
}

static void transform(Vec3 *out, const float *m, float x, float y, float z)
{
    out->x = y * m[3] + x * m[0] + z * m[6] + m[9];
    out->y = y * m[4] + x * m[1] + z * m[7] + m[10];
    out->z = y * m[5] + x * m[2] + z * m[8] + m[11];
}

static void clear_sprite_fields(struct SpriteParticle *self)
{
    self->spr = 0;
    self->act = 0;
    self->frame_delay = 0;
    self->roll_step = 0;
    self->sprite_offset.x = self->sprite_offset.y = self->sprite_offset.z = 0;
    self->action = 0;
    self->motion = 0;
    self->loop = SPRITE_LOOP;
}

struct SpriteParticle *sprite_particle_construct(struct SpriteParticle *self,
                                                                    Vec3 pos, int list_flags)
{
    particle_construct(&self->base, pos, list_flags);
    self->base.pool_kind = PARTICLE_KIND_SPRITE;
    particle_reset(&self->base);
    clear_sprite_fields(self);
    return self;
}

void sprite_particle_reset(struct SpriteParticle *self)
{
    particle_reset(&self->base);
    clear_sprite_fields(self);
}

void sprite_particle_set_act(struct SpriteParticle *self, const char *name)
{
    if (self->act)
        res_unlock(self->act);
    self->act = res_mgr_get(g_resMgr(), name);
    res_lock(self->act);
}

void sprite_particle_set_spr(struct SpriteParticle *self, const char *name)
{
    if (self->spr)
        res_unlock(self->spr);
    self->spr = res_mgr_get(g_resMgr(), name);
    if (self->spr)
        res_lock(self->spr);
}

static int advance_motion(struct SpriteParticle *self)
{
    int count = (int)actres_frame_count(self->act, self->action);
    int delay = (int)self->frame_delay;

    if (delay == 0 || (self->base.age >= delay && self->base.age % delay == 0))
        self->motion++;
    if (self->motion < count)
        return 1;
    switch (self->loop) {
    case SPRITE_LOOP:
        self->motion = 0;
        break;
    case SPRITE_ONCE:
        self->base.life_seconds = -1.0f;
        return 0;
    case SPRITE_HOLD:
        self->motion = count - 1;
        break;
    }
    return 1;
}

void sprite_particle_render(struct SpriteParticle *self)
{
    float view[12];
    float screen_x, screen_y, rhw, pixel_scale;
    Vec3 pos;
    struct Motion *motion;
    int i;

    if (!self->spr || !self->act)
        return;
    if (self->loop != SPRITE_STILL && !advance_motion(self))
        return;

    if (!game_copy_view_matrix(view))
        return;
    pos = self->base.pos;
    renderer_project_point(g_renderer, &pos, view, &screen_x, &screen_y, &rhw);
    pixel_scale = renderer_screen_scale(g_renderer, rhw);
    motion = act_res_get_motion(self->act, self->action, self->motion);

    for (i = 0; i < motion->clip_count; i++) {
        struct SprClip *clip = motion_get_clip(motion, i);
        struct SprImg *img;
        struct Texture *tex;
        struct SpriteRegion rect;
        struct TexDrawPrim *batch;
        float u_left, v_top, u_right, v_bottom;
        float half_w, half_h, unit_x, unit_y, depth;
        float clip_scale[12], clip_turn[12], clip_move[12], clip_scale_turn[12], clip_m[12];
        float own_scale[12], own_turn[12], own_move[12], own_scale_turn[12], own_placed[12], world[12];
        Vec3 corner[4];
        int k;

        if (clip->sprIndex == -1)
            continue;
        img = sprite_resource_image(self->spr, clip->clipType, clip->sprIndex);
        if (!img) continue;
        memset(&rect, 0, sizeof(rect));
        tex = img->texture;
        if (!tex) {
            tex = sprite_atlas_resolve(g_renderer, img, SPR_PALETTE(self->spr), &rect);
            u_left = rect.u;
            v_top = rect.v;
            u_right = rect.u1;
            v_bottom = rect.v1;
        } else {
            u_left = 0;
            v_top = 0;
            u_right = (float)(unsigned)img->texture->reqWidth / (float)(unsigned)img->texture->base.width;
            v_bottom = (float)(unsigned)img->texture->reqHeight / (float)(unsigned)img->texture->base.height;
        }
        if (!tex) continue;
        if (clip->flags & CLIP_MIRRORED)
            particle_quad_paint(&self->base, self->base.corners, v_bottom, v_top, u_right, u_left);
        else
            particle_quad_paint(&self->base, self->base.corners, v_bottom, v_top, u_left, u_right);

        batch = renderer_new_prim(g_renderer);
        memcpy(batch->base.verts, self->base.corners, sizeof(self->base.corners));
        batch->base.texture = tex;

        if (self->base.flags & PARTICLE_TINT_BY_CLIP) {
            const float to_unit = 0.003921569f;
            unsigned alpha = (unsigned char)(int)((float)self->base.alpha * to_unit * ((float)clip->color[3] * to_unit) * 255.0f);
            unsigned color = (((alpha << 8 | clip->color[0]) << 8) | clip->color[1]) << 8 | clip->color[2];
            for (k = 0; k < 4; k++)
                batch->base.verts[k].diffuse = color;
        }

        half_w = (float)img->width * 0.5f;
        half_h = (float)img->height * 0.5f;
        unit_x = clip->zoomX * pixel_scale / (g_renderer->proj[0] * rhw);
        unit_y = clip->zoomY * pixel_scale / (g_renderer->proj[1] * rhw);
        mat_set_scale(clip_scale, unit_x * half_w, unit_y * half_h, 1.0f);
        mat_set_rotate_z(clip_turn, degrees_to_radians((float)clip->angle));
        mat_set_translation(clip_move, ((float)clip->x + half_w) * unit_x, ((float)clip->y + half_h) * unit_y, 0);
        mat_multiply(clip_scale_turn, clip_scale, clip_turn);
        mat_multiply(clip_m, clip_scale_turn, clip_move);
        for (k = 0; k < 4; k++)
            transform(&corner[k], clip_m, batch->base.verts[k].x, batch->base.verts[k].y, batch->base.verts[k].z);

        mat_set_scale(own_scale, self->base.scale, self->base.scale, 1.0f);
        mat_set_rotate_z(own_turn, degrees_to_radians(self->base.roll));
        mat_rotate_x(own_turn, degrees_to_radians(game_camera_pitch()));
        mat_rotate_y(own_turn, degrees_to_radians(game_camera_heading()));
        pos = self->base.pos;
        mat_set_translation(own_move, pos.x, pos.y, pos.z);
        mat_multiply(own_scale_turn, own_scale, own_turn);
        mat_multiply(own_placed, own_scale_turn, own_move);
        mat_multiply(world, own_placed, self->base.matrix);

        for (k = 0; k < 4; k++) {
            Vec3 placed;
            transform(&placed, world, corner[k].x, corner[k].y, corner[k].z);
            renderer_project_vertex(g_renderer, &placed, view, &batch->base.verts[k]);
        }
        depth = (float)i * 1e-5f + self->base.depth_bias;
        if (depth != 0) {
            for (k = 0; k < 4; k++) {
                batch->base.verts[k].rhw += depth;
                batch->base.verts[k].z = renderer_project_depth(batch->base.verts[k].rhw);
            }
        }
        batch->base.srcBlend = GFX_BLEND_SRCALPHA;
        batch->base.destBlend = self->base.list_flags != LIST_WORLD ? GFX_BLEND_INVSRCALPHA : GFX_BLEND_ONE;
        renderer_add_prim(g_renderer, &batch->base, self->base.list_flags);
    }
}
