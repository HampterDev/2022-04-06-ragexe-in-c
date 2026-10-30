#include <stdlib.h>
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"

#define COS_DEG(d) g_cosTable[d]
#define SIN_DEG(d) g_sinTable[d]

#define BLEND_ALPHA 1

static void draw_quad(struct EffectPrim *self, int *count, short alpha, Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3,
                      const float *view, short blend, char texture, unsigned char in_world, unsigned char r,
                      unsigned char g, unsigned char b, unsigned char mode)
{
    prim_draw_quad(self, count, alpha, p0, p1, p2, p3, view, blend, texture, in_world, r, g, b, 0.0f, 1.0f, 0.0f,
                   1.0f, mode);
}

static Vec3 vec3(float x, float y, float z)
{
    Vec3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
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

static int quarter_on(int angle)
{
    return angle + 90 < 360 ? angle + 90 : angle - 270;
}

static void flake_respawn(const struct PrimAuraGroup *flakes, Vec3 *flake)
{
    flake->x = (float)(flakes->narrow == 0 ? rand_mod(45) - 22 : rand_mod(25) - 12);
    flake->y = (float)rand_mod(30);
    flake->z = (float)(rand_mod(11) - 5);
    if (flakes->swap_xz == 1) {
        int x = (int)flake->x;

        flake->x = flake->z;
        flake->z = (float)x;
    }
}

void prim_flakes_process(struct EffectPrim *self)
{
    struct PrimAuraGroup *fl;
    int i;

    face_camera(self);
    fl = self->records.aura_groups;
    for (i = 0; i < 4; i++, fl++) {
        int angle_no;
        float y;

        if (fl->active != 1)
            continue;
        for (angle_no = 0; angle_no < 8; angle_no++) {
            struct PrimSway *sw = &fl->sway[angle_no];
            float cur = sw->angle, target = sw->target;
            int whole_deg;

            if (target > cur) {
                cur = cur + 1.0f;
                if (cur > 359.0f)
                    cur = 359.0f;
                sw->angle = cur;
                if (cur > target) {
                    whole_deg = (int)cur;
                    sw->target = (float)(whole_deg == 0 ? 0 : rand_mod(whole_deg));
                }
            } else {
                cur = cur - 1.0f;
                if (0.0f > cur)
                    cur = 0.0f;
                sw->angle = cur;
                if (!(target < cur)) {
                    whole_deg = (int)cur;
                    sw->target = (float)(359 - (whole_deg == 0 ? 0 : rand_mod(whole_deg)));
                }
            }
        }

        y = fl->point[2].y;
        if (0.0f > y) {
            fl->point[2].x = SIN_DEG((int)fl->sway[0].angle) * 0.01f + fl->point[2].x;
            fl->point[2].z = SIN_DEG((int)fl->sway[1].angle) * 0.01f + fl->point[2].z;
            if (-8.0f > y)
                flake_respawn(fl, &fl->point[2]);
        }
        fl->point[2].y = fl->point[2].y - fl->speed;

        y = fl->point[0].y;
        if (0.0f > y) {
            fl->point[0].x = fl->point[0].x - SIN_DEG((int)fl->sway[2].angle) * 0.01f;
            fl->point[0].z = fl->point[0].z - SIN_DEG((int)fl->sway[3].angle) * 0.01f;
            if (-8.0f > y)
                flake_respawn(fl, &fl->point[0]);
        }
        fl->point[0].y = fl->point[0].y - fl->speed;

        y = fl->point[3].y;
        if (0.0f > y) {
            fl->point[3].x = SIN_DEG((int)fl->sway[4].angle) * 0.01f + fl->point[3].x;
            fl->point[3].z = fl->point[3].z - SIN_DEG((int)fl->sway[5].angle) * 0.01f;
            if (-8.0f > y)
                flake_respawn(fl, &fl->point[3]);
        }
        fl->point[3].y = fl->point[3].y - fl->speed;

        y = fl->point[1].y;
        if (0.0f > y) {
            fl->point[1].x = fl->point[1].x - SIN_DEG((int)fl->sway[6].angle) * 0.01f;
            fl->point[1].z = SIN_DEG((int)fl->sway[7].angle) * 0.01f + fl->point[1].z;
            if (-8.0f > y)
                flake_respawn(fl, &fl->point[1]);
        }
        fl->point[1].y = fl->point[1].y - fl->speed;
    }
}

void prim_spike_render(struct EffectPrim *self, const float *view)
{
    struct PrimSpike *sp = self->records.spikes;
    int count = 0;
    int i;

    for (i = 0; i < 4; i++, sp++) {
        float reach, flat, depth;
        Vec3 tip;

        if (sp->active != 1 || sp->alpha <= 0)
            continue;
        reach = SIN_DEG(sp->swell) * sp->length + sp->length;
        flat = COS_DEG(sp->tilt) * reach;
        depth = sp->depth;
        tip = vec3(COS_DEG(sp->turn) * flat, SIN_DEG(sp->tilt) * reach - depth, SIN_DEG(sp->turn) * flat);
        draw_quad(self, &count, sp->alpha, vec3(0.5f, -depth, 0.0f), tip, tip, vec3(-0.5f, -depth, 0.0f), view,
                  PRIM_BLEND_ADDITIVE, 0, 0, (unsigned char)(int)sp->red, (unsigned char)(int)sp->green,
                  (unsigned char)(int)sp->blue, PRIM_QUAD_PROJECTED);
        depth = sp->depth;
        draw_quad(self, &count, sp->alpha, vec3(0.0f, -depth, 0.5f), tip, tip, vec3(0.0f, -depth, -0.5f), view,
                  PRIM_BLEND_ADDITIVE, 0, 0, (unsigned char)(int)sp->red, (unsigned char)(int)sp->green,
                  (unsigned char)(int)sp->blue, PRIM_QUAD_PROJECTED);
    }
}

void prim_turned_square_render(struct EffectPrim *self, const float *view)
{
    struct PrimTurnedSquare *sq = self->records.turned_squares;
    int count = 0;
    int i;

    for (i = 0; i < 4; i++, sq++) {
        int deg0, deg1, deg2, deg3;
        float radius, x, y, z;
        Vec3 p0, p1, p2, p3;
        char tex;
        unsigned char r, g, b;

        if (sq->active != 1 || sq->alpha <= 0)
            continue;
        radius = sq->radius;
        x = sq->pos.x;
        y = sq->pos.y + 0.0f;
        z = sq->pos.z;
        deg0 = sq->turn + 45;
        if (deg0 >= 360)
            deg0 -= 360;
        deg1 = quarter_on(deg0);
        deg2 = quarter_on(deg1);
        deg3 = quarter_on(deg2);
        p0 = vec3(COS_DEG(deg0) * radius + x, y, SIN_DEG(deg0) * radius + z);
        p1 = vec3(COS_DEG(deg1) * radius + x, y, SIN_DEG(deg1) * radius + z);
        p2 = vec3(COS_DEG(deg2) * radius + x, y, SIN_DEG(deg2) * radius + z);
        p3 = vec3(COS_DEG(deg3) * radius + x, y, SIN_DEG(deg3) * radius + z);
        if (sq->style == 1) {
            tex = sq->age > 0 ? (char)(sq->age / 6 % 4) : 0;
            r = 205, g = 205, b = 255;
        } else if (sq->style == 2) {
            tex = 0;
            r = 0, g = 255, b = 0;
        } else {
            tex = 0;
            r = 200, g = 200, b = 255;
        }
        draw_quad(self, &count, sq->alpha, p0, p1, p2, p3, view, BLEND_ALPHA, tex, 1, r, g, b,
                  PRIM_QUAD_PROJECTED);
    }
}

static void fan_wedge(const struct PrimFanWedge *fw, int lo, int hi, Vec3 p[4])
{
    int heading = fw->heading, lo_deg, hi_deg;
    float length = fw->length, height = fw->height, tilt_cos = COS_DEG(fw->turn), tilt_sin = SIN_DEG(fw->turn), reach;

    lo_deg = heading + lo;
    if (lo_deg < 0)
        lo_deg += 360;
    reach = COS_DEG(lo_deg) * length;
    p[0] = vec3(tilt_cos * reach, tilt_sin * reach + height, SIN_DEG(lo_deg) * length);
    hi_deg = heading + hi;
    if (hi_deg >= 360)
        hi_deg -= 360;
    reach = COS_DEG(hi_deg) * length;
    p[1] = vec3(reach * tilt_cos, reach * tilt_sin + height, SIN_DEG(hi_deg) * length);
    p[2] = vec3(0.0f, height, 0.0f);
    p[3] = vec3(0.0f, height, 0.0f);
}

void prim_fan_wedge_render(struct EffectPrim *self, const float *view)
{
    struct PrimFanWedge *fw = self->records.fan_wedges;
    int count = 0;
    int i;

    for (i = 0; i < 4; i++, fw++) {
        float style;
        Vec3 p[4];
        unsigned char r, g, b, mode;
        short blend;

        if (fw->active != 1 || fw->alpha <= 0)
            continue;

        style = fw->style;
        if (style == 2.0f)
            fan_wedge(fw, -1, 1, p);
        else
            fan_wedge(fw, -2, 2, p);
        mode = PRIM_QUAD_PROJECTED;
        if (style == 2.0f) {
            r = 0, g = 0, b = 250;
        } else if (style == 3.0f) {
            r = 250, g = 250, b = 250;
        } else if (style == 4.0f) {
            r = 0, g = 0, b = 250;
        } else if (style == 5.0f) {
            r = 50, g = 50, b = 50;
        } else if (style == 6.0f) {
            mode = PRIM_QUAD_IN_FRONT;
            r = 0, g = 0, b = 255;
        } else {
            r = 0, g = 250, b = 250;
        }
        draw_quad(self, &count, fw->alpha, p[0], p[1], p[2], p[3], view, PRIM_BLEND_ADDITIVE, 0, 0, r, g, b, mode);

        style = fw->style;
        if (style == 2.0f)
            fan_wedge(fw, -2, 2, p);
        else
            fan_wedge(fw, -5, 5, p);
        mode = PRIM_QUAD_PROJECTED;
        if (style == 2.0f) {
            r = 250, g = 250, b = 0;
            blend = PRIM_BLEND_ADDITIVE;
        } else if (style == 3.0f) {
            r = 250, g = 250, b = 250;
            blend = BLEND_ALPHA;
        } else if (style == 4.0f) {
            r = 250, g = 250, b = 0;
            blend = BLEND_ALPHA;
        } else if (style == 5.0f) {
            r = 250, g = 250, b = 250;
            blend = BLEND_ALPHA;
        } else if (style == 6.0f) {
            mode = PRIM_QUAD_IN_FRONT;
            r = 50, g = 50, b = 255;
            blend = PRIM_BLEND_ADDITIVE;
        } else {
            r = 250, g = 0, b = 0;
            blend = PRIM_BLEND_ADDITIVE;
        }
        draw_quad(self, &count, fw->alpha, p[0], p[1], p[2], p[3], view, blend, 0, 0, r, g, b, mode);
    }
}

void prim_spike_process(struct EffectPrim *self)
{
    struct PrimSpike *sp = self->records.spikes;
    int i;

    follow_master(self);
    for (i = 0; i < 4; i++, sp++) {
        if ((signed char)sp->active <= 0)
            continue;
        sp->age++;
        if (10.0f > sp->length)
            sp->length = sp->length * 1.02f;
        if (sp->age > 500 && sp->alpha > 0)
            sp->alpha--;
        sp->swell = (short)(int)((float)sp->swell + sp->swell_speed);
        if (sp->swell >= 360)
            sp->swell -= 360;
    }
}

static void turned_square_speed(struct PrimTurnedSquare *sq, int age_before)
{
    if (age_before < 10) {
        sq->alpha += 10;
        sq->speed = sq->speed * 1.02f;
    } else {
        sq->alpha -= 5;
        sq->speed = sq->speed * 1.04f;
        if (sq->alpha < 0) {
            sq->alpha = 0;
            sq->active = 0;
        }
    }
}

static void turned_square_fly(struct PrimTurnedSquare *sq)
{
    sq->pos.x = COS_DEG(sq->turn) * sq->speed + sq->pos.x;
    sq->pos.z = SIN_DEG(sq->turn) * sq->speed + sq->pos.z;
}

void prim_turned_square_process(struct EffectPrim *self)
{
    struct PrimTurnedSquare *sq = self->records.turned_squares;
    int i;

    for (i = 0; i < 4; i++, sq++) {
        int age_before;

        if (sq->active != 1)
            continue;
        age_before = sq->age++;
        if (sq->age <= 0)
            continue;
        if (age_before == 0) {
            prim_heading_degrees(self->delta.x - self->base.pos.x, self->delta.z - self->base.pos.z, &sq->turn);
            if (sq->style == 1)
                sq->turn = (short)(sq->turn + (short)(rand_mod(11) - 5));
            else
                sq->turn = (short)(sq->turn + (short)(rand_mod(31) - 15));
            if (sq->turn >= 360)
                sq->turn -= 360;
            if (sq->turn < 0)
                sq->turn += 360;
        } else {
            turned_square_speed(sq, age_before);
        }
        turned_square_fly(sq);
    }
}

void prim_flying_square_process(struct EffectPrim *self)
{
    struct PrimTurnedSquare *sq = self->records.turned_squares;
    int i;

    for (i = 0; i < 4; i++, sq++) {
        int age_before;

        if (sq->active != 1)
            continue;
        age_before = sq->age++;
        if (sq->age <= 0)
            continue;
        if (age_before != 0)
            turned_square_speed(sq, age_before);
        turned_square_fly(sq);
    }
}

void prim_fan_wedge_process(struct EffectPrim *self)
{
    struct PrimFanWedge *fw = self->records.fan_wedges;
    int i;

    follow_master(self);
    for (i = 0; i < 4; i++, fw++) {
        int age;

        if (fw->active != 1)
            continue;
        age = ++fw->age;
        if (age <= 0)
            continue;
        if (fw->style == 2.0f || fw->style == 6.0f) {
            if (age <= 10) {
                fw->alpha += 10;
            } else if (age > 11) {
                fw->alpha -= 3;
                if (fw->alpha < 0)
                    fw->alpha = 0;
            }
            fw->length = fw->length + 3.0f;
        } else if (fw->style == 3.0f) {
            if (age <= 10) {
                fw->alpha += 6;
            } else if (age > 11) {
                fw->alpha -= 1;
                if (fw->alpha < 0)
                    fw->alpha = 0;
            }
            fw->length = fw->length + 3.0f;
        } else {
            if (age <= 10) {
                fw->alpha += 20;
            } else if (age > 12) {
                fw->alpha -= 15;
                if (fw->alpha < 0)
                    fw->alpha = 0;
            }
            fw->length = fw->length * 1.15f;
        }
    }
}
