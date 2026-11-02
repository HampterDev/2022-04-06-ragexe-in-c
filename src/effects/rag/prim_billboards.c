#include "actor/render_ops.h"
#include <stdlib.h>
#include <string.h>
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"

#define COS_DEG(d) g_cosTable[d]
#define SIN_DEG(d) g_sinTable[d]

#define EFFECT_ID_AIRTEXTURE 0x1a0

#define GAME_MSG_QUAKE 0x7a

#define BLEND_ALPHA 1

#define BB_COLOR_WHITE_TURNED 0
#define BB_COLOR_LILAC        1
#define BB_COLOR_BLUE         2
#define BB_COLOR_WHITE_SPIN   7
#define BB_COLOR_PINK_TURNED  15
#define BB_COLOR_CYCLING      23
#define BB_SHIFT_SIDE 10.0f

static int on_ecl_field(void)
{
    return strcmp(game_current_map_name(), "ecl_fild01.rsw") == 0;
}

static void draw_square(struct EffectPrim *self, int *count, short alpha, Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3,
                        const float *view, short blend, unsigned char texture, unsigned char r, unsigned char g,
                        unsigned char b)
{
    prim_draw_quad(self, count, alpha, p0, p1, p2, p3, view, blend, (char)texture, 1, r, g, b, 0.0f, 1.0f, 0.0f,
                   1.0f, PRIM_QUAD_PROJECTED);
}

void prim_billboards_render(struct EffectPrim *self, const float *view)
{
    struct PrimBillboard *bbs = self->records.billboards;
    int count = 0;
    int i;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimBillboard *bb = &bbs[i];
        Vec3 c0, c1, c2, c3;
        float radius;
        int turn0, turn1, turn2, turn3;
        short alpha;

        if (bb->active != 1 || bb->alpha <= 0)
            continue;
        if (on_ecl_field() && bb->alpha > 100)
            bb->alpha = 100;
        if (bb->phase >= 360.0f || 0.0f > bb->phase)
            bb->phase = 0.0f;

        radius = SIN_DEG((int)bb->phase) * bb->size * 0.05f + bb->size;
        turn0 = bb->spin;
        turn1 = turn0 + (turn0 < 270 ? 90 : -270);
        turn2 = turn1 + 90;
        if (turn2 >= 360)
            turn2 = turn1 - 270;
        turn3 = turn2 + (turn2 + 90 < 360 ? 90 : -270);
        c0.x = COS_DEG(turn0) * radius;
        c0.y = 0.0f;
        c0.z = SIN_DEG(turn0) * radius;
        c1.x = COS_DEG(turn1) * radius;
        c1.y = 0.0f;
        c1.z = SIN_DEG(turn1) * radius;
        c2.x = COS_DEG(turn2) * radius;
        c2.y = 0.0f;
        c2.z = SIN_DEG(turn2) * radius;
        c3.x = COS_DEG(turn3) * radius;
        c3.y = 0.0f;
        c3.z = SIN_DEG(turn3) * radius;
        if (bb->shift == BB_SHIFT_SIDE) {
            c0.x += 4.0f;
            c1.x += 4.0f;
            c2.x += 4.0f;
            c3.x += 4.0f;
        }
        prim_quad_turn(&c0, &c1, &c2, &c3, self->camera_yaw, self->camera_pitch);
        c0.x = bb->pos.x + c0.x;
        c0.y = bb->pos.y + c0.y;
        c0.z = bb->pos.z + c0.z;
        c1.x = bb->pos.x + c1.x;
        c1.y = bb->pos.y + c1.y;
        c1.z = bb->pos.z + c1.z;
        c2.x = bb->pos.x + c2.x;
        c2.y = bb->pos.y + c2.y;
        c2.z = bb->pos.z + c2.z;
        c3.x = bb->pos.x + c3.x;
        c3.y = bb->pos.y + c3.y;
        c3.z = bb->pos.z + c3.z;

        alpha = bb->alpha;
#define SQUARE(blend, r, g, b) draw_square(self, &count, alpha, c1, c0, c3, c2, view, (blend), bb->texture, (r), (g), (b))
#define SQUARE_TURNED(blend, r, g, b) \
    draw_square(self, &count, alpha, c0, c1, c2, c3, view, (blend), bb->texture, (r), (g), (b))
#define ADD PRIM_BLEND_ADDITIVE
        switch ((signed char)bbs[0].color_style) {
        case 0:  SQUARE_TURNED(BLEND_ALPHA, 0xff, 0xff, 0xff); break;
        case 1:  SQUARE_TURNED(ADD, 0x96, 0x96, 0xfa); break;
        case 2:  SQUARE(ADD, 0x82, 0x82, 0xfa); break;
        case 3:  SQUARE(ADD, 0xc8, 0xc8, 0x64); break;
        case 4:  SQUARE(BLEND_ALPHA, 0xff, 0xff, 0xff); break;
        case 7:  SQUARE(ADD, 0xff, 0xff, 0xff); break;
        case 8:  SQUARE(BLEND_ALPHA, 0x87, 0xff, 0x87); break;
        case 9:
            switch (bbs[0].color_substyle) {
            case 1: SQUARE(BLEND_ALPHA, 0xff, 0x87, 0x87); break;
            case 2: SQUARE(BLEND_ALPHA, 0xff, 0xcd, 0x87); break;
            case 3: SQUARE(BLEND_ALPHA, 0xff, 0xff, 0x87); break;
            case 4: SQUARE(BLEND_ALPHA, 0xff, 0xff, 0xff); break;
            case 5: SQUARE(BLEND_ALPHA, 0x87, 0x87, 0xff); break;
            case 6: SQUARE(ADD, 0xff, 0xff, 0xff); break;
            }
            break;
        case 10: SQUARE(ADD, 0xfa, 0xfa, 0x96); break;
        case 11: SQUARE(ADD, 0xdc, 0xfa, 0xdc); break;
        case 12: SQUARE(BLEND_ALPHA, 0xfc, 0xab, 0x8f); break;
        case 13: SQUARE(ADD, 0xfa, 0x64, 0x64); break;
        case 14: SQUARE(BLEND_ALPHA, 0x5e, 0x00, 0x00); break;
        case 15: SQUARE_TURNED(BLEND_ALPHA, 0xff, 0xc8, 0xff); break;
        case 16: SQUARE(BLEND_ALPHA, 0x00, 0x00, 0x00); break;
        case 17: SQUARE(BLEND_ALPHA, 0xff, 0xb4, 0xb4); break;
        case 18:
            alpha = (short)(bb->alpha / 2);
            SQUARE(ADD, 0xff, 0xff, 0xff);
            break;
        case 23: SQUARE(ADD, 0xff, 0xff, 0xff); break;
        case 24: SQUARE(ADD, 0x3f, 0x48, 0xcc); break;
        case 25:
            c0.y -= 110.0f;
            c1.y -= 110.0f;
            c2.y -= 110.0f;
            c3.y -= 110.0f;
            SQUARE(ADD, 0xff, 0xff, 0xff);
            break;
        }
#undef ADD
#undef SQUARE
#undef SQUARE_TURNED
    }
}

static int rand_mod(int n)
{
    int r = rand() % n;
    return r < 0 ? 0 : r;
}

static void face_camera(struct EffectPrim *self)
{
    self->camera_yaw = (short)(360 - (int)game_camera_heading());
    self->camera_pitch = (short)(450 - (int)game_camera_pitch());
    if (self->camera_pitch >= 360)
        self->camera_pitch -= 360;
}

static void follow_master(struct EffectPrim *self)
{
    self->base.pos = self->master->base.pos;
}

static void trail_copy(struct PrimBillboard *to, const struct PrimBillboard *from, int fade)
{
    to->spin = from->spin;
    to->size = from->size;
    to->phase = from->phase;
    to->alpha = (short)(from->alpha - fade);
    if (to->alpha < 0)
        to->alpha = 0;
    to->pos = from->pos;
    to->texture = from->texture;
}

static void trail_step(struct EffectPrim *self, int fade_head, int fade)
{
    struct PrimBillboard *bbs = self->records.billboards;

    if (bbs[0].age > 3)
        trail_copy(&bbs[3], &bbs[2], fade);
    if (bbs[0].age > 2)
        trail_copy(&bbs[2], &bbs[1], fade);
    if (bbs[0].age > 1)
        trail_copy(&bbs[1], &bbs[0], fade_head);
}

static void turn_spin(short *angle, int step)
{
    *angle += step;
    if (step < 0) {
        if (*angle < 0)
            *angle += 360;
    } else if (*angle >= 360) {
        *angle -= 360;
    }
}

static void fly_on(struct PrimBillboard *bb)
{
    bb->pos.x = COS_DEG(bb->heading) * bb->speed + bb->pos.x;
    bb->pos.z = SIN_DEG(bb->heading) * bb->speed + bb->pos.z;
}

static void wander(struct PrimBillboard *bb)
{
    int k;

    for (k = 0; k < 2; k++) {
        float *angle = &bb->wander[k * 2];
        float *target = &bb->wander[k * 2 + 1];
        float now = *angle;
        float goal = *target;
        int range;

        if (goal > now) {
            now = now + 1.0f;
            if (now > 359.0f)
                now = 359.0f;
            *angle = now;
            if (now > goal) {
                range = (int)now;
                *target = (float)(range != 0 ? rand_mod(range) : 0);
            }
        } else {
            now = now - 1.0f;
            if (0.0f > now)
                now = 0.0f;
            *angle = now;
            if (goal >= now) {
                range = (int)now;
                *target = (float)(359 - (range != 0 ? rand_mod(range) : 0));
            }
        }
    }
}

static void quake_screen(void)
{
    game_send_message(GAME_MSG_QUAKE, 0, 0, 0, 0);
}

#define MOTE_GATED     0.0f
#define MOTE_NEAR      2.0f
#define MOTE_GROUND    3.0f
#define MOTE_WIND      4.0f
#define MOTE_NEAR_SLOW 5.0f
#define MOTE_FROZEN    99.0f

static void mote_respawn(struct EffectPrim *self, struct PrimBillboard *bb)
{
    if (bb->style == MOTE_GATED || bb->style == MOTE_NEAR || bb->style == MOTE_NEAR_SLOW) {
        if (bb->style == MOTE_GATED && self->base.pos.y > bb->height_gate)
            bb->age = 300;
        bb->pos.x = (float)(rand_mod(300) - 150);
        bb->pos.z = (float)(rand_mod(300) - 150);
    } else if (bb->style == MOTE_GROUND) {
        struct C3dAttr *ground_map;
        int depth;
        float ground;

        bb->pos.x = (float)(rand_mod(300) - 150);
        bb->pos.z = (float)(rand_mod(300) - 150);
        ground_map = game_current_map_attr();
        depth = rand() % 5;
        if (!ground_map) {
            bb->active = 0;
            return;
        }
        ground = c3d_attr_get_height(ground_map, self->base.pos.x + bb->pos.x,
                            self->base.pos.z + bb->pos.z);
        bb->pos.y = (float)((double)ground - 20.0 - (double)(float)(depth < 0 ? 0 : depth));
    } else {
        bb->pos.x = (float)(rand_mod(200) + 25);
        bb->pos.z = (float)(rand_mod(200) + 25);
        if (on_ecl_field()) {
            bb->pos.x = (float)(rand_mod(400) + 25);
            bb->pos.z = (float)(rand_mod(400) + 25);
        }
        if (rand_mod(2) == 0)
            bb->pos.x = bb->pos.x * -1.0f;
        if (rand_mod(2) == 0)
            bb->pos.z = bb->pos.z * -1.0f;
    }
    bb->pos.x = self->base.pos.x + bb->pos.x;
    bb->pos.z = self->base.pos.z + bb->pos.z;
}

unsigned char prim_billboards_motes_process(struct EffectPrim *self)
{
    struct PrimBillboard *bbs;
    int i;

    face_camera(self);
    follow_master(self);
    bbs = self->records.billboards;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimBillboard *bb = &bbs[i];

        if (bb->active != 1)
            continue;
        if (MOTE_FROZEN > bb->style) {
            int age = ++bb->age;

            if (bb->color_style == BB_COLOR_CYCLING && age % 14 == 0) {
                bb->texture++;
                if ((signed char)bb->texture > 5)
                    bb->texture = 0;
            }

            if (bb->style == MOTE_GATED) {
                if (age < 80 && bb->height_gate > self->base.pos.y)
                    bb->alpha += 2;
            } else if (bb->style == MOTE_NEAR) {
                if (age < 80)
                    bb->alpha += 2;
            } else if (bb->style == MOTE_GROUND) {
                if (age < 170)
                    bb->alpha++;
            } else if (bb->style == MOTE_NEAR_SLOW) {
                if (age < 80)
                    bb->alpha += 2;
            } else if (bb->style == 11.0f || bb->style == 12.0f || bb->style == 13.0f || bb->style == 14.0f) {
                if (age < 70)
                    bb->alpha++;
            } else if (age < 80) {
                bb->alpha += 3;
            }

            if (age > bb->heading) {
                bb->alpha--;
                if (bb->alpha <= 0) {
                    bb->age = 0;
                    bb->alpha = 0;
                    mote_respawn(self, bb);
                }
            }

            wander(bb);
            if (bb->style == MOTE_GROUND) {
                bb->pos.x = SIN_DEG((int)bb->wander[0]) * 0.015f + bb->pos.x;
                bb->pos.z = SIN_DEG((int)bb->wander[2]) * 0.015f + bb->pos.z;
            } else if (bb->style == MOTE_WIND) {
                float sway = SIN_DEG((int)bb->wander[0]);
                if (sway > 0.0f)
                    bb->pos.x = sway * 0.2f + bb->pos.x;
                else
                    bb->pos.x = bb->pos.x - sway * 0.2f;
                bb->pos.z = SIN_DEG((int)bb->wander[2]) * 0.05f + bb->pos.z;
            } else if (bb->style == MOTE_NEAR_SLOW) {
                bb->pos.x = SIN_DEG((int)bb->wander[0]) * 0.035f + bb->pos.x;
                bb->pos.z = SIN_DEG((int)bb->wander[2]) * 0.035f + bb->pos.z;
            } else {
                bb->pos.x = SIN_DEG((int)bb->wander[0]) * 0.05f + bb->pos.x;
                bb->pos.z = SIN_DEG((int)bb->wander[2]) * 0.05f + bb->pos.z;
            }
        }
        bb->phase = bb->phase + 1.0f;
        if (bb->phase >= 360.0f)
            bb->phase = bb->phase - 360.0f;
    }
    return 1;
}

unsigned char prim_billboards_sway_process(struct EffectPrim *self)
{
    struct PrimBillboard *bbs;
    int i;

    face_camera(self);
    follow_master(self);
    bbs = self->records.billboards;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimBillboard *bb = &bbs[i];
        int age;

        if (bb->active != 1)
            continue;
        age = ++bb->age;
        if (age < 50)
            bb->alpha += 5;
        if (age > bb->heading)
            bb->alpha -= 5;
        wander(bb);
        bb->pos.x = SIN_DEG((int)bb->wander[0]) * 0.1f + bb->pos.x;
        bb->pos.z = SIN_DEG((int)bb->wander[2]) * 0.1f + bb->pos.z;
        bb->phase = bb->phase + 1.0f;
        if (bb->phase >= 360.0f)
            bb->phase = bb->phase - 360.0f;
        turn_spin(&bb->spin, 1);
    }
    return 1;
}

#define BOOMERANG_OUT     0
#define BOOMERANG_TURNING 1
#define BOOMERANG_BACK    2

void prim_billboards_boomerang_process(struct EffectPrim *self)
{
    struct PrimBillboard *bb;
    unsigned char style;
    int age;

    face_camera(self);
    follow_master(self);
    bb = &self->records.billboards[0];
    if (bb->active != 1)
        return;
    age = ++bb->age;
    if (age <= 0)
        return;

    style = bb->color_style;
    if (style == BB_COLOR_LILAC) {
        if (age <= 10)
            bb->alpha += 25;
        if (bb->alpha >= 255)
            bb->alpha = 255;
    }
    if (age % (style != BB_COLOR_LILAC ? 3 : 1) == 0)
        trail_step(self, 150, 25);
    turn_spin(&bb->spin, -15);
    if (bb->age == 1)
        prim_heading_degrees(self->delta.x - bb->pos.x, self->delta.z - bb->pos.z, &bb->heading);

    if (bb->state == BOOMERANG_OUT) {
        if ((float)bb->age * bb->speed > bb->range) {
            if (bb->color_style == BB_COLOR_LILAC) {
                bb->speed = bb->speed * 3.0f;
                bb->state = BOOMERANG_BACK;
                if (rand_mod(2) == 0) {
                    bb->heading = (short)(rand_mod(45) + bb->heading);
                    if (bb->heading >= 360)
                        bb->heading -= 360;
                } else {
                    bb->heading = (short)(bb->heading - rand_mod(45));
                    if (bb->heading < 0)
                        bb->heading += 360;
                }
            } else {
                bb->state = BOOMERANG_TURNING;
            }
        }
        bb->pos.y = bb->climb + bb->pos.y;
    } else if (bb->state == BOOMERANG_TURNING) {
        short home;
        prim_heading_degrees(self->base.pos.x - bb->pos.x, self->base.pos.z - bb->pos.z, &home);
        if (bb->heading >= home - 4 && bb->heading <= home + 4) {
            bb->state = BOOMERANG_BACK;
        } else {
            bb->heading += 5;
            if (bb->heading >= 360)
                bb->heading -= 360;
        }
    } else {
        bb->alpha -= 12;
        if (bb->alpha < 0)
            bb->alpha = 0;
        bb->pos.y = bb->pos.y - bb->climb;
    }
    fly_on(bb);
}

void prim_billboards_fly_process(struct EffectPrim *self)
{
    struct PrimBillboard *bb;
    unsigned char style;
    int age;

    face_camera(self);
    follow_master(self);
    bb = &self->records.billboards[0];
    if (bb->active != 1)
        return;
    age = ++bb->age;
    if (age <= 0)
        return;

    style = bb->color_style;
    if (style == BB_COLOR_LILAC) {
        if (age <= 10)
            bb->alpha += 25;
        if (bb->alpha >= 255)
            bb->alpha = 255;
    }
    if (age % (style != BB_COLOR_LILAC ? 3 : 1) == 0)
        trail_step(self, 150, 25);
    turn_spin(&bb->spin, -15);
    if (bb->age == 1)
        prim_heading_degrees(self->delta.x - bb->pos.x, self->delta.z - bb->pos.z, &bb->heading);
    fly_on(bb);

    if (bb->age % 4 == 0) {
        Vec3 none = {0.0f, 0.0f, 0.0f};
        struct RenderObject *player = (struct RenderObject *)game_current_player();
        struct RagEffect *e = player ? game_actor_launch_effect((struct GameActor *)player, EFFECT_ID_AIRTEXTURE, none, 0) : NULL;
        if (e) {
            Vec3 at = bb->pos;
            rag_effect_send_msg(e, NULL, RAG_MSG_SET_OFFSETS, (intptr_t)&at, 0, 0);
        }
    }
}

void prim_billboards_fly_fade_process(struct EffectPrim *self)
{
    struct PrimBillboard *bb;
    int age;

    face_camera(self);
    follow_master(self);
    bb = &self->records.billboards[0];
    if (bb->active != 1)
        return;
    age = ++bb->age;
    if (age <= 0)
        return;
    if (age % 3 == 0)
        trail_step(self, 150, 25);
    turn_spin(&bb->spin, -15);
    if (bb->age <= 5) {
        bb->alpha += 45;
        if (bb->alpha > 255)
            bb->alpha = 255;
    }
    if (bb->age >= 15) {
        bb->alpha -= 10;
        if (bb->alpha < 0)
            bb->alpha = 0;
    }
    fly_on(bb);
}

void prim_billboards_throw_process(struct EffectPrim *self)
{
    struct PrimBillboard *bb;
    float progress, lift;
    int age;

    face_camera(self);
    follow_master(self);
    bb = &self->records.billboards[0];
    if (bb->active != 1)
        return;
    age = ++bb->age;
    if (age % 2 == 1)
        trail_step(self, 150, 25);
    turn_spin(&bb->spin, bb->color_style != BB_COLOR_PINK_TURNED ? 10 : 45);
    if (bb->age == 1) {
        prim_heading_degrees(self->delta.x - bb->pos.x, self->delta.z - bb->pos.z, &bb->heading);
    }

    progress = (float)bb->age / (float)(signed char)bb->state;
    if (progress > 1.05f) {
        bb->alpha -= 20;
        if (bb->alpha < 0)
            bb->alpha = 0;
        return;
    }
    bb->local.y = bb->climb + bb->local.y;
    lift = SIN_DEG(-(int)((1.0f < progress ? 1.0f : progress) * -180.0f));
    if (bb->color_style == BB_COLOR_WHITE_TURNED)
        lift = lift * 10.0f;
    else if (bb->color_style != BB_COLOR_PINK_TURNED)
        lift = lift * 30.0f;
    bb->pos.y = bb->local.y - lift;
    fly_on(bb);
}

void prim_billboards_bob_process(struct EffectPrim *self)
{
    struct PrimBillboard *bbs;
    struct PrimBillboard *bb;
    int k;

    face_camera(self);
    follow_master(self);
    bbs = self->records.billboards;
    bb = &bbs[0];
    if (bb->active != 1)
        return;
    bb->age++;
    if (bb->age % 6 == 0) {
        for (k = 3; k >= 1; k--) {
            struct PrimBillboard *to = &bbs[k];
            const struct PrimBillboard *from = &bbs[k - 1];
            if ((signed char)to->active <= 0)
                continue;
            to->spin = from->spin;
            to->size = from->size + 0.5f;
            to->phase = from->phase;
            to->alpha = (short)(from->alpha - 50);
            if (to->alpha < 0)
                to->alpha = 0;
            to->pos = from->pos;
            to->texture = from->texture;
            to->color_style = BB_COLOR_BLUE;
            to->style = MOTE_GATED;
        }
    }
    bb->phase = bb->phase + 5.0f;
    if (bb->phase >= 360.0f)
        bb->phase = bb->phase - 360.0f;
    turn_spin(&bb->heading, 1);
    bb->pos.x = self->base.pos.x;
    bb->pos.z = self->base.pos.z;
    bb->local.y = self->base.pos.y - 6.0f;
    bb->pos.y = SIN_DEG(bb->heading) * 3.0f + bb->local.y;
    if (bb->color_style == BB_COLOR_WHITE_SPIN)
        turn_spin(&bb->spin, 10);
}

#define CIRCLE_CLIMBING 4
#define CIRCLE_QUAKED   0

void prim_billboards_circle_up_process(struct EffectPrim *self)
{
    struct PrimBillboard *bb;
    int age;
    unsigned char st;

    face_camera(self);
    follow_master(self);
    bb = &self->records.billboards[0];
    if (bb->active != 1)
        return;
    age = ++bb->age;
    if (age <= 0)
        return;
    trail_step(self, 100, 25);
    age = bb->age;
    if (age <= 10)
        bb->alpha += 20;

    st = bb->circle_state;
    if (st == CIRCLE_CLIMBING) {
        turn_spin(&bb->spin, -15);
        bb->local.y = bb->speed + bb->local.y;
        bb->pos.z = bb->local.z + self->base.pos.z;
        bb->pos.x = bb->local.x + self->base.pos.x;
        bb->pos.y = bb->local.y + self->base.pos.y;
        if (bb->local.y >= -60.0f && (unsigned short)(bb->spin - 250) <= 30) {
            bb->circle_state = CIRCLE_QUAKED;
            quake_screen();
        }
        return;
    }
    if (age <= 32) {
        if (st == CIRCLE_QUAKED || age <= 24) {
            turn_spin(&bb->spin, -15);
            bb->local.y = bb->speed + bb->local.y;
        }
        bb->pos.x = bb->local.x + self->base.pos.x;
        bb->pos.y = bb->local.y + self->base.pos.y;
        bb->pos.z = bb->local.z + self->base.pos.z;
        return;
    }
    if (st == CIRCLE_QUAKED && age == 33)
        quake_screen();
    bb->alpha -= 5;
    if (bb->alpha < 0)
        bb->alpha = 0;
}

void prim_billboards_rise_process(struct EffectPrim *self)
{
    struct PrimBillboard *bb;
    int age;

    face_camera(self);
    follow_master(self);
    bb = &self->records.billboards[0];
    if (bb->active != 1)
        return;
    age = ++bb->age;
    if (age <= 0)
        return;
    trail_step(self, 100, 25);
    if (bb->age <= 10)
        bb->alpha += 20;
    if (bb->age < (signed char)bb->rise_frames) {
        turn_spin(&bb->spin, -15);
        bb->local.y = bb->speed + bb->local.y;
        bb->pos.x = self->base.pos.x + bb->local.x;
        bb->pos.y = bb->local.y + self->base.pos.y;
        bb->pos.z = bb->local.z + self->base.pos.z;
        return;
    }
    bb->alpha -= 10;
    if (bb->alpha < 0)
        bb->alpha = 0;
}

void prim_billboards_scatter_process(struct EffectPrim *self)
{
    struct PrimBillboard *bbs;
    int i;

    face_camera(self);
    follow_master(self);
    bbs = self->records.billboards;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimBillboard *bb = &bbs[i];
        int prev;

        if (bb->active != 1)
            continue;
        prev = bb->age;
        bb->age = prev + 1;
        if (bb->age <= 0)
            continue;
        turn_spin(&bb->spin, -5);

        if (prev == 0) {
            prim_heading_degrees(self->delta.x - self->base.pos.x, self->delta.z - self->base.pos.z, &bb->heading);
            bb->heading = (short)(rand_mod(60) - 30 + bb->heading);
            if (bb->heading >= 360)
                bb->heading -= 360;
            if (bb->heading < 0)
                bb->heading += 360;
        } else if (prev < 10) {
            bb->alpha += 15;
        } else {
            bb->alpha -= 3;
            if (bb->alpha < 0) {
                bb->alpha = 0;
                bb->active = 0;
            }
        }
        bb->pos.y = bb->climb + bb->pos.y;
        fly_on(bb);
    }
}

void prim_billboards_sink_process(struct EffectPrim *self)
{
    struct PrimBillboard *bb = self->records.billboards;
    int i;

    face_camera(self);
    follow_master(self);
    for (i = 0; i < PRIM_RECORD_COUNT; i++, bb++) {
        if (bb->active != 1)
            continue;
        if (bb->age++ < 10) {
            bb->alpha += 25;
        } else {
            bb->alpha -= 5;
            if (bb->alpha <= 0) {
                float y;

                bb->alpha = 0;
                bb->age = 0;
                y = self->base.pos.y;
                if (bb->state != 0)
                    y = y - 5.0f;
                bb->pos.y = y;
            }
            bb->pos.y = bb->pos.y - 0.2f;
        }
        bb->phase = bb->phase + 5.0f;
        if (bb->phase >= 360.0f)
            bb->phase = bb->phase - 360.0f;
        if (bb->state == 1)
            turn_spin(&bb->spin, 5);
    }
}

void prim_billboards_glow_process(struct EffectPrim *self)
{
    struct PrimBillboard *bb = self->records.billboards;

    face_camera(self);
    follow_master(self);
    if (bb->active != 1)
        return;
    bb->age++;
    bb->phase = bb->phase + 3.0f;
    if (bb->phase >= 360.0f)
        bb->phase = bb->phase - 360.0f;
    if (bb->age <= 20)
        bb->alpha += 4;
    if (bb->age > 100) {
        bb->alpha -= 1;
        if (bb->alpha < 0)
            bb->alpha = 0;
    }
}

void prim_billboards_burst_process(struct EffectPrim *self)
{
    struct PrimBillboard *bb = self->records.billboards;
    int i;

    face_camera(self);
    follow_master(self);
    for (i = 0; i < PRIM_RECORD_COUNT; i++, bb++) {
        int flash = 36 - i * 7;

        if (bb->active != 1)
            continue;
        bb->age++;
        if (bb->age <= 0)
            continue;
        turn_spin(&bb->spin, -5);
        if (bb->age == 1) {
            bb->heading = (short)rand_mod(360);
        } else if (bb->age <= 10) {
            bb->alpha += 15;
        } else {
            if (bb->state == 1)
                bb->texture = bb->age == flash - 1 || bb->age == flash;
            bb->alpha -= 3;
            if (bb->alpha < 0) {
                bb->alpha = 0;
                bb->active = 0;
            }
        }
        bb->pos.y = bb->pos.y - bb->climb;
        fly_on(bb);
    }
}
