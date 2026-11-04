#include "game/cells.h"
#include "actor/render_ops.h"
#include "render/texture_cache.h"
#include "world/ground_api.h"
#include "session/state.h"
#include "world/actor_lookup.h"
#include <stdlib.h>
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"

#define COS(deg) g_cosTable[deg]
#define SIN(deg) g_sinTable[deg]

#define BLEND_ALPHA 1

static int rand_below(int n)
{
    int r = rand() % n;
    return r < 0 ? 0 : r;
}

static void face_camera(struct EffectPrim *self)
{
    short pitch;

    self->camera_yaw = (short)(360 - (short)(int)game_camera_heading());
    pitch = (short)(450 - (short)(int)game_camera_pitch());
    self->camera_pitch = pitch;
    if (pitch >= 360)
        self->camera_pitch = (short)(pitch - 360);
}

static void facing_square_corners(struct EffectPrim *self, float size, Vec3 c[4])
{
    int k;

    for (k = 0; k < 4; k++) {
        c[k].x = COS(45 + 90 * k) * size;
        c[k].y = 0.0f;
        c[k].z = SIN(45 + 90 * k) * size;
    }
    prim_quad_turn(&c[0], &c[1], &c[2], &c[3], self->camera_yaw, self->camera_pitch);
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

void prim_swing_tube_process(struct EffectPrim *self)
{
    int i, k, pt;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimSwingTube *tube = &self->records.swing_tubes[i];
        int frame;

        if (tube->active != 1)
            continue;
        frame = ++tube->frame;
        if (frame <= 0)
            continue;
        tube->reach = tube->reach + 0.6f;
        if (frame <= 10)
            tube->alpha = (short)(tube->alpha + (unsigned short)rand_below(6) + 15);
        if (frame > 20)
            tube->alpha = (short)(tube->alpha - 5);
        if (tube->alpha < 0)
            tube->alpha = 0;

        if (frame == 1) {
            for (k = 0; k < 21; k++) {
                float deg = tube->arc[0] + 18.0f;

                tube->arc[0] = deg;
                if (deg >= 360.0f) {
                    tube->arc[0] = deg - 360.0f;
                    continue;
                }
                for (pt = (int)tube->points; pt >= 1; pt--)
                    tube->arc[pt] = tube->arc[pt - 1];
            }
        } else {
            for (k = 0; tube->points >= (float)k; k++) {
                float deg = tube->arc[k] + 8.0f;

                tube->arc[k] = deg;
                if (deg >= 360.0f) {
                    deg = deg - 360.0f;
                    tube->arc[k] = deg;
                }
                if (0.0f > deg)
                    tube->arc[k] = deg + 360.0f;
            }
        }
    }
}

static int heading_of(float nx, float nz)
{
    unsigned short deg;

    for (deg = 0; (short)deg <= 180; deg++)
        if (nx >= COS((short)deg))
            return !(nz >= 0.0f) ? (unsigned short)(360 - deg) : deg;
    return 180;
}

static int tube_across(float dx, float dz)
{
    float len = float_sqrt(dx * dx + dz * dz);
    short deg = (short)(heading_of(dx / len, dz / len) + 90);

    return deg < 360 ? deg : (short)(deg - 360);
}

void prim_swing_tube_render(struct EffectPrim *self, const float *view)
{
    int count = 0;
    Vec3 prev0 = { 0.0f, 0.0f, 0.0f }, prev1 = { 0.0f, 0.0f, 0.0f };
    float shake_y[21], shake_x[21], shake_z[21];
    int i, seg;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        const struct PrimSwingTube *tube = &self->records.swing_tubes[i];
        int fade;

        for (seg = 0; tube->points > (float)seg; seg++) {
            shake_y[seg] = (float)rand_below(11) * 0.1f - 0.5f;
            shake_x[seg] = (float)rand_below(11) * 0.1f - 0.5f;
            shake_z[seg] = (float)rand_below(11) * 0.1f - 0.5f;
        }
        if (tube->active != 1 || tube->alpha <= 0 || tube->frame <= 0)
            continue;
        fade = 150;
        for (seg = 0; tube->points > (float)seg; seg++, fade -= 15) {
            float yaw_cos = COS(tube->yaw), yaw_sin = SIN(tube->yaw);
            float out0 = 0.0f, out1 = 0.0f;
            float x0, y0, z0, x1, y1, z1;
            int across, deg, ring;

            if (seg > 0)
                out0 = (float)seg / 5.0f * tube->reach + 0.0f;
            y0 = shake_y[seg] - 8.0f - SIN((short)(int)tube->arc[seg]) * 15.0f * tube->height;
            x0 = shake_x[seg] + yaw_cos * out0;
            z0 = shake_z[seg] + yaw_sin * out0;
            if (seg + 1 > 0)
                out1 = (float)(seg + 1) / 5.0f * tube->reach + 0.0f;
            y1 = shake_y[seg + 1] - 8.0f - SIN((short)(int)tube->arc[seg + 1]) * 15.0f * tube->height;
            x1 = yaw_cos * out1 + shake_x[seg + 1];
            z1 = yaw_sin * out1 + shake_z[seg + 1];
            across = tube_across(x0 - x1, z0 - z1);
            y0 = tube->centre.y + y0;
            x0 = tube->centre.x + x0;
            x1 = tube->centre.x + x1;
            y1 = tube->centre.y + y1;
            z0 = tube->centre.z + z0;
            z1 = tube->centre.z + z1;
            if ((unsigned)(seg - 1) > 17)
                continue;

            for (ring = 0, deg = 0; deg <= 360; deg += 120, ring++) {
                float out = COS(deg) * tube->radius;
                float up = SIN(deg) * tube->radius;
                float ox = COS(across) * out, oz = out * SIN(across);
                Vec3 q0 = vec3(x0 + ox, y0 + up, z0 + oz);
                Vec3 q1 = vec3(x1 + ox, y1 + up, z1 + oz);

                if (ring > 0) {
                    int alpha = fade > 0 ? tube->alpha + (seg * 15 - 135) : tube->alpha + fade;

                    if (alpha > 0) {
                        unsigned char g = (unsigned char)rand_below(121);
                        unsigned char r = (unsigned char)rand_below(121);

                        prim_draw_quad(self, &count, (short)alpha, q0, q1, prev1, prev0, view,
                                       PRIM_BLEND_ADDITIVE, 0, 1, r, g, 0xff, QUAD_UV);
                    }
                }
                prev0 = q0;
                prev1 = q1;
            }
        }
    }
}

static short turn_90(short deg)
{
    short next = (short)(deg + 90);

    return next < 360 ? next : (short)(next - 360);
}

void prim_square_stack_render(struct EffectPrim *self, const float *view)
{
    int count = 0;
    int i, layer;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        const struct PrimSquareStack *st = &self->records.square_stacks[i];
        float y, radius;

        if (st->active != 1 || st->alpha <= 0)
            continue;
        y = -st->top;
        radius = st->radius;
        for (layer = 0; layer < st->layers; layer++) {
            unsigned short turn;
            short deg0, deg1, deg2, deg3;
            Vec3 p0, p1, p2, p3;

            if (layer > 0) {
                y = y + st->y_step;
                radius = radius - (2.2f - (float)layer * 0.2f) * st->shrink;
            }
            turn = (unsigned short)(layer * 45 + st->turn);
            if ((short)turn >= 360)
                turn = (unsigned short)(turn % 360);
            deg0 = (short)turn;
            deg1 = turn_90(deg0);
            deg2 = turn_90(deg1);
            deg3 = turn_90(deg2);
            p0 = vec3(COS(deg0) * radius, y - 1.0f, SIN(deg0) * radius);
            p1 = vec3(COS(deg1) * radius, y - 1.0f, SIN(deg1) * radius);
            p2 = vec3(COS(deg2) * radius, y, SIN(deg2) * radius);
            p3 = vec3(COS(deg3) * radius, y, SIN(deg3) * radius);

            switch (st->style) {
            case 0:
                prim_draw_quad(self, &count, st->alpha, p0, p1, p2, p3, view, PRIM_BLEND_ADDITIVE, 0, 0, 0x32,
                               0x32, 0xff, QUAD_UV);
                break;
            case 1:
                prim_draw_quad(self, &count, st->alpha, p0, p1, p2, p3, view, BLEND_ALPHA, 0, 0, 0xa4, 0x88, 0x37,
                               QUAD_UV);
                break;
            case 2:
                prim_draw_quad(self, &count, st->alpha, p0, p1, p2, p3, view, BLEND_ALPHA, 0, 0, 0x9b, 0xff, 0x9b,
                               QUAD_UV);
                break;
            case 3:
                prim_draw_quad(self, &count, st->alpha, p0, p1, p2, p3, view, BLEND_ALPHA, 0, 0, 0x82, 0x82, 0xff,
                               QUAD_UV);
                break;
            case 4:
                prim_draw_quad(self, &count, st->alpha, p0, p1, p2, p3, view, BLEND_ALPHA, 0, 0, 0xff, 0x32, 0x32,
                               QUAD_UV);
                break;
            case 5:
                prim_draw_quad(self, &count, st->alpha, p0, p1, p2, p3, view, BLEND_ALPHA, 0, 0, 0x05, 0x05, 0x05,
                               QUAD_UV);
                break;
            case 6:
                prim_draw_quad(self, &count, st->alpha, p0, p1, p2, p3, view, BLEND_ALPHA, 0, 0, 0xff, 0xff, 0xff,
                               QUAD_UV);
                break;
            case 7:
                prim_draw_quad(self, &count, st->alpha, p0, p1, p2, p3, view, BLEND_ALPHA, 0, 0, 0xff, 0x7d, 0xff,
                               QUAD_UV);
                break;
            case 8:
                prim_draw_quad(self, &count, (short)(st->alpha / 10), p0, p1, p2, p3, view, BLEND_ALPHA, 0, 0,
                               0xe1, 0xe1, 0xff, QUAD_UV);
                break;
            case 9:
                break;
            case 10:
                prim_draw_quad(self, &count, st->alpha, p0, p1, p2, p3, view, PRIM_BLEND_ADDITIVE, 0, 0, 0x7d,
                               0x7d, 0xff, QUAD_UV);
                break;
            case 11:
                prim_draw_quad(self, &count, (short)(st->alpha / 3), p0, p1, p2, p3, view, BLEND_ALPHA, 0, 0,
                               0xff, 0xff, 0xff, QUAD_UV);
                prim_draw_quad(self, &count, st->alpha, p0, p1, p2, p3, view, PRIM_BLEND_ADDITIVE, 0, 0, 0xff,
                               0x37, 0x37, QUAD_UV);
                break;
            default:
                break;
            }
        }
    }
}

static int arc_angle(int start, int offset, short arc)
{
    int deg = start + offset;

    if (deg >= 360)
        deg -= 360;
    if ((unsigned short)arc == 360 && offset == arc)
        deg = start;
    return deg;
}

void prim_banded_sphere_render(struct EffectPrim *self, const float *view)
{
    int count = 0;
    Vec3 prev0 = { 0.0f, 0.0f, 0.0f }, prev1 = { 0.0f, 0.0f, 0.0f };
    int i, band, arc_pos;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        const struct PrimEnergyArc *sph = &self->records.energy_arcs[i];
        int step;

        if (sph->active != 1)
            continue;
        step = sph->arc / 10;
        for (band = 0; band < 180; band += 18) {
            for (arc_pos = 0; arc_pos <= sph->arc; arc_pos += step) {
                int deg = arc_angle(sph->turn, arc_pos, sph->arc);
                float ring0 = SIN(band) * sph->radius, ring1 = SIN(band + 18) * sph->radius;
                Vec3 q0 = vec3(ring0 * COS(deg) + sph->centre.x, COS(band) * sph->radius - sph->drop,
                               ring0 * SIN(deg) + sph->centre.z);
                Vec3 q1 = vec3(ring1 * COS(deg) + sph->centre.x, COS(band + 18) * sph->radius - sph->drop,
                               ring1 * SIN(deg) + sph->centre.z);

                if (arc_pos > 0)
                    prim_draw_quad(self, &count, sph->alpha, prev0, q0, q1, prev1, view, BLEND_ALPHA, 0, 0, 0xfa,
                                   0xfa, 0xff, QUAD_UV);
                prev0 = q0;
                prev1 = q1;
            }
        }
    }
}

void prim_energy_ring_render(struct EffectPrim *self, const float *view)
{
    int count = 0;
    Vec3 prev0 = { 0.0f, 0.0f, 0.0f }, prev1 = { 0.0f, 0.0f, 0.0f };
    int i, arc_pos;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        const struct PrimEnergyArc *er = &self->records.energy_arcs[i];
        int step, seg = 0;

        if (er->active != 1 || er->alpha <= 0)
            continue;
        step = er->arc / 20;
        for (arc_pos = 0; arc_pos <= er->arc; arc_pos += step) {
            int deg = arc_angle(er->start, arc_pos, er->arc);
            float cos_d = COS(deg);
            float radius = er->radius;
            float inner_out = radius * SIN(deg);
            float outer = er->width * radius * 0.1f + radius;
            float outer_out = outer * SIN(deg);
            Vec3 q0, q1;

            q0 = vec3(er->centre.x + inner_out * COS(er->turn), radius * cos_d - er->drop,
                      er->centre.z + inner_out * SIN(er->turn));
            q1 = vec3(outer_out * COS(er->turn) - er->centre.x * er->width, outer * cos_d - er->drop,
                      outer_out * SIN(er->turn) - er->centre.z * er->width);
            if (arc_pos > 0)
                prim_draw_strip(self, &count, seg - 1, er->alpha, prev0, q0, q1, prev1,
                                view, 0);
            if (++seg > 20)
                seg = 20;
            prev0 = q0;
            prev1 = q1;
        }
    }
}

void prim_tilted_ring_render(struct EffectPrim *self, const float *view)
{
    int count = 0;
    Vec3 prev0 = { 0.0f, 0.0f, 0.0f }, prev1 = { 0.0f, 0.0f, 0.0f };
    int i, arc_pos;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        const struct PrimEnergyArc *er = &self->records.energy_arcs[i];
        int step, seg = 0;

        if (er->active != 1 || er->alpha <= 0)
            continue;
        step = er->arc / 20;
        for (arc_pos = 0; arc_pos <= er->arc; arc_pos += step) {
            int deg = arc_angle(er->start, arc_pos, er->arc);
            int tilt0 = (int)er->tilt;
            int tilt1 = (int)(er->tilt + 10.0f);
            float turn_cos = COS(er->turn), turn_sin = SIN(er->turn);
            float x3 = er->centre.x * 3.0f, z3 = er->centre.z * 3.0f;
            float ring0 = SIN(tilt0) * er->radius, ring1;
            Vec3 q0, q1;

            if (tilt1 > 90)
                tilt1 = 90;
            q0.y = ring0 * COS(deg) - er->drop;
            q0.x = ring0 * SIN(deg) * turn_cos + x3 + er->centre.x * COS(tilt0) * er->drop;
            q0.z = ring0 * SIN(deg) * turn_sin + z3 + er->centre.z * COS(tilt0) * er->drop;
            ring1 = SIN(tilt1) * er->radius;
            q1.y = ring1 * COS(deg) - er->drop;
            q1.x = ring1 * SIN(deg) * turn_cos + x3 + er->centre.x * COS(tilt1) * er->drop;
            q1.z = ring1 * SIN(deg) * turn_sin + z3 + er->centre.z * COS(tilt1) * er->drop;
            if (arc_pos > 0) {
                prim_draw_strip(self, &count, seg - 1, er->alpha, prev0, q0, q1, prev1,
                                view, 0);
                prim_draw_strip(self, &count, seg - 1, er->alpha, prev0, q0, q1, prev1,
                                view, 0);
            }
            if (++seg > 20)
                seg = 20;
            prev0 = q0;
            prev1 = q1;
        }
    }
}

#define EFFECT_LAUNCHED_BY_109 0x1fa
#define RAG_MSG_OFFSET         0x0e

static void launch_109_on(struct RenderObject *actor, Vec3 centre)
{
    struct RagEffect *effect = game_actor_launch_effect((struct GameActor *)actor, EFFECT_LAUNCHED_BY_109, vec3(0.0f, 0.0f, 0.0f), 0);

    if (effect) {
        Vec3 at = centre;
        rag_effect_send_msg(effect, 0, RAG_MSG_OFFSET, (intptr_t)&at, 0, 0);
    }
}

void prim_launch_square_render(struct EffectPrim *self, const float *view)
{
    int count = 0;
    int i, k;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        const struct PrimLaunchSquare *sq = &self->records.launch_squares[i];
        Vec3 corner[4], p[4], centre;
        int alpha;

        if (sq->active != 1)
            continue;
        facing_square_corners(self, sq->size, corner);
        centre.x = COS(self->camera_yaw) * sq->dist;
        centre.y = sq->y;
        centre.z = SIN(self->camera_yaw) * sq->dist;
        for (k = 0; k < 4; k++) {
            p[k].x = centre.x + corner[k].x;
            p[k].y = centre.y + corner[k].y;
            p[k].z = centre.z + corner[k].z;
        }
        alpha = (int)sq->alpha;
        if (!(0.0f >= p[0].y))
            continue;

        if (sq->frame < 180) {
            float own = (float)(unsigned)g_session1.ownAid;
            float aid = sq->aid;
            struct RenderObject *actor = own == aid
                ? (struct RenderObject *)game_current_player()
                : (struct RenderObject *)game_find_actor((int)(unsigned)sq->aid);
            if (actor)
                launch_109_on(actor, centre);
        }
        prim_draw_quad(self, &count, (short)alpha, p[0], p[1], p[2], p[3], view, PRIM_BLEND_ADDITIVE, 0, 0, 0x7d,
                       0x7d, 0xff, QUAD_UV);
    }
}

static const unsigned char spark_spot[3] = { 2, 0, 3 };

static void spark_turn(float *angle, float *target)
{
    float cur = *angle;

    if (*target > cur) {
        cur = cur + 3.0f;
        if (cur > 359.0f)
            cur = 359.0f;
        *angle = cur;
        if (cur > *target) {
            int whole = (int)cur;
            *target = (float)(whole == 0 ? 0 : rand_below(whole));
        }
    } else {
        cur = cur - 2.0f;
        if (0.0f > cur)
            cur = 0.0f;
        *angle = cur;
        if (*target >= cur) {
            int whole = (int)cur;
            *target = (float)(359 - (whole == 0 ? 0 : rand_below(whole)));
        }
    }
}

unsigned char prim_spark_trio_process(struct EffectPrim *self)
{
    int i, k;

    face_camera(self);
    self->base.pos = self->master->base.pos;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimSparkTrio *sp = &self->records.spark_trios[i];
        Vec3 *p0 = &sp->spot[2], *p1 = &sp->spot[0], *p2 = &sp->spot[3];
        float speed, y, y1, fast;
        int frame;

        if (sp->active != 1)
            continue;
        sp->frame++;
        for (k = 0; k < 6; k++)
            spark_turn(&sp->turn[k].angle, &sp->turn[k].target);
        frame = sp->frame;
        for (k = 0; k < 3; k++) {
            if (frame < 10) {
                sp->alpha[k] = sp->alpha[k] + 20.0f;
            } else if (frame > 30) {
                float faded = sp->alpha[k] - 3.0f;

                sp->alpha[k] = faded;
                if (0.0f > faded)
                    sp->alpha[k] = 0.0f;
            }
        }

        speed = sp->speed;
        y = p0->y;
        if (0.0f > y) {
            p0->x = SIN((int)sp->turn[0].angle) * speed + p0->x;
            p0->z = SIN((int)sp->turn[1].angle) * speed + p0->z;
        }
        p0->y = y - speed;
        y1 = p1->y;
        if (0.0f > y1) {
            p1->x = p1->x - SIN((int)sp->turn[2].angle) * speed;
            p1->z = p1->z - SIN((int)sp->turn[3].angle) * speed;
        }
        p1->y = speed + y1;
        if (0.0f > p2->y) {
            fast = speed * 1.2f;
            p2->x = SIN((int)sp->turn[4].angle) * fast + p2->x;
            p2->z = p2->z - SIN((int)sp->turn[5].angle) * fast;
        }
    }
    return 1;
}

void prim_spark_burst_process(struct EffectPrim *self)
{
    int i, k;

    face_camera(self);
    self->base.pos = self->master->base.pos;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimSparkTrio *sp = &self->records.spark_trios[i];
        float speed;
        int frame;

        if (sp->active != 1)
            continue;
        frame = ++sp->frame;
        speed = sp->speed;
        for (k = 0; k < 3; k++) {
            Vec3 *p = &sp->spot[spark_spot[k]];
            float heading = sp->turn[2 * k].angle, pitch = sp->turn[2 * k + 1].angle;
            float flat;

            p->y = p->y - SIN((int)pitch) * speed;
            flat = COS((int)pitch) * speed;
            p->x = SIN((int)heading) * flat + p->x;
            p->z = COS((int)heading) * flat + p->z;
        }
        if (frame <= 25)
            continue;
        sp->speed = speed * 0.9f;
        for (k = 0; k < 3; k++) {
            float faded = sp->alpha[k] - 6.0f;

            sp->alpha[k] = faded;
            if (0.0f > faded)
                sp->alpha[k] = 0.0f;
        }
    }
}

static int spark_color(float tint, unsigned char deep, unsigned char rgb[3])
{
    if (tint == 0.0f) {
        rgb[0] = 0x50; rgb[1] = 0x50; rgb[2] = 0xff;
    } else if (tint == 1.0f) {
        rgb[0] = 0xff; rgb[1] = 0xc8; rgb[2] = 0xc8;
    } else if (tint == 2.0f) {
        rgb[0] = 0xff; rgb[1] = 0xa0; rgb[2] = 0xa0;
    } else if (tint == 3.0f) {
        rgb[0] = 0xff; rgb[1] = 0x78; rgb[2] = 0x78;
    } else if (tint == 4.0f) {
        rgb[0] = deep; rgb[1] = deep; rgb[2] = 0xff;
    } else {
        return 0;
    }
    return 1;
}

void prim_spark_trio_render(struct EffectPrim *self, const float *view)
{
    static const unsigned char deep[3] = { 0x28, 0x0a, 0x46 };
    int count = 0;
    int i, k, n;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        const struct PrimSparkTrio *sp = &self->records.spark_trios[i];
        Vec3 corner[4];

        if (sp->active != 1)
            continue;
        facing_square_corners(self, sp->size, corner);
        for (k = 0; k < 3; k++) {
            const Vec3 *at = &sp->spot[spark_spot[k]];
            unsigned char rgb[3];
            Vec3 p[4];
            int alpha = (int)sp->alpha[k];

            for (n = 0; n < 4; n++) {
                p[n].x = at->x + corner[n].x;
                p[n].y = at->y + corner[n].y;
                p[n].z = at->z + corner[n].z;
            }
            if (alpha < 0)
                alpha = 0;
            if (!(0.0f >= p[0].y))
                continue;
            if (!spark_color(sp->tint, deep[k], rgb))
                continue;
            prim_draw_quad(self, &count, (short)alpha, p[0], p[1], p[2], p[3], view, PRIM_BLEND_ADDITIVE, 0, 0,
                           rgb[0], rgb[1], rgb[2], QUAD_UV);
        }
    }
}

static void target_uv(const float corner[8], float scale, float uv[8])
{
    int k;

    for (k = 0; k < 8; k++)
        uv[k] = (corner[k] * scale + 3.5f) / 7.0f;
}

void prim_ground_target_render(struct EffectPrim *self, const float *view)
{
    int i, row, col;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        const struct PrimGroundTarget *gt = &self->records.ground_targets[i];
        struct C3dGround15 *ground;
        int cell_x, cell_y;
        float sin_t, cos_t;

        if (gt->active != 1)
            continue;
        ground = game_current_ground();
        if (!ground || !game_world_to_cell(self->base.pos.x, self->base.pos.z, &cell_x, &cell_y))
            return;
        cell_x -= 3;
        cell_y -= 3;
        sin_t = SIN(gt->turn);
        cos_t = COS(gt->turn);
        for (row = 0; row < 7; row++) {
            float u0 = (float)row - 3.5f, u1 = (float)(row + 1) - 3.5f;
            float u0s = u0 * sin_t, u0c = u0 * cos_t, u1s = u1 * sin_t, u1c = u1 * cos_t;

            for (col = 0; col < 7; col++) {
                float v0 = (float)col - 3.5f, v1 = (float)(col + 1) - 3.5f;
                float v0c = v0 * cos_t, v0s = v0 * sin_t, v1c = v1 * cos_t, v1s = v1 * sin_t;
                float corner[8], uv[8];

                corner[0] = u0s + v0c;
                corner[1] = u0c - v0s;
                corner[2] = v1c + u0s;
                corner[3] = u0c - v1s;
                corner[4] = u1s + v0c;
                corner[5] = u1c - v0s;
                corner[6] = u1s + v1c;
                corner[7] = u1c - v1s;
                target_uv(corner, gt->scale, uv);
                c3d_ground15_draw_attr_quad(ground, view, cell_x + col, cell_y + row, 0xaa9999ff,
                                          texture_cache_get(&g_textureCache, "effect\\magic_target.tga", 0, 0), uv);
                target_uv(corner, gt->scale + 0.1f, uv);
                c3d_ground15_draw_attr_quad(ground, view, cell_x + col, cell_y + row, 0x559999ff,
                                          texture_cache_get(&g_textureCache, "effect\\magic_target.tga", 0, 0), uv);
            }
        }
    }
}

static Vec3 spin_box_corner(int deg, double reach, float y)
{
    return vec3((float)((double)COS(deg) * reach), y, (float)((double)SIN(deg) * reach));
}

void prim_spin_box_render(struct EffectPrim *self, const float *view)
{
    int count = 0;
    int i;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimSpinBox *bx = &self->records.spin_boxes[i];
        Vec3 *c0 = &bx->corner[2], *c1 = &bx->corner[0], *c2 = &bx->corner[1], *c3 = &bx->corner[3];
        Vec3 l0, l1, l2, l3;
        float turn_sin, lift, swell;
        double reach;
        int deg0, deg1, deg2, deg3;
        unsigned char grey;

        if (bx->active != 1)
            continue;
        turn_sin = SIN(bx->turn);
        lift = turn_sin + turn_sin;
        swell = (SIN(bx->pulse) + 1.0f) * (bx->size * 0.1f);
        reach = (double)swell + (double)bx->size * 0.8;
        deg0 = bx->turn;
        deg1 = deg0 >= 270 ? deg0 - 270 : deg0 + 90;
        deg2 = deg1 + 90 >= 360 ? deg1 - 270 : deg1 + 90;
        deg3 = deg2 + (deg2 + 90 < 360 ? 90 : -270);
        *c0 = spin_box_corner(deg0, reach, bx->y + lift);
        *c1 = spin_box_corner(deg1, reach, bx->y + lift);
        *c2 = spin_box_corner(deg2, reach, bx->y + lift);
        *c3 = spin_box_corner(deg3, reach, bx->y + lift);
        l0 = vec3(c0->x, c0->y - bx->size, c0->z);
        l1 = vec3(c1->x, c1->y - bx->size, c1->z);
        l2 = vec3(c2->x, c2->y - bx->size, c2->z);
        l3 = vec3(c3->x, c3->y - bx->size, c3->z);

        grey = bx->grey;
        prim_draw_quad(self, &count, bx->alpha, *c0, *c1, *c2, *c3, view, PRIM_BLEND_ADDITIVE, 0, 0, grey, grey,
                       0xfa, QUAD_UV);
        prim_draw_quad(self, &count, bx->alpha, *c0, *c1, l1, l0, view, PRIM_BLEND_ADDITIVE, 0, 0, bx->grey,
                       bx->grey, 0xdc, QUAD_UV);
        prim_draw_quad(self, &count, bx->alpha, *c1, *c2, l2, l1, view, PRIM_BLEND_ADDITIVE, 0, 0, bx->grey,
                       bx->grey, 0xbe, QUAD_UV);
        prim_draw_quad(self, &count, bx->alpha, *c2, *c3, l3, l2, view, PRIM_BLEND_ADDITIVE, 0, 0, bx->grey,
                       bx->grey, 0x82, QUAD_UV);
        prim_draw_quad(self, &count, bx->alpha, *c3, *c0, l0, l3, view, PRIM_BLEND_ADDITIVE, 0, 0, bx->grey,
                       bx->grey, 0xa0, QUAD_UV);
        prim_draw_quad(self, &count, bx->alpha, l0, l1, l2, l3, view, PRIM_BLEND_ADDITIVE, 0, 0, bx->grey,
                       bx->grey, 0xfa, QUAD_UV);
    }
}

static void follow_master(struct EffectPrim *self)
{
    self->base.pos = self->master->base.pos;
}

static int quarter_on(int deg)
{
    return deg < 270 ? deg + 90 : deg - 270;
}

static short behind_master(const struct EffectPrim *self)
{
    short turn = (short)(int)(self->master->master->roty + 180.0f);

    if (turn >= 360)
        turn -= 360;
    return turn;
}

void prim_square_stack_process(struct EffectPrim *self)
{
    int i;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimSquareStack *st = &self->records.square_stacks[i];

        if (st->active != 1)
            continue;
        st->age++;
        if (st->age <= 0)
            continue;
        st->turn = (short)(st->turn + (short)(rand_below(3) + 3));
        if (st->turn >= 360)
            st->turn -= 360;
        if (st->fade == 1) {
            if (st->age > 120)
                st->alpha--;
        } else if (st->fade != 10) {
            if (st->age > 60)
                st->alpha--;
        }
        if (st->age <= 40)
            st->alpha++;
        if (st->alpha < 0)
            st->alpha = 0;
    }
}

void prim_banded_sphere_process(struct EffectPrim *self)
{
    int i;

    follow_master(self);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimEnergyArc *ea = &self->records.energy_arcs[i];
        int side;

        if (ea->active != 1)
            continue;
        ea->age++;
        ea->turn = behind_master(self);
        side = quarter_on(ea->turn);
        ea->centre.x = COS(side) * 5.0f;
        ea->centre.z = SIN(side) * 5.0f;
        if (ea->age <= 50 && ea->alpha < 120)
            ea->alpha += 5;
    }
}

void prim_energy_ring_process(struct EffectPrim *self)
{
    int i;

    follow_master(self);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimEnergyArc *ea = &self->records.energy_arcs[i];
        int side;

        if (ea->active != 1)
            continue;
        ea->age++;
        if (ea->age <= 0)
            continue;
        if (i < 2)
            ea->start++;
        if (ea->start >= 360)
            ea->start -= 360;
        ea->turn = behind_master(self);
        side = quarter_on(ea->turn);
        ea->centre.x = COS(side) * 5.0f;
        ea->centre.z = SIN(side) * 5.0f;
        if (ea->alpha < 40)
            ea->alpha += 5;
    }
}

void prim_tilted_ring_process(struct EffectPrim *self)
{
    int i;

    follow_master(self);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimEnergyArc *ea = &self->records.energy_arcs[i];
        int side;

        if (ea->active != 1)
            continue;
        ea->age++;
        if (ea->age <= 0)
            continue;
        if (90.0f > ea->tilt) {
            ea->tilt = ea->tilt + 1.0f;
            ea->alpha += 10;
            if (ea->alpha > 60)
                ea->alpha = 60;
        } else {
            ea->alpha = 10;
            ea->tilt = 0.0f;
        }
        if (ea->age > 30 && ea->alpha >= (45 - ea->age) * 4) {
            ea->alpha = (short)((45 - ea->age) * 4);
            if (ea->alpha < 0)
                ea->alpha = 0;
        }
        ea->start++;
        if (ea->start >= 360)
            ea->start -= 360;
        ea->turn = behind_master(self);
        side = quarter_on(ea->turn);
        ea->centre.x = COS(side);
        ea->centre.z = SIN(side);
    }
}

void prim_launch_square_process(struct EffectPrim *self)
{
    int i;

    face_camera(self);
    follow_master(self);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimLaunchSquare *sq = &self->records.launch_squares[i];
        float angle;
        int deg;

        if (sq->active != 1)
            continue;
        sq->frame++;
        if (sq->frame <= 0)
            continue;
        if (sq->frame <= 10) {
            sq->alpha = sq->alpha + 20.0f;
        } else if (sq->frame > 170) {
            sq->alpha = sq->alpha - 5.0f;
            if (0.0f > sq->alpha)
                sq->alpha = 0.0f;
        }
        angle = (float)sq->frame / 10.0f / 3.0f + sq->angle;
        sq->angle = angle;
        if (angle >= 359.0f) {
            sq->angle = 359.0f;
            angle = 359.0f;
        }
        deg = (int)angle;
        sq->dist = (float)deg / 6.0f - 30.0f;
        sq->y = -(SIN(deg) * 20.0f) - 40.0f;
    }
}

void prim_ground_target_process(struct EffectPrim *self)
{
    int i;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimGroundTarget *gt = &self->records.ground_targets[i];

        if (gt->active != 1)
            continue;
        gt->turn++;
        if (gt->turn >= 360)
            gt->turn -= 360;
        gt->scale = SIN(gt->turn) * 0.5f + 1.5f;
    }
}

void prim_spin_box_process(struct EffectPrim *self)
{
    struct PrimSpinBox *bx = &self->records.spin_boxes[0];

    follow_master(self);
    if (bx->alpha < 140)
        bx->alpha += 2;
    bx->pulse++;
    if (bx->pulse >= 360)
        bx->pulse -= 360;
    bx->turn++;
    if (bx->turn >= 360)
        bx->turn -= 360;
}
