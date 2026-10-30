#include "session/state.h"
#include "game/context.h"
#include "effects/rag/rag_skills_classic.h"

#define PARTICLE1_SPR SPR_DIR "particle1.spr"
#define PARTICLE1_ACT SPR_DIR "particle1.act"

#define GAME_MSG_QUAKE 0x7a

int g_blitzBeatRingCount;

void rag_spear_boomerang(struct RagEffect *self)
{
    struct EffectPrim *p;

    if (self->frame != 0 && self->frame != 4 && self->frame != 8 && self->frame != 12)
        return;
    rag_play_wave_here(self, "effect\\EF_FireBall.wav");
    p = self->last_prim = rag_launch_prim(self, 5, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 15;
    p->pattern |= 0x120;
    p->delta2.x = 0.0f;
    p->delta2.y = -15.0f;
    p->delta2.z = 0.0f;
    prim_aim_at_offset(self, p);
    p->scale = 1.0f;
    p->motion_period = 1;
    if (self->frame != 0) {
        if (self->frame == 4)
            p->alpha = 180.0f;
        else if (self->frame == 8)
            p->alpha = 130.0f;
        else
            p->alpha = 80.0f;
    }
    render_object_set_action_base(&self->base, render_object_camera_dir(self->master->roty, 1), 1);
    prim_set_sprite(self->last_prim, SPR_DIR "창.spr", SPR_DIR "창.act");
    prim_sync_particles(self->last_prim);
}

void rag_pierce(struct RagEffect *self)
{
    struct EffectPrim *p;
    int facing, i;

    if (self->frame % 20 != 0 || self->frame >= self->repeat_count * 20)
        return;
    facing = master_facing(self);
    p = self->last_prim = rag_launch_prim(self, 13, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 15;
    p->angle[0] = (float)facing;
    p->angle[1] = -90.0f;
    prim_tilt_then_turn(p);
    p->delta2.x = 0.0f;
    p->delta2.y = -10.0f;
    p->delta2.z = 0.0f;
    p->speed = 0.7f;
    p->accel = p->speed / (float)p->duration * -0.5f;
    p->radius[1] = (float)((double)self->frame * 0.1 + 11.0);
    p->radius[0] = (float)((double)self->frame * 0.1 + 6.0);
    p->size[2].value = 2.5f;
    p->fade_frame = p->duration - p->duration / 2;
    prim_set_texture(p, "effect\\ring_yellow.tga");
    if (self->frame <= 0)
        return;
    for (i = 0; i < 4; i++) {
        p = self->last_prim = rag_launch_prim(self, 5, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->angle[0] = (float)(rnd(80) + (facing - 40));
        p->angle[1] = (float)(rnd(100) - 50);
        prim_burst_out(p);
        p->speed = (float)(rnd(90) + 60) / 100.0f;
        p->duration = rnd(24) + 6;
        p->accel = p->speed / (float)p->duration * -0.5f;
        p->scale = (float)(rnd(100) + 60) / 100.0f;
        p->scale_speed = -(p->scale / (float)p->duration);
        p->fade_frame = 0;
        prim_set_sprite(p, PARTICLE1_SPR, PARTICLE1_ACT);
        p->particle_count = 3;
        prim_sync_particles(p);
    }
    for (i = 0; i < 4; i++) {
        p = self->last_prim = rag_launch_prim(self, 6, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->angle[0] = (float)(rnd(80) + (facing + 140));
        p->angle[1] = (float)(rnd(100) - 50);
        prim_burst_out(p);
        mat_set_rotate_y(p->matrix, deg_to_rad((int)p->angle[0]));
        p->duration = rnd(24) + 6;
        p->speed = (float)(rnd(90) + 60) / 100.0f;
        p->accel = p->speed / (float)p->duration * -0.5f;
        p->fall_speed = (float)(rnd(90) + 30) / -100.0f;
        p->gravity = -(p->fall_speed / (float)p->duration + p->fall_speed / (float)p->duration);
        p->scale = (float)(rnd(100) + 60) / 100.0f;
        p->scale_speed = -(p->scale / (float)p->duration);
        p->fade_frame = 0;
        prim_set_sprite(p, PARTICLE1_SPR, PARTICLE1_ACT);
        p->particle_count = 3;
        prim_sync_particles(p);
    }
}

void rag_turn_undead(struct RagEffect *self)
{
    struct EffectPrim *p;
    float speed_div;
    int n;

    if (self->frame == 37)
        rag_play_wave_here(self, "effect\\EF_Bash.wav");
    rag_project_master(self);
    if (self->frame == 0) {
        p = self->last_prim = rag_launch_prim(self, 4, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->size[0].value = 370.0f;
        p->size[0].speed = -(p->size[0].value / 20.0f);
        p->duration = 20;
        p->pattern |= 1;
        p->delta2.y = -40.0f;
        p->alpha = 0.0f;
        p->alpha_max = 130.0f;
        p->alpha_speed = p->alpha_max / 6.0f;
        p->fade_frame = p->duration - 10;
        prim_set_texture(p, "effect\\alpha_down.tga");
        for (n = self->frame != 0 ? 20 : 5; n != 0; n--) {
            p = self->last_prim = rag_launch_prim(self, 3, 0.0f, 0.0f, 0.0f);
            if (!p)
                return;
            p->duration = 20;
            p->pattern_arg = -1;
            p->velocity.x = self->master->pos.x;
            p->velocity.y = self->master->pos.z;
            p->delta2.y = -40.0f;
            p->angle[0] = (float)rnd(360);
            p->angle_speed[0] = (float)(rnd(60) + 10) / 10.0f;
            p->angle_accel[0] = p->angle_speed[0] / (float)p->duration / -1.5f;
            p->arc_degrees = (float)(rnd(25) + 5) / 10.0f;
            if (self->frame == 0) {
                p->size[2].value = (float)(rnd(30) + 350);
                p->size[2].speed = (float)(rnd(30) + 150) / -10.0f;
            } else {
                p->size[2].value = 0.0f;
                p->size[2].speed = (float)(rnd(20) + 50) / 10.0f;
                p->change_frame = 40;
                p->next_speed = (float)(rnd(10) + 15) / 10.0f;
                p->next_accel = 0.0f;
                p->next_size2_speed = 0.0f;
            }
            speed_div = 10.0f;
            p->alpha = 0.0f;
            p->alpha_max = 150.0f;
            p->alpha_speed = p->alpha_max / speed_div;
            p->fade_frame = p->duration - p->duration / 3;
            prim_set_texture(p, "EFFECT\\alpha_center.tga");
        }
    }
    self->base.pos = self->master->pos;
    if (self->frame != 50)
        return;
    p = self->last_prim = rag_launch_prim(self, 15, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 30;
    p->angle[2] = 90.0f;
    p->size[1].speed = 1.5f;
    p->size[2].speed = 1.5f;
    p->delta2.y = -7.0f;
    p->size[1].accel = p->size[1].speed / 10.0f;
    p->size[2].accel = p->size[2].speed / 10.0f;
    p->alpha = 150.0f;
    p->fade_frame = p->duration - 10;
    p->change_frame = 15;
    p->next_size1_speed = 0.5f;
    p->next_size1_accel = -0.033333335f;
    p->next_size2_speed = 0.5f;
    p->next_size2_accel = -0.033333335f;
    prim_sync_particles(p);
    prim_set_texture(p, "effect\\ring_liner.tga");
}

unsigned char rag_aspersio(struct RagEffect *self)
{
    struct EffectPrim *p;

    if (self->frame == 70)
        rag_play_wave_here(self, "effect\\priest_aspersio.wav");
    if (self->frame % 5 == 0 && self->frame > 60 && self->frame < 140) {
        p = self->last_prim = rag_launch_prim(self, 7, 0.0f, 0.0f, 0.0f);
        if (!p)
            return 0;
        self->duration = 400;
        p->duration = 40;
        p->size[0].value = 1.5f;
        p->fall_speed = 0.19f;
        p->gravity = 0.01f;
        p->angle[0] = (float)rnd(360);
        p->scale = 1.2f;
        mat_set_rotate_y(p->matrix, deg_to_rad((int)p->angle[0]));
        p->delta2 = mat_point(p->matrix, 0.0f, -30.0f, p->size[0].value);
        p->fade_frame = 30;
        prim_set_sprite(p, PARTICLE1_SPR, PARTICLE1_ACT);
        prim_sync_particles(p);
    }
    if (self->frame == self->duration)
        return 0;
    rag_str_step(self, 0.0f, -80.0f, 0.0f);
    return 1;
}

unsigned char rag_lex_divina(struct RagEffect *self)
{
    if (self->frame == 0)
        rag_play_wave_here(self, "effect\\priest_lexdivina.wav");
    if (self->frame == 20)
        rag_play_wave_here(self, "effect\\priest_lexdivina.wav");
    if (self->frame == 35)
        rag_play_wave_here(self, "effect\\priest_lexdivina.wav");
    if (self->frame == 60)
        rag_play_wave_here(self, "effect\\priest_lexdivina.wav");
    if (self->frame == 70)
        rag_play_wave_here(self, "effect\\priest_lexdivina.wav");
    return rag_str_step(self, 0.0f, -80.0f, 0.0f) != 0;
}

unsigned char rag_storm_gust(struct RagEffect *self)
{
    struct EffectPrim *p;
    int i, yaw, dist, angle;
    float sin_a, cos_a;

    if (self->frame == 0)
        rag_play_wave_here(self, "effect\\wizard_stormgust.wav");
    if (g_session1.map_rule_flags[0] == 0 && self->frame % 29 == 0 && (unsigned)(self->frame - 31) <= 119) {
        for (i = 0; i < 2; i++) {
            yaw = rnd(360);
            dist = rnd(25) + 3;
            p = self->last_prim = rag_launch_prim(self, 9, 0.0f, 0.0f, 0.0f);
            if (!p)
                return 0;
            p->duration = 215 - self->frame;
            p->pattern |= 1;
            angle = rnd(360);
            sin_a = get_sin_deg(angle);
            cos_a = get_cos_deg(angle);
            p->delta2.y = 20.0f;
            p->delta2.x = sin_a * (float)dist;
            p->delta2.z = -(cos_a * (float)dist);
            p->scale = (float)(rnd(100) + 400) / 100.0f;
            p->size[2].value = 20.0f;
            p->angle[0] = (float)yaw;
            p->angle[1] = 100.0f;
            prim_tilt_then_turn(p);
            memcpy(p->matrix2, p->matrix, sizeof(p->matrix2));
            p->speed = 1.5f;
            p->accel = 0.0f;
            p->alpha = 250.0f;
            p->pattern_arg = 3;
            p->fade_frame = p->duration - 10;
            prim_set_texture(p, "effect\\ice.tga");
        }
    }
    return rag_str_step(self, 0.0f, -80.0f, 0.0f) != 0;
}

void rag_jupitel(struct RagEffect *self, char variant)
{
    struct EffectPrim *p;

    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\hunter_shockwavetrap.wav");
    if (self->frame != 0)
        return;

    p = self->last_prim = rag_launch_prim(self, 15, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 20;
    p->pattern |= 0x40;
    p->pattern |= 0x400;
    p->list_flags |= 2;
    p->size[1].value = 3.5f;
    p->size[2].value = 3.5f;
    prim_aim_at_offset(self, p);
    p->delta2.y = -5.0f;
    p->alpha = 170.0f;
    prim_set_texture(p, variant == 0 ? "effect\\thunder_center.bmp" : "effect\\twirl_soft.bmp");

    p = self->last_prim = rag_launch_prim(self, 15, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 20;
    p->pattern = 0x40;
    p->pattern |= 0x400;
    p->list_flags |= 2;
    p->loop_motion = 1;
    p->size[1].value = 4.5f;
    p->size[2].value = 4.5f;
    prim_aim_at_offset(self, p);
    p->delta2.y = -5.0f;
    p->motion_period = 1;
    p->motion_count = 6;
    prim_alloc_textures(p);
    if (variant == 0) {
        p->textures[0] = rag_texture("effect\\thunder_ball_a.bmp");
        p->textures[1] = rag_texture("effect\\thunder_ball_b.bmp");
        p->textures[2] = rag_texture("effect\\thunder_ball_c.bmp");
        p->textures[3] = rag_texture("effect\\thunder_ball_d.bmp");
        p->textures[4] = rag_texture("effect\\thunder_ball_e.bmp");
    } else {
        p->textures[0] = rag_texture("effect\\twirl_soft.bmp");
        p->textures[1] = rag_texture("effect\\thunder_ball_b.bmp");
        p->textures[2] = rag_texture("effect\\twirl_soft.bmp");
        p->textures[3] = rag_texture("effect\\thunder_ball_d.bmp");
        p->textures[4] = rag_texture("effect\\twirl_soft.bmp");
    }
    p->textures[5] = rag_texture("effect\\thunder_ball_f.bmp");
}

void rag_jupitel_hit(struct RagEffect *self, char variant)
{
    struct EffectPrim *p;

    self->base.pos = self->master->pos;
    if (self->frame % 20 == 0) {
        p = self->last_prim = rag_launch_prim(self, 15, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = 10;
        p->pattern |= 0x40;
        p->list_flags |= 2;
        p->list_flags |= 0x200;
        p->delta2.y = -5.0f;
        p->size[1].speed = 2.5f;
        p->size[2].speed = 2.5f;
        p->change_frame = 10;
        p->next_size1_speed = 0.0f;
        p->next_size2_speed = 0.0f;
        p->next_size1_accel = p->next_size1_speed / -50.0f;
        p->next_size2_accel = p->next_size2_speed / -50.0f;
        p->fade_frame = 1;
        prim_set_texture(p, variant == 0 ? "effect\\thunder_pang.bmp" : "effect\\pokjuk_d.bmp");
    }
    if (self->frame != 10)
        return;
    p = self->last_prim = rag_launch_prim(self, 15, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 300;
    p->pattern |= 0x40;
    p->list_flags |= 0x200;
    p->list_flags |= 2;
    p->delta2.y = -5.0f;
    p->loop_motion = 1;
    p->size[1].value = 7.5f;
    p->size[2].value = 7.5f;
    p->motion_period = 1;
    p->motion_count = 5;
    prim_alloc_textures(p);
    if (variant == 0) {
        p->textures[0] = rag_texture("effect\\thunder_plazma_blast_a.bmp");
        p->textures[1] = rag_texture("effect\\thunder_plazma_blast_b.bmp");
        p->textures[2] = rag_texture("effect\\thunder_ball_d.bmp");
        p->textures[3] = rag_texture("effect\\thunder_ball_e.bmp");
        p->textures[4] = rag_texture("effect\\thunder_ball_f.bmp");
    } else {
        p->textures[0] = rag_texture("effect\\twirl_soft.bmp");
        p->textures[1] = rag_texture("effect\\thunder_ball_b.bmp");
        p->textures[2] = rag_texture("effect\\twirl_soft.bmp");
        p->textures[3] = rag_texture("effect\\thunder_ball_c.bmp");
        p->textures[4] = rag_texture("effect\\twirl_soft.bmp");
    }
}

unsigned char rag_fire_pillar(struct RagEffect *self)
{
    if (self->frame == 30)
        rag_play_wave_here(self, "effect\\wizard_fire_pillar_b.wav");
    if (self->frame == 35)
        game_send_message(GAME_MSG_QUAKE, 0, 0, 0, 0);
    return rag_str_step(self, 0.0f, -80.0f, 0.0f) != 0;
}

void rag_water_ball(struct RagEffect *self)
{
    struct EffectPrim *p;

    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\wizard_waterball_chulung.wav");
    p = self->last_prim = rag_launch_prim(self, 8, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    prim_set_sprite(p, SPR_DIR "Waterball.spr", SPR_DIR "Waterball.act");
    p = self->last_prim;
    p->duration = 40;
    p->delta2.x = 0.0f;
    p->delta2.y = -10.0f;
    p->delta2.z = 0.0f;
    p->size[0].value = render_object_calc_ground_dist(&p->base, self->offset.x, self->offset.z);
    render_object_turn_toward(&p->base, self->offset.x, self->offset.z);
    p->angle[0] = -p->base.roty;
    p->speed = p->size[0].value / (float)p->duration;
    p->fall_speed = 3.5f;
    p->gravity = p->fall_speed / (float)p->duration * -2.0f;
    p->angle_speed[1] = -2.0f;
    p->angle_accel[1] = p->angle_speed[1] / (float)p->duration * -2.0f;
    p->angle[2] = 0.0f;
    mat_set_rotate_y(p->matrix, deg_to_rad((int)p->angle[0]));
    p->drift = mat_point(p->matrix, 0.0f, 0.0f, 1.0f);
    p->swing_axis = p->drift;
    p->scale = 0.75f;
    p->particle_count = 12;
    prim_sync_particles(p);
}

void rag_adrenaline_rush(struct RagEffect *self)
{
    struct EffectPrim *p;
    int rays, yaw;

    if (self->frame == 0) {
        self->duration = 300;
        rag_play_wave_here(self, "effect\\black_adrenalinerush_a.wav");
    }
    if (self->frame == 30)
        rag_play_wave_here(self, "effect\\black_adrenalinerush_b.wav");
    rag_project_master(self);
    if (self->frame != 0)
        return;
    for (rays = 20; rays != 0; rays--) {
        p = self->last_prim = rag_launch_prim(self, 3, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = 80;
        p->delta2.y = -40.0f;
        p->angle[0] = (float)rnd(360);
        p->angle_speed[0] = (float)(rnd(60) + 10) / 10.0f;
        p->angle_accel[0] = p->angle_speed[0] / (float)p->duration / -1.5f;
        p->arc_degrees = (float)(rnd(25) + 5) / 10.0f;
        p->size[2].value = 0.0f;
        p->size[2].speed = (float)(rnd(20) + 50) / 13.0f;
        p->change_frame = 40;
        p->next_speed = (float)(rnd(10) + 15) / 10.0f;
        p->next_accel = 0.0f;
        p->next_size2_speed = 0.0f;
        prim_set_texture(p, "EFFECT\\alpha_center.tga");
    }
    if (self->frame != 0)
        return;
    for (yaw = 0; yaw < 360; yaw += 90) {
        p = self->last_prim = rag_launch_prim(self, 7, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = 100;
        p->size[0].value = 7.0f;
        p->fall_speed = -0.1f;
        p->gravity = -0.002f;
        p->angle[0] = (float)yaw;
        p->angle_accel[0] = 0.2f;
        p->angle_speed[0] = (float)self->frame * p->angle_accel[0] + 0.3f;
        p->scale = 1.2f;
        mat_set_rotate_y(p->matrix, deg_to_rad((int)p->angle[0]));
        p->fade_frame = p->duration - p->duration / 10;
        prim_set_sprite(p, PARTICLE1_SPR, PARTICLE1_ACT);
        prim_sync_particles(p);
    }
}

void rag_flasher(struct RagEffect *self)
{
    struct EffectPrim *p;
    int rays;

    if (self->frame == 0)
        rag_play_wave_here(self, "effect\\hunter_flasher.wav");
    rag_project_master(self);
    if (self->frame != 0)
        return;
    p = self->last_prim = rag_launch_prim(self, 15, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 70;
    p->pattern = 0x41;
    p->list_flags |= 2;
    p->delta2.y = -10.0f;
    p->size[1].value = 20.0f;
    p->size[2].value = 20.0f;
    p->alpha = 0.0f;
    p->alpha_max = 200.0f;
    p->alpha_speed = p->alpha_max / 6.0f;
    p->alpha_keys_set_alpha = 1;
    prim_add_alpha_key(p, 40.0f, 120.0f, 200.0f);
    prim_add_alpha_key(p, 100.0f, 0.0f, 80.0f);
    p->fade_frame = p->duration - 10;
    prim_set_texture(p, "effect\\thunder_center.bmp");
    for (rays = 20; rays != 0; rays--) {
        p = self->last_prim = rag_launch_prim(self, 3, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = 70;
        p->delta2.y = -40.0f;
        p->angle[0] = (float)rnd(360);
        p->angle_speed[0] = (float)(rnd(60) + 10) / 10.0f;
        p->angle_accel[0] = p->angle_speed[0] / (float)p->duration / -1.5f;
        p->arc_degrees = (float)(rnd(25) + 5) / 10.0f;
        p->size[2].value = (float)(rnd(40) + 20);
        p->size[2].speed = (float)(rnd(30) + 50) / 10.0f;
        p->alpha = 0.0f;
        p->alpha_max = 200.0f;
        p->alpha_speed = p->alpha_max / 10.0f;
        p->fade_frame = p->duration - p->duration / 3;
        prim_set_texture(p, "EFFECT\\alpha_center.tga");
    }
}

void rag_remove_trap(struct RagEffect *self)
{
    struct EffectPrim *p;
    int sparks;

    if (self->frame == 0)
        rag_play_wave_here(self, "effect\\hunter_removetrap.wav");
    if (self->frame != 0 && self->frame != 7)
        return;
    for (sparks = 12; sparks != 0; sparks--) {
        p = self->last_prim = rag_launch_prim(self, 7, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = 50;
        p->size[0].speed = 0.2f;
        rand();
        p->fall_speed = -2.0f;
        p->gravity = (float)rnd(10) / 1000.0f + 0.08f;
        p->angle[0] = (float)rnd(360);
        p->scale = 0.8f;
        mat_set_rotate_y(p->matrix, deg_to_rad((int)p->angle[0]));
        p->delta2 = mat_point(p->matrix, 0.0f, 0.0f, p->size[0].value);
        p->fade_frame = 35;
        prim_set_sprite(p, PARTICLE1_SPR, PARTICLE1_ACT);
        prim_sync_particles(p);
    }
}

void rag_violet_cross(struct RagEffect *self)
{
    struct EffectPrim *p;
    float facing_m[12], side_dist;
    int facing, side;

    if (self->frame != 0)
        return;
    facing = master_facing(self);
    mat_set_rotate_y(facing_m, deg_to_rad(facing));
    self->duration = 9999;
    for (side = 1; side >= -1; side -= 2) {
        p = self->last_prim = rag_launch_prim(self, 15, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = 9999;
        p->endless = 1;
        side_dist = (float)side * 5.0f;
        p->pattern = 0x28;
        p->size2_flip_period = 50;
        p->delta2 = mat_point(facing_m, 0.0f, 0.0f, side_dist);
        p->angle[0] = (float)(facing + 90);
        p->size[1].value = 5.0f;
        p->size[2].value = 15.0f;
        p->size[2].speed = 0.25f;
        p->alpha = 0.0f;
        p->alpha_max = 180.0f;
        p->alpha_speed = p->alpha_max / 6.0f;
        p->fade_frame = p->duration - 35;
        prim_set_texture(p, "effect\\magic_violet.tga");

        p = self->last_prim = rag_launch_prim(self, 15, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = 9999;
        p->endless = 1;
        p->pattern = 0x28;
        p->size2_flip_period = 50;
        p->delta2 = mat_point(facing_m, side_dist, 0.0f, 0.0f);
        p->angle[0] = (float)facing;
        p->size[1].value = 5.0f;
        p->size[2].value = 15.0f;
        p->size[2].speed = 0.25f;
        p->alpha = 0.0f;
        p->alpha_max = 180.0f;
        p->alpha_speed = p->alpha_max / 6.0f;
        p->fade_frame = p->duration - 35;
        prim_set_texture(p, "effect\\magic_violet.tga");
    }
}

void rag_blitz_beat(struct RagEffect *self)
{
    struct EffectPrim *p;
    float sin_a, cos_a;
    int flakes, angle, yaw, x_rnd, y_rnd;

    if (self->frame == 5)
        rag_play_wave_here(self, "effect\\hunter_blitzbeat_1st.wav");
    if (self->frame == 30)
        rag_play_wave_here(self, "effect\\hunter_blitzbeat.wav");
    self->base.pos = self->master->pos;
    if (self->frame == 0) {
        g_blitzBeatRingCount = 0;
        for (flakes = 10; flakes != 0; flakes--) {
            p = self->last_prim = rag_launch_prim(self, 16, 0.0f, 0.0f, 0.0f);
            if (!p)
                return;
            p->duration = 20;
            p->list_flags |= 0x200;
            p->angle[0] = self->master->roty;
            mat_set_rotate_y(p->matrix, deg_to_rad((int)p->angle[0]));
            angle = rnd(360);
            sin_a = get_sin_deg(angle);
            cos_a = get_cos_deg(angle);
            p->velocity = mat_point(p->matrix, 0.0f, 0.0f, 15.0f);
            p->delta.x = p->delta.x + p->velocity.x;
            p->delta.y = p->velocity.y + p->delta.y;
            p->delta.z = p->velocity.z + p->delta.z;
            p->delta2.y = -7.0f;
            p->delta2.x = sin_a * 3.5f;
            p->delta2.z = cos_a * -3.5f;
            p->speed = -1.2f;
            p->accel = -0.1f;
            p->size[1].value = (float)(rnd(40) + 20) / 10.0f;
            p->size[2].value = 0.2f;
            p->size[1].speed = 0.1f;
            p->size[1].accel = -0.01f;
            p->alpha = 0.0f;
            p->alpha_max = 250.0f;
            p->change_frame = 10;
            p->next_speed = -0.3f;
            p->next_accel = 0.001f;
            p->alpha_speed = p->alpha_max * 0.5f;
            p->fade_frame = p->duration - 2;
            prim_set_texture(p, "effect\\ac_center2.tga");
            self->last_prim->variant = 0;
        }
    }
    if (self->frame <= 35 || self->frame % 3 != 0 || g_blitzBeatRingCount >= self->hit_count)
        return;
    yaw = (int)self->master->roty;
    x_rnd = rnd(10);
    y_rnd = rnd(20);
    rand();
    p = self->last_prim = rag_launch_prim(self, 13, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 12;
    p->angle[0] = (float)yaw;
    p->angle[1] = -90.0f;
    prim_tilt_then_turn(p);
    p->velocity = mat_point(p->matrix, 0.0f, -2.0f, 1.0f);
    p->delta.x = p->velocity.x + p->delta.x;
    p->delta.y = p->velocity.y + p->delta.y;
    p->delta.z = p->velocity.z + p->delta.z;
    p->delta2.z = 0.0f;
    p->delta2.x = (float)((x_rnd + 1) / 5);
    p->delta2.y = (float)((y_rnd + 1) / 2 - 12);
    p->speed = 0.4f;
    p->accel = p->speed / (float)p->duration * -0.5f;
    p->radius[1] = 1.5f;
    p->radius[0] = 0.5f;
    p->size[2].value = 3.5f;
    p->fade_frame = p->duration - p->duration / 2;
    prim_set_texture(p, "effect\\magic_green.tga");
    g_blitzBeatRingCount++;
}

void rag_detecting(struct RagEffect *self, char variant)
{
    struct EffectPrim *p;
    const char *wave;

    if (self->frame != 0)
        return;
    switch (variant) {
    case 0:  wave = "effect\\hunter_detecting.wav"; break;
    case 2:  wave = "effect\\priest_benedictio.wav"; break;
    case 4:  wave = "effect\\cru_grand cross.wav"; break;
    case 5:  wave = "effect\\ANALYZE.wav"; break;
    default: wave = NULL; break;
    }
    if (wave != NULL)
        rag_play_wave_here(self, wave);
    if (self->frame != 0)
        return;
    p = self->last_prim = rag_launch_prim(self, 15, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 57;
    p->size[1].speed = 1.5f;
    p->size[2].speed = 1.5f;
    p->size[1].accel = p->size[1].speed / 14.0f;
    p->size[2].accel = p->size[2].speed / 14.0f;
    p->delta2.y = -7.0f;
    p->velocity.y = -0.5f;
    p->angle[2] = 90.0f;
    if (variant == 3 || variant == 6) {
        p->change_frame = 10;
        p->fade_frame = 30;
    } else {
        p->change_frame = 17;
        p->fade_frame = 40;
    }
    switch (variant) {
    case 4: p->r = 255; p->b = 140; p->g = 140; break;
    case 5: p->r = 210; p->b = 210; p->g = 250; break;
    case 6: p->r = 10;  p->b = 10;  p->g = 10;  break;
    case 7: p->r = 250; p->b = 140; p->g = 210; break;
    case 8: p->r = 140; p->b = 140; p->g = 255; break;
    }
    p->next_size1_speed = 0.1f;
    p->next_size2_speed = 0.1f;
    p->next_size1_accel = p->next_size1_speed / 30.0f;
    p->next_size2_accel = p->next_size2_speed / 30.0f;
    p->delta2.y = -5.0f;
    prim_set_texture(p, "effect\\fashasha.tga");
}
