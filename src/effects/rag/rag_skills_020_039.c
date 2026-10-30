#include "session/state.h"
#include <stdio.h>
#include "effects/rag/rag_skills_classic.h"
#include "actor/game_actor.h"

#define PARTICLE2_SPR SPR_DIR "particle2.spr"
#define PARTICLE2_ACT SPR_DIR "particle2.act"
#define PARTICLE3_SPR SPR_DIR "particle3.spr"
#define PARTICLE3_ACT SPR_DIR "particle3.act"

static void bubble(struct EffectPrim *p, float scale_div, const char *spr, const char *act)
{
    mat_set_rotate_y(p->matrix, deg_to_rad(rnd(360)));
    p->delta2 = mat_point(p->matrix, 0.0f, 0.0f, (float)(rnd(6) + 2));
    mat_set_rotate_x(p->matrix, deg_to_rad(90));
    p->speed = (float)(rnd(30) + 30) / 100.0f;
    p->accel = p->speed / (float)p->duration * -0.5f;
    p->scale = (float)(rnd(50) + 40) / scale_div;
    p->angle_speed[2] = 3.0f;
    p->alpha = 0.0f;
    p->alpha_speed = p->alpha_max / 10.0f;
    p->fade_frame = p->duration - p->duration / 5;
    prim_set_sprite(p, spr, act);
}

void rag_poison_attack(struct RagEffect *self, char variant)
{
    struct EffectPrim *p;

    if (self->frame == 0 && variant == 0)
        rag_play_two_waves(self, "effect\\assasin_enchantpoison.wav", "effect\\EF_PoisonAttack.wav");
    if (variant == 1) {
        if (self->frame % 9 != 0)
            return;
        p = self->last_prim = rag_launch_prim(self, 5, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = 40;
        bubble(p, 170.0f, PARTICLE3_SPR, PARTICLE3_ACT);
        if (self->effect_id == 772)
            self->last_prim->list_flags = 0x201;
    } else {
        if (self->frame % 5 != 0)
            return;
        p = self->last_prim = rag_launch_prim(self, 5, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = 40;
        bubble(p, 100.0f, PARTICLE3_SPR, PARTICLE3_ACT);
    }
    prim_sync_particles(self->last_prim);
}

void rag_detoxication(struct RagEffect *self)
{
    struct EffectPrim *p;

    if (self->frame % (g_session1.map_rule_flags[0] != 0 ? 15 : 5) != 0)
        return;
    if (self->frame == 0)
        rag_play_wave_here(self, "effect\\EF_Detoxication.wav");
    p = self->last_prim = rag_launch_prim(self, 5, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->alpha = 0.0f;
    p->duration = 40;
    bubble(p, 100.0f, PARTICLE2_SPR, PARTICLE2_ACT);
    prim_sync_particles(p);
}

void rag_sight(struct RagEffect *self)
{
    struct EffectPrim *p;
    float dx, dz;
    int angle;

    self->base.pos = self->master->pos;
    rag_project_master(self);
    if (self->frame & 1)
        return;
    angle = (int)((float)self->frame * -5.0f);
    dx = get_sin_deg(angle);
    dz = get_cos_deg(angle);
    deg_to_rad(angle);

    p = self->last_prim = rag_launch_prim(self, 5, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 20;
    dx = dx * 15.0f;
    dz = dz * -15.0f;
    p->delta2.x = dx;
    p->delta2.y = -20.0f;
    p->delta2.z = dz;
    p->velocity_accel.y = 0.1f;
    p->motion_period = 3;
    p->scale = 2.5f;
    p->scale_speed = -0.1f;
    p->alpha = 150.0f;
    p->alpha_speed = -3.0f;
    prim_set_sprite(p, SPR_DIR "Sight.spr", SPR_DIR "Sight.act");
    prim_sync_particles(p);

    p = self->last_prim = rag_launch_prim(self, 5, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 20;
    p->delta2.x = dx;
    p->delta2.y = 0.0f;
    p->delta2.z = dz;
    p->motion_period = 2;
    p->scale = 1.5f;
    p->scale_speed = -0.06f;
    p->alpha = 120.0f;
    p->alpha_speed = -1.0f;
    prim_set_sprite(p, "Shadow.spr", "Shadow.act");
    prim_sync_particles(p);
}

void rag_ruwach(struct RagEffect *self)
{
    struct EffectPrim *p;
    float dx, dz;
    int angle;

    self->base.pos = self->master->pos;
    if (self->frame % 3 != 0)
        return;
    angle = (int)(float)((double)self->frame / 1.3 * -5.0);
    dx = get_sin_deg(angle);
    dz = get_cos_deg(angle);
    deg_to_rad(angle);

    p = self->last_prim = rag_launch_prim(self, 5, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 25;
    dx = dx * 15.0f;
    p->delta2.x = dx;
    dz = dz * -15.0f;
    p->delta2.y = -10.0f;
    p->delta2.z = dz;
    prim_set_sprite(p, PARTICLE2_SPR, PARTICLE2_ACT);
    p->scale = 3.0f;
    p->scale_speed = -0.1f;
    p->alpha = 250.0f;
    p->alpha_speed = -3.0f;
    p->fade_frame = p->duration - 6;
    prim_sync_particles(p);

    p = self->last_prim = rag_launch_prim(self, 5, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 25;
    p->delta2.x = dx;
    p->delta2.y = 0.0f;
    p->delta2.z = dz;
    prim_set_sprite(p, "Shadow.spr", "Shadow.act");
    p->motion_period = 2;
    p->scale = 1.5f;
    p->scale_speed = -0.06f;
    p->alpha = 150.0f;
    p->alpha_speed = -2.5f;
    p->fade_frame = p->duration - 6;
    prim_sync_particles(p);
}

unsigned char rag_str_only(struct RagEffect *self)
{
    return rag_str_step(self, 0.0f, -80.0f, 0.0f) != 0;
}

void rag_fire_ball(struct RagEffect *self)
{
    struct EffectPrim *p;

    if (self->frame != 20 && self->frame != 24 && self->frame != 28 && self->frame != 32)
        return;
    rag_play_wave_here(self, "effect\\EF_FireBall.wav");
    p = self->last_prim = rag_launch_prim(self, 5, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 20;
    p->pattern |= 0x120;
    p->delta2.x = 0.0f;
    p->delta2.y = -15.0f;
    p->delta2.z = 0.0f;
    prim_aim_at_offset(self, p);
    p->scale = 1.3f;
    p->motion_period = 1;
    if (self->frame != 20) {
        p->r = 246;
        p->g = 199;
        p->b = 76;
        if (self->frame == 24)
            p->alpha = 180.0f;
        else if (self->frame == 28)
            p->alpha = 130.0f;
        else
            p->alpha = 80.0f;
    }
    prim_set_sprite(p, SPR_DIR "fireball.spr", SPR_DIR "fireball.act");
    prim_sync_particles(p);
}

static struct EffectPrim *arrow_launch(struct RagEffect *self)
{
    struct EffectPrim *p = self->last_prim = rag_launch_prim(self, 16, 0.0f, 0.0f, 0.0f);
    if (!p)
        return NULL;
    int ahead_rnd, right_rnd, aim_z_rnd, aim_x_rnd;
    float dir, ground_dist;

    p->duration = 70;
    p->pattern = 0x14;
    ahead_rnd = rnd(10);
    right_rnd = rnd(10);
    p->delta2.y = -60.0f;
    p->delta2.x = (float)(right_rnd + 25);
    p->delta2.z = (float)(ahead_rnd + 15);
    aim_z_rnd = rnd(4);
    aim_x_rnd = rnd(4);
    dir = render_object_calc_dir(p->delta2.x, p->delta2.z, (float)(aim_x_rnd - 2),
                                 (float)(aim_z_rnd - 2));
    p->angle[0] = -dir;
    self->master_roty = -dir;
    ground_dist = float_sqrt(abs_f(p->delta2.x) * abs_f(p->delta2.x) + abs_f(p->delta2.z) * abs_f(p->delta2.z));
    dir = render_object_calc_dir(-abs_f(p->delta2.y), ground_dist, 0.0f, 0.0f);
    p->angle[1] = dir;
    self->pitch = dir;
    prim_tilt_then_turn(p);
    return p;
}

static void arrow_ring(struct RagEffect *self, const char *texture)
{
    struct EffectPrim *p;

    if (self->id_state == 0)
        return;
    self->id_state = 0;
    p = self->last_prim = rag_launch_prim(self, 12, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 30;
    p->angle[1] = 90.0f;
    p->size[0].speed = 1.2f;
    p->size[0].accel = p->size[0].speed / (float)(p->duration + 40) * -2.0f;
    p->radius[0] = 10.0f;
    p->alpha = 0.0f;
    p->alpha_speed = p->alpha_max / 10.0f;
    p->fade_frame = p->duration - 10;
    prim_sync_particles(p);
    prim_set_texture(p, texture);
}

void rag_ice_arrow(struct RagEffect *self)
{
    struct EffectPrim *p;
    char wave[52];

    self->base.pos = self->master->pos;
    if (self->frame == 12) {
        sprintf(wave, "effect\\EF_IceArrow%d.wav", rnd(3) + 1);
        rag_play_wave_here(self, wave);
        self->hit_count *= 10;
    }
    if (self->frame >= 12 && (self->frame - 12) % 10 == 0 && self->frame - 12 < self->hit_count) {
        p = arrow_launch(self);
        if (!p)
            return;
        p->speed = -2.75f;
        p->size[1].value = 11.5f;
        p->size[2].value = 3.8f;
        p->fade_frame = p->duration - 20;
        prim_set_texture(p, "effect\\icearrow.tga");
        self->last_prim->variant = 0;
    }
    arrow_ring(self, "effect\\ring_blue.tga");
}

void rag_fire_arrow(struct RagEffect *self)
{
    struct EffectPrim *p;
    char wave[52];

    self->base.pos = self->master->pos;
    if (self->frame == 12) {
        sprintf(wave, "effect\\EF_FireArrow%d.wav", rnd(3) + 1);
        rag_play_wave_here(self, wave);
        self->hit_count *= 10;
    }
    if (self->frame >= 12 && (self->frame - 12) % 10 == 0 && self->frame - 12 < self->hit_count) {
        p = arrow_launch(self);
        if (!p)
            return;
        p->speed = -2.75f;
        p->size[1].value = 14.0f;
        p->size[2].value = 3.5f;
        p->fade_frame = p->duration - 20;
        p->motion_count = 6;
        p->motion_period = 1;
        prim_alloc_textures(p);
        p->textures[0] = rag_texture("effect\\불화살" "1.tga");
        p->textures[1] = rag_texture("effect\\불화살" "2.tga");
        p->textures[2] = rag_texture("effect\\불화살" "3.tga");
        p->textures[3] = rag_texture("effect\\불화살" "4.tga");
        p->textures[4] = rag_texture("effect\\불화살" "5.tga");
        p->textures[5] = rag_texture("effect\\불화살" "6.tga");
        self->last_prim->variant = 0;
    }
    if (self->frame > 0 && self->frame % 4 == 0) {
        p = self->last_prim = rag_launch_prim(self, 5, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = rnd(24) + 6;
        p->angle[0] = (float)rnd(40) + (self->master_roty - 20.0f);
        p->angle[1] = (float)rnd(20) + (self->pitch - 10.0f);
        prim_tilt_then_turn(p);
        p->delta2 = self->offset;
        p->speed = (float)(rnd(90) + 60) / 100.0f;
        p->accel = p->speed / (float)p->duration / -1.5f;
        p->scale = (float)(rnd(30) + 20) / 100.0f;
        p->scale_speed = -(p->scale / (float)p->duration);
        p->fade_frame = 0;
        prim_set_sprite(p, SPR_DIR "particle4.spr", SPR_DIR "particle4.act");
        prim_sync_particles(p);
    }
    arrow_ring(self, "effect\\ring_yellow.tga");
}

void rag_frost_diver(struct RagEffect *self, char variant)
{
    Vec3 *head = &self->head_offset;
    struct EffectPrim *p;
    float facing_m[12], spread;
    Vec3 saved;
    int angle;

    if (g_session1.map_rule_flags[0] != 0)
        return;
    if (self->frame == 0) {
        render_object_turn_toward(&self->base, self->offset.x, self->offset.z);
        mat_set_rotate_y(facing_m, deg_to_rad((int)-self->base.roty));
        self->offset2 = mat_point(facing_m, 0.0f, 0.0f, 5.0f);
        head->x -= self->offset2.x;
        head->y -= self->offset2.y;
        head->z -= self->offset2.z;
        self->offset2 = mat_point(facing_m, 0.0f, 0.0f, 2.0f);
    }
    if (self->id_state != 0)
        return;

    saved = self->base.pos;
    self->base.pos.x = head->x + self->base.pos.x;
    self->base.pos.y = head->y + self->base.pos.y;
    self->base.pos.z = head->z + self->base.pos.z;
    if (2.5f >= render_object_calc_ground_dist(&self->base, self->offset.x, self->offset.z))
        self->id_state = 1;
    self->base.pos = saved;

    p = self->last_prim = rag_launch_prim(self, 9, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 40;
    self->last_prim->pattern |= 1;
    self->last_prim->records.pyramid.style = variant == 3 ? PYRAMID_STYLE_FLICKER : 0;
    angle = rnd(360);
    spread = (float)(rnd(10) + 5) / 10.0f;
    p = self->last_prim;
    p->delta2.x = get_sin_deg(angle) * spread;
    p->delta2.y = 30.0f;
    p->delta2.z = -(get_cos_deg(angle) * spread);
    p->delta2.x += head->x;
    p->delta2.y += head->y;
    p->delta2.z += head->z;
    p->scale = (float)(rnd(40) + 60) / 100.0f;
    p->size[2].value = (float)(rnd(30) + 150) / 10.0f;
    p->angle[0] = (float)rnd(360);
    p->angle[1] = (float)(rnd(30) + 75);
    prim_tilt_then_turn(p);
    memcpy(p->matrix2, p->matrix, sizeof(p->matrix2));
    p->speed = 3.0f;
    p->accel = p->speed / (float)p->duration * -2.0f;
    p->pattern_arg = 20;
    p->fade_frame = p->duration - 10;
    prim_set_texture(p, "effect\\ice.tga");
    head->x -= self->offset2.x;
    head->y -= self->offset2.y;
    head->z -= self->offset2.z;
}

void rag_frost_diver2(struct RagEffect *self)
{
    struct EffectPrim *p;
    float spread;
    int i, angle;

    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\EF_FrostDiver2.wav");
    for (i = 0; i < 8; i++) {
        p = self->last_prim = rag_launch_prim(self, 9, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = 40;
        p->pattern |= 1;
        angle = rnd(360);
        spread = (float)(rnd(4) + 1) / 10.0f;
        p->delta2.x = get_sin_deg(angle) * spread;
        p->delta2.y = 30.0f;
        p->delta2.z = -(get_cos_deg(angle) * spread);
        p->scale = (float)(rnd(250) + 100) / 100.0f;
        p->size[2].value = (float)(rnd(100) + 200) / 10.0f;
        p->angle[0] = (float)rnd(360);
        p->angle[1] = (float)(rnd(20) + 80);
        prim_tilt_then_turn(p);
        memcpy(p->matrix2, p->matrix, sizeof(p->matrix2));
        p->speed = 3.0f;
        p->accel = p->speed / (float)p->duration * -2.0f;
        p->pattern_arg = 20;
        p->alpha = 200.0f;
        p->fade_frame = p->duration - 10;
        prim_set_texture(p, "effect\\ice.tga");
    }
}

void rag_napalm_beat(struct RagEffect *self)
{
    struct EffectPrim *p;
    char name[52];
    float dist;
    int angle, i;

    rag_project_master(self);
    if (self->frame >= 15)
        return;
    if (self->frame < self->frame % 5)
        return;
    if (self->frame == 0)
        rag_play_wave_here(self, "effect\\EF_NapalmBeat.wav");
    p = self->last_prim = rag_launch_prim(self, 0, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 30;
    dist = (float)(rnd(50) + 10);
    angle = rnd(360);
    self->last_prim->delta2.x = get_sin_deg(angle) * dist;
    self->last_prim->delta2.y = -(get_cos_deg(angle) * dist);
    self->last_prim->delta2.y = self->last_prim->delta2.y - 20.0f;
    p = self->last_prim;
    p->size[1].value = p->size[2].value = (float)(rnd(40) + 20);
    p->alpha = 0.0f;
    p->alpha_speed = p->alpha_max / 10.0f;
    p->fade_frame = p->duration - p->duration / 3;
    p->loop_motion = 0;
    p->motion_period = 2;
    p->motion_count = 8;
    prim_alloc_textures(p);
    for (i = 0; i < self->last_prim->motion_count; i++) {
        sprintf(name, "effect\\폭발%d.TGA", i + 1);
        self->last_prim->textures[i] = rag_texture(name);
    }
    self->last_prim->variant = 0;
}

void rag_portal(struct RagEffect *self)
{
    struct EffectPrim *p;

    if (self->frame == 0) {
        p = self->last_prim = rag_launch_prim(self, 13, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = self->duration;
        p->arc_degrees = 36.0f;
        p->radius[1] = 3.5f;
        p->radius[0] = 3.5f;
        p->angle_speed[0] = 4.0f;
        p->size[2].value = 40.0f;
        p->alpha = 0.0f;
        p->alpha_max = 128.0f;
        p->alpha_speed = p->alpha_max / 6.0f;
        p->fade_frame = p->duration - 6;
        prim_set_texture(p, "effect\\ring_blue.tga");

        p = self->last_prim = rag_launch_prim(self, 13, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = self->duration;
        p->arc_degrees = 36.0f;
        p->radius[1] = 4.0f;
        p->radius[0] = 4.0f;
        p->angle_speed[0] = 5.0f;
        p->size[2].value = 50.0f;
        p->alpha = 0.0f;
        p->alpha_max = 128.0f;
        p->alpha_speed = p->alpha_max / 6.0f;
        p->fade_frame = p->duration - 6;
        prim_set_texture(p, "effect\\ring_blue.tga");
    }
    if (self->frame % 14 != 0)
        return;
    p = self->last_prim = rag_launch_prim(self, 12, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 30;
    p->delta2.y = -1.0f;
    p->radius[0] = 8.0f;
    p->angle[1] = 90.0f;
    mat_set_rotate_x(p->matrix, deg_to_rad((int)p->angle[1]));
    p->size[0].speed = 1.0f;
    p->size[0].accel = p->size[0].speed / (float)p->duration * -0.5f;
    p->fade_frame = p->duration - p->duration / 2;
    prim_sync_particles(p);
    prim_set_texture(p, "effect\\ring_blue.tga");
}

void rag_inc_agility(struct RagEffect *self)
{
    struct EffectPrim *p;

    rag_project_master(self);
    self->base.pos = self->master->pos;
    if (self->frame == 0) {
        rag_play_wave_here(self, "effect\\EF_IncAgility.wav");
        agility_picture(self, "effect\\agi_up.bmp", PRIM_VARIANT_EVERY_FRAME);
    }
    if (self->frame & 1)
        return;
    p = agility_flake(self, 90, 0.0f);
    if (!p)
        return;
    p->speed = (float)(rnd(50) + 20) / 100.0f;
    agility_flake_look(p, PRIM_VARIANT_EVERY_FRAME);
}

void rag_dec_agility(struct RagEffect *self)
{
    struct EffectPrim *p;

    rag_project_master(self);
    self->base.pos = self->master->pos;
    if (self->frame == 0) {
        rag_play_wave_here(self, "effect\\EF_DecAgility.wav");
        p = self->last_prim = rag_launch_prim(self, 0, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = self->duration;
        p->delta2.y = p->delta2.y - 80.0f;
        p->angle[0] = 180.0f;
        p->speed = 1.0f;
        p->accel = -(p->speed / (float)p->duration);
        p->size[1].value = 40.0f;
        p->size[2].value = 10.0f;
        p->alpha = 0.0f;
        p->alpha_max = 200.0f;
        p->alpha_speed = p->alpha_max / 15.0f;
        p->fade_frame = p->duration - 15;
        prim_set_texture(p, "effect\\slow.bmp");
        p->variant = 0;
    }
    if (self->frame & 1)
        return;
    p = agility_flake(self, -90, 20.0f);
    if (!p)
        return;
    p->accel = 0.015f;
    agility_flake_look(p, 0);
}
