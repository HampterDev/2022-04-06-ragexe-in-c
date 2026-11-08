#include "session/state.h"
#include "effects/rag/rag_skills_classic.h"

#define LENS1 "effect\\lens1.tga"
#define LENS2 "effect\\lens2.tga"

static struct EffectPrim *lens_ring(struct RagEffect *self, int facing, int duration)
{
    struct EffectPrim *p = self->last_prim = rag_launch_prim(self, 13, 0.0f, 0.0f, 0.0f);
    if (!p)
        return NULL;

    p->duration = duration;
    p->angle[0] = (float)facing;
    p->angle[1] = -90.0f;
    prim_tilt_then_turn(p);
    p->delta2.x = 0.0f;
    p->delta2.y = -10.0f;
    p->delta2.z = 0.0f;
    p->speed = 0.7f;
    p->accel = p->speed / (float)p->duration * -0.5f;
    return p;
}

static void hit_spark(struct RagEffect *self, int facing, int speed_span, int speed_from)
{
    struct EffectPrim *p = self->last_prim = rag_launch_prim(self, 5, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;

    p->angle[0] = (float)(rnd(80) + (facing - 40));
    p->angle[1] = (float)(rnd(100) - 50);
    prim_burst_out(p);
    p->speed = (float)(rnd(speed_span) + speed_from) / 100.0f;
    p->duration = rnd(24) + 6;
    p->accel = p->speed / (float)p->duration * -0.5f;
    p->scale = (float)(rnd(100) + 60) / 100.0f;
    p->scale_speed = -(p->scale / (float)p->duration);
    p->fade_frame = 0;
    prim_set_sprite(p, SPR_DIR "particle1.spr", SPR_DIR "particle1.act");
    p->particle_count = 3;
    prim_sync_particles(p);
}

void rag_hit1(struct RagEffect *self)
{
    struct EffectPrim *p;
    int facing, i;

    if (self->frame != 0)
        return;
    facing = master_facing(self);
    p = self->last_prim = rag_launch_prim(self, 13, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 10;
    p->angle[0] = (float)facing;
    p->angle[1] = -90.0f;
    mat_set_rotate_x(p->matrix, deg_to_rad((int)p->angle[1]));
    mat_rotate_y(p->matrix, deg_to_rad((int)p->angle[0]));
    p->delta2.x = 0.0f;
    p->delta2.y = -10.0f;
    p->delta2.z = 0.0f;
    p->speed = 0.7f;
    p->accel = p->speed / (float)p->duration * -0.5f;
    p->radius[1] = 10.0f;
    p->radius[0] = 5.0f;
    p->size[2].value = 3.5f;
    p->fade_frame = p->duration - p->duration / 2;
    prim_set_texture(p, "effect\\ring_blue.tga");

    if (g_session1.map_rule_flags[0] != 0)
        return;
    for (i = 0; i < 2; i++) {
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
        p->fade_frame = 0;
        prim_set_sprite(p, SPR_DIR "particle1.spr", SPR_DIR "particle1.act");
        p->particle_count = 3;
        prim_sync_particles(p);
    }
    for (i = 0; i < 2; i++) {
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
        p->fade_frame = 0;
        prim_set_sprite(p, SPR_DIR "particle1.spr", SPR_DIR "particle1.act");
        p->particle_count = 3;
        prim_sync_particles(p);
    }
}

void rag_hit2(struct RagEffect *self, int tinted)
{
    struct EffectPrim *p;
    float angle, dist;
    int i, deg;

    rag_project_master(self);
    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\EF_hit2.wav");
    for (i = 0, angle = 0.0f; 360.0f > angle; i++, angle += 45.0f) {
        p = self->last_prim = rag_launch_prim(self, 0, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->pattern = 1;
        p->duration = rnd(20) + 10;
        p->angle[2] = (float)rnd(30) + (angle - 15.0f);
        p->angle[0] = p->angle[2];
        p->speed = (float)(rnd(45) + 5) / 10.0f;
        p->accel = p->speed / (float)p->duration * -0.5f;
        p->size[1].value = (float)(rnd(15) + 5);
        p->size[2].value = (float)(rnd(20) + 20);
        p->size[1].speed = -(p->size[1].value / (float)p->duration);
        p->size[2].speed = 1.5f;
        p->size[2].accel = 0.25f;
        dist = (float)rnd(5);
        deg = (int)angle;
        p->delta2.x = get_sin_deg(deg) * dist;
        p->delta2.y = -(get_cos_deg(deg) * dist);
        p->delta2.y = p->delta2.y - 20.0f;
        if (tinted) {
            p->r = 120;
            p->g = 120;
        }
        p->alpha = 0.0f;
        p->alpha_speed = p->alpha_max * 0.125f;
        p->variant = 0;
        prim_set_texture(p, (i & 1) ? LENS2 : LENS1);
    }
}

void rag_hit3(struct RagEffect *self)
{
    struct EffectPrim *p;
    int facing, i;

    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\EF_hit3.wav");
    facing = master_facing(self);
    p = lens_ring(self, facing, 15);
    if (!p)
        return;
    p->radius[1] = 1.5f;
    p->radius[0] = 1.5f;
    p->size[2].speed = 0.5f;
    p->size[2].accel = 0.2f;
    p->fade_frame = p->duration - p->duration / 3;
    prim_set_texture(p, LENS2);

    p = lens_ring(self, facing, 15);
    if (!p)
        return;
    p->radius[1] = 4.0f;
    p->radius[0] = 1.5f;
    p->size[2].speed = 0.25f;
    p->size[2].accel = 0.2f;
    p->fade_frame = p->duration - p->duration / 3;
    prim_set_texture(p, LENS2);

    for (i = 0; i < 8; i++)
        hit_spark(self, facing, 120, 80);
}

void rag_hit4(struct RagEffect *self)
{
    struct EffectPrim *p;
    int facing, i;

    rag_project_master(self);
    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\EF_hit4.wav");
    facing = master_facing(self);
    p = lens_ring(self, facing, 15);
    if (!p)
        return;
    p->radius[1] = 4.0f;
    p->radius[0] = 0.5f;
    p->size[2].speed = 0.25f;
    p->size[2].accel = 0.15f;
    p->fade_frame = p->duration - p->duration / 3;
    prim_set_texture(p, LENS2);

    for (i = 0; i < 5; i++)
        hit_spark(self, facing, 90, 60);
}

static void crossed_rays(struct RagEffect *self, const char *wave, int width_from, int length_span, int length_from,
                         float spread)
{
    struct EffectPrim *p;
    int angles[2];
    int i;

    rag_project_master(self);
    if (self->frame != 0)
        return;
    rag_play_wave_here(self, wave);
    angles[0] = rnd(360);
    angles[1] = angles[0] + 90;
    for (i = 0; i < 2; i++) {
        p = self->last_prim = rag_launch_prim(self, 0, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = 17;
        p->delta2.y = -20.0f;
        p->angle[2] = (float)angles[i];
        p->angle_speed[2] = -5.0f;
        p->angle_accel[2] = p->angle_speed[2] / (float)p->duration / -1.5f;
        p->size[1].value = (float)(rnd(5) + width_from);
        p->size[2].value = (float)(rnd(length_span) + length_from);
        p->size[1].speed = -(p->size[1].value / (float)p->duration);
        p->size[2].speed = spread;
        p->size[2].accel = 0.5f;
        p->fade_frame = p->duration - p->duration / 2;
        prim_set_texture(p, LENS2);
        p->variant = 0;
    }
}

void rag_hit5(struct RagEffect *self)
{
    crossed_rays(self, "effect\\EF_hit5.wav", 20, 10, 30, 2.5f);
}

void rag_hit6(struct RagEffect *self)
{
    crossed_rays(self, "effect\\EF_hit6.wav", 10, 5, 15, 1.7f);
}

void rag_entry(struct RagEffect *self)
{
    struct EffectPrim *p;

    if (self->frame == 0) {
        p = self->last_prim = rag_launch_prim(self, 13, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = 55;
        prim_tilt_then_turn(p);
        p->accel = -7e-05f;
        p->angle_speed[0] = -10.0f;
        p->radius[1] = 6.0f;
        p->radius_speed[1] = 0.08f;
        p->radius[0] = 5.0f;
        p->size[2].value = 6.5f;
        p->size[2].speed = -0.1f;
        p->alpha = 5.0f;
        p->alpha_max = 245.0f;
        p->alpha_speed = 10.0f;
        p->fade_frame = p->duration - p->duration / 5;
        prim_set_texture(p, "effect\\ring_blue.tga");
    }
    if (self->frame == 0) {
        p = self->last_prim = rag_launch_prim(self, 13, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = 55;
        p->arc_degrees = 45.0f;
        p->angle_speed[0] = 10.0f;
        p->radius[1] = 4.5f;
        p->radius[0] = 4.5f;
        p->size[2].speed = 2.5f;
        p->size[2].accel = p->size[2].speed / (float)p->duration * -2.0f;
        p->alpha = 160.0f;
        p->fade_frame = p->duration - p->duration / 3;
        prim_set_texture(p, "effect\\ring_blue.tga");
    }
}

static void rising_sparkle(struct RagEffect *self, int duration, const char *spr, const char *act)
{
    struct EffectPrim *p = self->last_prim = rag_launch_prim(self, 7, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;

    rand();
    p->size[0].value = 3.0f;
    p->duration = duration;
    p->angle[0] = (float)rnd(360);
    p->fall_speed = -1.2f;
    p->gravity = p->fall_speed / (float)p->duration / -1.5f;
    mat_set_rotate_y(p->matrix, deg_to_rad((int)p->angle[0]));
    p->delta2 = mat_point(p->matrix, 0.0f, 0.0f, p->size[0].value);
    p->scale = 0.55f;
    p->fade_frame = p->duration - p->duration / 3;
    prim_set_sprite(p, spr, act);
    prim_sync_particles(p);
}

void rag_exit(struct RagEffect *self)
{
    struct EffectPrim *p;

    self->base.pos = self->master->pos;
    if (self->frame == 0) {
        p = self->last_prim = rag_launch_prim(self, 13, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        self->duration = 100;
        p->duration = 100;
        p->radius[1] = 4.5f;
        p->radius[0] = 4.5f;
        p->size[2].value = 35.0f;
        p->alpha = 0.0f;
        p->alpha_max = 90.0f;
        p->alpha_speed = p->alpha_max / 30.0f;
        p->fade_frame = p->duration - 20;
        prim_set_texture(p, "effect\\alpha_down.tga");
        rag_play_wave_here(self, "_heal_effect.wav");
    }
    if (g_session1.map_rule_flags[0] == 0 && self->frame % 9 == 0)
        rising_sparkle(self, 50, SPR_DIR "particle1.spr", SPR_DIR "particle1.act");
}

void rag_warp(struct RagEffect *self)
{
    struct EffectPrim *p;
    int big;

    if (self->frame % 20 == 0) {
        p = self->last_prim = rag_launch_prim(self, 12, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = self->duration;
        p->list_flags |= 0x200;
        p->size[0].value = 2.0f;
        p->arc_degrees = 22.5f;
        p->radius[0] = 10.0f;
        p->angle[1] = 90.0f;
        mat_set_rotate_x(p->matrix, p->angle[1] / 180.0f * 3.141592f);
        p->size[0].speed = 1.25f;
        p->size[0].accel = p->size[0].speed / (float)p->duration * -0.5f;
        p->alpha = 200.0f;
        p->fade_frame = p->duration - p->duration / 2;
        prim_sync_particles(p);
        prim_set_texture(p, "EFFECT\\RING_YELLOW.TGA");
    }
    if (self->frame & 1)
        return;
    p = self->last_prim = rag_launch_prim(self, 7, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    big = rnd(200) / 100;
    p->size[0].value = (float)big * 0.7f + 1.0f;
    p->duration = 40;
    p->angle[0] = (float)rnd(360);
    p->angle_speed[0] = (float)big * 3.0f + 3.0f;
    p->fall_speed = (float)(60 - rnd(240)) / 100.0f;
    p->gravity = p->fall_speed / (float)p->duration * -0.5f;
    mat_set_rotate_y(p->matrix, deg_to_rad((int)p->angle[0]));
    p->delta2 = mat_point(p->matrix, 0.0f, 0.0f, p->size[0].value);
    p->scale = 0.2f;
    p->fade_frame = p->duration - p->duration / 3;
    prim_set_sprite(p, SPR_DIR "particle1.spr", SPR_DIR "particle1.act");
    prim_sync_particles(p);
}

void rag_enhance(struct RagEffect *self)
{
    struct EffectPrim *p;
    float dist;

    if (self->frame == 0) {
        p = self->last_prim = rag_launch_prim(self, 12, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = self->duration;
        p->arc_degrees = 22.5f;
        p->scale = 7.0f;
        p->angle[1] = 90.0f;
        mat_set_rotate_x(p->matrix, deg_to_rad((int)p->angle[1]));
        p->alpha = 128.0f;
        p->fade_frame = p->duration - p->duration / 3;
        prim_sync_particles(p);
        prim_set_texture(p, "effect\\ring_yellow.tga");

        p = self->last_prim = rag_launch_prim(self, 13, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = self->duration;
        p->radius[1] = 6.0f;
        p->radius[0] = 6.0f;
        p->size[2].speed = 2.0f;
        p->size[2].accel = p->size[2].speed / (float)p->duration / -1.5f;
        p->size2_max = 50.0f;
        p->alpha = 128.0f;
        p->fade_frame = p->duration - p->duration / 3;
        prim_set_texture(p, "effect\\alpha_down.tga");
    }
    if (self->frame % (g_session1.map_rule_flags[0] == 0 ? 5 : 15) != 0)
        return;
    p = self->last_prim = rag_launch_prim(self, 16, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 50;
    mat_set_rotate_y(p->matrix, deg_to_rad(rnd(360)));
    dist = (float)(rnd(5) + 2);
    p->delta2 = mat_point(p->matrix, 0.0f, 0.0f, dist);
    mat_set_rotate_x(p->matrix, deg_to_rad(90));
    p->speed = 2.0f;
    p->size[1].value = 7.0f;
    p->size[2].value = 0.25f;
    prim_set_texture(p, "effect\\alpha_down.tga");
    p->variant = 0;
}

unsigned char rag_coin(struct RagEffect *self)
{
    struct EffectPrim *p;
    Vec3 spot;
    int i;

    if (g_session1.map_rule_flags[0] == 0 && self->frame == 20) {
        for (i = 0; i < 5; i++) {
            p = self->last_prim = rag_launch_prim(self, 6, 0.0f, 0.0f, 0.0f);
            if (!p)
                return 0;
            p->duration = rnd(30) + 40;
            p->angle[0] = (float)(rnd(12) * 30);
            mat_set_rotate_y(p->matrix, deg_to_rad((int)p->angle[0]));
            spot = mat_point(p->matrix, 0.0f, 0.0f, (float)(rnd(2) + 2));
            p->delta2.x = spot.x + 0.0f;
            p->delta2.y = spot.y - 10.0f;
            p->delta2.z = spot.z + 0.0f;
            mat_set_rotate_y(p->matrix, deg_to_rad((int)p->angle[0]));
            p->speed = (float)(rnd(50) + 50) / 100.0f;
            p->accel = p->speed / (float)p->duration / -3.5f;
            p->fall_speed = (float)(rnd(100) + 50) / -25.0f;
            p->gravity = -(p->fall_speed / (float)p->duration * 4.0f);
            p->scale = 2.0f;
            p->fade_frame = p->duration - p->duration / 3;
            p->bounce_clink = 1;
            p->motion_period = 1;
            prim_set_sprite(p, SPR_DIR "동전.spr", SPR_DIR "동전.act");
            p->particle_count = 2;
            prim_sync_particles(p);
        }
    }
    rag_str_step(self, 0.0f, -80.0f, 0.0f);
    return self->frame < 100;
}

void rag_endure(struct RagEffect *self)
{
    struct EffectPrim *p;
    float dist;

    rag_project_master(self);
    if (self->frame == 0) {
        rag_play_wave_here(self, "effect\\EF_Endure.wav");
        p = self->last_prim = rag_launch_prim(self, 0, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = self->duration;
        p->pattern |= 2;
        p->delta2.y = -20.0f;
        p->size[1].value = 150.0f;
        p->size[2].value = 150.0f;
        p->size[1].speed = -5.0f;
        p->size[2].speed = -5.0f;
        p->pattern_arg = p->duration - 35;
        p->size[1].accel = -(p->size[1].speed / (float)p->pattern_arg);
        p->size[2].accel = -(p->size[2].speed / (float)p->pattern_arg);
        p->alpha = 0.0f;
        p->alpha_speed = p->alpha_max / 20.0f;
        p->fade_frame = p->duration - p->duration / 3;
        prim_set_texture(p, "effect\\endure.tga");
        p->variant = 0;
    }
    if (self->frame >= 50)
        return;
    p = self->last_prim = rag_launch_prim(self, 0, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 40;
    p->angle[2] = (float)rnd(360);
    p->angle[0] = p->angle[2];
    p->speed = -4.0f;
    p->accel = -(p->speed / (float)p->duration);
    p->size[1].value = (float)(rnd(100) + 60) / 100.0f;
    p->size[2].value = (float)(rnd(20) + 30);
    p->alpha = 0.0f;
    p->alpha_speed = p->alpha_max / 20.0f;
    p->fade_frame = p->duration - p->duration / 3;
    dist = (float)(rnd(40) + 100);
    p->delta2.x = get_sin_deg((int)p->angle[0]) * dist;
    p->delta2.y = -(get_cos_deg((int)p->angle[0]) * dist);
    p->delta2.y = p->delta2.y - 20.0f;
    prim_set_texture(p, "effect\\alpha_down.tga");
    p->variant = 0;
}

void rag_casting_circle(struct RagEffect *self, int alpha, const char *texture,
                                   char style)
{
    static const short turn_start[4] = { 180, 270, 0, 90 };
    struct EffectPrim *p;
    struct PrimRing *ring;
    int i, height;

    self->base.pos = self->master->pos;
    if (self->frame != 0)
        return;
    if (self->duration < 56)
        self->duration = 56;
    p = self->last_prim = rag_launch_prim(self, 30, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    if (p == NULL)
        return;
    p->list_flags = 5;
    p->duration = self->duration;
    prim_set_texture(p, texture);
    switch (style) {
    case 1:    p->scale = 8.0f; break;
    case 2:    p->scale = 5.0f; break;
    case 3:    p->scale = 10.0f; break;
    case 4:    p->scale = 11.0f; break;
    case 5:    p->scale = 4.0f; break;
    case 6:    p->scale = 12.0f; break;
    case 0x0f: p->scale = 7.0f; break;
    case 0x14: p->scale = 24.0f; break;
    case 0x15: p->scale = 25.0f; break;
    case 0x16: p->scale = 26.0f; break;
    case 0x17: p->scale = 27.0f; break;
    case 0x1f: p->scale = 31.0f; break;
    case 0x2a: p->scale = 42.0f; break;
    case 0x2b: p->scale = 43.0f; break;
    case 0x2c: p->scale = 44.0f; break;
    case 0x2d: p->scale = 45.0f; break;
    case 0x2e: p->scale = 46.0f; break;
    case 0x2f: p->scale = 47.0f; break;
    case 0x30: p->scale = 48.0f; break;
    case 0x31: p->scale = 49.0f; break;
    case 0x32: p->scale = 50.0f; break;
    default:   p->scale = 5.0f; break;
    }
    height = style == 2 ? 25 : style == 0x16 ? 15 : 20;
    for (i = 0; i < 4; i++) {
        ring = &p->records.rings[i];
        ring->active = 1;
        ring->age = 0;
        ring->span = 360;
        ring->height = (float)(height - i);
        ring->turn = turn_start[i];
        ring->alpha = (short)(alpha + 135 - 45 * i);
        ring->radius = 4.1f;
        ring->tilt = 80;
        ring->mode = style;
        ring->offset_flag = 0;
        if (self->duration <= 60) {
            ring->alpha = 0;
            ring->age = -5 * i;
        }
    }
}

unsigned char rag_glass_wall(struct RagEffect *self)
{
    struct EffectPrim *p;
    float facing_m[12];
    int facing, side;

    if (self->frame == 0) {
        rag_play_wave_here(self, "effect\\EF_GlassWall.wav");
        facing = master_facing(self);
        mat_set_rotate_y(facing_m, deg_to_rad(facing));
        for (side = 1; side >= -1; side -= 2) {
            p = self->last_prim = rag_launch_prim(self, 15, 0.0f, 0.0f, 0.0f);
            if (!p)
                return 0;
            p->duration = self->duration;
            p->pattern = 0x28;
            p->size2_flip_period = 50;
            p->delta2 = mat_point(facing_m, 0.0f, 0.0f, (float)side * 2.6f);
            p->angle[0] = (float)(facing + 90);
            p->size[1].value = 3.0f;
            p->size[2].value = 40.0f;
            p->size[2].speed = 0.25f;
            p->alpha = 0.0f;
            p->alpha_max = 180.0f;
            p->alpha_speed = p->alpha_max / 6.0f;
            p->fade_frame = p->duration - 6;
            prim_set_texture(p, "effect\\ring_blue.tga");

            p = self->last_prim = rag_launch_prim(self, 15, 0.0f, 0.0f, 0.0f);
            if (!p)
                return 0;
            p->duration = self->duration;
            p->pattern = 0x28;
            p->size2_flip_period = 50;
            p->delta2 = mat_point(facing_m, (float)side * 3.0f, 0.0f, 0.0f);
            p->angle[0] = (float)facing;
            p->size[1].value = 2.6f;
            p->size[2].value = 40.0f;
            p->size[2].speed = 0.25f;
            p->alpha = 0.0f;
            p->alpha_max = 180.0f;
            p->alpha_speed = p->alpha_max / 6.0f;
            p->fade_frame = p->duration - 6;
            prim_set_texture(p, "effect\\ring_blue.tga");
        }
    }
    rag_str_step(self, 0.0f, -80.0f, 0.0f);
    return self->frame != self->duration;
}

static void heal_sp_ring(struct RagEffect *self, float size, float height, float alpha_max)
{
    struct EffectPrim *p = self->last_prim = rag_launch_prim(self, 13, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;

    p->duration = self->duration;
    p->angle_speed[0] = 5.0f;
    p->radius[1] = size;
    p->radius[0] = size;
    p->size[2].value = height;
    p->r = 32;
    p->g = 176;
    p->b = 232;
    p->alpha = 0.0f;
    p->alpha_max = alpha_max;
    p->alpha_speed = p->alpha_max / 30.0f;
    p->fade_frame = p->duration - 20;
    prim_set_texture(p, "effect\\alpha_down.tga");
}

void rag_heal_sp(struct RagEffect *self)
{
    self->base.pos = self->master->pos;
    if (self->frame == 0) {
        self->duration = 100;
        heal_sp_ring(self, 2.5f, 15.0f, 16.0f);
        heal_sp_ring(self, 3.5f, 25.0f, 32.0f);
        heal_sp_ring(self, 4.5f, 35.0f, 64.0f);
        rag_play_wave_here(self, "_heal_effect.wav");
    }
    if (self->frame & 1)
        return;
    rising_sparkle(self, 30, SPR_DIR "particle2.spr", SPR_DIR "particle2.act");
}

void rag_soul_strike(struct RagEffect *self, char ghost)
{
    struct EffectPrim *p;
    int count = 0, fan_step = 0, fan_from = 0;
    int tick, index;

    switch (self->hit_count) {
    case 1: count = 1; fan_step = 1; fan_from = 0; break;
    case 2: count = 2; fan_step = 180; fan_from = -90; break;
    case 3: count = 3; fan_step = 90; fan_from = -90; break;
    case 4: count = 4; fan_step = 60; fan_from = -90; break;
    case 5: count = 5; fan_step = 45; fan_from = -90; break;
    case 6: count = 6; fan_step = 30; fan_from = -90; break;
    case 7: count = 7; fan_step = 25; fan_from = -90; break;
    }
    tick = self->frame + 5;
    if (tick % 11 != 0)
        return;
    index = tick / 11;
    if (index > count)
        return;
    rag_play_wave_here(self, "effect\\EF_SoulStrike.wav");
    p = self->last_prim = rag_launch_prim(self, 8, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
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
    p->angle[2] = (float)(index * fan_step + fan_from);
    mat_set_rotate_y(p->matrix, deg_to_rad((int)p->angle[0]));
    p->drift = mat_point(p->matrix, 0.0f, 0.0f, 1.0f);
    p->swing_axis = p->drift;
    p->scale = 2.75f;
    if (ghost)
        prim_set_sprite(p, SPR_DIR "particle5.spr", SPR_DIR "particle5.act");
    else
        prim_set_sprite(p, SPR_DIR "particle1.spr", SPR_DIR "particle1.act");
    p->particle_count = 12;
    prim_sync_particles(p);
}

void rag_bash(struct RagEffect *self)
{
    struct EffectPrim *p;
    int shards;

    if (g_session1.map_rule_flags[0] != 0)
        return;
    rag_project_master(self);
    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\EF_Bash.wav");
    p = self->last_prim = rag_launch_prim(self, 4, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 40;
    p->pattern |= 1;
    p->delta2.y = -40.0f;
    p->size[0].value = 100.0f;
    p->alpha = 0.0f;
    p->alpha_max = 170.0f;
    p->alpha_speed = p->alpha_max / 6.0f;
    p->fade_frame = p->duration - 10;
    prim_set_texture(p, "effect\\alpha_down.tga");

    for (shards = g_session1.map_rule_flags[0] != 0 ? 10 : 20; shards != 0; shards--) {
        p = self->last_prim = rag_launch_prim(self, 3, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = 40;
        p->delta2.y = -40.0f;
        p->angle[0] = (float)rnd(360);
        p->angle_speed[0] = (float)(rnd(60) + 10) / 10.0f;
        p->angle_accel[0] = p->angle_speed[0] / (float)p->duration / -1.5f;
        p->arc_degrees = (float)(rnd(25) + 5) / 10.0f;
        p->size[2].value = (float)(rnd(40) + 20);
        p->size[2].speed = (float)(rnd(30) + 20) / 10.0f;
        p->alpha = 0.0f;
        p->alpha_max = 200.0f;
        p->alpha_speed = p->alpha_max / 10.0f;
        p->fade_frame = p->duration - p->duration / 3;
        prim_set_texture(p, "EFFECT\\alpha_center.tga");
    }
}

static void magnum_ring(struct RagEffect *self, int duration)
{
    struct EffectPrim *p = self->last_prim = rag_launch_prim(self, 12, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;

    p->duration = duration;
    p->list_flags |= 0x200;
    p->angle[1] = 90.0f;
    p->size[0].speed = 1.75f;
    p->size[0].accel = p->size[0].speed / (float)p->duration * -0.5f;
    p->radius[0] = 12.0f;
    p->alpha = 0.0f;
    p->alpha_speed = p->alpha_max / 15.0f;
    p->fade_frame = p->duration - 15;
    prim_sync_particles(p);
    prim_set_texture(p, "effect\\ring_yellow.tga");
}

void rag_magnum_break(struct RagEffect *self)
{
    struct EffectPrim *p;

    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\EF_MagnumBreak.wav");
    magnum_ring(self, self->duration);

    p = self->last_prim = rag_launch_prim(self, 14, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = self->duration;
    p->size[0].speed = 1.15f;
    p->size[0].accel = p->size[0].speed / (float)p->duration * -0.5f;
    p->angle_speed[0] = 3.0f;
    p->alpha = 0.0f;
    p->alpha_speed = p->alpha_max / 15.0f;
    p->fade_frame = p->duration - 15;
    prim_set_texture(p, "effect\\대폭발.tga");

    magnum_ring(self, 30);
}

void rag_steal(struct RagEffect *self)
{
    struct EffectPrim *p;
    int i;

    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\EF_Steal.wav");
    for (i = 0; i < 10; i++) {
        p = self->last_prim = rag_launch_prim(self, 6, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->angle[0] = (float)rnd(360);
        p->angle[1] = (float)(rnd(100) - 50);
        prim_burst_out(p);
        mat_set_rotate_y(p->matrix, deg_to_rad((int)p->angle[0]));
        p->duration = self->duration;
        p->speed = (float)(rnd(50) + 50) / 100.0f;
        p->accel = p->speed / (float)p->duration * -0.5f;
        p->fall_speed = (float)(rnd(90) + 60) / -100.0f;
        p->gravity = p->fall_speed / (float)p->duration * -2.0f;
        p->scale = (float)(rnd(100) + 100) / 100.0f;
        p->scale_speed = -(p->scale / (float)p->duration);
        p->fade_frame = p->duration - p->duration / 3;
        prim_set_sprite(p, SPR_DIR "particle7.spr", SPR_DIR "particle7.act");
        prim_sync_particles(p);
    }
}
