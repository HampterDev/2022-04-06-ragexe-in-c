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

static Vec3 vec3_add(Vec3 a, Vec3 b)
{
    return vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

#define QUAD_UV 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_PROJECTED

#define GLOW_AT(g, n) (&(g)->glow[(n) == 0 ? 2 : (n) == 1 ? 0 : (n) == 2 ? 3 : 1])

static short abs_short(float v)
{
    short whole = (short)(int)v;

    return whole < 0 ? (short)-whole : whole;
}

static short glow_reach(const Vec3 *at)
{
    short reach = abs_short(at->x);

    if (reach < 30)
        reach = abs_short(at->z);
    return reach;
}

static void glow_fade(struct PrimWanderGlows *glows, int n)
{
    Vec3 *at = GLOW_AT(glows, n);

    glows->alpha[n] = (signed char)(glows->alpha[n] - 2);
    if (glows->alpha[n] <= 1) {
        at->x = (float)(rand() % 31 - 15);
        at->z = (float)(rand() % 31 - 15);
    }
}

static void glow_flash(struct PrimWanderGlows *glows, int n, int odds)
{
    signed char frame;

    if (rand() % odds == 0 && glows->alpha[n] > 100 && glows->flash[n] == 0)
        glows->flash[n] = 1;
    frame = glows->flash[n];
    if (frame < 1)
        return;
    frame = (signed char)(frame + 1);
    glows->flash[n] = frame;
    if (frame <= 90) {
        glows->alpha[n] = (signed char)(glows->alpha[n] - 6);
        if (glows->alpha[n] < 0)
            glows->alpha[n] = 0;
    } else if (frame < 120) {
        glows->alpha[n] = (signed char)(glows->alpha[n] + 4);
        if (glows->alpha[n] > 120)
            glows->alpha[n] = 120;
    } else {
        glows->flash[n] = 0;
    }
}

unsigned char prim_wander_glows_process(struct EffectPrim *self)
{
    int i, k, n;

    face_camera(self);
    self->base.pos = self->master->base.pos;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimWanderGlows *glows = &self->records.wander_glows[i];
        float speed = 0.05f;
        Vec3 *at;

        glows->frame++;
        if (glows->active != 1)
            continue;
        if (glows->alpha[0] < 120)
            glows->alpha[0]++;
        if (glows->mode == 1) {
            speed = 0.03f;
        } else {
            for (n = 1; n < 4; n++)
                if (glows->alpha[n] < 120)
                    glows->alpha[n]++;
        }

        for (k = 0; k < 8; k++) {
            float cur = glows->wander[k].angle, target = glows->wander[k].target;

            if (target > cur) {
                cur = cur + 3.0f;
                if (cur > 359.0f)
                    cur = 359.0f;
                glows->wander[k].angle = cur;
                if (cur > target) {
                    int whole = (int)cur;

                    glows->wander[k].target = (float)(whole != 0 ? rand_below(whole) : 0);
                }
            } else {
                cur = cur - 2.0f;
                if (0.0f > cur)
                    cur = 0.0f;
                glows->wander[k].angle = cur;
                if (target >= cur) {
                    int whole = (int)cur;

                    glows->wander[k].target = (float)(359 - (whole != 0 ? rand_below(whole) : 0));
                }
            }
        }

        at = GLOW_AT(glows, 0);
        if (glows->mode == 1) {
            at->y = SIN((int)glows->wander[0].angle) * speed + at->y;
        } else {
            at->x = SIN((int)glows->wander[0].angle) * speed + at->x;
            at->z = SIN((int)glows->wander[1].angle) * speed + at->z;
        }
        if ((glows->mode == 1 && glows->frame > 3000) || glow_reach(at) >= 30) {
            glows->alpha[0] = (signed char)(glows->alpha[0] - 2);
            if (glows->alpha[0] <= 1) {
                if (glows->mode == 1) {
                    glows->frame = 0;
                    at->y = 0.0f;
                    at->x = 0.0f;
                    at->z = 0.0f;
                } else {
                    at->x = (float)(rand() % 31 - 15);
                    at->z = (float)(rand() % 31 - 15);
                }
            }
        } else {
            glow_flash(glows, 0, glows->mode == 1 ? 20 : 125);
        }
        if (glows->mode == 1)
            return 1;

        at = GLOW_AT(glows, 1);
        at->x = at->x - SIN((int)glows->wander[2].angle) * speed;
        at->z = at->z - SIN((int)glows->wander[3].angle) * speed;
        if (glow_reach(at) >= 30)
            glow_fade(glows, 1);
        else
            glow_flash(glows, 1, 125);

        at = GLOW_AT(glows, 2);
        at->x = SIN((int)glows->wander[4].angle) * speed + at->x;
        at->z = at->z - SIN((int)glows->wander[5].angle) * speed;
        if (glow_reach(at) > 30)
            glow_fade(glows, 2);
        else
            glow_flash(glows, 2, 125);

        at = GLOW_AT(glows, 3);
        at->x = at->x - SIN((int)glows->wander[6].angle) * speed;
        at->z = SIN((int)glows->wander[7].angle) * speed + at->z;
        if (glow_reach(at) > 30)
            glow_fade(glows, 3);
        else
            glow_flash(glows, 3, 125);
    }
    return 1;
}

void prim_wander_glows_render(struct EffectPrim *self, const float *view)
{
    int count = 0;
    int i, n, k;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        const struct PrimWanderGlows *glows = &self->records.wander_glows[i];
        Vec3 corner[4];

        if (glows->active != 1)
            continue;
        facing_square_corners(self, glows->size, corner);
        for (n = 0; n < 4; n++) {
            const Vec3 *at = &glows->glow[n == 0 ? 2 : n == 1 ? 0 : n == 2 ? 3 : 1];
            Vec3 p[4];
            int alpha = glows->alpha[n] * 2;
            unsigned char r, g, b;

            for (k = 0; k < 4; k++)
                p[k] = vec3_add(*at, corner[k]);
            if (alpha < 0)
                alpha = 0;
            if (glows->style == 7) {
                r = 0xfa; g = 0x46; b = 0x64;
            } else if (glows->style == 8) {
                r = 0xfa; g = 0xfa; b = 0x0a;
            } else if (glows->style == 10) {
                r = 0xfa; g = 0xfa; b = 0xfa;
            } else {
                r = 0x46; g = 0x78; b = 0x64;
            }
            prim_draw_quad(self, &count, (short)alpha, p[0], p[1], p[2], p[3], view, PRIM_BLEND_ADDITIVE, 0, 0,
                           r, g, b, QUAD_UV);
        }
    }
}

unsigned char prim_nested_squares_process(struct EffectPrim *self)
{
    int i, k;

    face_camera(self);
    if (self->records.nested_squares[0].style == 1)
        self->base.pos = self->master->base.pos;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimNestedSquares *sq = &self->records.nested_squares[i];

        if (sq->active != 1)
            continue;
        if (++sq->frame <= 0)
            continue;
        if (sq->frame < 20) {
            for (k = 0; k < 10; k++) {
                float cap = (float)(70 + 20 * k);
                float faded = sq->alpha[k] + 13.0f;

                sq->alpha[k] = faded;
                if (faded >= cap)
                    sq->alpha[k] = cap;
            }
        } else {
            for (k = 0; k < 9; k++) {
                float faded = sq->alpha[k] - 5.0f;

                sq->alpha[k] = faded;
                if (0.0f > faded)
                    sq->alpha[k] = 0.0f;
            }
            if (sq->frame > 100) {
                float faded = sq->alpha[9] - 5.0f;

                sq->alpha[9] = faded;
                if (faded > 0.0f)
                    sq->size = sq->size * 1.01f;
                if (0.0f > faded)
                    sq->alpha[9] = 0.0f;
            }
        }
        sq->size = sq->size * 1.01f;
    }
    return 1;
}

void prim_nested_squares_render(struct EffectPrim *self, const float *view)
{
    int count = 0;
    int i, square;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        const struct PrimNestedSquares *sq = &self->records.nested_squares[i];

        if (sq->active != 1)
            continue;
        for (square = 10; square >= 1; square--) {
            int grow = 10 - square;
            float size, drop;
            Vec3 q[4];
            int alpha;

            if (sq->style != 2)
                grow = grow * 3;
            size = (float)grow + sq->size;
            q[0] = vec3(size * COS(45) + sq->shift_x, 0.0f, size * SIN(45));
            q[1] = vec3(size * COS(135) + sq->shift_x, 0.0f, size * SIN(135));
            q[2] = vec3(size * COS(225) + sq->shift_x, 0.0f, size * SIN(225));
            q[3] = vec3(size * COS(315) + sq->shift_x, 0.0f, size * SIN(315));
            prim_quad_turn(&q[0], &q[1], &q[2], &q[3], self->camera_yaw, self->camera_pitch);
            drop = sq->style == 2 ? 40.0f : 30.0f;
            q[0].y = q[0].y - drop;
            q[1].y = q[1].y - drop;
            q[2].y = q[2].y - drop;
            q[3].y = q[3].y - drop;
            alpha = (int)sq->alpha[square - 1];
            if (alpha <= 0)
                continue;
            prim_draw_quad(self, &count, (short)alpha, q[1], q[0], q[3], q[2], view, PRIM_BLEND_ADDITIVE,
                           sq->texture, 0, square == 10 ? 0x37 : 5, square == 10 ? 0x37 : 5, 0xff, QUAD_UV);
        }
    }
}

static int square_shade(float shade, unsigned char *v)
{
    if (shade == 0.0f)
        *v = 0x69;
    else if (shade == 1.0f)
        *v = 5;
    else if (shade == 2.0f)
        *v = 0x37;
    else if (shade == 3.0f)
        *v = 0x9b;
    else
        return 0;
    return 1;
}

void prim_standing_square_render(struct EffectPrim *self, const float *view)
{
    int count = 0;
    unsigned char r = 0, g = 0, b = (unsigned char)(unsigned)(size_t)view;
    int i;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimStandingSquare *sq = &self->records.standing_squares[i];
        float half;
        int yaw, back;
        unsigned char level;
        int has_shade, mode;
        Vec3 bottom, bottom_back;

        if (sq->active != 1 || sq->alpha <= 0 || sq->frame <= 0)
            continue;
        half = sq->width * 0.5f;
        yaw = sq->yaw;
        back = yaw + (yaw < 180 ? 180 : -180);
        sq->top.x = half * COS(yaw) - SIN(yaw) * 0.0f;
        sq->top.z = half * SIN(yaw) + COS(yaw) * 0.0f;
        sq->top.y = sq->width * 0.5f - sq->drop;
        sq->top_back.x = half * COS(back) - SIN(back) * 0.0f;
        sq->top_back.z = half * SIN(back) + COS(back) * 0.0f;
        sq->top_back.y = sq->width * 0.5f - sq->drop;
        bottom_back = vec3(sq->top_back.x, sq->top_back.y - sq->width, sq->top_back.z);
        bottom = vec3(sq->top.x, sq->top.y - sq->width, sq->top.z);

        has_shade = square_shade(sq->shade, &level);
        mode = PRIM_QUAD_IN_FRONT;
        if (sq->style == 2.0f) {
            if (has_shade) { r = level; b = level; g = 0xff; }
        } else if (sq->style == 3.0f) {
            if (has_shade) { r = level; g = level; b = 0xff; }
        } else if (sq->style == 4.0f) {
            if (has_shade) { b = level; r = 0xff; g = 0xff; }
        } else if (sq->style == 5.0f) {
            if (has_shade) { g = level; b = level; r = 0xff; }
        } else if (sq->style == 6.0f) {
            if (has_shade) { g = level; b = 0xff; r = 0xff; }
        } else if (sq->style == 7.0f) {
            if (has_shade) { r = level; b = 0xff; g = 0xff; }
        } else if (sq->style == 8.0f) {
            if (has_shade && sq->shade == 3.0f) {
                r = 0xff; b = 0xff; g = 0xff;
            } else if (has_shade) {
                r = level; g = level; b = level;
            }
        } else {
            if (has_shade) { r = level; g = level; b = 0xff; }
            mode = PRIM_QUAD_PROJECTED;
        }
        if (!(sq->width > 0.0f))
            continue;
        prim_draw_quad(self, &count, sq->alpha, sq->top, sq->top_back, bottom_back, bottom, view,
                       PRIM_BLEND_ADDITIVE, 0, 0, r, g, b, 0.0f, 1.0f, 0.0f, 1.0f, (unsigned char)mode);
    }
}

static int turn_back(int deg, int step)
{
    deg -= step;
    return deg < 0 ? deg + 360 : deg;
}

static void shape_corners(struct EffectPrim *self, const struct PrimShapeRow *row, float radius, Vec3 corner[4])
{
    static const int steps[3][3] = { { 90, 90, 90 }, { 60, 120, 60 }, { 130, 50, 130 } };
    int deg = row->angle, k, shape;

    if (row->shape == 0.0f)
        shape = 0;
    else if (row->shape == 1.0f)
        shape = 1;
    else if (row->shape == 2.0f)
        shape = 2;
    else
        shape = -1;
    for (k = 0; k < 4; k++) {
        if (k && shape >= 0)
            deg = turn_back(deg, steps[shape][k - 1]);
        corner[k] = vec3(COS(deg) * radius, 0.0f, SIN(deg) * radius);
    }
    prim_quad_turn(&corner[0], &corner[1], &corner[2], &corner[3], self->camera_yaw, self->camera_pitch);
}

static float target_distance(const struct EffectPrim *self, const struct PrimShapeRow *row)
{
    float dx = row->target.x - self->base.pos.x;
    float dz = row->target.z - self->base.pos.z;

    return float_sqrt(dx * dx + dz * dz);
}

void prim_growing_row_render(struct EffectPrim *self, const float *view)
{
    int count = 0;
    int i, n, k;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        const struct PrimShapeRow *row = &self->records.shape_rows[i];

        if (row->active != 1 || row->alpha <= 0)
            continue;
        for (n = 4; n < 18; n++) {
            float fn = (float)n;
            float radius = fn * row->size / 14.0f * (float)(n + 46) / 50.0f +
                      SIN((int)row->wobble) * row->size * 0.05f;
            Vec3 corner[4], q[4], base;
            float dist;
            signed char look;

            shape_corners(self, row, radius, corner);
            dist = (float)((double)target_distance(self, row) / 12.0 * (double)fn);
            base.x = COS(row->heading) * dist + self->base.pos.x;
            base.y = self->base.pos.y - 12.0f + 0.0f;
            base.z = SIN(row->heading) * dist + self->base.pos.z;
            for (k = 0; k < 4; k++)
                q[k] = vec3_add(base, corner[k]);
            look = self->records.shape_rows[0].look;
            if (look == 0 || look == 3)
                prim_draw_quad(self, &count, row->alpha, q[0], q[1], q[2], q[3], view, BLEND_ALPHA,
                               (char)(unsigned char)row->texture, 1, 0xff, 0xff, 0xff, QUAD_UV);
            else if (look == 4)
                prim_draw_quad(self, &count, row->alpha, q[1], q[0], q[3], q[2], view, PRIM_BLEND_ADDITIVE,
                               (char)((short)(row->texture + n) % self->motion_count), 1, 0xff, 0x96, 0x96,
                               QUAD_UV);
        }
    }
}

void prim_advancing_row_render(struct EffectPrim *self, const float *view)
{
    static const unsigned char blues[4] = { 0x37, 0x69, 0x4b, 0x55 };
    int count = 0;
    int i, k, n;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        const struct PrimShapeRow *row = &self->records.shape_rows[i];
        float radius;
        Vec3 corner[4];

        if (row->active != 1 || row->alpha <= 0)
            continue;
        radius = SIN((int)row->wobble) * row->size * 0.05f + row->size;
        shape_corners(self, row, radius, corner);
        for (k = 0; k < 5; k++) {
            const struct PrimShapeRow *first = &self->records.shape_rows[0];
            float dist = target_distance(self, row);
            float reach = dist / (float)first->pace * (float)row->frame - (float)(2 * k);
            Vec3 base, q[4];
            int alpha = (unsigned short)row->alpha;
            unsigned char blue;

            if (0.0f > reach)
                reach = 0.0f;
            base.x = COS(row->heading) * reach + self->base.pos.x;
            base.y = self->base.pos.y - 12.0f + 0.0f;
            base.z = SIN(row->heading) * reach + self->base.pos.z;
            for (n = 0; n < 4; n++)
                q[n] = vec3_add(base, corner[n]);
            if (k > 0) {
                alpha = alpha - 20 * (5 + k);
                if ((short)alpha < 0)
                    alpha = 0;
            }
            if ((unsigned)(int)first->look > 3)
                continue;
            blue = blues[first->look];
            prim_draw_quad(self, &count, (short)alpha, q[1], q[0], q[3], q[2], view, PRIM_BLEND_ADDITIVE,
                           (char)(unsigned char)row->texture, 1, blue, blue, 0xff, QUAD_UV);
        }
    }
}

#define WIDE_RING_FADING 11

void prim_wide_rings_process(struct EffectPrim *self)
{
    struct PrimWideRing *ring = self->records.wide_rings;
    int i;

    for (i = 0; i < PRIM_RECORD_COUNT; i++, ring++) {
        if ((signed char)ring->active <= 0)
            continue;
        ring->frame++;
        if (ring->angle >= 360)
            ring->angle -= 360;
        if (ring->style == WIDE_RING_FADING) {
            if (ring->frame <= 40)
                ring->alpha++;
            else if (ring->frame > 140)
                ring->alpha--;
            if (ring->alpha <= 0)
                ring->alpha = 0;
        }
        ring->radius = SIN(ring->frame % 360) * 0.1f + ring->base_radius;
    }
}

void prim_wide_rings_render(struct EffectPrim *self, const float *view)
{
    const struct PrimWideRing *ring = self->records.wide_rings;
    Vec3 outer_prev = vec3(0.0f, 0.0f, 0.0f);
    Vec3 inner_prev = vec3(0.0f, 0.0f, 0.0f);
    int count = 0;
    int i, k;

    for (i = 0; i < PRIM_RECORD_COUNT; i++, ring++) {
        float radius, wide;

        if (ring->active != 1 || ring->alpha <= 0)
            continue;
        radius = ring->radius;
        wide = radius * 4.0f;
        for (k = 0; k <= 10; k++) {
            short deg = (short)(k * 36 + ring->angle);
            Vec3 outer, inner;

            if (!(deg < 360))
                deg -= 360;
            outer = vec3(wide * COS(deg) + ring->outer_centre.x, ring->outer_centre.y,
                         wide * SIN(deg) + ring->outer_centre.z);
            inner = vec3(radius * COS(deg) + ring->inner_centre.x, ring->inner_centre.y,
                         radius * SIN(deg) + ring->inner_centre.z);
            if (k > 0)
                prim_draw_quad(self, &count, ring->alpha, outer_prev, outer, inner, inner_prev, view,
                               PRIM_BLEND_ADDITIVE, 0, 1, 0xe6, 0xff, 0xe6, QUAD_UV);
            outer_prev = outer;
            inner_prev = inner;
        }
    }
}

void prim_standing_square_process(struct EffectPrim *self)
{
    int i;

    self->base.pos = self->master->base.pos;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimStandingSquare *sq = &self->records.standing_squares[i];
        float swing, width;

        if (sq->active != 1)
            continue;
        sq->frame++;
        if (sq->frame <= 0)
            continue;
        sq->phase++;
        if (sq->phase >= 360)
            sq->phase -= 360;
        sq->grow = sq->grow * 1.02f;
        if (sq->grow > 20.0f) {
            sq->alpha -= 3;
            if (sq->alpha <= 0) {
                if (sq->style >= 2.0f) {
                    if (sq->frame > 300) {
                        sq->alpha = 0;
                        sq->active = 0;
                    } else {
                        sq->grow = 10.0f;
                        sq->yaw = (short)rand_below(360);
                        sq->alpha = 100;
                    }
                } else {
                    sq->grow = 10.0f;
                    sq->yaw = (short)rand_below(360);
                    sq->alpha = 200;
                }
            }
        }
        swing = SIN(sq->phase) * sq->grow * 0.1f;
        sq->width = swing;
        width = sq->grow * 0.9f + swing;
        sq->width = width;
        if (0.0f > width)
            sq->width = 0.0f;
    }
}

static void alpha_to_byte(short *alpha)
{
    if (*alpha >= 255)
        *alpha = 255;
    else if (*alpha < 0)
        *alpha = 0;
}

void prim_growing_row_process(struct EffectPrim *self)
{
    struct PrimShapeRow *row = &self->records.shape_rows[0];

    face_camera(self);
    if (row->active != 1)
        return;
    row->frame++;
    if (row->frame % 4 == 0) {
        row->texture++;
        if (row->texture >= self->motion_count)
            row->texture = 0;
    }
    if (row->frame < 30)
        row->alpha += 10;
    else if (row->frame > 80)
        row->alpha -= 3;
    alpha_to_byte(&row->alpha);
}

void prim_advancing_row_process(struct EffectPrim *self)
{
    int i;

    face_camera(self);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimShapeRow *row = &self->records.shape_rows[i];

        if (row->active != 1)
            continue;
        row->frame++;
        row->angle++;
        if (row->angle >= 360)
            row->angle -= 360;
        if (row->frame < 10)
            row->alpha += 25;
        else if (row->frame > 40)
            row->alpha -= 5;
        alpha_to_byte(&row->alpha);
    }
}
