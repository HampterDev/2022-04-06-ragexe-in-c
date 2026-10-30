#include "render/rp_pool.h"
#include "render/texture_cache.h"
#include <stdlib.h>
#include <string.h>
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"

#define FX "이팩트\\"

static struct EffectPrim *launch(struct RagEffect *self, int kind)
{
    self->last_prim = rag_launch_prim(self, kind, 0.0f, 0.0f, 0.0f);
    if (!self->last_prim)
        return NULL;
    return self->last_prim;
}

static void one_texture(struct EffectPrim *prim, const char *texture)
{
    prim->textures = rag_prim_new_textures(prim);
    prim->textures[0] = texture_cache_get(&g_textureCache, texture, 0, 0);
}

static void sprite(struct EffectPrim *prim, const char *spr, const char *act)
{
    render_object_load_spr_res(&prim->base, spr);
    render_object_load_act_res(&prim->base, act);
}

static void place_above_ground(Vec3 *position, float rise)
{
    void *attr = game_current_map_attr();
    if (attr)
        position->y = c3d_attr_get_height(attr, position->x, position->z) - rise;
}

static void matrix_point(const float *m, float x, float y, float z, Vec3 *out)
{
    out->x = m[0] * x + m[3] * y + m[6] * z + m[9];
    out->y = m[1] * x + m[4] * y + m[7] * z + m[10];
    out->z = m[2] * x + m[5] * y + m[8] * z + m[11];
}

static void project_master(struct RagEffect *self)
{
    float view[12];
    struct TexDrawVertex v;

    if (!self->master || !game_copy_view_matrix(view))

        return;
    prim_project_vertex(&v, view, self->master->pos);
    self->screen_pos.x = v.x;
    self->screen_pos.y = v.y;
}

#define RAG_SPRITE_SCALE(e)  ((e)->msg_params[0])
#define RAG_SPRITE_PERIOD(e) ((e)->msg_params[1])

void rag_holy_water(struct RagEffect *self)
{
    struct EffectPrim *prim;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\EF_Aqua.wav");
    prim = launch(self, 5);
    if (!prim)
        return;
    prim->duration = self->duration;
    prim->delta2.y = -20.0f;
    prim->motion_period = 2;
    prim->loop_motion = 0;
    sprite(prim, FX "성수뜨기.spr", FX "성수뜨기.act");
    prim_sync_particles(prim);
}

void rag_christmas_sprite(struct RagEffect *self)
{
    struct EffectPrim *prim;
    int pick;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    prim = launch(self, 5);
    if (!prim)
        return;
    prim->pattern |= 0x200;
    prim->duration = 125;
    prim->delta2.y = -10.0f;
    if (RAG_SPRITE_SCALE(self) <= 0.0f)
        RAG_SPRITE_SCALE(self) = 1.0f;
    if (RAG_SPRITE_PERIOD(self) < 1.0f)
        RAG_SPRITE_PERIOD(self) = 1.0f;
    prim->motion_period = (int)RAG_SPRITE_PERIOD(self);
    sprite(prim, FX "크리스마스.spr", FX "크리스마스.act");
    pick = rand() % 10;
    prim->scale = pick % 3 == 1 ? 0.01f : 1.0f;
    render_object_set_action_base(&prim->base, pick / 2, 1);
    prim_sync_particles(prim);
}

void rag_big_explosion(struct RagEffect *self)
{
    struct EffectPrim *prim;

    if (self->frame != 0)
        return;
    prim = launch(self, 14);
    if (!prim)
        return;
    prim->duration = 250;
    prim->size[0].value = 13.5f;
    prim->delta2.y = -10.0f;
    prim->angle[0] = 2.0f;
    prim->alpha = 100.0f;
    one_texture(prim, "effect\\대폭발.tga");
}

void rag_particle_spray(struct RagEffect *self)
{
    struct EffectPrim *prim;

    if (self->frame % 130 != 0)
        return;
    prim = launch(self, 7);
    if (!prim)
        return;
    self->duration = 200;
    prim->duration = 300;
    prim->size[0].value = 0.1f;
    prim->size[0].speed = 0.2f;
    prim->fall_speed = 0.1f;
    prim->gravity = 0.005f;
    prim->angle[0] = 2.0f;
    prim->angle_speed[0] = 15.0f;
    prim->angle_accel[0] = -0.01f;
    prim->scale = 1.2f;
    mat_set_rotate_y(prim->matrix, deg_to_rad((int)prim->angle[0]));
    matrix_point(prim->matrix, 0.0f, -30.0f, prim->size[0].value, &prim->delta2);
    sprite(prim, FX "particle1.spr", FX "particle1.act");
    prim_sync_particles(prim);
}

void rag_curse_ring(struct RagEffect *self)
{
    struct EffectPrim *prim;

    if (self->frame != 0)
        return;
    prim = launch(self, 15);
    if (!prim)
        return;
    prim->duration = 9999;
    prim->angle[0] = 0;
    prim->angle_speed[0] = 4.5f;
    prim->delta2.y = prim->delta2.y - 30.0f;
    prim->size[1].value = 4.0f;
    prim->size[2].value = 4.0f;
    one_texture(prim, "effect\\curse.bmp");
}

void rag_dark_breath(struct RagEffect *self)
{
    struct EffectPrim *prim;

    if (self->frame != 0)
        return;
    prim = launch(self, 5);
    if (!prim)
        return;
    prim->duration = 65;
    prim->list_flags |= 0x200;
    prim->delta2.y = prim->delta2.y - 20.0f;
    prim->scale = 0.8f;
    prim->motion_period = 1;
    prim->g = 0;
    prim->b = 0;
    prim->fade_frame = 60;
    sprite(prim, FX "DarkBreath.spr", FX "DarkBreath.act");
    prim_sync_particles(prim);
}

static void chimney_puff(struct EffectPrim *prim)
{
    mat_set_rotate_x(prim->matrix, deg_to_rad(rag_rand_mod(30) + 75));
    mat_rotate_y(prim->matrix, deg_to_rad(rag_rand_mod(360)));
}

void rag_chimney_smoke(struct RagEffect *self)
{
    struct EffectPrim *prim;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    prim = rag_launch_prim(self, 5, self->offset.x, self->offset.y, self->offset.z);
    if (!prim)
        return;
    self->last_prim = prim;
    prim->duration = self->duration;
    prim->angle_speed[2] = 0.75f;
    chimney_puff(prim);
    prim->delta2.y = -9.0f;
    prim->speed = (float)(rag_rand_mod(3) + 3) / 10.0f;
    prim->scale = 1.5f;
    prim->fade_frame = prim->duration - prim->duration / 3;
    sprite(prim, FX "굴뚝연기.spr", FX "굴뚝연기.act");
    prim_sync_particles(prim);
}

void rag_chimney_smokes(struct RagEffect *self)
{
    struct EffectPrim *prim;
    int n;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    for (n = rag_rand_mod(4) + 1; n > 0; n--) {
        prim = launch(self, 5);
        if (!prim)
            return;
        prim->duration = self->duration;
        chimney_puff(prim);
        prim->delta2.y = -9.0f;
        prim->angle_speed[2] = 0.75f;
        prim->speed = (float)(rag_rand_mod(3) + 3) / 10.0f;
        prim->scale = 1.5f;
        prim->fade_frame = prim->duration - prim->duration / 3;
        sprite(prim, FX "굴뚝연기.spr", FX "굴뚝연기.act");
        prim_sync_particles(prim);
    }
}

static void sinking_ring(struct RagEffect *self, struct EffectPrim *prim)
{
    prim->duration = self->duration;
    prim->pattern = 0x40;
    prim->pattern |= 0x100;
    prim->list_flags |= 2;
}

void rag_fire_ivy(struct RagEffect *self)
{
    struct EffectPrim *prim;

    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\wizard_fire_ivy.wav");
    if (self->frame != 0)
        return;
    prim = launch(self, 15);
    if (!prim)
        return;
    sinking_ring(self, prim);
    prim->size[1].value = 3.5f;
    prim->size[2].value = 3.5f;
    prim->velocity.y = -(25.0f / (float)prim->duration);
    prim->delta2.y = -5.0f;
    one_texture(prim, "effect\\fire_ivy.bmp");
}

void rag_water_out(struct RagEffect *self)
{
    struct EffectPrim *prim;

    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\wizard_waterball_chulung.wav");
    if (self->frame != 0)
        return;
    prim = launch(self, 15);
    if (!prim)
        return;
    sinking_ring(self, prim);
    prim->loop_motion = 1;
    prim->size[1].value = 3.5f;
    prim->size[2].value = 3.5f;
    prim->velocity.y = -(25.0f / (float)prim->duration);
    prim->delta2.y = -5.0f;
    prim->motion_period = 5;
    prim->motion_count = 3;
    prim->textures = rag_prim_new_textures(prim);
    prim->textures[0] = texture_cache_get(&g_textureCache, "effect\\water_out_a.bmp", 0, 0);
    prim->textures[1] = texture_cache_get(&g_textureCache, "effect\\water_out_b.bmp", 0, 0);
    prim->textures[2] = texture_cache_get(&g_textureCache, "effect\\water_out_c.bmp", 0, 0);
}

void rag_fire_ring(struct RagEffect *self)
{
    struct EffectPrim *prim;
    int angle;

    project_master(self);
    angle = rand() % 360;
    if (self->frame != 0)
        return;
    prim = launch(self, 0);
    if (!prim)
        return;
    prim->duration = 30;
    prim->delta2.y = -20.0f;
    prim->size[1].value = 10.0f;
    prim->size[2].value = 10.0f;
    prim->size[1].speed = 5.7f;
    prim->size[2].speed = 5.7f;
    prim->angle[2] = (float)(angle < 0 ? 0 : angle);
    prim->angle_speed[2] = -18.0f;
    prim->angle_accel[2] = prim->angle_speed[2] / (float)prim->duration / -2.5f;
    prim->fade_frame = prim->duration - prim->duration / 2;
    one_texture(prim, "effect\\FireRing.tga");
    prim->variant = 0;
}

unsigned char rag_kings_grace(struct RagEffect *self)
{
    struct EffectPrim *prim;
    struct PrimFacingSquare *sq;
    int angle;

    rag_follow_master(self);
    if (self->frame == 0) {
        for (angle = 0; angle < 360; angle += 90) {
            prim = launch(self, 171);
            if (!prim)
                return 0;
            prim->list_flags = 5;
            prim->duration = self->duration;
            sq = &prim->records.facing_square;
            sq->active = 1;
            sq->wave_x = 0.0f;
            sq->wave_z = 0.0f;
            sq->wave_y = 0.0f;
            sq->orbit = (float)angle;
            sq->lift = -8.0f;
            sq->pulse = 0.0f;
            sq->fading = 0.0f;
            sq->size = 8.0f;
            sq->alpha = 0;
            sq->age = 0;
            prim->motion_count = 1;
            one_texture(prim, "effect\\lg_kings_grace.tga");
        }
    }
    return 1;
}

void rag_party_ring(struct RagEffect *self)
{
    struct EffectPrim *prim;

    rag_follow_master(self);
    if (self->frame == 9998)
        self->frame = 0;
    if (self->frame == 0) {
        prim = launch(self, 15);
        if (!prim)
            return;
        prim->duration = 9999;
        prim->angle[0] = 0;
        prim->angle[2] = 90.0f;
        prim->size[1].value = 4.0f;
        prim->size[2].value = 4.0f;
        prim->r = 50;
        prim->b = 253;
        prim->g = 50;
        one_texture(prim, "effect\\party.tga");
    }
    place_above_ground(&self->base.pos, 2.0f);
}

void rag_poison_hit(struct RagEffect *self)
{
    struct EffectPrim *prim;

    if (self->frame != 0)
        return;
    prim = launch(self, 5);
    if (!prim)
        return;
    prim->duration = self->duration;
    prim->scale = 1.5f;
    prim->motion_period = 2;
    prim->pattern |= 0x200;
    prim->loop_motion = 0;
    sprite(prim, FX "PoisonHit.spr", FX "PoisonHit.act");
    prim_sync_particles(prim);
}

void rag_ready_portal_rings(struct RagEffect *self)
{
    struct EffectPrim *prim;

    if (self->frame % 14 != 0)
        return;
    rag_play_wave_here(self, "effect\\EF_ReadyPortal.wav");
    prim = launch(self, 12);
    if (!prim)
        return;
    prim->duration = 30;
    prim->delta2.y = -1.0f;
    prim->radius[0] = 8.0f;
    prim->angle[1] = 90.0f;
    mat_set_rotate_x(prim->matrix, deg_to_rad((int)prim->angle[1]));
    prim->size[0].speed = 1.0f;
    prim->size[0].accel = prim->size[0].speed / (float)prim->duration * -0.5f;
    prim->fade_frame = prim->duration - prim->duration / 2;
    prim_sync_particles(prim);
    one_texture(prim, "effect\\ring_blue.tga");
}

void rag_yellow_pillar(struct RagEffect *self)
{
    struct EffectPrim *prim;

    if (self->frame % 25 != 0)
        return;
    prim = launch(self, 13);
    if (!prim)
        return;
    prim->duration = 60;
    prim->angle_speed[0] = 1.5f;
    prim->angle_accel[0] = 0.03f;
    prim->radius[1] = 10.5f;
    prim->radius_speed[0] = -0.12f;
    prim->radius_accel[0] = 0.003f;
    prim->radius_speed[1] = -0.12f;
    prim->radius_accel[1] = 0.003f;
    prim->radius[0] = 7.5f;
    prim->size[2].value = 7.5f;
    prim->alpha = 240.0f;
    prim->fade_frame = prim->duration - prim->duration / 2;
    if (self->frame == 0) {
        prim->duration = self->duration;
        prim->radius_speed[0] = 0.0f;
        prim->radius_accel[0] = 0.0f;
        prim->angle_speed[0] = 1.5f;
        prim->radius[1] = 11.0f;
        prim->radius[0] = 7.5f;
        prim->radius_speed[1] = 0.0f;
        prim->radius_accel[1] = 0.0f;
        prim->fade_frame = prim->duration - prim->duration / 10;
    }
    one_texture(prim, "effect\\ring_yellow.tga");
}

void rag_stone_curse(struct RagEffect *self)
{
    struct EffectPrim *prim;

    project_master(self);
    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\EF_StoneCurse.wav");
    prim = launch(self, 4);
    if (!prim)
        return;
    prim->duration = self->duration;
    prim->fade_frame = 0;
    prim->delta2.y = -20.0f;
    prim->arc_degrees = 10.0f;
    prim->radius[0] = 10.0f;
    prim->size[0].speed = 7.0f;
    prim->size[0].accel = prim->size[0].speed / (float)(prim->duration + 40) * -2.0f;
    one_texture(prim, "effect\\alpha_center.tga");
}

void rag_teleport_ring(struct RagEffect *self)
{
    struct EffectPrim *prim;

    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\EF_Teleportation.wav");
    prim = launch(self, 13);
    if (!prim)
        return;
    prim->duration = 60;
    prim->pattern |= 1;
    prim->radius[1] = 4.0f;
    prim->radius[0] = 4.0f;
    prim->size[2].speed = 5.0f;
    prim->size[2].accel = 0.25f;
    prim->angle_speed[0] = 2.0f;
    prim->fade_frame = 0;
    one_texture(prim, "effect\\ring_blue.tga");
}

static void rising_spark(struct RagEffect *self)
{
    struct EffectPrim *prim = launch(self, 5);
    if (!prim)
        return;

    prim->duration = 10;
    mat_set_rotate_y(prim->matrix, deg_to_rad(rag_rand_mod(360)));
    mat_set_rotate_x(prim->matrix, deg_to_rad(90));
    prim->speed = (float)(rag_rand_mod(20) + 20) / 80.0f;
    prim->accel = prim->speed / (float)prim->duration * -0.5f;
    prim->scale = 0.8f;
    prim->angle_speed[2] = 3.0f;
    prim->alpha = 70.0f;
    prim->alpha_speed = prim->alpha_max / 10.0f;
    prim->fade_frame = prim->duration - prim->duration / 5;
    sprite(prim, FX "particle3.spr", FX "particle3.act");
    prim_sync_particles(prim);
}

void rag_rising_sparks_every5(struct RagEffect *self)
{
    if (self->frame % 5 == 0)
        rising_spark(self);
}

void rag_thunderstorm_sprite(struct RagEffect *self)
{
    struct EffectPrim *prim;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\EF_ThunderStorm.wav");
    prim = launch(self, 5);
    if (!prim)
        return;
    prim->duration = self->duration;
    prim->list_flags |= 0x200;
    prim->pattern |= 0x200;
    prim->delta2.y = -4.5f;
    prim->motion_period = 2;
    prim->scale = 2.5f;
    sprite(prim, FX "썬더스톰.spr", FX "썬더스톰.act");
    prim_sync_particles(prim);
}

void rag_lock_on_ground(struct RagEffect *self)
{
    struct EffectPrim *prim;

    rag_follow_master(self);
    if (self->frame == 9998)
        self->frame = 0;
    if (self->frame == 0) {
        prim = launch(self, 15);
        if (!prim)
            return;
        prim->duration = 9999;
        prim->angle[0] = 0;
        prim->angle_speed[0] = 4.5f;
        prim->angle[2] = 90.0f;
        prim->size[1].value = 4.0f;
        prim->size[2].value = 4.0f;
        if (self->hit_count != 11) {
            prim->r = 250;
            prim->b = (10 - self->hit_count) * 25;
            prim->g = (10 - self->hit_count) * 25;
        } else {
            prim->r = 0;
            prim->b = 0;
            prim->g = 250;
        }
        one_texture(prim, "effect\\LockOn128.tga");
    }
    place_above_ground(&self->base.pos, 2.0f);
}

void rag_torch(struct RagEffect *self)
{
    struct EffectPrim *prim;
    float scale;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    prim = launch(self, 5);
    if (!prim)
        return;
    prim->pattern |= 0x200;
    prim->duration = self->duration;
    prim->delta2.y = -10.0f;
    scale = RAG_SPRITE_SCALE(self);
    if (scale <= 0.0f) {
        RAG_SPRITE_SCALE(self) = 1.0f;
        scale = 1.0f;
    }
    if (RAG_SPRITE_PERIOD(self) < 1.0f)
        RAG_SPRITE_PERIOD(self) = 1.0f;
    prim->scale = scale;
    prim->motion_period = (int)RAG_SPRITE_PERIOD(self);
    sprite(prim, FX "torch_01.spr", FX "torch_01.act");
    prim_sync_particles(prim);
}
