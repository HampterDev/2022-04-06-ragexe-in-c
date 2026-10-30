#include "render/texture_cache.h"
#include "game/context.h"
#include <stdlib.h>
#include <string.h>
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"

#define EFFECT_DIR "이팩트\\"

#define GAME_MSG_QUAKE 0x7a

static struct Texture *texture(const char *name)
{
    return texture_cache_get(&g_textureCache, name, 0, 0);
}

static struct EffectPrim *launch_prim(struct RagEffect *self, int kind, int motions)
{
    struct EffectPrim *p = rag_launch_prim(self, kind, 0.0f, 0.0f, 0.0f);
    if (!p)
        return NULL;

    self->last_prim = p;
    p->list_flags = 5;
    p->duration = self->duration;
    if (motions != 0)
        p->motion_count = motions;
    p->textures = rag_prim_new_textures(p);
    return p;
}

static void follow_master(struct RagEffect *self)
{
    self->base.pos = self->master->pos;
}

static int rand_below(int n)
{
    int r = rand() % n;
    return r < 0 ? 0 : r;
}

static void wall_ring(struct EffectPrim *p, int i, short span, float height, short turn, float radius,
                      short tilt)
{
    struct PrimRing *ring = &p->records.rings[i];

    ring->active = 1;
    ring->age = 0;
    ring->span = span;
    ring->height = height;
    ring->turn = turn;
    ring->alpha = 0;
    ring->radius = radius;
    ring->tilt = tilt;
    ring->offset_flag = 0.0f;
}

void rag_poison_particles(struct RagEffect *self)
{
    struct EffectPrim *p;
    const float *m;
    float d;
    int f = self->frame;

    if (f == 0 || f == 5 || f == 11 || f == 18 || f == 26 || f == 40)
        rag_play_wave_here(self, "effect\\assasin_enchantpoison.wav");
    if (self->frame % 3 != 0)
        return;
    p = rag_launch_prim(self, 5, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    self->last_prim = p;
    p->duration = 40;
    mat_set_rotate_y(p->matrix, deg_to_rad(rand_below(360)));
    d = (float)(rand_below(6) + 2);
    m = p->matrix;
    p->delta2.x = m[3] * 0.0f + m[0] * 0.0f + m[6] * d + m[9];
    p->delta2.y = m[4] * 0.0f + m[1] * 0.0f + m[7] * d + m[10];
    p->delta2.z = m[5] * 0.0f + m[2] * 0.0f + m[8] * d + m[11];
    mat_set_rotate_x(p->matrix, deg_to_rad(90));
    p->speed = (float)(rand_below(50) + 30) / 100.0f;
    p->accel = p->speed / (float)p->duration * -0.5f;
    p->scale = (float)(rand_below(20) + 20) / 100.0f;
    p->angle_speed[2] = 3.0f;
    p->alpha = 0.0f;
    p->alpha_speed = p->alpha_max / 10.0f;
    p->fade_frame = p->duration - p->duration / 5;
    render_object_load_spr_res(&p->base, EFFECT_DIR "particle3.spr");
    render_object_load_act_res(&p->base, EFFECT_DIR "particle3.act");
    prim_sync_particles(p);
}

void rag_tumbling_items(struct RagEffect *self, const char *tex, float size, char look)
{
    struct EffectPrim *p;
    struct PrimOrbitSquare *sq;
    int i;

    if (self->frame != 0)
        return;
    follow_master(self);
    p = launch_prim(self, 43, 0);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    self->base.pos.y -= 12.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        sq = &p->records.orbit_squares[i];
        sq->active = 1;
        sq->pos.x = self->offset.x;
        sq->pos.y = self->offset.y;
        sq->pos.z = self->offset.z;
        sq->age = -18 - rand_below(30);
        sq->radius = 12.0f;
        sq->radius_speed = 1.0f;
        sq->heading = (short)rand_below(360);
        sq->orbit_tilt = (short)rand_below(360);
        sq->spin = (short)rand_below(360);
        sq->tilt = (float)rand_below(360);
        sq->size = size;
        sq->alpha = 0;
        if (look == 1)
            sq->color = 6;
        else
            sq->color = look == 7 ? 7 : 2;
    }
}

void rag_guard_ring(struct RagEffect *self, const char *tex)
{
    struct EffectPrim *p;
    struct PrimSlash *slash;

    follow_master(self);
    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\black_maximize_power_sword_bic.wav");
    p = launch_prim(self, 61, 2);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    p->textures[1] = texture("effect\\a01.bmp");
    p->scale = 11.0f;
    slash = p->records.slashes;
    slash[3].active = 0;
    slash[2].active = 0;
    slash[1].active = 0;
    slash[0].active = 1;
    slash[0].age = 0;
    slash[0].radius = 10.0f;
    slash[0].alpha = 0;
    slash[0].width = 1.0f;
    slash[0].flare = 0.0f;
    slash[0].color = 1;
}

void rag_soul_light(struct RagEffect *self, const char *tex)
{
    struct EffectPrim *p;
    struct PrimLaunchSquare *sq;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = rag_launch_prim(self, 109, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    self->last_prim = p;
    p->list_flags = 0;
    p->list_flags |= 1;
    p->list_flags |= 5;
    p->duration = self->duration;
    p->textures = rag_prim_new_textures(p);
    p->textures[0] = texture(tex);
    p->scale = 0.0f;
    sq = &p->records.launch_squares[0];
    sq->active = 1;
    sq->frame = 0;
    sq->size = 2.5f;
    sq->aid = self->offset.y;
    sq->alpha = 0.0f;
    rand_below(360);
    rand_below(360);
    rand_below(360);
    rand_below(360);
    sq->angle = 0.0f;
    sq->dist = g_sinTable[90] * -30.0f;
    sq->y = -(g_sinTable[0] * 20.0f) - 40.0f;
}

static void letters(struct RagEffect *self, const char *const names[4], int age0, int x0)
{
    struct EffectPrim *p;
    struct PrimHalo *halo;
    int i, k;

    p = launch_prim(self, 68, 4);
    if (!p)
        return;
    for (i = 0; i < 4; i++)
        p->textures[i] = texture(names[i]);
    p->scale = 0.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        halo = &p->records.halos[i];
        halo->active = 1;
        halo->age = age0 + i * 20;
        halo->size = 5.0f;
        halo->shift = (float)(x0 - i * 6);
        halo->texture = (unsigned char)i;
        halo->style = 2;
        for (k = 0; k < HALO_LAYERS; k++)
            halo->layer_alpha[k] = 0.0f;
    }
}

void rag_soul_link_letters(struct RagEffect *self)
{
    static const char *const soul[4] = {
        "effect\\soul_l.tga", "effect\\soul_u.tga", "effect\\soul_o.tga", "effect\\soul_s.tga",
    };
    static const char *const link[4] = {
        "effect\\soul_k.tga", "effect\\soul_n.tga", "effect\\soul_i.tga", "effect\\soul_l.tga",
    };

    follow_master(self);
    if (self->frame == 1)
        letters(self, soul, -60, -4);
    if (self->frame == 21)
        letters(self, link, -120, 22);
}

void rag_light_spheres(struct RagEffect *self, const char *tex, char look)
{
    struct EffectPrim *p;
    struct PrimSparkTrio *sp;
    int i;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_prim(self, 111, 0);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    p->scale = 0.0f;
    for (i = 0; i < 2; i++) {
        sp = &p->records.spark_trios[i];
        sp->active = 1;
        sp->speed = 0.07f;
        sp->size = (float)rand_below(11) * 0.1f + 1.0f;
        sp->frame = 0;
        sp->spot[2].x = self->offset.x;
        sp->spot[2].y = self->offset.y;
        sp->spot[2].z = self->offset.z;
        sp->turn[0].angle = (float)rand_below(360);
        sp->turn[0].target = (float)rand_below(360);
        sp->turn[1].angle = (float)rand_below(360);
        sp->turn[1].target = (float)rand_below(360);
        sp->alpha[0] = 0.0f;
        sp->spot[0].x = self->offset.x;
        sp->spot[0].y = self->offset.y;
        sp->spot[0].z = self->offset.z;
        sp->turn[2].angle = (float)rand_below(360);
        sp->turn[2].target = (float)rand_below(360);
        sp->turn[3].angle = (float)rand_below(360);
        sp->turn[3].target = (float)rand_below(360);
        sp->alpha[1] = 0.0f;
        sp->spot[3].x = self->offset.x;
        sp->spot[3].y = self->offset.y;
        sp->spot[3].z = self->offset.z;
        sp->turn[4].angle = (float)rand_below(360);
        sp->turn[4].target = (float)rand_below(360);
        sp->turn[5].angle = (float)rand_below(360);
        sp->turn[5].target = (float)rand_below(360);
        sp->alpha[2] = 0.0f;
        if (look == 2)
            sp->tint = 4.0f;
        else if (look == 3)
            sp->tint = (float)(rand() % 3 + 1);
        else
            sp->tint = 0.0f;
    }
}

void rag_magic_target(struct RagEffect *self)
{
    struct EffectPrim *p;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_prim(self, 112, 0);
    if (!p)
        return;
    p->textures[0] = texture("effect\\magic_target.tga");
    p->records.ground_targets[0].active = 1;
    p->records.ground_targets[0].turn = 0;
}

void rag_explosion_rings(struct RagEffect *self, const char *tex)
{
    struct EffectPrim *p;
    struct PrimRing *ring;
    int i;

    follow_master(self);
    if (self->frame != 0)
        return;
    follow_master(self);
    p = launch_prim(self, 113, 0);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    p->scale = 4.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        ring = &p->records.rings[i];
        ring->active = 1;
        ring->age = 0;
        ring->span = 315;
        ring->height = 15.0f;
        ring->turn = (short)rand_below(360);
        ring->alpha = 0;
        ring->radius = 20.0f - (float)i * 0.5f;
        ring->tilt = 50;
        ring->offset_flag = 0.0f;
    }
}

void rag_floor_disc(struct RagEffect *self, const char *tex)
{
    struct EffectPrim *p;
    struct PrimSpinBox *bx;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_prim(self, 114, 0);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    p->scale = 5.0f;
    bx = &p->records.spin_boxes[0];
    bx->active = 1;
    p->records.spin_boxes[1].active = 0;
    p->records.spin_boxes[2].active = 0;
    p->records.spin_boxes[3].active = 0;
    bx->size = 1.6f;
    bx->pulse = (short)rand_below(360);
    bx->turn = (short)rand_below(360);
    bx->alpha = 0;
    bx->y = -12.0f;
    bx->grey = (unsigned char)rand_below(31);
}

void rag_light_orbits(struct RagEffect *self, const char *tex)
{
    struct EffectPrim *p;
    struct PrimSparkTrio *sp;
    int i;

    follow_master(self);
    if (self->frame != 30)
        return;
    p = launch_prim(self, 115, 0);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    p->scale = 0.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        sp = &p->records.spark_trios[i];
        sp->active = 1;
        sp->speed = 1.0f;
        sp->size = (float)rand_below(6) * 0.1f + 2.5f;
        sp->frame = 0;
        sp->spot[2].x = 0.0f;
        sp->spot[2].y = 0.0f;
        sp->spot[2].z = 0.0f;
        sp->turn[0].angle = (float)rand_below(360);
        sp->turn[1].angle = (float)rand_below(180);
        sp->alpha[0] = 230.0f;
        sp->spot[0].x = 0.0f;
        sp->spot[0].y = 0.0f;
        sp->spot[0].z = 0.0f;
        sp->turn[2].angle = (float)rand_below(360);
        sp->turn[3].angle = (float)rand_below(180);
        sp->alpha[1] = 230.0f;
        sp->spot[3].x = 0.0f;
        sp->spot[3].y = 0.0f;
        sp->spot[3].z = 0.0f;
        sp->turn[4].angle = (float)rand_below(360);
        sp->turn[5].angle = (float)rand_below(180);
        sp->alpha[2] = 230.0f;
        sp->tint = (float)(rand_below(3) + 1);
    }
}

void rag_shield_boomerang_volley(struct RagEffect *self, const char *tex)
{
    struct EffectPrim *p;
    struct PrimBillboard *bb;
    int f;

    follow_master(self);
    f = self->frame;
    if (f == 0)
        rag_play_wave_here(self, "effect\\cru_shield boomerang.wav");
    else if (f != 3 && f != 6 && f != 9 && f != 12)
        return;
    p = launch_prim(self, 116, 0);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    p->scale = 0.0f;
    bb = &p->records.billboards[0];
    bb->active = 1;
    bb->age = 0;
    bb->pos.x = self->base.pos.x;
    bb->pos.z = self->base.pos.z;
    bb->pos.y = self->base.pos.y - 8.0f;
    bb->size = 5.0f;
    bb->phase = 0.0f;
    bb->alpha = 25;
    bb->heading = (short)rand_below(360);
    bb->speed = 2.5f;
    bb->pos.x -= bb->speed * 15.0f * g_cosTable[bb->heading];
    bb->pos.z -= bb->speed * 15.0f * g_sinTable[bb->heading];
    bb->spin = 0;
    bb->texture = 0;
    bb->color_style = 0;
    p->records.billboards[1].active = 1;
    p->records.billboards[1].alpha = 0;
    p->records.billboards[2].active = 1;
    p->records.billboards[2].alpha = 0;
    p->records.billboards[3].active = 1;
    p->records.billboards[3].alpha = 0;
}

void rag_stone_and_ice(struct RagEffect *self)
{
    struct EffectPrim *p;
    int x, z, k;

    if (self->frame != 0)
        return;
    game_send_message(GAME_MSG_QUAKE, 0, 0, 0, 0);
    for (z = -10; z < 15; z += 5) {
        for (x = -10; x < 15; x += 5) {
            for (k = 0; k < 4; k++) {
                p = rag_launch_prim(self, 117, 0.0f, 0.0f, 0.0f);
                if (!p)
                    return;
                self->last_prim = p;
                p->pattern = 1;
                rand_below(360);
                rand();
                p->delta2.y = 10.0f;
                p->delta2.x = (float)x;
                p->delta2.z = (float)z;
                p->duration = 250;
                p->scale = (float)(rand_below(50) + 300) / 100.0f;
                p->size[2].value = 18.0f;
                p->angle[0] = (float)rand_below(360);
                p->angle[1] = (float)(rand_below(40) + 60);
                mat_set_rotate_x(p->matrix, deg_to_rad((int)p->angle[1]));
                mat_rotate_y(p->matrix, deg_to_rad((int)p->angle[0]));
                memcpy(p->matrix2, p->matrix, sizeof p->matrix2);
                p->speed = 1.0f;
                p->accel = 0.01f;
                p->alpha = 20.0f;
                p->pattern_arg = 15;
                p->change_frame = 12;
                p->next_speed = -1.2f;
                p->next_accel = 0.0f;
                p->fade_frame = p->duration - 10;
                p->textures = rag_prim_new_textures(p);
                p->textures[0] = texture(k < 2 ? "effect\\stone.bmp" : "effect\\ice.tga");
            }
        }
    }
}

void rag_fire_wall_rings(struct RagEffect *self, const char *tex)
{
    struct EffectPrim *p;

    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\EF_FireWall.wav");
    follow_master(self);
    p = launch_prim(self, 118, 0);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    p->scale = 13.0f;
    wall_ring(p, 0, 315, 25.0f, 0, 2.4f, 70);
    wall_ring(p, 1, 315, 22.0f, 90, 2.7f, 57);
    wall_ring(p, 2, 315, 19.0f, 180, 3.0f, 45);
    wall_ring(p, 3, 360, 30.0f, 0, 2.2f, 90);
}

static void pok_bits(struct EffectPrim *p, const Vec3 *at, float size_step, float size, int spread,
                     unsigned char color_style)
{
    struct PrimBillboard *bb;
    int i;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        bb = &p->records.billboards[i];
        bb->active = 1;
        bb->age = 0;
        bb->size = (float)rand_below(10) * size_step + size;
        bb->phase = 0.0f;
        bb->alpha = 0;
        bb->pos.x = (float)rand_below(spread * 2 + 1) + at->x - (float)spread;
        bb->pos.z = (float)rand_below(spread * 2 + 1) + at->z - (float)spread;
        bb->pos.y = at->y - 6.0f;
        bb->heading = 0;
        bb->speed = 0.0f;
        bb->climb = (float)rand_below(20) * 0.01f + 0.1f;
        bb->spin = 0;
        bb->texture = 0;
        bb->state = 1;
        if (color_style != 0xff)
            bb->color_style = color_style;
        bb->style = 0.0f;
    }
}

void rag_pok_bursts(struct RagEffect *self, char look)
{
    static const unsigned char styles[4] = { 11, 1, 1, 13 };
    struct EffectPrim *p;
    int f;

    follow_master(self);
    f = self->frame;
    if (f < (look == 2 ? 40 : 0) || f > (look == 2 ? 120 : 80) || f % 4 != 0)
        return;
    p = launch_prim(self, 59, 2);
    if (!p)
        return;
    p->textures[0] = texture("effect\\pok1.tga");
    p->textures[1] = texture("effect\\pok3.tga");
    p->scale = 0.0f;
    p->base.pos = self->base.pos;
    pok_bits(p, &self->base.pos, 0.1f, 2.0f, 5,
             (unsigned char)look <= 3 ? styles[(unsigned char)look] : 0xff);
}

void rag_rising_columns(struct RagEffect *self, const char *tex)
{
    struct EffectPrim *p;
    struct PrimFlatSquare *sq;
    int i;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_prim(self, 119, 1);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    p->scale = 0.0f;
    p->delta = self->offset;
    p->base.pos = self->base.pos;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        sq = &p->records.flat_squares[i];
        sq->active = 1;
        sq->frame = i - 3;
        sq->size = 20.0f;
        sq->alpha = 0;
        sq->pos.x = self->base.pos.x;
        sq->pos.z = self->base.pos.z;
        sq->pos.y = (float)rand_below(5) + (self->base.pos.y - 10.0f);
        sq->heading = 0;
        sq->yaw = (short)i;
        sq->speed = 0.8f;
        sq->style = 0;
    }
}

void rag_columns(struct RagEffect *self, char look)
{
    struct EffectPrim *p;
    struct PrimEffectSpawner *sp;
    int i;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = rag_launch_prim(self, 120, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    self->last_prim = p;
    p->duration = self->duration;
    p->motion_count = 0;
    p->scale = 0.0f;
    p->delta = self->offset;
    p->base.pos = self->base.pos;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        sp = &p->records.effect_spawners[i];
        sp->active = 1;
        sp->frame = i - 3;
        sp->at.x = self->base.pos.x;
        sp->at.z = self->base.pos.z;
        sp->at.y = (float)rand_below(5) + (self->base.pos.y - 10.0f);
        sp->heading = 0;
        sp->speed = 0.8f;
        sp->mode = (unsigned char)look;
    }
}

void rag_three_rings(struct RagEffect *self, const char *tex)
{
    struct EffectPrim *p;
    struct PrimArcWall *wall;
    int flat;

    if (self->frame != 0)
        return;
    p = launch_prim(self, 121, 0);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    p->scale = 4.0f;
    p->delta = self->offset;
    p->delta2 = self->offset2;
    flat = self->offset2.y == 0.0f;
    wall = &p->records.arc_walls[0];
    wall->active = 1;
    wall->age = 0;
    wall->arc = 315;
    wall->full_height = 13.0f;
    wall->start = 0;
    wall->alpha = 0;
    wall->radius = flat ? 3.4f : 2.4f;
    wall->lean = 55;
    wall = &p->records.arc_walls[1];
    wall->active = 1;
    wall->age = 0;
    wall->arc = 315;
    wall->full_height = 11.0f;
    wall->start = 90;
    wall->alpha = 0;
    wall->radius = flat ? 3.6f : 2.6f;
    wall->lean = 50;
    wall = &p->records.arc_walls[2];
    wall->active = 1;
    wall->age = 0;
    wall->arc = 315;
    wall->full_height = 9.0f;
    wall->start = 180;
    wall->alpha = 0;
    wall->radius = flat ? 3.8f : 2.8f;
    wall->lean = 45;
    p->records.arc_walls[3].active = 0;
}

void rag_columns_at_offset(struct RagEffect *self, const char *tex, char look)
{
    struct EffectPrim *p;
    struct PrimFlatSquare *sq;
    int i;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_prim(self, 122, 1);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    p->scale = 0.0f;
    p->delta = self->offset;
    p->base.pos = self->base.pos;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        sq = &p->records.flat_squares[i];
        sq->active = 1;
        sq->frame = 0;
        if (look == 4 || look == 5) {
            sq->size = 25.0f;
            sq->style = 3;
        } else if (look > 1) {
            sq->size = 6.0f;
            sq->style = 2;
        } else {
            sq->size = 25.0f;
            sq->style = 0;
        }
        sq->alpha = 0;
        sq->pos.x = self->base.pos.x;
        sq->pos.z = self->base.pos.z;
        sq->pos.y = self->base.pos.y - 10.0f;
        sq->heading = 0;
        sq->yaw = (short)i;
        sq->speed = 2.0f;
        sq->state = 0;
        if (look == 0 || look == 2 || look == 4)
            sq->turn_up = 0;
        else if (look == 1 || look == 3 || look == 5)
            sq->turn_up = 1;
        sq->fades_early = (look == 2 || look == 3) ? 1 : 0;
    }
}

void rag_pok_burst_at_offset(struct RagEffect *self, char look)
{
    struct EffectPrim *p;

    if (self->frame != 0)
        return;
    p = launch_prim(self, 59, 2);
    if (!p)
        return;
    if (look == 1) {
        p->textures[0] = texture("effect\\thunder_ball_0002.bmp");
        p->textures[1] = texture("effect\\thunder_ball_0003.bmp");
    } else if (look == 2) {
        p->textures[0] = texture("effect\\meteo_comet_b.bmp");
        p->textures[1] = texture("effect\\meteo_comet_c.bmp");
    } else {
        p->textures[0] = texture("effect\\pok1.tga");
        p->textures[1] = texture("effect\\pok3.tga");
    }
    p->scale = 0.0f;
    p->base.pos = self->base.pos;
    pok_bits(p, &self->offset, 0.08f, 1.2f, 4, look == 0 ? 11 : look == 1 ? 1 : look == 2 ? 7 : 0xff);
}

void rag_water_pillar(struct RagEffect *self, char lengthwise, char look, char textures)
{
    struct EffectPrim *p;
    struct PrimScrollColumn *col;
    int i, k;

    if (self->frame != 0)
        return;
    follow_master(self);
    p = launch_prim(self, 79, 3);
    if (!p)
        return;
    if (textures == 0) {
        p->textures[0] = texture("effect\\waterfall11.tga");
        p->textures[1] = texture("effect\\waterfall12.tga");
        p->textures[2] = texture("effect\\waterfall13.tga");
    } else if (textures == 1) {
        p->textures[0] = texture("effect\\waterfall31.tga");
        p->textures[1] = texture("effect\\waterfall32.tga");
        p->textures[2] = texture("effect\\waterfall33.tga");
    } else if (textures == 10) {
        p->textures[0] = texture("effect\\alpha_down.tga");
        p->textures[1] = texture("effect\\alpha_down.tga");
        p->textures[2] = texture("effect\\alpha_down.tga");
    }
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        col = &p->records.scroll_columns[i];
        col->active = 1;
        col->age = 0;
        col->alpha = 120;
        col->lengthwise = (unsigned char)lengthwise;
        col->width = (float)(i + 36);
        col->squash = 10;
        for (k = 0; k < 6; k++)
            col->edge[k] = (float)(k * 40);
        col->depth = (float)i * 0.25f - 1.0f;
        if (look == 0) {
            col->steps = (float)(80 - 13 * i);
        } else if (look == 1) {
            col->steps = (float)(30 - 6 * i);
        } else if (look == 10) {
            col->steps = 0.0f;
            col->alpha = 70;
        }
        col->style = look == 10 ? 10 : 1;
    }
}

void rag_line_burst_quake(struct RagEffect *self, unsigned char with_wave)
{
    int i;

    for (i = 0; i < 4; i++) {
        rag_line_bursts(self, 10);
        rag_line_bursts(self, 11);
    }
    if (self->frame == 44) {
        game_send_message(GAME_MSG_QUAKE, 0, 0, 0, 0);
        if (with_wave) {
            rag_follow_master(self);
            rag_play_wave_here(self, "effect\\연주붕격.wav");
        }
    }
    if ((unsigned)(self->frame - 20) <= 80) {
        self->master->overlay_flags |= 8;
        render_object_set_argb(self->master, -1, 255, 255, 150);
    }
}
