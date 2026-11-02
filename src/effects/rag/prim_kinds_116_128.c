#include "actor/render_ops.h"
#include "session/state.h"
#include "world/actor_lookup.h"
#include <stdlib.h>
#include "effects/rag/rag_effect.h"

#define COS(deg) g_cosTable[deg]
#define SIN(deg) g_sinTable[deg]

#define BLEND_ALPHA 1

static int rand_below(int n)
{
    int r = rand() % n;
    return r < 0 ? 0 : r;
}

static Vec3 vec3(float x, float y, float z)
{
    Vec3 v;

    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

#define QUAD_UV 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_PROJECTED

static int next_corner(int deg)
{
    return deg + 90 >= 360 ? deg - 270 : deg + 90;
}

#define SQUARE_HOMING     0
#define SQUARE_FADING     10
#define SQUARE_RETURNING  20

void prim_flat_square_render(struct EffectPrim *self, const float *view)
{
    int count = 0;
    int i, k;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        const struct PrimFlatSquare *sq = &self->records.flat_squares[i];
        Vec3 p[4];
        float y;
        int deg;

        if (sq->active != 1 || sq->alpha <= 0)
            continue;
        deg = sq->yaw + (sq->yaw < 270 ? 90 : -270);
        y = sq->pos.y + 0.0f;
        for (k = 0; k < 4; k++) {
            if (k)
                deg = next_corner(deg);
            p[k] = vec3(COS(deg) * sq->size + sq->pos.x, y, SIN(deg) * sq->size + sq->pos.z);
        }
        switch (sq->style) {
        case 0:
            prim_draw_quad(self, &count, sq->alpha, p[0], p[1], p[2], p[3], view, PRIM_BLEND_ADDITIVE, 0, 1,
                           0x50, 0x50, 0xff, QUAD_UV);
            break;
        case 1:
            prim_draw_quad(self, &count, sq->alpha, p[0], p[1], p[2], p[3], view, PRIM_BLEND_ADDITIVE, 0, 1,
                           0x50, 0xff, 0x50, QUAD_UV);
            break;
        case 2:
            prim_draw_quad(self, &count, sq->alpha, p[0], p[1], p[2], p[3], view, BLEND_ALPHA, 0, 1, 0xff, 0xff,
                           0xff, QUAD_UV);
            break;
        case 3:
            prim_draw_quad(self, &count, sq->alpha, p[0], p[1], p[2], p[3], view, BLEND_ALPHA, 0, 1, 0x46, 0x46,
                           0x46, QUAD_UV);
            break;
        }
    }
}

static void square_turn(struct PrimFlatSquare *sq, int step)
{
    if (sq->turn_up == 0) {
        sq->heading = (short)(sq->heading - step);
        if (sq->heading < 0)
            sq->heading = (short)(sq->heading + 360);
    } else {
        sq->heading = (short)(sq->heading + step);
        if (sq->heading >= 360)
            sq->heading = (short)(sq->heading - 360);
    }
}

static int square_steer(struct PrimFlatSquare *sq, float dx, float dz)
{
    short to;

    prim_heading_degrees(dx, dz, &to);
    if (sq->heading >= to - 3 && sq->heading <= to + 3)
        return 1;
    square_turn(sq, 5);
    return 0;
}

static void squares_fade(struct EffectPrim *self)
{
    int i;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimFlatSquare *sq = &self->records.flat_squares[i];

        sq->alpha = (short)(sq->alpha - 5);
        if (sq->alpha < 0) {
            sq->alpha = 0;
            sq->active = 0;
        }
    }
}

static void square_fly(struct EffectPrim *self, struct PrimFlatSquare *sq)
{
    int k;

    sq->pos.x = COS(sq->heading) * sq->speed + sq->pos.x;
    sq->pos.z = SIN(sq->heading) * sq->speed + sq->pos.z;
    if (sq->frame % 3 != 0)
        return;
    for (k = 2; k >= 0; k--) {
        struct PrimFlatSquare *from = &self->records.flat_squares[k];
        struct PrimFlatSquare *to = &self->records.flat_squares[k + 1];

        to->pos = from->pos;
        to->alpha = (short)(int)((float)from->alpha * 0.8f);
        to->yaw = from->yaw;
    }
}

static struct RenderObject *square_target(struct EffectPrim *self)
{
    if (self->delta.y == (float)(unsigned)g_session1.ownAid)
        return (struct RenderObject *)game_current_player();
    return (struct RenderObject *)game_find_actor((int)(unsigned)self->delta.y);
}

void prim_homing_square_process(struct EffectPrim *self)
{
    struct PrimFlatSquare *sq = &self->records.flat_squares[0];
    struct RenderObject *actor;
    int frame;

    if (sq->active != 1)
        return;
    frame = ++sq->frame;
    if (frame <= 0)
        return;
    actor = square_target(self);
    if (!actor) {
        if (sq->state < SQUARE_FADING)
            sq->state = SQUARE_RETURNING;
        if (sq->frame == 1)
            return;
    }
    sq->yaw = (short)(sq->yaw + 30);
    if (sq->yaw >= 360)
        sq->yaw = (short)(sq->yaw - 360);

    frame = sq->frame;
    if (frame == 1) {
        prim_heading_degrees(actor->pos.x - self->base.pos.x, actor->pos.z - self->base.pos.z, &sq->heading);
        if (sq->turn_up == 0) {
            sq->heading = (short)(sq->heading + 90);
            if (sq->heading >= 360)
                sq->heading = (short)(sq->heading - 360);
        } else {
            sq->heading = (short)(sq->heading - 90);
            if (sq->heading < 0)
                sq->heading = (short)(sq->heading + 360);
        }
    } else if (frame <= 20) {
        sq->alpha = (short)(sq->alpha + 15);
        if (sq->alpha > 250)
            sq->alpha = 250;
    } else if (frame > 140 || (frame > 90 && sq->fades_early == 1)) {
        sq->state = SQUARE_FADING;
    }

    if (sq->frame > 5) {
        if (sq->state == SQUARE_HOMING) {
            square_steer(sq, actor->pos.x - sq->pos.x, actor->pos.z - sq->pos.z);
        } else if (sq->state == SQUARE_FADING) {
            squares_fade(self);
            square_turn(sq, 1);
        } else if (sq->state == SQUARE_RETURNING) {
            square_steer(sq, self->base.pos.x - sq->pos.x, self->base.pos.z - sq->pos.z);
            if (sq->frame > 50)
                sq->state = SQUARE_FADING;
        }
    }
    square_fly(self, sq);
}

void prim_veering_square_process(struct EffectPrim *self)
{
    struct PrimFlatSquare *sq = &self->records.flat_squares[0];
    struct RenderObject *actor;
    int frame;

    if (sq->active != 1)
        return;
    frame = ++sq->frame;
    if (frame <= 0)
        return;
    actor = square_target(self);
    if (!actor) {
        sq->state = SQUARE_FADING;
        if (sq->frame == 1)
            return;
    }
    sq->yaw = (short)(sq->yaw + 30);
    if (sq->yaw >= 360)
        sq->yaw = (short)(sq->yaw - 360);

    frame = sq->frame;
    if (frame == 1) {
        prim_heading_degrees(actor->pos.x - self->base.pos.x, actor->pos.z - self->base.pos.z, &sq->heading);
        if (sq->turn_up == 0) {
            sq->heading = (short)((unsigned short)rand_below(21) + 25 + sq->heading);
            if (sq->heading >= 360)
                sq->heading = (short)(sq->heading - 360);
        } else {
            int veer = rand_below(21);

            sq->heading = (short)(sq->heading - veer - 25);
            if (sq->heading < 0)
                sq->heading = (short)(sq->heading + 360);
        }
    } else if (frame <= 20) {
        sq->alpha = (short)(sq->alpha + 8);
        if (sq->alpha > 120)
            sq->alpha = 120;
    } else {
        sq->state = SQUARE_FADING;
    }

    if (sq->frame > 5) {
        if (sq->state == SQUARE_HOMING) {
            if (square_steer(sq, actor->pos.x - sq->pos.x, actor->pos.z - sq->pos.z))
                sq->state = SQUARE_FADING;
        } else if (sq->state == SQUARE_FADING) {
            squares_fade(self);
            square_turn(sq, 1);
        }
    }
    square_fly(self, sq);
}

#define SPAWN_EVERY_8        0
#define SPAWN_EVERY_FRAME    1
#define SPAWN_EVERY_8_B      99
#define EFFECT_SPAWNED_0     0x228
#define EFFECT_SPAWNED_1     0x22c
#define EFFECT_SPAWNED_99    0x4b5
#define RAG_MSG_OFFSET       0x0e

void prim_spawn_effects_render(struct EffectPrim *self)
{
    struct PrimEffectSpawner *sp = &self->records.effect_spawners[0];
    struct RagEffect *effect;
    struct RenderObject *player;
    Vec3 at, beat;
    int id;

    if (sp->active != 1)
        return;
    if (sp->mode == SPAWN_EVERY_FRAME) {
        player = (struct RenderObject *)game_current_player();
        if (!player)
            return;
        effect = game_actor_launch_effect((struct GameActor *)player, EFFECT_SPAWNED_1,
                       vec3(0.0f, 0.0f, 0.0f), 0);
        if (!effect)
            return;
        at = sp->at;
        rag_effect_send_msg(effect, 0, RAG_MSG_OFFSET, (intptr_t)&at, 0, 0);
        return;
    }
    if (sp->mode == SPAWN_EVERY_8)
        id = EFFECT_SPAWNED_0;
    else if (sp->mode == SPAWN_EVERY_8_B)
        id = EFFECT_SPAWNED_99;
    else
        return;
    if (sp->frame % 8 != 0)
        return;
    player = (struct RenderObject *)game_current_player();
    if (!player)
        return;
    effect = game_actor_launch_effect((struct GameActor *)player, id,
                                     vec3(0.0f, 0.0f, 0.0f), 0);
    if (!effect)
        return;
    at = sp->at;
    beat.x = (float)sp->frame;
    beat.y = sp->frame % 16 != 0 ? 1.0f : 0.0f;
    beat.z = 0.0f;
    rag_effect_send_msg(effect, 0, RAG_MSG_OFFSET, (intptr_t)&at, (intptr_t)&beat, 0);
}

void prim_arc_wall_render(struct EffectPrim *self, const float *view)
{
    int count = 0;
    Vec3 prev_a = vec3(0.0f, 0.0f, 0.0f), prev_b = vec3(0.0f, 0.0f, 0.0f);
    int i;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        const struct PrimArcWall *wall = &self->records.arc_walls[i];
        int step, arc_pos, seg;

        if (wall->active != 1)
            continue;
        step = wall->arc / 20;
        seg = 0;
        if (wall->arc < 0)
            continue;
        for (arc_pos = 0; arc_pos <= wall->arc; arc_pos += step) {
            int deg = (wall->start + arc_pos) % 360;
            float cos_d, ring_z, ring_x, lean_out, height;
            Vec3 pa, pb;

            if ((unsigned short)wall->arc == 360 && seg == 20)
                deg = wall->start;
            cos_d = COS(deg);
            ring_z = wall->radius * SIN(deg);
            height = wall->height[seg];
            lean_out = COS(wall->lean) * height;
            ring_x = wall->radius * cos_d;
            pa.x = ring_x + self->delta.x;
            pa.y = self->delta.y + 0.0f;
            pa.z = ring_z + self->delta.z;
            pb.x = lean_out * cos_d + ring_x + self->delta.x;
            pb.z = lean_out * SIN(deg) + ring_z + self->delta.z;
            pb.y = self->delta.y - SIN(wall->lean) * height;
            if (seg > 0) {
                float u0 = (float)(seg - 1) / 21.0f, u1 = (float)seg / 21.0f;

                if (wall->look == 99)
                    prim_draw_quad(self, &count, wall->alpha, prev_a, pa, pb, prev_b, view, PRIM_BLEND_ALPHA, 0, 1,
                                   0x46, 0x46, 0x46, 0.0f, 1.0f, u0, u1, PRIM_QUAD_PROJECTED);
                else
                    prim_draw_quad(self, &count, wall->alpha, prev_a, pa, pb, prev_b, view, PRIM_BLEND_ADDITIVE, 0,
                                   1, 0x5a, 0x5a, 0xff, 0.0f, 1.0f, u0, u1, PRIM_QUAD_PROJECTED);
            }
            prev_a = pa;
            prev_b = pb;
            seg++;
            if (seg >= 21)
                seg = 20;
        }
    }
}

void prim_leaning_square_render(struct EffectPrim *self, const float *view)
{
    int count = 0;
    int i, k;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        const struct PrimFlatSquare *sq = &self->records.flat_squares[i];
        float tilt_cos, tilt_sin;
        Vec3 p[4];
        int deg;

        if (sq->active != 1 || sq->alpha <= 0)
            continue;
        tilt_cos = COS(sq->heading);
        tilt_sin = SIN(sq->heading);
        deg = sq->yaw + (sq->yaw < 270 ? 90 : -270);
        for (k = 0; k < 4; k++) {
            float out;

            if (k)
                deg = next_corner(deg);
            out = COS(deg) * sq->size;
            p[k] = vec3(out * tilt_cos + sq->pos.x, SIN(deg) * sq->size + sq->pos.y,
                        out * tilt_sin + sq->pos.z);
        }
        prim_draw_quad(self, &count, sq->alpha, p[0], p[1], p[2], p[3], view, PRIM_BLEND_ADDITIVE, 0, 1, 0x50,
                       0xff, 0x50, QUAD_UV);
    }
}

static void flat_square_step(struct EffectPrim *self, int gain, int cap, int fade)
{
    struct PrimFlatSquare *sq = &self->records.flat_squares[0];

    if (sq->active != 1)
        return;
    sq->frame++;
    if (sq->frame <= 0)
        return;
    sq->yaw = (short)(sq->yaw + 30);
    if (sq->yaw >= 360)
        sq->yaw = (short)(sq->yaw - 360);
    if (sq->frame == 1) {
        prim_heading_degrees(self->delta.x - self->base.pos.x, self->delta.z - self->base.pos.z, &sq->heading);
    } else if (sq->frame <= 10) {
        sq->alpha = (short)(sq->alpha + gain);
        if (sq->alpha > cap)
            sq->alpha = (short)cap;
        sq->speed = sq->speed * 1.02f;
    } else {
        sq->alpha = (short)(sq->alpha - fade);
        sq->speed = sq->speed * 1.04f;
        if (sq->alpha < 0) {
            sq->alpha = 0;
            sq->active = 0;
        }
    }
    square_fly(self, sq);
}

void prim_flat_square_process(struct EffectPrim *self)
{
    flat_square_step(self, 25, 250, 5);
}

void prim_leaning_square_process(struct EffectPrim *self)
{
    flat_square_step(self, 6, 60, 3);
}

void prim_spawn_effects_process(struct EffectPrim *self)
{
    int i;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimEffectSpawner *sp = &self->records.effect_spawners[i];

        if (sp->active != 1)
            continue;
        sp->frame++;
        if (sp->frame <= 0)
            continue;
        if (sp->frame == 1)
            prim_heading_degrees(self->delta.x - self->base.pos.x, self->delta.z - self->base.pos.z, &sp->heading);
        sp->at.x = COS(sp->heading) * sp->speed + sp->at.x;
        sp->at.z = SIN(sp->heading) * sp->speed + sp->at.z;
    }
}

void prim_arc_wall_process(struct EffectPrim *self)
{
    int i, seg;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimArcWall *wall = &self->records.arc_walls[i];

        if (wall->active != 1)
            continue;
        wall->age++;
        wall->start = (short)(wall->start + (short)(i + 3));
        if (wall->start >= 360)
            wall->start -= 360;
        if (wall->age >= self->duration - 40) {
            wall->alpha -= 5;
            if (wall->alpha < 0)
                wall->alpha = 0;
        } else if (wall->age < 40) {
            wall->alpha += 5;
            if (wall->alpha > 180)
                wall->alpha = 180;
        }
        if (self->delta2.x > 50.0f) {
            float cap = 200.0f - (self->delta2.x - 50.0f) * 5.0f;

            if ((float)wall->alpha > cap)
                wall->alpha = (short)(int)cap;
            if (wall->alpha < 0)
                wall->alpha = 0;
        }
        for (seg = 0; seg < 21; seg++) {
            float full;

            if (wall->risen[seg])
                continue;
            if (wall->age <= 90)
                wall->height[seg] = SIN(seg * 9) * wall->full_height * SIN(wall->age);
            full = SIN(seg * 9) * wall->full_height;
            if (wall->height[seg] > full) {
                wall->height[seg] = full;
                wall->risen[seg] = 1;
            }
            if (0.0f > wall->height[seg])
                wall->height[seg] = 0.0f;
        }
    }
}
