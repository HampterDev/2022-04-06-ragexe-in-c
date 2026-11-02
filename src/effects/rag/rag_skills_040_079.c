#include "game/context.h"
#include <stdio.h>
#include "effects/rag/rag_skills_classic.h"

#define PARTICLE1_SPR SPR_DIR "particle1.spr"
#define PARTICLE1_ACT SPR_DIR "particle1.act"

#define GAME_MSG_QUAKE 0x7a

unsigned char rag_signum(struct RagEffect *self)
{
    struct EffectPrim *p;
    int i;

    rag_project_master(self);
    if (self->frame == 30) {
        rag_play_two_waves(self, "effect\\EF_Signum.wav", "effect\\EF_Bash.wav");
        p = self->last_prim = rag_launch_prim(self, 4, 0.0f, 0.0f, 0.0f);
        if (!p)
            return 0;
        p->duration = 40;
        p->pattern |= 1;
        p->delta2.y = -40.0f;
        p->size[0].value = 100.0f;
        p->alpha = 0.0f;
        p->alpha_max = 170.0f;
        p->alpha_speed = p->alpha_max / 6.0f;
        p->fade_frame = p->duration - p->duration / 3;
        prim_set_texture(p, "effect\\alpha_down.tga");
        for (i = 0; i < 20; i++) {
            p = self->last_prim = rag_launch_prim(self, 3, 0.0f, 0.0f, 0.0f);
            if (!p)
                return 0;
            p->duration = 35;
            p->delta2.y = -40.0f;
            p->alpha = 0.0f;
            p->angle[0] = (float)rnd(360);
            p->angle_speed[0] = (float)(rnd(60) + 10) / 10.0f;
            p->angle_accel[0] = p->angle_speed[0] / (float)p->duration / -1.5f;
            p->arc_degrees = (float)(rnd(25) + 15) / 10.0f;
            p->size[2].value = (float)(rnd(60) + 30);
            p->size[2].speed = (float)(rnd(35) + 95) / 10.0f;
            p->alpha_max = 200.0f;
            p->alpha_speed = p->alpha_max / 10.0f;
            prim_set_texture(p, "EFFECT\\alpha_center.tga");
        }
    }
    return rag_str_step(self, 0.0f, -80.0f, 0.0f) != 0;
}

void rag_blessing(struct RagEffect *self)
{
    struct EffectPrim *p;

    self->base.pos = self->master->pos;
    if (self->frame == 0) {
        rag_play_wave_here(self, "effect\\EF_Blessing.wav");
        p = self->last_prim = rag_launch_prim(self, 12, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = self->duration;
        p->pattern |= 1;
        p->list_flags |= 0x200;
        p->size[0].value = 10.0f;
        p->angle[1] = 90.0f;
        p->r = 32;
        p->g = 176;
        p->b = 232;
        p->alpha = 0.0f;
        p->alpha_max = 100.0f;
        p->alpha_speed = p->alpha_max / 30.0f;
        p->fade_frame = p->duration - 30;
        prim_sync_particles(p);
        prim_set_texture(p, "effect\\alpha_down.tga");
    }
    if (self->frame % 3 == 0 && self->frame < 10) {
        p = self->last_prim = rag_launch_prim(self, 5, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = self->duration;
        p->motion_period = 2;
        p->alpha = (float)(200 - self->frame * 6);
        p->delta2.x = 0.0f;
        p->delta2.y = -25.0f;
        p->delta2.z = 0.0f;
        prim_set_sprite(p, SPR_DIR "축복.spr", SPR_DIR "축복.act");
        prim_sync_particles(p);
    }
    if (self->frame % 4 != 0)
        return;
    p = self->last_prim = rag_launch_prim(self, 5, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 80;
    p->pattern |= 0x80;
    mat_set_rotate_y(p->matrix, deg_to_rad(rnd(360)));
    p->delta2 = mat_point(p->matrix, 0.0f, -25.0f, (float)(rnd(7) + 2));
    mat_set_rotate_x(p->matrix, deg_to_rad(-90));
    p->speed = (float)(rnd(20) + 20) / 100.0f;
    p->accel = p->speed / (float)p->duration * -0.5f;
    p->alpha = 0.0f;
    p->scale = 0.5f;
    p->fade_frame = p->duration - p->duration / 5;
    p->alpha_speed = p->alpha_max / 10.0f;
    render_object_load_act_res(&p->base, SPR_DIR "particle6.act");
    render_object_load_spr_res(&p->base, SPR_DIR "particle6.spr");
    prim_sync_particles(p);
}

void rag_inc_agi_dex(struct RagEffect *self)
{
    struct EffectPrim *p;

    rag_project_master(self);
    self->base.pos = self->master->pos;
    if (self->frame == 0) {
        rag_play_wave_here(self, "effect\\EF_IncAgiDex.wav");
        agility_picture(self, "effect\\dex_agi_up.bmp", 0);
    }
    if (self->frame & 1)
        return;
    p = agility_flake(self, 90, 0.0f);
    if (!p)
        return;
    p->speed = (float)(rnd(50) + 20) / 100.0f;
    agility_flake_look(p, 0);
}

void rag_firefly(struct RagEffect *self)
{
    struct EffectPrim *p;
    Vec3 v;

    if (self->frame != 0)
        return;
    p = self->last_prim = rag_launch_prim(self, 5, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->pattern |= 4;
    p->pattern |= 0x80;
    p->duration = 140;
    self->duration = 140;
    p->speed = (float)(rnd(35) + 15) / 100.0f;
    p->angle[0] = (float)rnd(360);
    p->angle[1] = (float)rnd(360);
    prim_turn_then_tilt(p);
    v = mat_point(p->matrix, 0.0f, 0.0f, (float)(rnd(13) + 2));
    p->delta2.x = v.x + 0.0f;
    p->delta2.y = v.y - 10.0f;
    p->delta2.z = v.z + 0.0f;
    p->pattern_arg = rnd(25) + 5;
    p->scale = (float)(rnd(70) + 30) / 100.0f;
    p->alpha = 0.0f;
    p->alpha_max = 80.0f;
    p->alpha_speed = p->alpha_max / 1.5f;
    p->alpha_keys_set_alpha = 1;
    p->alpha_key = 0;
    prim_add_alpha_key(p, 40.0f, 120.0f, 200.0f);
    prim_add_alpha_key(p, 100.0f, 0.0f, 80.0f);
    prim_set_sprite(p, PARTICLE1_SPR, PARTICLE1_ACT);
    prim_sync_particles(p);
}

void rag_sand_wind(struct RagEffect *self)
{
    struct EffectPrim *p;

    rag_project_master(self);
    if (self->frame >= 60 || (self->frame & 1))
        return;
    p = self->last_prim = rag_launch_prim(self, 0, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->pattern = 8;
    p->duration = self->duration - rnd(100);
    p->speed = (float)(rnd(300) + 260) / 100.0f;
    p->angle[0] = (float)(rnd(15) + 130);
    p->size[1].value = 30.0f;
    p->size[2].value = 30.0f;
    p->delta2.x = (float)(rnd(640) - 500);
    p->delta2.y = (float)(rnd(50) - 240);
    p->pattern_arg = rnd(10) + 10;
    p->alpha = 128.0f;
    p->fade_frame = p->duration - p->duration / 3;
    p->motion_count = 1;
    prim_set_texture(p, "effect\\sand1.bmp");
    self->last_prim->variant = 0;
}

void rag_spray_pond(struct RagEffect *self)
{
    struct EffectPrim *p;
    Vec3 spot;
    int angle;

    if (self->frame == 0) {
        for (angle = 0; angle < 360; angle += 45) {
            p = self->last_prim = rag_launch_prim(self, 15, 0.0f, 0.0f, 0.0f);
            if (!p)
                return;
            p->pattern = 2;
            p->duration = 130;
            mat_set_rotate_y(p->matrix, deg_to_rad((int)((float)angle + 22.5f)));
            spot = mat_point(p->matrix, 0.0f, 0.0f, 12.2f);
            p->delta2.x = spot.x + 0.0f;
            p->delta2.y = spot.y - 2.0f;
            p->delta2.z = spot.z + 0.0f;
            mat_set_rotate_y(p->matrix2, deg_to_rad((int)((float)(angle + 90) + 22.5f)));
            p->alpha = 120.0f;
            p->size[1].value = 5.0f;
            p->size[2].value = 2.7f;
            prim_set_texture(p, "effect\\pond_water.bmp");
        }
    }
    if (self->frame % 10 == 0) {
        for (angle = 0; angle < 360; angle += 45) {
            p = self->last_prim = rag_launch_prim(self, 15, 0.0f, 0.0f, 0.0f);
            if (!p)
                return;
            p->duration = 30;
            mat_set_rotate_y(p->matrix, deg_to_rad((int)((float)angle + 22.5f)));
            p->delta2 = mat_point(p->matrix, 0.0f, 0.0f, 13.8f);
            mat_set_rotate_z(p->matrix2, deg_to_rad(90));
            mat_rotate_y(p->matrix2, deg_to_rad((int)((float)(angle + 90) + 22.5f)));
            p->alpha = 100.0f;
            p->fade_frame = 0;
            p->size[1].value = 5.5f;
            p->size[2].value = 1.0f;
            prim_set_texture(p, "effect\\alpha_down.tga");
        }
    }
    if (self->frame % 20 != 0)
        return;
    p = self->last_prim = rag_launch_prim(self, 12, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 25;
    p->size[0].value = 13.0f;
    p->arc_degrees = 10.0f;
    p->scale = 1.25f;
    p->angle[1] = 90.0f;
    mat_set_rotate_x(p->matrix, deg_to_rad((int)p->angle[1]));
    p->size[0].speed = 0.25f;
    p->size[0].accel = -(p->size[0].speed / (float)p->duration);
    p->alpha = 100.0f;
    p->fade_frame = 0;
    prim_sync_particles(p);
    prim_set_texture(p, "effect\\alpha_center.tga");
}

void rag_cold_hit(struct RagEffect *self)
{
    struct EffectPrim *p;
    float shard_angle, speed, dist;
    int i, deg;

    if (self->frame == 0) {
        i = 0;
        shard_angle = 0.0f;
        do {
            p = self->last_prim = rag_launch_prim(self, 2, 0.0f, 0.0f, 0.0f);
            if (!p)
                return;
            p->duration = 15;
            p->angle[2] = p->angle[0] = (float)rnd(60) + (shard_angle - 15.0f);
            if (i % 3 == 0) {
                p->size[2].value = (float)(rnd(10) + 100);
                p->size[1].value = 10.0f;
                speed = (float)((double)(float)(rnd(45) / 10) + 12.8);
            } else {
                p->size[2].value = (float)(rnd(5) + 45);
                p->size[1].value = 4.0f;
                speed = (float)((double)(float)(rnd(45) / 10) + 4.8);
            }
            p->speed = speed;
            p->accel = (float)((double)-(p->speed / (float)p->duration) / 1.5);
            dist = (float)rnd(5);
            deg = (int)shard_angle;
            p->delta2.x = get_sin_deg(deg) * dist;
            p->delta2.y = -(get_cos_deg(deg) * dist);
            p->delta2.y = p->delta2.y - 20.0f;
            prim_tilt_then_turn(p);
            memcpy(p->matrix2, p->matrix, sizeof(p->matrix2));
            p->pattern_arg = 20;
            p->alpha = 200.0f;
            p->fade_frame = p->duration - 10;
            prim_set_texture(p, "effect\\ice.tga");
            i++;
            shard_angle = shard_angle + 40.0f;
        } while (360.0f > shard_angle);
    }
    rag_project_master(self);
    if (self->frame != 0 && self->frame != 7)
        return;
    p = self->last_prim = rag_launch_prim(self, 0, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 25;
    p->speed = 0.85f;
    p->angle[0] = (float)(rnd(40) - 20);
    p->size[1].value = 1.0f;
    p->size[2].value = 1.0f;
    p->next_size1_speed = 0.5f;
    p->next_size2_speed = 0.5f;
    p->size[2].speed = 6.0f;
    p->size[1].speed = 6.0f;
    p->change_frame = 10;
    p->fade_frame = p->duration - p->duration / 3;
    prim_set_texture(p, "effect\\smoke.tga");
    self->last_prim->variant = 0;
}

void rag_ice_wall(struct RagEffect *self)
{
    struct EffectPrim *p;
    float sin_a, cos_a;
    int side, angle;

    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\wizard_icewall.wav");
    self->duration = 9999;
    for (side = -2; side < 4; side += 2) {
        p = self->last_prim = rag_launch_prim(self, 9, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = self->duration;
        p->pattern |= 1;
        angle = rnd(360);
        sin_a = get_sin_deg(angle);
        cos_a = get_cos_deg(angle);
        p->delta2.y = 30.0f;
        p->delta2.x = sin_a * 0.0f;
        p->delta2.z = -((float)side * cos_a);
        p->scale = (float)(rnd(40) / 100) + 2.5f;
        p->size[2].value = (float)(rnd(100) + 300) / 10.0f;
        p->angle[0] = (float)rnd(360);
        p->angle[1] = (float)(rnd(6) + 87);
        p->pattern_arg = 20;
        p->alpha = 200.0f;
        prim_tilt_then_turn(p);
        memcpy(p->matrix2, p->matrix, sizeof(p->matrix2));
        p->speed = 3.0f;
        p->accel = p->speed / 40.0f * -2.0f;
        p->fade_frame = p->duration - 10;
        prim_set_texture(p, "effect\\ice.tga");
    }
}

void rag_earth_spike(struct RagEffect *self, char variant)
{
    struct EffectPrim *p;
    const char *texture;
    char wave[52];
    float quake_a, quake_b, quake_c, sin_a, cos_a, yaw;

    int angle, jitter, sector_end;

    if (self->frame != 0)
        return;
    if (variant == 0) {
        rag_play_wave_here(self, "effect\\wizard_earthspike.wav");
        texture = "effect\\stone.bmp";
    } else if (variant == 2) {
        sprintf(wave, "effect\\EF_IceArrow%d.wav", rnd(3) + 1);
        rag_play_wave_here(self, wave);
        texture = "effect\\ice.tga";
    } else {
        texture = NULL;
    }
    game_send_message(GAME_MSG_QUAKE, 1, 0, 0, 0);
    quake_a = 1.0f;
    quake_b = 0.24f;
    quake_c = 0.2f;
    game_send_message(0x87, (intptr_t)&quake_a, (intptr_t)&quake_b, (intptr_t)&quake_c, 0);

    p = self->last_prim = rag_launch_prim(self, 9, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 240;
    p->pattern = 1;
    p->pattern |= 2;
    rnd(360);
    rand();
    p->delta2.x = 0.0f;
    p->delta2.y = 10.0f;
    p->delta2.z = 0.0f;
    p->scale = (float)(rnd(50) + 300) / 100.0f;
    p->size[2].value = 18.0f;
    p->angle[0] = (float)rnd(360);
    p->angle[1] = (float)(rnd(20) + 80);
    prim_tilt_then_turn(p);
    memcpy(p->matrix2, p->matrix, sizeof(p->matrix2));
    p->speed = 1.0f;
    p->accel = 0.01f;
    p->alpha = 250.0f;
    p->pattern_arg = 15;
    p->change_frame = 12;
    p->next_speed = -1.2f;
    p->next_accel = 0.0f;
    p->fade_frame = p->duration - 10;
    prim_set_texture(p, texture);

    for (sector_end = 60; sector_end < 420; sector_end += 60) {
        jitter = sector_end != 0 ? rnd(sector_end) : 0;
        p = self->last_prim = rag_launch_prim(self, 9, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = 240;
        p->pattern |= 1;
        angle = rnd(360);
        rand();
        sin_a = get_sin_deg(angle);
        cos_a = get_cos_deg(angle);
        p->delta2.y = 20.0f;
        p->delta2.x = sin_a * 3.5f;
        p->delta2.z = cos_a * -3.5f;
        p->scale = (float)(rnd(100) + 400) / 100.0f;
        p->size[2].value = 20.0f;
        yaw = (float)(sector_end - 60 + jitter);
        p->angle[0] = 360.0f < yaw ? 360.0f : yaw;
        p->angle[1] = 100.0f;
        prim_tilt_then_turn(p);
        memcpy(p->matrix2, p->matrix, sizeof(p->matrix2));
        p->speed = 1.5f;
        p->accel = 0.0f;
        p->alpha = 250.0f;
        p->pattern_arg = 2;
        p->fade_frame = p->duration - 10;
        prim_set_texture(p, texture);
    }
}

static void casting_ring(struct PrimRing *ring, short alpha, short turn, short span, float height,
                         float radius, short tilt)
{
    ring->active = 1;
    ring->age = 0;
    ring->alpha = alpha;
    ring->span = span;
    ring->turn = turn;
    ring->height = height;
    ring->radius = radius;
    ring->tilt = tilt;
    ring->offset_flag = 0;
}

void rag_casting_rings(struct RagEffect *self, const char *texture, char style)
{
    struct EffectPrim *p;
    struct PrimRing *rings;

    if (self->duration < 70)
        self->duration = 70;
    self->base.pos = self->master->pos;
    if (style == 0x21) {
        style = 0;
        p = self->last_prim = rag_launch_prim(self, 25, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
    } else {
        p = self->last_prim = rag_launch_prim(self, 23, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
    }
    p->list_flags = 5;
    p->duration = self->duration;
    prim_set_texture(p, texture);
    p->scale = (float)style;
    rings = p->records.rings;
    casting_ring(&rings[0], 180, 0, 315, 25.0f, 4.5f, 70);
    casting_ring(&rings[1], 180, 90, 315, 22.0f, 5.0f, 57);
    casting_ring(&rings[2], 180, 180, 315, 19.0f, 5.5f, 45);
    casting_ring(&rings[3], 70, 0, 360, 250.0f, 4.0f, 89);
}

static void lock_on_cross(struct RagEffect *self, float yaw)
{
    struct EffectPrim *p = self->last_prim = rag_launch_prim(self, 15, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;

    p->duration = self->duration;
    p->pattern |= 0x800;
    p->angle[0] = yaw;
    p->angle_speed[0] = 4.5f;
    p->angle[2] = 90.0f;
    p->next_size1_accel = 0.0f;
    p->next_size2_accel = 0.0f;
    p->size[1].value = 30.0f;
    p->size[2].value = 30.0f;
    p->size[1].speed = -1.0f;
    p->size[2].speed = -1.0f;
    p->change_frame = 20;
    p->next_size1_speed = 0.0f;
    p->next_size2_speed = 0.0f;
    p->color_hold_frames = 20;
    p->r = 250;
    p->b = 150;
    p->b_speed = -15;
    p->g = 150;
    p->g_speed = -15;
    prim_set_texture(p, "effect\\LockOn128.tga");
}

void rag_lock_on(struct RagEffect *self)
{
    void *attr = game_current_map_attr();
    if (!self->master || !attr)
        return;
    self->base.pos = self->master->pos;
    if (self->frame == 0) {
        lock_on_cross(self, 0.0f);
        lock_on_cross(self, 45.0f);
    }
    self->base.pos.y = c3d_attr_get_height(attr, self->base.pos.x,
                                  self->base.pos.z) - 2.0f;
}

void rag_warp_zone(struct RagEffect *self)
{
    struct EffectPrim *p;

    if (self->frame == 145)
        self->frame = 0;
    if (self->frame == 0) {
        p = self->last_prim = rag_launch_prim(self, 12, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = 158;
        p->pattern |= 1;
        p->size[0].value = 15.0f;
        p->delta2.y = p->delta2.y - 1.0f;
        p->angle[1] = 90.0f;
        p->alpha = 0.0f;
        p->alpha_max = 128.0f;
        p->alpha_speed = p->alpha_max / 10.0f;
        p->fade_frame = p->duration - 10;
        prim_sync_particles(p);
        prim_set_texture(p, "effect\\alpha_down.tga");
    }
    if (self->frame % 28 == 0 && self->frame < 117) {
        p = self->last_prim = rag_launch_prim(self, 12, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = 80;
        p->radius[0] = 4.0f;
        p->angle[1] = 90.0f;
        p->delta2.y = p->delta2.y - 1.0f;
        mat_set_rotate_x(p->matrix, deg_to_rad((int)p->angle[1]));
        p->size[0].value = 14.0f;
        p->size[0].speed = -0.15f;
        p->alpha = 5.0f;
        p->alpha_max = 200.0f;
        p->alpha_speed = 10.0f;
        alpha_key_push(&p->alpha_key_frames, 20.0f);
        alpha_key_push(&p->alpha_key_values, 0.0f);
        p->fade_frame = p->duration - p->duration / 5;
        prim_sync_particles(p);
        prim_set_texture(p, "effect\\ring_blue.tga");
    }
    if (self->frame % 10 != 0)
        return;
    p = self->last_prim = rag_launch_prim(self, 7, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->size[0].value = (float)(rnd(200) / 100) * 0.75f + 5.0f;
    p->size[0].speed = 0.0005f;
    p->duration = 130;
    p->angle[0] = (float)rnd(360);
    rand();
    p->angle_speed[0] = 2.0f;
    p->fall_speed = -0.2f;
    p->gravity = p->fall_speed / (float)p->duration / -1.5f;
    mat_set_rotate_y(p->matrix, deg_to_rad((int)p->angle[0]));
    p->delta2 = mat_point(p->matrix, 0.0f, 0.0f, p->size[0].value);
    p->scale = (float)(rnd(10) / 10) + 0.7f;
    p->fade_frame = p->duration - p->duration / 3;
    prim_set_sprite(p, PARTICLE1_SPR, PARTICLE1_ACT);
    prim_sync_particles(p);
}

void rag_sightrasher(struct RagEffect *self)
{
    struct EffectPrim *p;
    float sin_a, cos_a, dist;
    int i, angle;

    rag_project_master(self);
    if (self->frame == 0) {
        render_object_send(self->master, 0, 0x6d, 0x16, 0, 0, 0);
        rag_play_wave_here(self, "effect\\wizard_sightrasher.wav");
    }
    if (self->frame % 5 == 0 && self->frame < 20) {
        for (i = 0; i < 8; i++) {
            angle = (int)((float)i * -45.0f);
            sin_a = get_sin_deg(angle);
            cos_a = get_cos_deg(angle);
            deg_to_rad(angle);
            p = self->last_prim = rag_launch_prim(self, 5, 0.0f, 0.0f, 0.0f);
            if (!p)
                return;
            p->duration = 85 - self->frame;
            p->pattern |= 0x400;
            dist = (float)(self->frame / 5) + 5.0f;
            p->delta2.x = dist * sin_a;
            p->delta2.z = -(dist * cos_a);
            p->delta2.y = (float)(self->frame / 3 - 20);
            p->velocity.y = 0.0f;
            p->velocity.x = (float)(double)sin_a;
            p->velocity.z = (float)((double)cos_a * -1.0);
            p->motion_period = rnd(2) + 3;
            p->scale = 2.5f - (float)self->frame / 10.0f;
            p->alpha = (float)((75 - self->frame) * 2);
            p->velocity_accel.x = 0.0f;
            p->velocity_accel.y = 0.0f;
            p->velocity_accel.z = 0.0f;
            p->next_velocity.x = (float)((double)sin_a * 0.5);
            p->next_velocity.y = 0.0f;
            p->next_velocity.z = (float)((double)cos_a * -0.5);
            p->change_frame = 35 - self->frame;
            p->next_velocity_accel.x = -0.004f;
            p->next_velocity_accel.y = 0.0f;
            p->next_velocity_accel.z = 0.004f;
            p->fade_frame = 65 - self->frame;
            prim_set_sprite(p, SPR_DIR "Sight.spr", SPR_DIR "Sight.act");
            prim_sync_particles(p);
        }
        for (i = 0; i < 8; i++) {
            angle = (int)((float)i * -45.0f);
            sin_a = get_sin_deg(angle);
            cos_a = get_cos_deg(angle);
            deg_to_rad(angle);
            p = self->last_prim = rag_launch_prim(self, 5, 0.0f, 0.0f, 0.0f);
            if (!p)
                return;
            p->duration = 85 - self->frame;
            p->pattern |= 0x400;
            dist = (float)(self->frame / 5) + 5.0f;
            p->delta2.y = 0.0f;
            p->delta2.z = -(dist * cos_a);
            p->delta2.x = dist * sin_a;
            p->velocity.y = 0.0f;
            p->velocity.x = (float)(double)sin_a;
            p->velocity.z = (float)((double)cos_a * -1.0);
            p->motion_period = 3;
            p->scale = 1.0f - (float)self->frame / 30.0f;
            p->alpha = (float)((75 - self->frame) * 2);
            p->next_velocity.x = (float)((double)sin_a * 0.5);
            p->next_velocity.z = (float)((double)cos_a * -0.5);
            p->change_frame = 35 - self->frame;
            p->next_velocity_accel.x = -0.004f;
            p->next_velocity_accel.z = 0.004f;
            p->fade_frame = 65 - self->frame;
            prim_set_sprite(p, "Shadow.spr", "Shadow.act");
            prim_sync_particles(p);
        }
    }
    if (self->frame != 10)
        return;
    p = self->last_prim = rag_launch_prim(self, 12, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 30;
    p->angle[1] = 90.0f;
    p->size[0].speed = 2.5f;
    p->size[0].accel = p->size[0].speed / (float)p->duration / -1.5f;
    p->radius[0] = -10.0f;
    p->alpha = 150.0f;
    p->fade_frame = p->duration - 10;
    prim_sync_particles(p);
    prim_set_texture(p, "effect\\ring_yellow.tga");
}

void rag_sphere(struct RagEffect *self)
{
    struct EffectPrim *p;

    self->base.pos = self->master->pos;
    if (self->frame % 10 != 0)
        return;
    p = self->last_prim = rag_launch_prim(self, 7, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    self->duration = 500 - self->frame;
    p->duration = 300;
    p->size[0].value = 0.1f;
    p->size[0].speed = 0.15f;
    p->fall_speed = 0.15f;
    p->angle[0] = 2.0f;
    p->angle_speed[0] = 8.0f;
    p->angle_accel[0] = -0.01f;
    p->scale = 1.2f;
    mat_set_rotate_y(p->matrix, deg_to_rad((int)p->angle[0]));
    p->delta2 = mat_point(p->matrix, 0.0f, -30.0f, p->size[0].value);
    p->alpha_keys_set_size = 1;
    p->alpha_key = 0;
    alpha_key_push(&p->alpha_key_frames, 95.0f);
    alpha_key_push(&p->alpha_key_values, -0.15f);
    vec_push(&p->alpha_key_ceilings, &p->size[0].accel, sizeof p->size[0].accel);
    prim_set_sprite(p, PARTICLE1_SPR, PARTICLE1_ACT);
    prim_sync_particles(p);
}

void rag_bowling_bash(struct RagEffect *self)
{
    struct EffectPrim *p;
    int facing;

    if (self->frame == 0 || self->frame == 5) {
        facing = master_facing(self);
        p = self->last_prim = rag_launch_prim(self, 13, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = 20 - self->frame;
        p->angle[0] = (float)(facing + self->frame * 20);
        prim_tilt_then_turn(p);
        p->radius[1] = 8.0f;
        p->radius_speed[1] = 0.5f;
        p->radius_accel[1] = -0.03f;
        p->radius[0] = 3.0f;
        p->radius_speed[0] = 0.5f;
        p->radius_accel[0] = -0.03f;
        p->size[2].value = 3.5f;
        p->alpha = (float)(240 - self->frame * 7);
        p->fade_frame = p->duration - p->duration / 2;
        prim_set_texture(p, "effect\\ring_blue.tga");
    }
    if (self->frame != 0)
        return;
    p = self->last_prim = rag_launch_prim(self, 12, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 50;
    p->list_flags |= 0x200;
    p->angle[1] = 90.0f;
    p->size[0].value = 8.0f;
    p->size[0].speed = 0.7f;
    p->size[0].accel = -(p->size[0].speed / 30.0f);
    p->radius[0] = 5.0f;
    p->change_frame = 20;
    p->next_accel = 0.0f;
    p->next_speed = 0.08f;
    p->alpha = 45.0f;
    p->fade_frame = 35;
    prim_sync_particles(p);
    prim_set_texture(p, "effect\\ring_yellow.tga");
}
