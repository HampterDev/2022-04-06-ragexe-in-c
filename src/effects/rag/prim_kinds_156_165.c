#include "effects/rag/rag_effect.h"

#define BLEND_ALPHA 1

static Vec3 on_circle(int deg, float radius)
{
    Vec3 p;

    p.x = g_cosTable[deg] * radius;
    p.y = 0.0f;
    p.z = g_sinTable[deg] * radius;
    return p;
}

static Vec3 shifted(Vec3 p, float dx, float dy, float dz, int down)
{
    if (down) {
        p.x = p.x - dx;
        p.y = p.y - dy;
        p.z = p.z - dz;
    } else {
        p.x = p.x + dx;
        p.y = p.y + dy;
        p.z = p.z + dz;
    }
    return p;
}

void prim_jitter_quad_process(struct EffectPrim *self)
{
    struct PrimFanQuad *q = self->records.fan_quads;
    float base;

    prim_face_camera(self);
    self->base.pos = self->master->base.pos;
    if (q->active != 1)
        return;
    q->frame++;
    q->spin = q->spin + 5.0f;
    if (!(q->spin < 360.0f))
        q->spin = q->spin - 360.0f;
    q->bob++;
    if (q->bob >= 360)
        q->bob -= 360;
    q->pos.x = self->base.pos.x;
    q->pos.z = self->base.pos.z;
    base = self->base.pos.y - 6.0f;
    q->base_y = base;
    q->pos.y = g_sinTable[q->bob] * 3.0f + base;
}

static int jitter_step(int deg)
{
    int next = rand() % 16 + deg;

    next = next + (next + 85 < 360 ? 85 : -275);
    if (next < 0)
        next += 360;
    return next;
}

void prim_jitter_quad_render(struct EffectPrim *self, const float *view)
{
    struct PrimFanQuad *q = self->records.fan_quads;
    int count = 0;
    int i;

    for (i = 0; i < 4; i++, q++) {
        Vec3 p[4];
        float radius;
        int deg0, deg1, deg2, deg3, k;
        signed char style;
        unsigned char cr, cg, cb;
        short half;

        if (q->active != 1 || (short)q->alpha <= 0)
            continue;
        if (q->spin >= 360.0f || 0.0f > q->spin)
            q->spin = 0.0f;
        deg0 = q->angle;
        radius = g_sinTable[(int)q->spin] * q->size * 0.05f + q->size;
        p[0] = on_circle(deg0, radius);
        deg1 = jitter_step(deg0);
        p[1] = on_circle(deg1, radius);
        deg2 = jitter_step(deg1);
        p[2] = on_circle(deg2, radius);
        deg3 = jitter_step(deg2);
        p[3] = on_circle(deg3, radius);
        prim_quad_turn(&p[0], &p[1], &p[2], &p[3], self->camera_yaw, self->camera_pitch);
        for (k = 0; k < 4; k++) {
            p[k].x = p[k].x + q->pos.x;
            p[k].y = q->pos.y + p[k].y;
            p[k].z = q->pos.z + p[k].z;
        }

        style = self->records.fan_quads[0].jitter_style;
        prim_draw_quad(self, &count, q->alpha, p[1], p[0], p[3], p[2], view,
                       style == 2 ? PRIM_BLEND_ADDITIVE : BLEND_ALPHA, q->texture, 1, 255, 255, 255, 0.0f, 1.0f, 0.0f,
                       1.0f, PRIM_QUAD_PROJECTED);
        if (style == 1)
            cr = 5, cg = 5, cb = 255;
        else
            cr = 255, cg = 255, cb = 255;
        half = (short)((short)q->alpha / 2);
        prim_draw_quad(self, &count, half, shifted(p[1], 0.2f, 0.0f, 0.0f, 0), shifted(p[0], 0.2f, 0.0f, 0.0f, 0),
                       shifted(p[3], 0.2f, 0.0f, 0.0f, 0), shifted(p[2], 0.2f, 0.0f, 0.0f, 0), view,
                       PRIM_BLEND_ADDITIVE, q->texture, 1, cr, cg, cb, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_PROJECTED);
        prim_draw_quad(self, &count, half, shifted(p[1], 0.2f, 0.0f, 0.0f, 1), shifted(p[0], 0.2f, 0.0f, 0.0f, 1),
                       shifted(p[3], 0.2f, 0.0f, 0.0f, 1), shifted(p[2], 0.2f, 0.0f, 0.0f, 1), view,
                       PRIM_BLEND_ADDITIVE, q->texture, 1, cr, cg, cb, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_PROJECTED);
        prim_draw_quad(self, &count, half, shifted(p[1], 0.0f, 0.0f, 0.2f, 0), shifted(p[0], 0.0f, 0.0f, 0.2f, 0),
                       shifted(p[3], 0.0f, 0.0f, 0.2f, 0), shifted(p[2], 0.0f, 0.0f, 0.2f, 0), view,
                       PRIM_BLEND_ADDITIVE, q->texture, 1, cr, cg, cb, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_PROJECTED);
        prim_draw_quad(self, &count, half, shifted(p[1], 0.0f, 0.0f, 0.2f, 1), shifted(p[0], 0.0f, 0.0f, 0.2f, 1),
                       shifted(p[3], 0.0f, 0.0f, 0.2f, 1), shifted(p[2], 0.0f, 0.0f, 0.2f, 1), view,
                       PRIM_BLEND_ADDITIVE, q->texture, 1, cr, cg, cb, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_PROJECTED);
    }
}

unsigned char prim_drop_square_process(struct EffectPrim *self)
{
    struct PrimDropSquare *sq = self->records.drop_squares;
    int i;

    prim_face_camera(self);
    for (i = 0; i < 4; i++, sq++) {
        unsigned short alpha;
        short angle;

        if (sq->active != 1)
            continue;
        if (++sq->frame < 10)
            sq->alpha += 25;
        alpha = sq->alpha;
        if ((short)alpha >= 255) {
            sq->alpha = 255;
            alpha = 255;
        }
        if (sq->pos.y >= 0.0f) {
            alpha = (unsigned short)(alpha - 15);
            sq->alpha = alpha;
        } else {
            sq->pos.y = sq->fall + sq->pos.y;
        }
        angle = sq->angle;
        sq->pos.x = g_cosTable[angle] * sq->radius;
        sq->pos.z = g_sinTable[angle] * sq->radius;
        if ((short)alpha < 0) {
            sq->alpha = 0;
            sq->active = 0;
        }
        if (angle >= 360)
            sq->angle = (short)(angle - 360);
    }
    return 1;
}

void prim_drop_square_render(struct EffectPrim *self, const float *view)
{
    struct PrimDropSquare *sq = self->records.drop_squares;
    int count = 0;
    int i, k;

    for (i = 0; i < 4; i++, sq++) {
        Vec3 square[4], corner[4];
        short alpha;
        unsigned char g;

        if (sq->active != 1)
            continue;
        prim_camera_square(self, sq->size, square);
        for (k = 0; k < 4; k++) {
            corner[k].x = sq->pos.x + square[k].x;
            corner[k].y = sq->pos.y + square[k].y;
            corner[k].z = sq->pos.z + square[k].z;
        }
        alpha = (short)sq->alpha < 0 ? 0 : (short)sq->alpha;
        g = sq->style == 0 ? 255 : sq->style == 1 ? 160 : 90;
        if (sq->style <= 2)
            prim_draw_quad(self, &count, alpha, corner[0], corner[1], corner[2], corner[3], view,
                           PRIM_BLEND_ADDITIVE, 0, 0, 255, g, g, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_IN_FRONT);
        for (k = 0; k < 4; k++) {
            corner[k].x = square[k].x * 1.1f + sq->pos.x;
            corner[k].y = square[k].y * 1.1f + sq->pos.y;
            corner[k].z = square[k].z * 1.1f + sq->pos.z;
        }
        prim_draw_quad(self, &count, alpha, corner[0], corner[1], corner[2], corner[3], view, PRIM_BLEND_ADDITIVE, 0,
                       0, 255, 1, 1, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_IN_FRONT);
    }
}

void prim_lean_ring_process(struct EffectPrim *self)
{
    struct PrimWaveRing *ring = self->records.wave_rings;
    int i, j;

    self->base.pos = self->master->base.pos;
    for (i = 0; i < 4; i++, ring++) {
        unsigned short from;

        if (ring->active != 1)
            continue;
        if (++ring->frame <= 0)
            continue;
        ring->radius = ring->radius + 0.07f;
        ring->tilt--;
        if (ring->tilt < 10) {
            ring->tilt = 10;
            from = 0;
        } else {
            from = ring->alpha;
        }
        if (ring->radius >= 3.5f) {
            from = (unsigned short)(from - 3);
            ring->alpha = from;
            if ((short)from <= 0) {
                ring->alpha = 0;
                ring->radius = 2.87f;
                ring->tilt = 74;
            }
        } else {
            ring->alpha = (unsigned short)(from + 10);
        }
        for (j = 0; j < WAVE_RING_POINTS; j++) {
            int phase = i * 45 < 90 ? ring->frame + i * 90 : (ring->frame + i * 45) * 2;
            float base = ring->base_height;
            ring->heights[j] = base * g_sinTable[j * 9] * 0.15f * g_sinTable[phase % 360] + base;
        }
    }
}

void prim_lean_ring_render(struct EffectPrim *self, const float *view)
{
    struct PrimWaveRing *ring = self->records.wave_rings;
    Vec3 low = {0.0f, 0.0f, 0.0f}, high = {0.0f, 0.0f, 0.0f};
    Vec3 prev_low = {0.0f, 0.0f, 0.0f}, prev_high = {0.0f, 0.0f, 0.0f};
    int count = 0;
    int i;

    for (i = 0; i < 4; i++, ring++) {
        int step, along, point;

        if (ring->active != 1 || ring->frame <= 0)
            continue;
        step = ring->arc / 20;
        point = 0;
        if (ring->arc < 0)
            continue;
        low.y = 5.0f;
        for (along = 0; along <= ring->arc; along += step) {
            int deg = (ring->arc_start + along) % 360;
            float cos_d, sin_d, out;

            if ((unsigned short)ring->arc == 360 && point == 20)
                deg = ring->arc_start;
            cos_d = g_cosTable[deg];
            sin_d = g_sinTable[deg];
            low.x = ring->radius * cos_d;
            low.z = ring->radius * sin_d;
            out = g_cosTable[ring->tilt] * ring->heights[point];
            high.x = out * cos_d + low.x;
            high.z = out * sin_d + low.z;
            high.y = 5.0f - g_sinTable[ring->tilt] * ring->heights[point];
            if (point > 0)
                prim_draw_strip(self, &count, point - 1, ring->alpha, prev_low, low, high, prev_high,
                                view, (char)(ring->frame % self->motion_count));
            if (++point >= 21)
                point = 20;
            prev_low.x = low.x;
            prev_low.y = 5.0f;
            prev_low.z = low.z;
            prev_high = high;
        }
    }
}

void prim_whirl_disc_process(struct EffectPrim *self)
{
    struct PrimWhirlDisc *disc = self->records.whirl_discs;
    int i;

    self->base.pos = self->master->base.pos;
    for (i = 0; i < 4; i++, disc++) {
        float dist, along;
        int spin = 0;

        if (disc->active != 1)
            continue;
        if (++disc->frame <= 0)
            continue;
        if (disc->frame < 20) {
            disc->alpha += 5;
            if ((short)disc->alpha > 255)
                disc->alpha = 255;
        }
        dist = float_sqrt(disc->pos.x * disc->pos.x + disc->pos.y * disc->pos.y + disc->pos.z * disc->pos.z);
        if (disc->phase == 0) {
            if (12.0f > dist) {
                disc->phase = 1;
                disc->spins = 0;
                spin = 1;
            }
        } else if (disc->phase == 1) {
            spin = 1;
        }
        if (spin) {
            disc->yaw += 9;
            if (disc->yaw >= 360)
                disc->yaw -= 360;
            if (disc->pitch >= 360)
                disc->pitch -= 360;
            if (++disc->spins >= 10)
                disc->phase = 2;
            disc->alpha += 0xfffe;
            disc->size = disc->size * 0.99f;
            if ((short)disc->alpha < 0)
                disc->alpha = 0;
        }
        if (disc->phase == 2) {
            disc->yaw -= 3;
            if (disc->yaw < 0)
                disc->yaw += 360;
            if (disc->yaw >= 360)
                disc->yaw -= 360;
            disc->pitch += 1;
            if (disc->pitch < 0)
                disc->pitch += 360;
            if (disc->pitch >= 360)
                disc->pitch -= 360;
            if (disc->frame > 300) {
                disc->alpha += 0xffff;
                disc->size = disc->size * 0.99f;
                if ((short)disc->alpha < 0)
                    disc->alpha = 0;
            }
        }
        along = g_cosTable[disc->yaw] * disc->speed;
        disc->pos.x = disc->pos.x - g_cosTable[disc->pitch] * along;
        disc->pos.y = disc->pos.y - (g_sinTable[disc->pitch] * along + 0.0f);
        disc->pos.z = disc->pos.z - g_sinTable[disc->yaw] * disc->speed;
    }
}

static Vec3 disc_point(const struct PrimWhirlDisc *disc, int deg, float pitch_cos, float pitch_sin)
{
    float along = g_cosTable[deg] * disc->size;
    Vec3 p;

    p.x = pitch_cos * along + disc->pos.x;
    p.y = pitch_sin * along + disc->lift + disc->pos.y;
    p.z = disc->size * g_sinTable[deg] + disc->pos.z;
    return p;
}

static void disc_blade(const struct PrimWhirlDisc *disc, int half, Vec3 p[4])
{
    int yaw = disc->yaw;
    float pitch_cos = g_cosTable[disc->pitch], pitch_sin = g_sinTable[disc->pitch];

    p[0] = disc_point(disc, yaw + (yaw >= half ? -half : 360 - half), pitch_cos, pitch_sin);
    p[1] = disc_point(disc, yaw + (yaw < 360 - half ? half : half - 360), pitch_cos, pitch_sin);
    p[2] = disc_point(disc, yaw + (yaw >= 180 + half ? -180 - half : 180 - half), pitch_cos, pitch_sin);
    p[3] = disc_point(disc, yaw + (yaw < 180 - half ? 180 + half : -180 + half), pitch_cos, pitch_sin);
}

void prim_whirl_disc_render(struct EffectPrim *self, const float *view)
{
    static const unsigned char thin[4][3] = {{0, 250, 250}, {250, 250, 250}, {250, 100, 100}, {200, 200, 255}};
    static const unsigned char wide[4][3] = {{250, 0, 0}, {250, 250, 0}, {255, 166, 210}, {5, 5, 255}};
    struct PrimWhirlDisc *disc = self->records.whirl_discs;
    int count = 0;
    int i;

    for (i = 0; i < 4; i++, disc++) {
        Vec3 p[4];

        if (disc->active != 1 || (short)disc->alpha <= 0)
            continue;
        disc_blade(disc, 2, p);
        if (disc->tint <= 3)
            prim_draw_quad(self, &count, disc->alpha, p[0], p[1], p[2], p[3], view, PRIM_BLEND_ADDITIVE, 0, 0,
                           thin[disc->tint][0], thin[disc->tint][1], thin[disc->tint][2], 0.0f, 1.0f, 0.0f, 1.0f,
                           PRIM_QUAD_PROJECTED);
        disc_blade(disc, 8, p);
        if (disc->tint <= 3)
            prim_draw_quad(self, &count, disc->alpha, p[0], p[1], p[2], p[3], view, PRIM_BLEND_ADDITIVE, 0, 0,
                           wide[disc->tint][0], wide[disc->tint][1], wide[disc->tint][2], 0.0f, 1.0f, 0.0f, 1.0f,
                           PRIM_QUAD_PROJECTED);
    }
}

void prim_pulse_fan_process(struct EffectPrim *self)
{
    struct PrimFanQuad *q = self->records.fan_quads;

    prim_face_camera(self);
    self->base.pos = self->master->base.pos;
    if (q->active != 1)
        return;
    if (++q->frame < 30) {
        if (q->quick == 31) {
            q->alpha += 8;
        } else {
            q->alpha += 3;
            if ((short)q->alpha >= 60)
                q->alpha = 60;
        }
    }
    if ((short)q->alpha >= 255)
        q->alpha = 255;
    else if ((short)q->alpha < 0)
        q->alpha = 0;
    q->spin = q->spin + 1.0f;
    if (!(q->spin < 360.0f))
        q->spin = q->spin - 360.0f;
    q->pos.y = q->base_y;
    q->pos.x = self->base.pos.x;
    q->pos.z = self->base.pos.z;
}

static int back_by(int deg, int step)
{
    deg -= step;
    return deg < 0 ? deg + 360 : deg;
}

static void fan_corners(struct EffectPrim *self, const struct PrimFanQuad *q, float size, Vec3 c[4])
{
    int deg = q->angle;
    float nearest;
    int k;

    c[0] = on_circle(deg, size);
    for (k = 1; k < 4; k++) {
        if (q->shape == 0.0f)
            deg = back_by(deg, 90);
        else if (q->shape == 1.0f)
            deg = back_by(deg, k == 2 ? 120 : 60);
        else if (q->shape == 2.0f)
            deg = back_by(deg, k == 2 ? 50 : 130);
        c[k] = on_circle(deg, size);
    }
    nearest = c[1].z > c[0].z ? c[1].z : c[0].z;
    nearest = c[2].z > nearest ? c[2].z : nearest;
    nearest = c[3].z > nearest ? c[3].z : nearest;
    for (k = 0; k < 4; k++)
        c[k].z = c[k].z - nearest;
    prim_quad_turn(&c[0], &c[1], &c[2], &c[3], self->camera_yaw, self->camera_pitch);
    for (k = 0; k < 4; k++) {
        c[k].x = q->pos.x + c[k].x;
        c[k].y = q->pos.y + c[k].y;
        c[k].z = q->pos.z + c[k].z;
    }
}

static void fan_draw(struct EffectPrim *self, const struct PrimFanQuad *q, const float *view, int *count,
                     const Vec3 c[4], int turned, short blend, unsigned char r, unsigned char g, unsigned char b)
{
    if (turned)
        prim_draw_quad(self, count, q->alpha, c[1], c[0], c[3], c[2], view, blend, q->texture, 1, r, g, b,
                       0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_PROJECTED);
    else
        prim_draw_quad(self, count, q->alpha, c[0], c[1], c[2], c[3], view, blend, q->texture, 1, r, g, b,
                       0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_PROJECTED);
}

void prim_pulse_fan_render(struct EffectPrim *self, const float *view)
{
    struct PrimFanQuad *q = self->records.fan_quads;
    int count = 0;
    int i;

    for (i = 0; i < 4; i++, q++) {
        Vec3 c[4];

        if (q->active != 1 || (short)q->alpha <= 0)
            continue;
        fan_corners(self, q, q->size, c);
        switch (q->fan_style) {
        case 0:
        case 3:
            fan_draw(self, q, view, &count, c, 0, BLEND_ALPHA, 255, 255, 255);
            break;
        case 19:
            fan_draw(self, q, view, &count, c, 1, PRIM_BLEND_ADDITIVE, 205, 255, 205);
            break;
        case 20:
            fan_draw(self, q, view, &count, c, 1, PRIM_BLEND_ADDITIVE, 255, 5, 5);
            fan_draw(self, q, view, &count, c, 1, PRIM_BLEND_ADDITIVE, 255, 155, 155);
            break;
        case 21:
            fan_draw(self, q, view, &count, c, 1, BLEND_ALPHA, 225, 255, 225);
            break;
        }
        fan_corners(self, q, g_sinTable[(int)q->spin] * q->size * 0.05f + q->size, c);
        switch (q->fan_style) {
        case 0:
        case 3:
            fan_draw(self, q, view, &count, c, 0, BLEND_ALPHA, 255, 255, 255);
            break;
        case 19:
            fan_draw(self, q, view, &count, c, 1, PRIM_BLEND_ADDITIVE, 105, 255, 105);
            break;
        case 20:
            fan_draw(self, q, view, &count, c, 1, PRIM_BLEND_ADDITIVE, 255, 5, 5);
            fan_draw(self, q, view, &count, c, 1, PRIM_BLEND_ADDITIVE, 255, 155, 155);
            break;
        }
    }
}

static Vec3 line_point(int deg, float len, float pitch_cos, float pitch_sin, float y)
{
    float along = g_cosTable[deg] * len;
    Vec3 p;

    p.x = along * pitch_cos;
    p.y = along * pitch_sin + y;
    p.z = g_sinTable[deg] * len;
    return p;
}

static void next_texture(struct EffectPrim *self, struct PrimDashLine *line)
{
    line->texture = line->texture + 1.0f;
    if (!(line->texture < (float)self->motion_count))
        line->texture = 0.0f;
}

void prim_blue_wedge_process(struct EffectPrim *self)
{
    struct PrimDashLine *line = self->records.dash_lines;
    int i;

    self->base.pos = self->master->base.pos;
    for (i = 0; i < 4; i++, line++)
        if (line->active == 1)
            line->frame++;
}

void prim_blue_wedge_render(struct EffectPrim *self, const float *view)
{
    struct PrimDashLine *line = self->records.dash_lines;
    int count = 0;
    int i;

    for (i = 0; i < 4; i++, line++) {
        Vec3 p0, p1, foot;
        float pitch_cos, pitch_sin;
        int yaw;

        if (line->active != 1 || (short)line->alpha <= 0)
            continue;
        yaw = line->yaw;
        pitch_cos = g_cosTable[line->pitch];
        pitch_sin = g_sinTable[line->pitch];
        p0 = line_point(yaw + (yaw >= 5 ? -5 : 355), line->reach, pitch_cos, pitch_sin, line->y);
        p1 = line_point(yaw + (yaw < 355 ? 5 : -355), line->reach, pitch_cos, pitch_sin, line->y);
        foot.x = 0.0f;
        foot.y = line->y;
        foot.z = 0.0f;
        prim_draw_quad(self, &count, line->alpha, p0, p1, foot, foot, view, PRIM_BLEND_ADDITIVE, 0, 0, 0, 0, 255,
                       0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_IN_FRONT);
    }
}

void prim_blue_blade_process(struct EffectPrim *self)
{
    struct PrimDashLine *line = self->records.dash_lines;
    int i;

    self->base.pos = self->master->base.pos;
    for (i = 0; i < 4; i++, line++) {
        if (line->active != 1)
            continue;
        if (++line->frame > 0)
            next_texture(self, line);
    }
}

void prim_blue_blade_render(struct EffectPrim *self, const float *view)
{
    struct PrimDashLine *line = self->records.dash_lines;
    int count = 0;
    int i;

    for (i = 0; i < 4; i++, line++) {
        Vec3 p0, p1, p2, p3;
        float pitch_cos, pitch_sin;
        int yaw;

        if (line->active != 1 || (short)line->alpha <= 0)
            continue;
        yaw = line->yaw;
        pitch_cos = g_cosTable[line->pitch];
        pitch_sin = g_sinTable[line->pitch];
        p0 = line_point(yaw + (yaw >= 90 ? -90 : 270), 7.0f, pitch_cos, pitch_sin, line->y);
        p1 = line_point(yaw + (yaw >= 7 ? -7 : 353), line->reach, pitch_cos, pitch_sin, line->y);
        p2 = line_point(yaw + (yaw < 353 ? 7 : -353), line->reach, pitch_cos, pitch_sin, line->y);
        p3 = line_point(yaw + (yaw >= 270 ? -270 : 90), 7.0f, pitch_cos, pitch_sin, line->y);
        prim_draw_quad(self, &count, line->alpha, p0, p1, p2, p3, view, PRIM_BLEND_ADDITIVE,
                       (char)(unsigned char)(int)line->texture, 0, 105, 105, 255, 0.0f, 1.0f, 0.0f, 1.0f,
                       PRIM_QUAD_PROJECTED);
    }
}

static void dash_corners(int yaw, int half, float length, float pitch_cos, float pitch_sin, float y, Vec3 c[4])
{
    int minus = yaw - half, plus = yaw + half;

    c[0] = line_point(plus - 180 < 0 ? plus + 180 : plus - 180, length, pitch_cos, pitch_sin, y);
    c[1] = line_point(minus < 0 ? minus + 360 : minus, length, pitch_cos, pitch_sin, y);
    c[2] = line_point(plus < 360 ? plus : plus - 360, length, pitch_cos, pitch_sin, y);
    c[3] = line_point(minus + (minus + 180 < 360 ? 180 : -180), length, pitch_cos, pitch_sin, y);
}

void prim_dash_line_process(struct EffectPrim *self)
{
    struct PrimDashLine *line = self->records.dash_lines;
    int i;

    self->base.pos = self->master->base.pos;
    for (i = 0; i < 4; i++, line++) {
        if (line->active != 1)
            continue;
        if (++line->frame <= 0)
            continue;
        next_texture(self, line);
        line->reach = line->reach - 1.0f;
        if (0.0f > line->reach)
            line->reach = line->length + line->reach;
    }
}

void prim_dash_line_render(struct EffectPrim *self, const float *view)
{
    struct PrimDashLine *line = self->records.dash_lines;
    int count = 0;
    int i;

    for (i = 0; i < 4; i++, line++) {
        Vec3 c[4];
        float yaw_cos, yaw_sin;
        short n;

        if (line->active != 1 || (short)line->alpha <= 0)
            continue;
        dash_corners(line->yaw, 20, line->length, g_cosTable[line->pitch], g_sinTable[line->pitch], line->y, c);
        yaw_cos = g_cosTable[line->yaw];
        yaw_sin = g_sinTable[line->yaw];
        for (n = 0; n < 6; n++) {
            float dist = (float)n * line->length + line->reach;
            float dx = dist * yaw_cos, dz = dist * yaw_sin;
            Vec3 p[4];
            int k;

            if (dist > 100.0f)
                break;
            if (!(dist > 0.0f))
                continue;
            for (k = 0; k < 4; k++) {
                p[k].x = c[k].x + dx;
                p[k].y = c[k].y + 0.0f;
                p[k].z = c[k].z + dz;
            }
            prim_draw_quad(self, &count, line->alpha, p[0], p[1], p[2], p[3], view, PRIM_BLEND_ADDITIVE,
                           (char)(unsigned char)(int)line->texture, 0, 105, 105, 255, 0.0f, 1.0f, 0.0f, 1.0f,
                           PRIM_QUAD_PROJECTED);
        }
    }
}

void prim_bent_dashes_process(struct EffectPrim *self)
{
    struct PrimDashLine *line = self->records.dash_lines;
    int i;

    self->base.pos = self->master->base.pos;
    for (i = 0; i < 4; i++, line++) {
        if (line->active != 1)
            continue;
        if (++line->frame <= 0)
            continue;
        line->alpha += 2;
        if ((short)line->alpha > 255)
            line->alpha = 255;
        next_texture(self, line);
        line->reach = line->reach - 0.66f;
        line->trail = line->trail - 1.0f;
        if (0.0f > line->reach)
            line->reach = line->length + line->reach;
        if (0.0f > line->trail)
            line->trail = line->length + line->trail;
    }
}

static const float bend_at[6] = {20.0f, 33.0f, 46.0f, 59.0f, 71.0f, 84.0f};

static void bent_dashes(struct EffectPrim *self, const struct PrimDashLine *line, const float *view, int *count,
                        float start, int half, unsigned char rg)
{
    static const float piece[6] = {20.0f, 13.0f, 13.0f, 13.0f, 12.0f, 13.0f};
    float pitch_cos = g_cosTable[line->pitch], pitch_sin = g_sinTable[line->pitch];
    short n;

    for (n = 0; n < 7; n++) {
        float dist = (float)n * line->length + start;
        unsigned short head = (unsigned short)line->yaw;
        Vec3 seg[6], c[4], p[4];
        float rest = dist;
        int deg = line->yaw;
        int k, j;

        for (j = 0; j < 6; j++)
            if (dist > bend_at[j])
                head += 5;
        dash_corners((short)head < 360 ? (short)head : (short)(head - 360), half, line->length, pitch_cos, pitch_sin,
                     line->y, c);
        for (j = 0; j < 6; j++) {
            seg[j].x = 0.0f;
            seg[j].y = 0.0f;
            seg[j].z = 0.0f;
            if (dist > bend_at[j]) {
                if (j > 0)
                    deg = deg + (deg < 355 ? 5 : -355);
                seg[j].x = g_cosTable[deg] * piece[j];
                seg[j].z = g_sinTable[deg] * piece[j];
                rest = dist - bend_at[j];
            }
        }
        if (dist > 20.0f) {
            deg += 5;
            if (deg >= 360)
                deg -= 360;
        }
        for (k = 0; k < 4; k++) {
            p[k].x = seg[0].x + c[k].x;
            p[k].y = seg[0].y + c[k].y;
            p[k].z = seg[0].z + c[k].z;
            for (j = 1; j < 6; j++) {
                p[k].x = p[k].x + seg[j].x;
                p[k].y = p[k].y + seg[j].y;
                p[k].z = p[k].z + seg[j].z;
            }
            p[k].x = p[k].x + g_cosTable[deg] * rest;
            p[k].y = p[k].y + 0.0f;
            p[k].z = p[k].z + g_sinTable[deg] * rest;
        }
        if (dist > 100.0f)
            break;
        if (dist > 0.0f)
            prim_draw_quad(self, count, line->alpha, p[0], p[1], p[2], p[3], view, PRIM_BLEND_ADDITIVE,
                           (char)(unsigned char)(int)line->texture, 0, rg, rg, 255, 0.0f, 1.0f, 0.0f, 1.0f,
                           PRIM_QUAD_PROJECTED);
    }
}

void prim_bent_dashes_render(struct EffectPrim *self, const float *view)
{
    struct PrimDashLine *line = self->records.dash_lines;
    int count = 0;
    int i;

    for (i = 0; i < 4; i++, line++) {
        if (line->active != 1 || (short)line->alpha <= 0)
            continue;
        bent_dashes(self, line, view, &count, line->reach, 15, 65);
        bent_dashes(self, line, view, &count, line->trail, 25, 35);
    }
}
