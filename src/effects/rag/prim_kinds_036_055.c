#include "session/state.h"
#include "world/actor_lookup.h"
#include <stdlib.h>
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"

#define COS_DEG(d) g_cosTable[d]
#define SIN_DEG(d) g_sinTable[d]

#define RECORD_COUNT 4

#define BLEND_ALPHA 1

static void draw_quad(struct EffectPrim *self, int *count, short alpha, Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3,
                      const float *view, short blend, char texture, unsigned char in_world, unsigned char r,
                      unsigned char g, unsigned char b, unsigned char mode)
{
    prim_draw_quad(self, count, alpha, p0, p1, p2, p3, view, blend, texture, in_world, r, g, b, 0.0f, 1.0f, 0.0f,
                   1.0f, mode);
}

static Vec3 box_corner(float x, float z, float c, float s)
{
    Vec3 v;
    v.x = x * c - z * s;
    v.y = -1.0f;
    v.z = x * s + z * c;
    return v;
}

static Vec3 lifted(Vec3 v, float y)
{
    v.y = y;
    return v;
}

void prim_box_render(struct EffectPrim *self, const float *view)
{
    struct PrimBox *box = self->records.boxes;
    int count = 0;
    int i;

    for (i = 0; i < RECORD_COUNT; i++, box++) {
        float c, s, top;
        Vec3 *q = box->corner;
        Vec3 t0, t1, t2, t3;
        short half;

        if (box->active != 1 || box->alpha <= 0)
            continue;
        c = COS_DEG(box->angle);
        s = SIN_DEG(box->angle);
        q[2] = box_corner(box->half_x, box->half_z, c, s);
        q[0] = box_corner(box->half_x, -box->half_z, c, s);
        q[1] = box_corner(-box->half_x, -box->half_z, c, s);
        q[3] = box_corner(-box->half_x, box->half_z, c, s);
        top = -1.0f - box->height;
        t0 = lifted(q[0], top);
        t1 = lifted(q[1], top);
        t2 = lifted(q[2], top);
        t3 = lifted(q[3], top);

        draw_quad(self, &count, box->alpha, q[2], q[0], t0, t2, view, BLEND_ALPHA, 0, 0, 255, 255, 255,
                  PRIM_QUAD_PROJECTED);
        draw_quad(self, &count, box->alpha, q[0], q[1], t1, t0, view, BLEND_ALPHA, 0, 0, 255, 255, 255,
                  PRIM_QUAD_PROJECTED);
        draw_quad(self, &count, box->alpha, q[1], q[3], t3, t1, view, BLEND_ALPHA, 0, 0, 255, 255, 255,
                  PRIM_QUAD_PROJECTED);
        draw_quad(self, &count, box->alpha, q[3], q[2], t2, t3, view, BLEND_ALPHA, 0, 0, 255, 255, 255,
                  PRIM_QUAD_PROJECTED);
        half = (short)(box->alpha / 2);
        draw_quad(self, &count, half, q[2], q[0], q[1], q[3], view, PRIM_BLEND_ADDITIVE, 0, 0, 255, 255, 255,
                  PRIM_QUAD_PROJECTED);
        draw_quad(self, &count, half, q[3], q[1], q[0], q[2], view, PRIM_BLEND_ADDITIVE, 0, 0, 255, 255, 255,
                  PRIM_QUAD_PROJECTED);
    }
}

#define TUBE_STYLE_TEAL   2.0f
#define TUBE_STYLE_PINK   3.0f
#define TUBE_STYLE_SHADOW 4.0f

void prim_tube_render(struct EffectPrim *self, const float *view)
{
    struct PrimTube *tube = &self->records.tube;
    int count = 0;
    short heading;
    int side, pass, step;
    float radius = 0.0f;
    Vec3 end, a, b;
    Vec3 prev_a = {0.0f, 0.0f, 0.0f}, prev_b = {0.0f, 0.0f, 0.0f};

    if (tube->active != 1 || tube->alpha <= 0)
        return;
    prim_heading_degrees(tube->start.x - tube->end.x, tube->start.z - tube->end.z, &heading);
    side = (short)(heading + 90);
    if (side >= 360)
        side = (short)(side - 360);
    if (tube->age < 15) {
        float t = SIN_DEG(tube->age * 6);
        end.x = (tube->end.x - tube->start.x) * t + tube->start.x;
        end.y = (tube->end.y - tube->start.y) * t + tube->start.y;
        end.z = (tube->end.z - tube->start.z) * t + tube->start.z;
    } else {
        end = tube->end;
    }

    for (pass = 0; pass < 2; pass++) {
        for (step = 0; step <= 10; step++) {
            float ring_y, ring_xz, dx, dz;
            short alpha;

            if (pass == 0)
                radius = tube->radius;
            else if (pass == 1)
                radius = tube->radius + tube->radius;
            ring_y = SIN_DEG(step * 36) * radius;
            ring_xz = COS_DEG(step * 36) * radius;
            dx = COS_DEG(side) * ring_xz;
            dz = ring_xz * SIN_DEG(side);
            a.x = tube->start.x + dx;
            a.y = tube->start.y + ring_y;
            a.z = tube->start.z + dz;
            b.x = end.x + dx;
            b.y = end.y + ring_y;
            b.z = end.z + dz;

            if (step > 0) {
                if (pass == 0) {
                    alpha = tube->alpha;
                    if (tube->style == TUBE_STYLE_TEAL)
                        draw_quad(self, &count, alpha, prev_a, a, b, prev_b, view, PRIM_BLEND_ADDITIVE, 0, 1, 0, 100,
                                  150, PRIM_QUAD_PROJECTED);
                    else if (tube->style == TUBE_STYLE_PINK)
                        draw_quad(self, &count, alpha, prev_a, a, b, prev_b, view, PRIM_BLEND_ADDITIVE, 0, 1, 255,
                                  89, 182, PRIM_QUAD_PROJECTED);
                    else if (tube->style == TUBE_STYLE_SHADOW)
                        draw_quad(self, &count, (short)(alpha / 2), prev_a, a, b, prev_b, view, BLEND_ALPHA, 0, 1,
                                  50, 50, 50, PRIM_QUAD_PROJECTED);
                    else
                        draw_quad(self, &count, alpha, prev_a, a, b, prev_b, view, PRIM_BLEND_ADDITIVE, 0, 1, 255,
                                  255, 0, PRIM_QUAD_PROJECTED);
                } else if (pass == 1) {
                    alpha = (short)(tube->alpha * 2);
                    if (tube->style == TUBE_STYLE_TEAL)
                        draw_quad(self, &count, alpha, prev_a, a, b, prev_b, view, PRIM_BLEND_ADDITIVE, 0, 1, 255,
                                  50, 0, PRIM_QUAD_PROJECTED);
                    else if (tube->style == TUBE_STYLE_PINK)
                        draw_quad(self, &count, alpha, prev_a, a, b, prev_b, view, PRIM_BLEND_ADDITIVE, 0, 1, 255,
                                  89, 182, PRIM_QUAD_PROJECTED);
                    else if (tube->style == TUBE_STYLE_SHADOW)
                        draw_quad(self, &count, (short)(tube->alpha / 2), prev_a, a, b, prev_b, view, BLEND_ALPHA,
                                  0, 1, 50, 50, 50, PRIM_QUAD_PROJECTED);
                    else
                        draw_quad(self, &count, alpha, prev_a, a, b, prev_b, view, PRIM_BLEND_ADDITIVE, 0, 1, 0, 0,
                                  255, PRIM_QUAD_PROJECTED);
                }
            }
            prev_a = a;
            prev_b = b;
        }
    }
}

#define ORBIT_COLOR_FACING 2

static int quarter_on(int angle)
{
    return angle + 90 < 360 ? angle + 90 : angle - 270;
}

void prim_orbit_square_render(struct EffectPrim *self, const float *view)
{
    struct PrimOrbitSquare *sq = self->records.orbit_squares;
    int count = 0;
    int i;

    for (i = 0; i < RECORD_COUNT; i++, sq++) {
        short alpha;
        int spin, spin1, spin2, spin3, tilt;
        float cos_o, sin_o, reach, centre_y, centre_x, centre_z, cos_h, sin_h, cos_t, sin_t, size, arm;
        float sin_t0, cos_t0;
        Vec3 c0, c1, c2, c3, p0, p1, p2, p3;
        unsigned char r, g, b, mode;

        if (sq->active != 1 || sq->alpha <= 0)
            continue;
        alpha = sq->alpha;

        cos_o = COS_DEG(sq->orbit_tilt);
        sin_o = SIN_DEG(sq->orbit_tilt);
        spin = sq->spin;
        centre_y = sin_o * sq->radius + cos_o * 0.0f;
        reach = cos_o * sq->radius - sin_o * 0.0f;
        cos_h = COS_DEG(sq->heading);
        sin_h = SIN_DEG(sq->heading);
        centre_x = reach * cos_h - sin_h * 0.0f;
        centre_z = reach * sin_h + cos_h * 0.0f;

        tilt = (int)sq->tilt;
        sin_t = SIN_DEG(tilt);
        cos_t = COS_DEG(tilt);
        size = sq->size;
        sin_t0 = sin_t * 0.0f;
        cos_t0 = cos_t * 0.0f;
        arm = COS_DEG(spin) * size;
        c0.y = SIN_DEG(spin) * size;
        c0.x = cos_t * arm - sin_t0;
        c0.z = sin_t * arm + cos_t0;
        spin1 = spin + 90;
        if (spin1 >= 360)
            spin1 -= 360;
        c1.y = SIN_DEG(spin1) * size;
        arm = COS_DEG(spin1) * size;
        spin2 = quarter_on(spin1);
        c1.x = cos_t * arm - sin_t0;
        c1.z = sin_t * arm + cos_t0;
        arm = COS_DEG(spin2) * size;
        c2.y = SIN_DEG(spin2) * size;
        c2.x = cos_t * arm - sin_t0;
        c2.z = sin_t * arm + cos_t0;
        spin3 = quarter_on(spin2);
        arm = COS_DEG(spin3) * size;
        c3.y = SIN_DEG(spin3) * size;
        c3.x = cos_t * arm - sin_t0;
        c3.z = sin_t * arm + cos_t0;

        if (sq->color == ORBIT_COLOR_FACING) {
            int t1 = spin < 270 ? spin + 90 : spin - 270;
            int t2 = quarter_on(t1);
            int t3 = quarter_on(t2);
            c0.x = COS_DEG(spin) * size;
            c0.y = 0.0f;
            c0.z = SIN_DEG(spin) * size;
            c1.x = COS_DEG(t1) * size;
            c1.y = 0.0f;
            c1.z = SIN_DEG(t1) * size;
            c2.x = COS_DEG(t2) * size;
            c2.y = 0.0f;
            c2.z = SIN_DEG(t2) * size;
            c3.x = COS_DEG(t3) * size;
            c3.y = 0.0f;
            c3.z = SIN_DEG(t3) * size;
            prim_quad_turn(&c0, &c1, &c2, &c3, self->camera_yaw, self->camera_pitch);
        }

        p0.x = (centre_x + c0.x) + sq->pos.x;
        p0.y = (centre_y + c0.y) + sq->pos.y;
        p0.z = (centre_z + c0.z) + sq->pos.z;
        p1.x = (centre_x + c1.x) + sq->pos.x;
        p1.y = (centre_y + c1.y) + sq->pos.y;
        p1.z = (centre_z + c1.z) + sq->pos.z;
        p2.x = (centre_x + c2.x) + sq->pos.x;
        p2.y = (centre_y + c2.y) + sq->pos.y;
        p2.z = (centre_z + c2.z) + sq->pos.z;
        p3.x = (centre_x + c3.x) + sq->pos.x;
        p3.y = (centre_y + c3.y) + sq->pos.y;
        p3.z = (centre_z + c3.z) + sq->pos.z;

        mode = PRIM_QUAD_PROJECTED;
        switch (sq->color) {
        case 0: r = 100; g = 150; b = 255; break;
        case 1: r = 250; g = 250; b = 155; break;
        case 2: r = 250; g = 100; b = 100; break;
        case 3: r = 50; g = 50; b = 255; break;
        case 4: r = 255; g = 50; b = 50; break;
        case 5: r = 50; g = 255; b = 50; break;
        case 6: r = 250; g = 100; b = 100; mode = PRIM_QUAD_IN_FRONT; break;
        case 7: r = 5; g = 5; b = 255; mode = PRIM_QUAD_IN_FRONT; break;
        default: continue;
        }
        draw_quad(self, &count, alpha, p0, p1, p2, p3, view, PRIM_BLEND_ADDITIVE, 0, 1, r, g, b, mode);
    }
}

static Vec3 bar_corner(float u, float y, float c, float s, float s0, float c0)
{
    Vec3 v;
    v.x = u * c - s0;
    v.y = y;
    v.z = u * s + c0;
    return v;
}

void prim_upright_bar_render(struct EffectPrim *self, const float *view)
{
    struct PrimUprightBar *bar = self->records.upright_bars;
    int count = 0;
    int i;

    for (i = 0; i < RECORD_COUNT; i++, bar++) {
        short alpha;
        float shift_len, far_end, near_end, c, s, cw, sw, cos_h, sin_h, sin_h0, cos_h0, nw, cnw, snw;
        float c_far, s_far, c_near, s_near;
        Vec3 p0, p1, p2, p3;

        if (bar->active != 1 || bar->alpha <= 0)
            continue;
        alpha = bar->alpha;
        shift_len = (float)bar->shift * bar->half_width;
        far_end = shift_len + bar->length;
        near_end = bar->length - shift_len;
        c = COS_DEG(bar->angle);
        s = SIN_DEG(bar->angle);
        cw = c * bar->half_width;
        sw = s * bar->half_width;
        cos_h = COS_DEG(bar->heading);
        sin_h = SIN_DEG(bar->heading);
        sin_h0 = sin_h * 0.0f;
        cos_h0 = cos_h * 0.0f;
        s_far = s * far_end;
        c_far = c * far_end;
        p1 = bar_corner(c_far - sw, s_far + cw, cos_h, sin_h, sin_h0, cos_h0);
        c_near = c * near_end;
        s_near = s * near_end;
        p2 = bar_corner(c_near - sw, s_near + cw, cos_h, sin_h, sin_h0, cos_h0);
        nw = bar->half_width * -1.0f;
        snw = s * nw;
        cnw = c * nw;
        p3 = bar_corner(c_near - snw, cnw + s_near, cos_h, sin_h, sin_h0, cos_h0);
        p0 = bar_corner(c_far - snw, cnw + s_far, cos_h, sin_h, sin_h0, cos_h0);

        switch (bar->color) {
        case 0:
            draw_quad(self, &count, alpha, p0, p1, p2, p3, view, PRIM_BLEND_ADDITIVE, 0, 0, 225, 225, 80,
                      PRIM_QUAD_PROJECTED);
            break;
        case 1:
            draw_quad(self, &count, alpha, p0, p1, p2, p3, view, PRIM_BLEND_ADDITIVE, 0, 0, 255, 255, 255,
                      PRIM_QUAD_PROJECTED);
            break;
        case 2:
            draw_quad(self, &count, alpha, p0, p1, p2, p3, view, PRIM_BLEND_ADDITIVE, 0, 0, 65, 184, 240,
                      PRIM_QUAD_IN_FRONT);
            break;
        default:
            draw_quad(self, &count, alpha, p0, p1, p2, p3, view, BLEND_ALPHA, 0, 0, 200, 200, 255,
                      PRIM_QUAD_PROJECTED);
            break;
        }
    }
}

void prim_wall_render(struct EffectPrim *self, const float *view)
{
    struct PrimWall *wall = self->records.walls;
    int count = 0;
    int i;

    for (i = 0; i < RECORD_COUNT; i++, wall++) {
        short alpha;
        float top;
        Vec3 p0, p1, p2, p3;
        float style;

        if (wall->active != 1 || wall->alpha <= 0)
            continue;
        alpha = wall->alpha;
        top = -wall->height;
        p0.x = wall->from.x;
        p0.y = 0.0f;
        p0.z = wall->from.z;
        p1.x = wall->to.x;
        p1.y = 0.0f;
        p1.z = wall->to.z;
        p2.x = wall->to.x;
        p2.y = top;
        p2.z = wall->to.z;
        p3.x = wall->from.x;
        p3.y = top;
        p3.z = wall->from.z;
        style = wall->style;
        if (style == 1.0f)
            draw_quad(self, &count, alpha, p0, p1, p2, p3, view, BLEND_ALPHA, 0, 0, 255, 155, 155,
                      PRIM_QUAD_PROJECTED);
        else if (style == 2.0f)
            draw_quad(self, &count, alpha, p0, p1, p2, p3, view, BLEND_ALPHA, 0, 0, 155, 255, 155,
                      PRIM_QUAD_PROJECTED);
        else if (style == 3.0f)
            draw_quad(self, &count, alpha, p0, p1, p2, p3, view, PRIM_BLEND_ADDITIVE, 0, 0, 255, 255, 100,
                      PRIM_QUAD_PROJECTED);
        else if (style == 4.0f)
            draw_quad(self, &count, alpha, p0, p1, p2, p3, view, PRIM_BLEND_ADDITIVE, 0, 0, 255, 150, 150,
                      PRIM_QUAD_PROJECTED);
        else
            draw_quad(self, &count, alpha, p0, p1, p2, p3, view, PRIM_BLEND_ADDITIVE, 0, 0, 255, 255, 255,
                      PRIM_QUAD_PROJECTED);
    }
}

#define SPARK_SINKING 0
#define SPARK_FLYING  1

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

void prim_sparks_process(struct EffectPrim *self)
{
    struct PrimSpark *sp = self->records.sparks;
    int i;

    face_camera(self);
    follow_master(self);
    for (i = 0; i < RECORD_COUNT; i++, sp++) {
        int age;
        short spin;

        if (sp->active != 1)
            continue;
        age = ++sp->age;
        if (age <= 0)
            continue;
        if (age % 3 == 0) {
            sp->trail_3 = sp->trail_2;
            sp->trail_2 = sp->trail_1;
            sp->trail_1 = sp->pos;
        }
        if (age <= sp->flash_shift + 180 && age >= sp->flash_shift + 160) {
            char phase = (char)(age % 20);
            if (phase == 15 || phase == 19)
                sp->flash = 1;
            else
                sp->flash = phase == 17 ? 2 : 0;
        } else {
            sp->flash = 0;
        }
        spin = (short)(sp->spin + 5);
        sp->spin = spin;
        if (spin >= 360)
            sp->spin = (short)(spin - 360);

        if (sp->state == SPARK_SINKING) {
            sp->pos.y = sp->pos.y - 0.25f;
            if (sp->age > 154) {
                sp->state = SPARK_FLYING;
                sp->alpha = 250;
            }
        } else if (sp->state == SPARK_FLYING) {
            float speed = sp->speed;
            float along = speed * COS_DEG(sp->launch);
            sp->pos.y = SIN_DEG(sp->launch) * speed + sp->pos.y + 0.02f;
            sp->alpha--;
            sp->pos.x = along * COS_DEG(sp->heading) + sp->pos.x;
            sp->pos.z = along * SIN_DEG(sp->heading) + sp->pos.z;
            sp->speed = speed * 0.98f;
            if (rand_mod(2) == 0)
                sp->alpha--;
            if (sp->alpha < 0)
                sp->alpha = 0;
        }

        if (sp->age > 2000) {
            sp->age = 0;
            sp->state = SPARK_SINKING;
            sp->heading = (short)rand_mod(360);
            sp->pos = self->base.pos;
            sp->pos.x = self->base.pos.x - 15.0f;
            sp->speed = (float)rand_mod(100) * 0.004f + 0.4f;
        }
    }
}

void prim_sparks_render(struct EffectPrim *self, const float *view)
{
    struct PrimSpark *sp = self->records.sparks;
    int count = 0;
    int i, j;

    for (i = 0; i < RECORD_COUNT; i++, sp++) {
        unsigned char r, g, b;
        float shrink = 0.0f;

        switch (sp->color) {
        case 0: r = 70; g = 70; b = 255; break;
        case 1: r = 255; g = 70; b = 70; break;
        case 2: r = 70; g = 255; b = 70; break;
        case 3: r = 255; g = 255; b = 90; break;
        default: r = 255; g = 70; b = 255; break;
        }
        if (sp->active != 1 || sp->alpha <= 0)
            continue;

        for (j = 0; j < 2; j++) {
            int turn0 = sp->spin, turn1, turn2, turn3;
            float radius = sp->size - shrink;
            const Vec3 *at;
            Vec3 p0, p1, p2, p3;
            short alpha;

            turn1 = turn0 < 270 ? turn0 + 90 : turn0 - 270;
            turn2 = quarter_on(turn1);
            turn3 = quarter_on(turn2);
            p0.x = COS_DEG(turn0) * radius;
            p0.y = 0.0f;
            p0.z = SIN_DEG(turn0) * radius;
            p1.x = COS_DEG(turn1) * radius;
            p1.y = 0.0f;
            p1.z = SIN_DEG(turn1) * radius;
            p2.x = COS_DEG(turn2) * radius;
            p2.y = 0.0f;
            p2.z = SIN_DEG(turn2) * radius;
            p3.x = COS_DEG(turn3) * radius;
            p3.y = 0.0f;
            p3.z = SIN_DEG(turn3) * radius;
            prim_quad_turn(&p0, &p1, &p2, &p3, self->camera_yaw, self->camera_pitch);

            if (j == 0)
                at = &sp->pos;
            else if (j == 1)
                at = &sp->trail_1;
            else if (j == 2)
                at = &sp->trail_2;
            else
                at = &sp->trail_3;
            p0.x = at->x + p0.x;
            p0.y = at->y + p0.y;
            p0.z = at->z + p0.z;
            p1.x = at->x + p1.x;
            p1.y = at->y + p1.y;
            p1.z = at->z + p1.z;
            p2.x = at->x + p2.x;
            p2.y = at->y + p2.y;
            p2.z = at->z + p2.z;
            p3.x = at->x + p3.x;
            p3.y = at->y + p3.y;
            p3.z = at->z + p3.z;

            alpha = (short)(sp->alpha / (j + 1));
            if (sp->flash == 0)
                draw_quad(self, &count, alpha, p0, p1, p2, p3, view, PRIM_BLEND_ADDITIVE, 0, 1, r, g, b,
                          PRIM_QUAD_PROJECTED);
            else
                draw_quad(self, &count, (short)(alpha / 4), p0, p1, p2, p3, view, PRIM_BLEND_ADDITIVE,
                          (char)sp->flash, 1, 255, 255, 255, PRIM_QUAD_PROJECTED);
            shrink += j == 0 ? 0.5f : 0.25f;
        }
    }
}

static void needle(struct EffectPrim *self, const struct PrimStripe *st, int spread, int *count, short alpha,
                   const float *view, unsigned char r, unsigned char g, unsigned char b)
{
    int h = st->heading;
    int left = h - spread < 0 ? h - spread + 360 : h - spread;
    int right = h + spread < 360 ? h + spread : h + spread - 360;
    float length = st->length;
    Vec3 p0, p1, p2, p3;

    p3.x = 0.0f;
    p3.y = 0.0f;
    p3.z = 0.0f;
    p0.x = 0.0f;
    p0.y = 0.0f;
    p0.z = 0.0f;
    p1.y = 0.0f;
    p2.y = 0.0f;
    p1.x = COS_DEG(left) * length;
    p1.z = SIN_DEG(left) * length;
    p2.x = COS_DEG(right) * length;
    p2.z = SIN_DEG(right) * length;
    prim_quad_turn(&p0, &p1, &p2, &p3, self->camera_yaw, self->camera_pitch);
    draw_quad(self, count, alpha, p0, p1, p2, p3, view, PRIM_BLEND_ADDITIVE, 0, 0, r, g, b, PRIM_QUAD_PROJECTED);
}

void prim_needle_render(struct EffectPrim *self, const float *view)
{
    struct PrimStripe *st = self->records.stripes;
    int count = 0;
    int i;

    for (i = 0; i < RECORD_COUNT; i++, st++) {
        if (st->active != 1 || st->alpha <= 0)
            continue;
        needle(self, st, 3, &count, (short)(int)((float)st->alpha * 0.5f), view, 255, 255, 255);
        needle(self, st, 5, &count, st->alpha, view, 130, 130, 255);
    }
}

void prim_stripes_process(struct EffectPrim *self)
{
    struct PrimStripe *st = self->records.stripes;
    int i;

    face_camera(self);
    follow_master(self);
    for (i = 0; i < RECORD_COUNT; i++, st++) {
        if (st->active != 1)
            continue;
        st->age++;
        if (st->age <= 20) {
            st->alpha += 4;
        } else if (st->age > 50) {
            st->alpha -= 2;
            if (st->alpha < 0)
                st->alpha = 0;
        }
    }
}

void prim_stripes_render(struct EffectPrim *self, const float *view)
{
    struct PrimStripe *st = self->records.stripes;
    int count = 0;
    int i;

    for (i = 0; i < RECORD_COUNT; i++, st++) {
        int h, right, ahead, back, left;
        float shift_x, shift_z, right_x, right_z, left_x, left_z, length;
        Vec3 a, b, c, d;

        if (st->active != 1 || st->alpha <= 0)
            continue;
        h = st->heading;
        right = h >= 90 ? h - 90 : h + 270;
        shift_x = COS_DEG(right) * st->side_shift;
        shift_z = SIN_DEG(right) * st->side_shift;
        right_x = COS_DEG(right) * st->half_width + shift_x;
        right_z = SIN_DEG(right) * st->half_width + shift_z;
        ahead = h < 0 ? h + 360 : h;
        length = st->length;
        a.x = COS_DEG(ahead) * length + right_x;
        a.y = 0.0f;
        a.z = SIN_DEG(ahead) * length + right_z;
        back = h >= 180 ? h - 180 : h + 180;
        b.x = COS_DEG(back) * length + right_x;
        b.y = 0.0f;
        b.z = SIN_DEG(back) * length + right_z;
        left = h < 270 ? h + 90 : h - 270;
        left_x = COS_DEG(left) * st->half_width + shift_x;
        left_z = SIN_DEG(left) * st->half_width + shift_z;
        d.x = COS_DEG(ahead) * length + left_x;
        d.y = 0.0f;
        d.z = SIN_DEG(ahead) * length + left_z;
        c.x = COS_DEG(back) * length + left_x;
        c.y = 0.0f;
        c.z = SIN_DEG(back) * length + left_z;
        draw_quad(self, &count, st->alpha, d, c, b, a, view, BLEND_ALPHA, 0, 0, 255, 255, 255, PRIM_QUAD_PROJECTED);
    }
}

void prim_box_process(struct EffectPrim *self)
{
    struct PrimBox *box = self->records.boxes;
    int i;

    for (i = 0; i < RECORD_COUNT; i++, box++) {
        if (box->active != 1)
            continue;
        box->age++;
        if (box->age >= 10) {
            box->alpha--;
            if (box->alpha <= 0) {
                box->alpha = 0;
                if (box->age > 30)
                    box->active = 0;
            }
        } else {
            box->alpha += 9;
        }
        box->half_z = box->half_z + 0.005f;
        box->half_x = box->half_x + 0.005f;
        box->height = SIN_DEG(box->age) * box->full_height;
    }
}

void prim_tube_process(struct EffectPrim *self)
{
    struct PrimTube *tube = &self->records.tube;

    self->base.pos = self->master->base.pos;
    tube->age++;
    tube->start = self->base.pos;
    tube->start.y = tube->start.y - 8.0f;
    if (tube->style == TUBE_STYLE_TEAL) {
        if (tube->age > 40) {
            tube->alpha -= 3;
            if (tube->alpha < 0)
                tube->alpha = 0;
        } else if (tube->age < 20) {
            tube->alpha += 6;
        }
    } else if (tube->style == TUBE_STYLE_PINK) {
        int phase = tube->age % 360;
        if (tube->age <= 80)
            tube->alpha++;
        tube->radius = SIN_DEG(phase) * 0.005f + tube->radius;
    } else {
        tube->alpha = (short)(tube->age % 10 < 5 ? tube->alpha + 2 : tube->alpha - 2);
    }

    if (tube->target > 0.0f) {
        struct RenderObject *actor;

        if (tube->target == (float)(unsigned)g_session1.ownAid)
            actor = (struct RenderObject *)game_current_player();
        else
            actor = (struct RenderObject *)game_find_actor((int)(unsigned)tube->target);
        if (actor) {
            tube->end = actor->pos;
            tube->end.y = tube->end.y - 8.0f;
            return;
        }
    }
    tube->active = 0;
    tube->alpha = 0;
}

void prim_orbit_square_process(struct EffectPrim *self)
{
    struct PrimOrbitSquare *sq = self->records.orbit_squares;
    int i;

    face_camera(self);
    for (i = 0; i < RECORD_COUNT; i++, sq++) {
        int age;

        if (sq->active != 1)
            continue;
        age = ++sq->age;
        if (age > 0)
            sq->radius = sq->radius_speed + sq->radius;
        if (age >= 1 && age <= 10) {
            sq->alpha += sq->color != 0 ? 25 : 15;
        } else if (age > 25) {
            sq->alpha -= 7;
            if (sq->alpha <= 0)
                sq->alpha = 0;
        }
        if (sq->color == ORBIT_COLOR_FACING)
            sq->spin += 3;
        if (sq->spin >= 360)
            sq->spin -= 360;
        sq->tilt = sq->tilt + (sq->color == ORBIT_COLOR_FACING ? 3.0f : 5.0f);
        if (sq->tilt >= 360.0f)
            sq->tilt = sq->tilt - 360.0f;
    }
}

void prim_upright_bar_process(struct EffectPrim *self)
{
    struct PrimUprightBar *bar = self->records.upright_bars;
    int i;

    for (i = 0; i < RECORD_COUNT; i++, bar++) {
        int age;

        if (bar->active != 1)
            continue;
        age = ++bar->age;
        if (age > 0)
            bar->length = bar->length + bar->length_speed;
        if (age >= 11 && age <= 20) {
            if (bar->shift == 0x10)
                bar->alpha += 15;
            else if (bar->shift == 0x18)
                bar->alpha += g_session1.map_rule_flags[0] == 1 ? 8 : 15;
            else
                bar->alpha += 15;
        } else {
            bar->alpha -= 5;
            if (bar->alpha <= 0)
                bar->alpha = 0;
        }
    }
}

void prim_wall_process(struct EffectPrim *self)
{
    struct PrimWall *wall = self->records.walls;
    int i;

    follow_master(self);
    for (i = 0; i < RECORD_COUNT; i++, wall++) {
        if (wall->active != 1)
            continue;
        wall->age++;
        wall->phase += wall->style == 1.0f ? 4 : 2;
        if (wall->phase >= 360)
            wall->phase -= 360;
        wall->height = SIN_DEG(wall->phase) * 10.0f + 20.0f;
    }
}

void prim_needle_process(struct EffectPrim *self)
{
    struct PrimStripe *st = self->records.stripes;
    int i;

    face_camera(self);
    for (i = 0; i < RECORD_COUNT; i++, st++) {
        if (st->active != 1)
            continue;
        st->age++;
        if (st->age <= 10) {
            st->alpha += 18;
        } else if (st->age > 20) {
            st->alpha -= 15;
            if (st->alpha < 0)
                st->alpha = 0;
        }
    }
}
