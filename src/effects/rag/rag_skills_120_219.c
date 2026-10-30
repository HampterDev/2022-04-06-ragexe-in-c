#include "actor/actor_ops.h"
#include "core/object_type.h"
#include "actor/render_ops.h"
#include "game/own_states.h"
#include "session/state.h"
#include "game/context.h"
#include "effects/rag/rag_prim_setup.h"
#include "session/session_skills.h"

void rag_cloaking(struct RagEffect *self)
{
    if (self->frame == 0)
        rag_play_wave_here(self, "effect\\assasin_cloaking.wav");
    render_object_set_argb(self->master, 50, -1, -1, -1);
    self->base.has_shadow = 0;
}

unsigned char rag_concentration(struct RagEffect *self)
{
    if (self->frame == 0)
        rag_play_wave_here(self, "effect\\ac_concentration.wav");
    return rag_str_step(self, 0.0f, -80.0f, 0.0f) != 0;
}

unsigned char rag_two_hand_quicken(struct RagEffect *self)
{
    if (self->frame == 0) {
        self->duration = 9999;
        rag_play_wave_here(self, "effect\\knight_twohandquicken.wav");
    }
    return rag_str_step(self, 0.0f, -80.0f, 0.0f) != 0;
}

static void launch_drain_trail(struct RagEffect *self, int angle, int r, int g, int b)
{
    struct EffectPrim *p = rag_launch_prim(self, 8, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;

    self->last_prim = p;
    p->duration = 60;
    p->delta2.x = 0.0f;
    p->delta2.y = -10.0f;
    p->delta2.z = 0.0f;
    p->size[0].value = render_object_calc_ground_dist(&p->base, self->offset.x, self->offset.z);
    render_object_turn_toward(&p->base, self->offset.x, self->offset.z);
    p->angle[0] = -p->base.roty;
    p->speed = p->size[0].value / (float)p->duration * 2.0f;
    p->fall_speed = 2.0f;
    p->gravity = p->fall_speed / (float)p->duration * -2.0f;
    p->angle_speed[1] = -2.0f;
    p->angle_accel[1] = p->angle_speed[1] / (float)p->duration * -2.0f;
    p->angle[2] = (float)angle;
    prim_matrix_yaw(p, p->angle[0]);
    mat_apply(p->matrix, 0.0f, 0.0f, 1.0f, &p->drift);
    p->swing_axis = p->drift;
    p->r = r;
    p->g = g;
    p->b = b;
    p->scale = 1.7f;
    render_object_load_spr_res(&p->base, RAG_SPRITE_DIR "particle1.spr");
    render_object_load_act_res(&p->base, RAG_SPRITE_DIR "particle1.act");
    p->particle_count = 7;
    prim_sync_particles(p);
}

void rag_blood_drain(struct RagEffect *self, int count)
{
    int angle;

    if (self->frame != 0 || count <= 0)
        return;
    for (angle = -90; count != 0; angle += 90, count--)
        launch_drain_trail(self, angle, 250, 100, 100);
}

void rag_energy_drain(struct RagEffect *self)
{
    int angle;

    if (self->frame != 0)
        return;
    for (angle = -90; angle < 180; angle += 90)
        launch_drain_trail(self, angle, 100, 100, 250);
}

static void launch_green_ring(struct RagEffect *self, const Vec3 *offset, int yaw)
{
    struct EffectPrim *p = rag_launch_prim(self, 15, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;

    self->last_prim = p;
    p->duration = 9999;
    p->endless = 1;
    p->pattern = 0x28;
    p->size2_flip_period = 0x32;
    p->delta2 = *offset;
    p->angle[0] = (float)yaw;
    p->size[1].value = 2.5f;
    p->size[2].value = 15.0f;
    p->size[2].speed = 0.25f;
    p->alpha = 180.0f;
    p->fade_frame = 9964;
    prim_set_texture(p, "effect\\magic_green.tga");
}

void rag_green_rings(struct RagEffect *self)
{
    float turn[12];
    int yaw, side;

    if (self->frame != 0)
        return;
    yaw = (int)(180.0f - self->master->roty);
    if (yaw < 0)
        yaw += 360;
    mat_set_rotate_y(turn, deg_to_rad(yaw));
    self->duration = 9999;
    for (side = 1; side >= -1; side -= 2) {
        float d = (float)side * 2.5f;
        Vec3 offset;

        mat_apply(turn, 0.0f, 0.0f, d, &offset);
        launch_green_ring(self, &offset, yaw + 90);
        mat_apply(turn, d, 0.0f, 0.0f, &offset);
        launch_green_ring(self, &offset, yaw);
    }
}

void rag_fire_pillar_on(struct RagEffect *self, unsigned char white)
{
    int i, step = 6, ring = 4;

    if (self->frame != 0)
        return;
    for (i = 1; i <= 3; i++, step -= 2, ring--) {
        struct EffectPrim *p = rag_launch_prim(self, 13, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;

        self->last_prim = p;
        p->duration = self->duration;
        p->angle_speed[0] = -3.0f;
        p->radius[1] = (float)(i + 3);
        p->radius[0] = (float)((double)i + 1.5);
        p->size[2].value = (float)(step * ring + 1) + 2.5f;
        p->angle_speed[2] = -0.5f;
        p->alpha = (float)(240 - self->frame * 7);
        prim_new_textures(p);
        p->textures[0] = rag_texture(white ? "effect\\ring_white.tga" : "effect\\magic_red.tga");
    }
}

void rag_potion_pillar(struct RagEffect *self, float rise, int frames)
{
    struct EffectPrim *p = rag_launch_prim(self, 13, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;

    self->last_prim = p;
    p->duration = frames;
    p->radius[1] = 4.5f;
    p->radius[0] = 4.5f;
    p->size[2].value = 10.0f;
    p->size[2].speed = rise;
    p->size[2].accel = 0.01f;
    p->alpha = 0.0f;
    p->alpha_max = 90.0f;
    p->alpha_speed = p->alpha_max / 20.0f;
    p->fade_frame = p->duration - 10;
    prim_set_texture(p, "effect\\alpha_down.tga");
}

unsigned char rag_cart_revolution(struct RagEffect *self)
{
    if (self->frame == 7 || self->frame == 20) {
        struct EffectPrim *p;

        rag_play_wave_here(self, "effect\\EF_MagnumBreak.wav");
        p = rag_launch_prim(self, 12, 0.0f, 0.0f, 0.0f);
        if (!p)
            return 0;
        self->last_prim = p;
        p->duration = 20;
        p->list_flags |= 0x200;
        p->alpha_max = 180.0f;
        p->angle[1] = 90.0f;
        p->size[0].speed = 1.75f;
        p->size[0].accel = p->size[0].speed / (float)p->duration * -0.5f;
        p->radius[0] = 5.0f;
        p->alpha = 0.0f;
        p->alpha_speed = p->alpha_max / 15.0f;
        p->fade_frame = p->duration - 15;
        prim_sync_particles(p);
        prim_set_texture(p, "effect\\ring_yellow.tga");

        p = rag_launch_prim(self, 14, 0.0f, 0.0f, 0.0f);
        if (!p)
            return 0;
        self->last_prim = p;
        p->duration = 20;
        p->size[0].speed = 1.35f;
        p->size[0].accel = p->size[0].speed / (float)p->duration * -0.5f;
        p->angle_speed[0] = 3.0f;
        p->alpha = 0.0f;
        p->alpha_max = 240.0f;
        p->alpha_speed = p->alpha_max / 7.0f;
        p->fade_frame = p->duration - 15;
        prim_set_texture(p, "effect\\대폭발.tga");
    }
    if (self->frame >= 300)
        return 0;
    rag_str_step(self, 0.0f, -80.0f, 0.0f);
    return 1;
}

void rag_sonic_blow_hit(struct RagEffect *self)
{
    struct EffectPrim *p;
    int yaw, x_rnd, y_rnd;

    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\assasin_sonicblow.wav");
    if (self->frame != 0)
        return;
    yaw = (int)(180.0f - self->master->roty);
    x_rnd = rand() % 10;
    y_rnd = rand() % 20;
    rand();
    p = rag_launch_prim(self, 13, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    self->last_prim = p;
    p->duration = 12;
    p->angle[0] = (float)(yaw < 0 ? yaw + 360 : yaw);
    p->angle[1] = -90.0f;
    prim_matrix_pitch(p, p->angle[1]);
    prim_matrix_turn_yaw(p, p->angle[0]);
    mat_apply(p->matrix, 0.0f, -2.0f, 0.0f, &p->velocity);
    p->delta.x = p->velocity.x + p->delta.x;
    p->delta.y = p->velocity.y + p->delta.y;
    p->delta.z = p->velocity.z + p->delta.z;
    p->delta2.z = 0.0f;
    p->delta2.x = (float)(((x_rnd < 0 ? 0 : x_rnd) + 1) / 4);
    p->delta2.y = (float)(((y_rnd < 0 ? 0 : y_rnd) + 1) / 2 - 11);
    p->speed = 0.4f;
    p->accel = p->speed / (float)p->duration * -0.5f;
    p->radius[1] = 7.0f;
    p->radius[0] = 4.0f;
    p->size[2].value = 3.5f;
    p->fade_frame = p->duration - p->duration / 2;
    prim_set_texture(p, "effect\\magic_red.tga");
}

void rag_over_thrust(struct RagEffect *self)
{
    struct EffectPrim *p;

    if (self->frame == 0)
        rag_play_wave_here(self, "effect\\black_overthrust.wav");
    rag_project_master(self);
    if (self->frame != 0)
        return;
    self->duration = 9999;
    rag_play_wave_here(self, "effect\\EF_StoneCurse.wav");
    p = rag_launch_prim(self, 4, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    self->last_prim = p;
    p->duration = 40;
    p->delta2.y = -20.0f;
    p->arc_degrees = 10.0f;
    p->radius[0] = 10.0f;
    p->size[0].speed = 7.0f;
    p->size[0].accel = p->size[0].speed / (float)(p->duration + 40) * -2.0f;
    p->fade_frame = 0;
    prim_set_texture(p, "effect\\alpha_center.tga");
}

static struct EffectPrim *launch_grimtooth_shard(struct RagEffect *self)
{
    struct EffectPrim *p = rag_launch_prim(self, 9, 0.0f, 0.0f, 0.0f);
    if (!p)
        return NULL;

    self->last_prim = p;
    p->pattern |= 1;
    p->duration = 40;
    rand_below(360);
    rand();
    return p;
}

void rag_grimtooth(struct RagEffect *self)
{
    Vec3 *walker = &self->head_offset;
    Vec3 *step = &self->offset2;

    if (self->frame == 0) {
        float turn[12];

        rag_play_wave_here(self, "effect\\EF_FrostDiver.wav");
        render_object_turn_toward(&self->base, self->offset.x, self->offset.z);
        mat_set_rotate_y(turn, deg_to_rad((int)-self->base.roty));
        mat_apply(turn, 0.0f, 0.0f, 2.0f, step);
        walker->x -= step->x;
        walker->y -= step->y;
        walker->z -= step->z;
    }
    if (self->id_state != 0)
        return;
    if (self->frame % 3 == 0) {
        struct EffectPrim *p;
        Vec3 pos = self->base.pos;
        float dist = render_object_calc_ground_dist(&self->base, self->offset.x, self->offset.z);

        if (2.5f >= dist)
            self->id_state = 1;
        self->base.pos = pos;
        p = launch_grimtooth_shard(self);
        if (!p)
            return;
        p->delta2.x = 0.0f;
        p->delta2.y = 20.0f;
        p->delta2.z = 0.0f;
        p->delta2.x = p->delta2.x + walker->x;
        p->delta2.y = walker->y + p->delta2.y;
        p->delta2.z = walker->z + p->delta2.z;
        p->scale = (float)(rand_below(40) + 60) / 100.0f;
        p->size[2].value = 10.0f;
        p->angle[0] = (float)rand_below(360);
        p->angle[1] = (float)(rand_below(30) + 75);
        prim_matrix_pitch(p, p->angle[1]);
        prim_matrix_turn_yaw(p, p->angle[0]);
        prim_keep_matrix(p);
        p->speed = 3.0f;
        p->accel = p->speed / (float)p->duration * -2.0f;
        p->pattern_arg = 6;
        p->fade_frame = p->duration - 10;
        prim_set_texture(p, "effect\\stone.bmp");
    }
    walker->x -= step->x;
    walker->y -= step->y;
    walker->z -= step->z;
}

void rag_grimtooth_attack(struct RagEffect *self)
{
    Vec3 *walker = &self->head_offset;
    int i;

    if (self->frame != 0)
        return;
    for (i = 0; i < 3; i++) {
        struct EffectPrim *p = rag_launch_prim(self, 9, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;

        self->last_prim = p;
        p->pattern |= 1;
        p->list_flags |= 0x200;
        self->duration = 1000;
        p->duration = 1000;
        switch (i) {
        case 0: p->delta2.x = 0.0f;   p->delta2.z = -12.0f; p->delta2.y = 40.0f; break;
        case 1: p->delta2.x = 12.0f;  p->delta2.z = 6.0f;   p->delta2.y = 40.0f; break;
        case 2: p->delta2.x = -12.0f; p->delta2.z = 6.0f;   p->delta2.y = 40.0f; break;
        }
        p->scale = 0.9f;
        p->size[2].value = 25.0f;
        p->angle[0] = (float)((6 - i) * 120);
        p->angle[1] = 75.0f;
        prim_matrix_pitch(p, p->angle[1]);
        prim_matrix_turn_yaw(p, p->angle[0]);
        prim_keep_matrix(p);
        p->speed = 3.5f;
        p->accel = 0.001f;
        p->pattern_arg = 10;
        p->fade_frame = p->duration - 10;
        prim_set_texture(p, "effect\\stone.bmp");
        walker->x -= self->offset2.x;
        walker->y -= self->offset2.y;
        walker->z -= self->offset2.z;
    }
}

void rag_heavens_drive(struct RagEffect *self)
{
    int x, z;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\wizard_earthspike.wav");
    game_send_message(GAME_MSG_QUAKE, 0, 0, 0, 0);
    self->duration += 50;
    for (z = -10; z < 15; z += 5) {
        for (x = -10; x < 15; x += 5) {
            struct EffectPrim *p = rag_launch_prim(self, 9, 0.0f, 0.0f, 0.0f);
            if (!p)
                return;
            int size_rnd;

            self->last_prim = p;
            p->duration = rand_below(30) + (self->duration - 10);
            p->pattern = 1;
            p->pattern |= 2;
            rand_below(360);
            rand();
            size_rnd = rand_below(60);
            p->delta2.y = 10.0f;
            p->delta2.x = (float)x;
            p->delta2.z = (float)z;
            p->scale = (float)(rand_below(50) + 300) / 100.0f;
            p->size[2].value = (float)(size_rnd + 1) / 10.0f + 9.0f;
            p->angle[0] = (float)rand_below(360);
            p->angle[1] = (float)(rand_below(20) + 80);
            prim_matrix_pitch(p, p->angle[1]);
            prim_matrix_turn_yaw(p, p->angle[0]);
            prim_keep_matrix(p);
            p->next_velocity.y = -1.0f;
            p->next_velocity_accel.y = -0.01f;
            p->speed = 1.0f;
            p->accel = 0.01f;
            p->change_frame = 11;
            p->next_speed = -1.2f;
            p->next_accel = 0.0f;
            p->pattern_arg = 14;
            prim_set_texture(p, "effect\\stone.bmp");
        }
    }
}

static void launch_dark_spray(struct RagEffect *self, int kind, int yaw)
{
    struct EffectPrim *p = rag_launch_prim(self, kind, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    float reach;

    self->last_prim = p;
    p->angle[0] = (float)(rand_below(80) + yaw);
    p->angle[1] = (float)(rand_below(100) - 50);
    prim_matrix_yaw(p, p->angle[0]);
    prim_matrix_turn_pitch(p, p->angle[1]);
    reach = (float)(rand_below(4) + 4);
    mat_apply(p->matrix, 0.0f, 0.0f, reach, &p->drift);
    p->delta2.x = p->drift.x + 0.0f;
    p->delta2.y = p->drift.y - 10.0f;
    p->delta2.z = p->drift.z + 0.0f;
    if (kind == 5) {
        p->speed = (float)(rand_below(90) + 60) / 100.0f;
        p->duration = rand_below(24) + 6;
        p->accel = p->speed / (float)p->duration * -0.5f;
    } else {
        prim_matrix_yaw(p, p->angle[0]);
        p->duration = rand_below(24) + 6;
        p->speed = (float)(rand_below(90) + 60) / 100.0f;
        p->accel = p->speed / (float)p->duration * -0.5f;
        p->fall_speed = (float)(rand_below(90) + 30) / -100.0f;
        p->gravity = -(p->fall_speed / (float)p->duration * 2.0f);
    }
    p->scale = (float)(rand_below(100) + 60) / 100.0f;
    p->r = 20;
    p->g = 20;
    p->b = 20;
    p->fade_frame = 0;
    render_object_load_spr_res(&p->base, RAG_SPRITE_DIR "particle1.spr");
    render_object_load_act_res(&p->base, RAG_SPRITE_DIR "particle1.act");
    p->particle_count = 3;
    prim_sync_particles(p);
}

void rag_dark_hit(struct RagEffect *self)
{
    struct EffectPrim *p;
    int yaw, i;

    if (self->frame != 0)
        return;
    yaw = (int)(180.0f - self->master->roty);
    if (yaw < 0)
        yaw += 360;
    p = rag_launch_prim(self, 13, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    self->last_prim = p;
    p->duration = 10;
    p->angle[0] = (float)yaw;
    p->angle[1] = -90.0f;
    prim_matrix_pitch(p, p->angle[1]);
    prim_matrix_turn_yaw(p, p->angle[0]);
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

    for (i = 0; i < 2; i++)
        launch_dark_spray(self, 5, yaw - 40);
    for (i = 0; i < 2; i++)
        launch_dark_spray(self, 6, yaw + 140);
}

void rag_make_blur(struct RagEffect *self)
{
    struct World *world = game_current_world();
    struct RenderObject *m = self->master;
    void *blur;

    if (!world || !self->master)
        return;
    if (MASTER_TINTED(m)) {
        render_object_set_argb(self->master, -1, HALF_TINT(self, m, 0, self->base.pos.x),
                               HALF_TINT(self, m, 1, self->base.pos.y), HALF_TINT(self, m, 2, self->base.pos.z));
        if (self->frame == 9998)
            self->frame = 0;
        if (self->frame % 5 != 0)
            return;
        if (!object_as_type(self->master, OBJECT_PC))
            return;
        blur = mem_alloc(sizeof(struct BlurPC));
        blur = blur ? blur_pc_construct(blur, (struct Pc *)self->master) : NULL;
        if (!blur)
            return;
        m = self->master;
        render_object_set_argb((struct RenderObject *)blur, -1, HALF_TINT(self, m, 0, self->base.pos.x),
                               HALF_TINT(self, m, 1, self->base.pos.y), HALF_TINT(self, m, 2, self->base.pos.z));
        effect_add_world_object(world, blur);
        return;
    }

    render_object_set_argb(self->master, -1, (int)self->base.pos.x, (int)self->base.pos.y,
                           (int)self->base.pos.z);
    if (self->frame == 9998)
        self->frame = 0;
    if (self->base.pos.x == -1.0f) {
        if (rand() % 14 > 0)
            render_object_set_argb(self->master, -1, 250, 200, 200);
        else
            render_object_set_argb(self->master, -1, 250, 250, 250);
        return;
    }
    if (self->base.pos.x == -2.0f) {
        render_object_set_argb(self->master, -1, 250, 250, 200);
        return;
    }
    if (self->duration != 30) {
        int action;

        if (self->frame % 5 != 0)
            return;
        action = self->master->state;
        if (action == 0 || action == 10 || action == 6 || action == 11 || action == 13)
            return;
    }
    if (!object_as_type(self->master, OBJECT_PC))
        return;
    blur = mem_alloc(sizeof(struct BlurPC));
    blur = blur ? blur_pc_construct(blur, (struct Pc *)self->master) : NULL;
    if (!blur)
        return;
    render_object_send((struct RenderObject *)blur, &self->base, 0,
                                                  (int)self->base.pos.x, (int)self->base.pos.y,
                                                  (int)self->base.pos.z, 0);
    effect_add_world_object(world, blur);
}

void rag_snow(struct RagEffect *self)
{
    const struct RenderObject *player = (const struct RenderObject *)game_current_player();
    void *attr = game_current_map_attr();
    int i;

    if (!player || !attr)
        return;
    self->base.pos = player->pos;
    if (rag_effect_get_duration(self) - self->frame <= 160)
        return;
    for (i = 0; i < 2; i++) {
        struct EffectPrim *p = rag_launch_prim(self, 21, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        float height;
        int dist_rnd;

        self->last_prim = p;
        p->master_pos = self->base.pos;
        p->duration = 320;
        mat_set_rotate_y(p->matrix, deg_to_rad(rand_below(360)));
        dist_rnd = rand_below(300);
        height = c3d_attr_get_height(attr, self->base.pos.x, self->base.pos.z) - 100.0f;
        mat_apply(p->matrix, 0.0f, height, (float)(dist_rnd - 300), &p->delta2);
        mat_set_rotate_x(p->matrix, deg_to_rad(90));
        p->speed = -0.5f;
        p->accel = 0.0f;
        p->scale = (float)(rand_below(15) + 25) / 50.0f;
        p->angle_speed[2] = 0.0f;
        p->alpha = 0.0f;
        p->alpha_speed = p->alpha_max / 10.0f;
        p->fade_frame = p->duration - p->duration / 5;
        render_object_load_spr_res(&p->base, RAG_SPRITE_DIR "ef_snow.spr");
        render_object_load_act_res(&p->base, RAG_SPRITE_DIR "ef_snow.act");
        prim_sync_particles(p);
    }
}

void rag_status_icons(struct RagEffect *self)
{
    int count = 0, row = 0, x = 0, y = 0, bottom, pass, i;
    size_t state_index;

    if (g_renderer == NULL)
        return;
    if (self->frame == RAG_FOREVER - 1)
        self->frame = 0;
    if (self->hit_count != 0) {
        self->hit_count = 0;
        self->frame = 0;
        rag_effect_send_msg(self, NULL, RAG_MSG_PRIM_GONE, 100, 0, 0);
    }
    if (self->frame != 0)
        return;
    bottom = g_renderer->height - 50;
    for (i = 0; i < 100; i++)
        g_session1.stateIconSlots[i] = -1;
    for (pass = 0; pass < 5; pass++) {
        for (state_index = 0; state_index < own_states_count(); ++state_index) {
            struct EffectPrim *p;
            int id = own_states_at(state_index)->index, wrap;
            const char *image = session_get_efst_image(&g_session1, id, pass);

            if (id == 0x2a)
                id = 0xc;
            if (image == NULL)
                continue;
            for (i = 0; i < count; i++)
                if (g_session1.stateIconSlots[i] == id)
                    break;
            if (i < count)
                continue;
            p = rag_launch_prim(self, 0, 0.0f, 0.0f, 0.0f);
            if (!p)
                return;
            self->last_prim = p;
            p->list_flags = 0x201;
            p->use_list_flags = 1;
            p->pattern = 4;
            p->base.motion = 0;
            p->duration = RAG_FOREVER;
            p->size[1].value = 16.0f;
            p->size[2].value = 16.0f;
            wrap = !(row * 35 + 150 < bottom);
            row = wrap ? 0 : row;
            y = wrap ? 35 : y + 35;
            x = wrap ? x - 45 : x;
            p->delta2.x = (float)(g_renderer->width - 32 + x);
            p->delta2.y = (float)(y + 150);
            p->variant = PRIM_VARIANT_STATE_ICON;
            p->records.state_icon.state_id = (short)id;
            p->motion_count = 2;
            prim_new_textures(p);
            p->textures[0] = rag_texture(image);
            p->textures[1] = rag_texture("effect\\white02.bmp");
            if (count < 99) {
                g_session1.stateIconSlots[count] = id;
                count++;
                row++;
            }
        }
    }
}

void rag_summon_slave(struct RagEffect *self)
{
    int i;

    rag_project_master(self);
    if (self->frame != 0) {
        if (self->frame == 35 && self->master)
            render_object_send(self->master, NULL, 0x3f, 0, 0, 0, 0);
        return;
    }
    for (i = 0; i < 20; i++) {
        struct EffectPrim *p = rag_launch_prim(self, 3, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;

        self->last_prim = p;
        p->duration = 40;
        p->delta2.y = -40.0f;
        p->angle[0] = (float)rand_below(360);
        p->angle_speed[0] = (float)(rand_below(60) + 10) / 10.0f;
        p->angle_accel[0] = p->angle_speed[0] / (float)p->duration / -1.5f;
        p->arc_degrees = (float)(rand_below(25) + 5) / 10.0f;
        p->size[2].value = (float)(rand_below(40) + 20);
        p->size[2].speed = (float)(rand_below(30) + 20) / 10.0f;
        p->alpha = 0.0f;
        p->alpha_max = 200.0f;
        p->alpha_speed = p->alpha_max / 10.0f;
        p->fade_frame = p->duration - p->duration / 3;
        prim_set_texture(p, "EFFECT\\alpha_center.tga");
    }
}

static struct EffectPrim *launch_orbit(struct RagEffect *self, int kind)
{
    struct EffectPrim *p = rag_launch_prim(self, kind, 0.0f, 0.0f, 0.0f);
    if (!p)
        return NULL;

    self->last_prim = p;
    p->list_flags = 0;
    p->list_flags |= 1;
    p->list_flags |= 5;
    p->duration = self->duration;
    return p;
}

void rag_gumgang(struct RagEffect *self, char count, char red)
{
    struct EffectPrim *p;
    int i, n;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_orbit(self, 0x23);
    if (!p)
        return;
    p->motion_count = 5;
    prim_new_textures(p);
    p->textures[0] = rag_texture("effect\\super1.bmp");
    p->textures[1] = rag_texture("effect\\super2.bmp");
    p->textures[2] = rag_texture("effect\\super3.bmp");
    p->textures[3] = rag_texture("effect\\super4.bmp");
    p->textures[4] = rag_texture("effect\\super5.bmp");
    n = count;
    for (i = 0; n > 0; i++, n--) {
        struct PrimSparkGroup *g = &p->records.spark_groups[i];

        g->active = 1;
        g->size = 3.5f;
        g->point[2].y = (float)-rand_below(10);
        g->spark[0].age = 10.0f;
        g->spark[0].alpha = 70.0f;
        g->spark[0].texture = (float)rand_below(5);
        g->spark[0].angle = (float)rand_below(360);
        g->point[0].y = (float)-rand_below(10);
        g->spark[1].age = 10.0f;
        g->spark[1].alpha = 130.0f;
        g->spark[1].texture = (float)rand_below(5);
        g->spark[1].angle = (float)rand_below(360);
        g->point[3].y = (float)-rand_below(10);
        g->spark[2].age = 10.0f;
        g->spark[2].alpha = 190.0f;
        g->spark[2].texture = (float)rand_below(5);
        g->spark[2].angle = (float)rand_below(360);
        g->point[1].y = (float)-rand_below(10);
        g->spark[3].age = 10.0f;
        g->spark[3].alpha = 250.0f;
        g->spark[3].texture = (float)rand_below(5);
        g->spark[3].angle = (float)rand_below(360);
        g->tint = (unsigned char)red;
    }
}

static void level99_layer(struct PrimRing *ring, float height, short turn, float radius, short tilt)
{
    ring->active = 1;
    ring->age = 0;
    ring->span = 315;
    ring->height = height;
    ring->turn = turn;
    ring->alpha = 120;
    ring->radius = radius;
    ring->tilt = tilt;
    ring->offset_flag = 0.0f;
}

void rag_level99_aura(struct RagEffect *self, const char *texture, char big)
{
    struct EffectPrim *p;

    rag_follow_master(self);
    if (!rag_restart_forever(self))
        return;
    p = launch_orbit(self, 0x17);
    if (!p)
        return;
    prim_set_texture(p, texture);
    p->scale = big == 1 ? 7.0f : 4.0f;
    level99_layer(&p->records.rings[0], 15.0f, 0, 3.9f, 55);
    level99_layer(&p->records.rings[1], 13.0f, 90, 4.1f, 50);
    level99_layer(&p->records.rings[2], 11.0f, 180, 4.3f, 45);
    p->records.rings[3].active = 0;
}

void rag_level99_aura2(struct RagEffect *self, const char *texture, char size)
{
    struct EffectPrim *p;
    struct PrimTexRecord *recs;

    rag_follow_master(self);
    if (!rag_restart_forever(self))
        return;
    p = rag_launch_prim(self, 0x1c, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    self->last_prim = p;
    p->list_flags = 0;
    p->list_flags = 0xd;
    p->duration = self->duration;
    prim_set_texture(p, texture);
    if (size == 1)
        p->scale = 15.0f;
    else if (size == 2)
        p->scale = 17.0f;
    else if (size == 3)
        p->scale = 18.0f;
    else
        p->scale = 4.0f;
    recs = p->records.tex_records;
    recs[0].active = 1;
    recs[1].active = 1;
    recs[2].active = 0;
    recs[3].active = 0;
    recs[0].size = size == 3 ? 24.0f : 15.0f;
    recs[1].size = recs[0].size;
    recs[0].bob = 0;
    recs[1].bob = 180;
    recs[0].alpha = 200;
    recs[1].alpha = 200;
    recs[0].y = -1.0f;
    recs[1].y = -1.0f;
}

static void level99_3_drift(Vec3 *v, char style)
{
    if (style == 3) {
        v->x = (float)(rand_below(15) - 8);
        v->z = (float)(rand_below(15) - 8);
        return;
    }
    v->x = style == 6 ? (float)(rand_below(35) - 17) : 0.0f;
    v->z = 0.0f;
}

void rag_level99_aura3(struct RagEffect *self, char style)
{
    struct EffectPrim *p;
    int i;

    rag_follow_master(self);
    if (!rag_restart_forever(self))
        return;
    p = launch_orbit(self, 0x1d);
    if (!p)
        return;
    prim_new_textures(p);
    p->textures[0] = rag_texture(style == 3 ? "effect\\ghost.bmp"
                                 : style >= 1 ? "effect\\whitelight.tga" : "effect\\freezing_circle.bmp");
    p->scale = 0.0f;
    if (style == 2)
        p->scale = 7.0f;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimAuraGroup *g = &p->records.aura_groups[i];
        int hidden = (style == 3 && i >= 1) || ((style == 7 || style == 8) && i >= 2);

        g->active = hidden ? 0 : 1;
        g->speed = 0.15f;
        if (style == 3) {
            g->size = 3.2f;
            g->speed = 0.6f;
            g->point[2].x = (float)(rand_below(15) - 8);
            g->point[2].z = (float)(rand_below(15) - 8);
        } else {
            if (style == 1 || style == 4 || style == 5 || style >= 8 || style == 6)
                g->size = 2.4f;
            else
                g->size = 0.8f;
            g->point[2].x = style == 6 ? (float)(rand_below(35) - 17) : 0.0f;
            g->point[2].z = 0.0f;
        }
        g->point[2].y = (float)rand_below(100);
        g->sway[0].angle = (float)rand_below(360);
        g->sway[0].target = (float)rand_below(360);
        g->sway[1].angle = (float)rand_below(360);
        g->sway[1].target = (float)rand_below(360);
        level99_3_drift(&g->point[0], style);
        g->point[0].y = (float)rand_below(100);
        g->sway[2].angle = (float)rand_below(360);
        g->sway[2].target = (float)rand_below(360);
        g->sway[3].angle = (float)rand_below(360);
        g->sway[3].target = (float)rand_below(360);
        level99_3_drift(&g->point[3], style);
        g->point[3].y = (float)rand_below(100);
        g->sway[4].angle = (float)rand_below(360);
        g->sway[4].target = (float)rand_below(360);
        g->sway[5].angle = (float)rand_below(360);
        g->sway[5].target = (float)rand_below(360);
        level99_3_drift(&g->point[1], style);
        g->point[1].y = (float)rand_below(100);
        g->sway[6].angle = (float)rand_below(360);
        g->sway[6].target = (float)rand_below(360);
        g->sway[7].angle = (float)rand_below(360);
        g->sway[7].target = (float)rand_below(360);

        if (style == 4) {
            g->point[2].y = -40.0f;
            g->point[0].y = -50.0f;
            g->point[3].y = -60.0f;
            g->point[1].y = -70.0f;
            g->style = 14;
            continue;
        }
        if (style == 6) {
            g->point[2].y = (float)(-40 - rand_below(40));
            g->point[0].y = (float)(-40 - rand_below(40));
            g->point[3].y = (float)(-40 - rand_below(40));
            g->point[1].y = (float)(-40 - rand_below(40));
            g->style = 0x10;
            continue;
        }
        if (style == 5 || style == 7) {
            g->point[2].y = 0.0f;
            g->point[0].y = 0.0f;
            g->point[3].y = 0.0f;
            g->point[1].y = 0.0f;
        }
        if (style == 1) {
            g->style = 1;
        } else if (style == 3) {
            g->style = 11;
        } else if (style == 5 || style == 7 || style == 8) {
            g->style = 15;
            g->drift_count[0] = 0;
            g->drift_count[1] = 0;
            g->drift_count[2] = 0;
            g->drift_count[3] = 0;
            g->tint = (unsigned char)style;
        } else {
            g->style = 0;
        }
    }
}

void rag_sakura(struct RagEffect *self)
{
    struct EffectPrim *p;
    struct PrimSpriteMover *mv;

    rag_follow_master(self);
    if (self->frame == RAG_FOREVER - 1) {
        self->frame = 0;
        self->duration = RAG_FOREVER;
    }
    if ((self->frame & 1) != 0 || self->frame >= 300)
        return;
    p = rag_launch_prim(self, 0x28, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    self->last_prim = p;
    p->duration = self->duration;
    mv = &p->records.sprite_mover;
    mv->active = 1;
    mv->speed = (float)(rand_below(2) + 2) * 0.1f;
    p->scale = (float)(rand_below(6) + 22) / 100.0f;
    mv->sway[0][0] = (float)rand_below(360);
    mv->sway[0][1] = (float)rand_below(360);
    mv->sway[1][0] = (float)rand_below(360);
    mv->sway[1][1] = (float)rand_below(360);
    mv->pos.x = (float)rand_below(300) + self->base.pos.x - 150.0f;
    mv->pos.y = self->base.pos.y - 200.0f;
    mv->pos.z = (float)rand_below(300) + self->base.pos.z - 150.0f;
    mv->style = 0;
    p->alpha = 200.0f;
    render_object_load_spr_res(&p->base, RAG_SPRITE_DIR "sakura01.spr");
    render_object_load_act_res(&p->base, RAG_SPRITE_DIR "sakura01.act");
    render_object_set_action_base(&p->base, rand_below(3), 0);
    prim_sync_particles(p);
}
