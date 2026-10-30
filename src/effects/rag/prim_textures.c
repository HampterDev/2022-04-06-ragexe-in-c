#include "effects/rag/rag_effect.h"
#include "render/render_api.h"

static void keep_camera_angles(struct EffectPrim *self)
{
    short pitch;

    self->camera_yaw = (short)(360 - (int)game_camera_heading());
    pitch = (short)(450 - (int)game_camera_pitch());
    self->camera_pitch = pitch;
    if (pitch >= 360)
        self->camera_pitch = (short)(pitch - 360);
}

static void spin_down(struct PrimTexRecord *rec, int age)
{
    short spin;

    if ((age & 1) == 0)
        rec->spin--;
    spin = rec->spin;
    if (spin < 0)
        rec->spin = (short)(spin + 360);
}

static void spin_up(struct PrimTexRecord *rec, int age)
{
    short spin;

    if ((age & 1) == 0)
        rec->spin++;
    spin = rec->spin;
    if (spin >= 360)
        rec->spin = (short)(spin - 360);
}

void prim_texture_square_process(struct EffectPrim *self)
{
    struct PrimTexRecord *rec = &self->records.tex_records[0];
    int age;
    short value;

    keep_camera_angles(self);
    self->base.pos = self->master->base.pos;
    if (rec->active != 1)
        return;
    age = rec->age + 1;
    rec->age = age;
    if (age < 300) {
        signed char style = rec->style;

        if (style == 1 || style == 3) {
            value = rec->alpha;
            if (age < 200) {
                value = (short)(value + 10);
                rec->alpha = value;
                if (value >= 220)
                    rec->alpha = 220;
            } else {
                value = (short)(value - 5);
                rec->alpha = value;
                if (value < 0)
                    rec->alpha = 0;
            }
        } else if (style == 22) {
            rec->alpha += 10;
            if (rec->alpha >= 220)
                rec->alpha = 220;
        } else if (style == 0) {
            rec->alpha++;
        } else if (style == 20) {
            rec->alpha += 5;
            if (rec->alpha >= 200)
                rec->alpha = 200;
        } else if ((unsigned char)(style - 16) <= 2) {
            rec->alpha = 100;
        } else if ((unsigned char)(style - 10) <= 2) {
            rec->alpha = 150;
        } else if (style == 14) {
            rec->alpha = 50;
        } else if (style >= 10) {
            rec->alpha = 250;
        }
    }
    if (rec->style == 2 && age > 40) {
        value = (short)((age >= 62 ? -8 : 8) + rec->alpha);
        rec->alpha = value;
        if (value > 0)
            rec->size = rec->size + 0.7f;
    }

    {
        signed char style = rec->style;

        if (style == 16 || style == 17) {
            spin_down(rec, age);
        } else if (style >= 20) {
        } else if (style == 18) {
            value = rec->spin;
            if (value < 0)
                rec->spin = (short)(value + 360);
        } else if (style == 10 || style >= 13) {
            spin_up(rec, age);
        } else if (style == 11 || style == 12) {
            spin_down(rec, age);
        } else if (age > 300 && rec->alpha > 0) {
            rec->alpha--;
        }
    }

    value = rec->alpha;
    if (value >= 255)
        rec->alpha = 255;
    else if (value < 0)
        rec->alpha = 0;

    {
        float pulse = rec->pulse;

        if (rec->style < 30) {
            pulse = pulse + 2.0f;
            rec->pulse = pulse;
        }
        if (pulse >= 360.0f)
            rec->pulse = pulse - 360.0f;
    }
    rec->point[0].y = self->base.pos.y - rec->height;
    rec->point[0].x = self->base.pos.x;
    rec->point[0].z = self->base.pos.z;
}

void prim_texture_frames_process(struct EffectPrim *self)
{
    int i;

    keep_camera_angles(self);
    for (i = 0; i < 4; i++) {
        struct PrimTexRecord *rec = &self->records.tex_records[i];
        short alpha;

        if (rec->active != 1)
            continue;
        rec->age++;
        if (rec->age % 3 == 0) {
            rec->texture++;
            if (rec->texture >= self->motion_count) {
                rec->texture = (signed char)(self->motion_count - 1);
                rec->alpha += -15;
            }
        }
        alpha = rec->alpha;
        if (alpha >= 255)
            rec->alpha = 255;
        else if (alpha < 0)
            rec->alpha = 0;
    }
}

static void bob_fade(struct EffectPrim *self, struct PrimTexRecord *rec)
{
    int age = rec->age + 1;
    short alpha;

    rec->bob += 3;
    rec->age = age;
    if (rec->bob >= 360)
        rec->bob = (short)(rec->bob - 360);
    if (age > self->duration - 25) {
        alpha = (short)(rec->alpha - 8);
        rec->alpha = alpha;
        if (alpha < 0)
            rec->alpha = 0;
    }
}

unsigned char prim_bob_fade_process(struct EffectPrim *self)
{
    self->base.pos = self->master->base.pos;
    bob_fade(self, &self->records.tex_records[0]);
    bob_fade(self, &self->records.tex_records[1]);
    return 1;
}

static int next_quarter(int deg)
{
    return deg + 90 < 360 ? deg + 90 : deg - 270;
}

static Vec3 ground_corner(int deg, double radius, float y)
{
    Vec3 corner;

    corner.x = (float)((double)g_cosTable[deg] * radius);
    corner.y = y;
    corner.z = (float)((double)g_sinTable[deg] * radius);
    return corner;
}

void prim_ground_squares_render(struct EffectPrim *self, const float *view)
{
    int count = 0;
    int slot_deg;

    for (slot_deg = 0; slot_deg < 92; slot_deg += 23) {
        struct PrimTexRecord *rec = &self->records.tex_records[slot_deg / 23];
        float scale, size;
        double radius;
        int deg;
        Vec3 *p0 = &rec->point[2], *p1 = &rec->point[0], *p2 = &rec->point[1], *p3 = &rec->point[3];

        if (rec->active != 1)
            continue;
        scale = self->scale;
        if (scale == 100.0f || scale >= 110.0f || (scale == 4.0f && rec->ground_style == 4.0f) || scale == 10.0f || scale == 0.0f)
            deg = rec->angle;
        else
            deg = slot_deg;
        size = rec->size;
        radius = (double)((g_sinTable[rec->bob] + 1.0f) * (size * 0.1f)) + (double)size * 0.8;
        *p0 = ground_corner(deg, radius, rec->y);
        deg = next_quarter(deg);
        *p1 = ground_corner(deg, radius, rec->y);
        deg = next_quarter(deg);
        *p2 = ground_corner(deg, radius, rec->y);
        deg = next_quarter(deg);
        *p3 = ground_corner(deg, radius, rec->y);

#define GROUND_SQUARE(blend, r_, g_, b_, mode)                                                     \
    prim_draw_quad(self, &count, rec->alpha, *p0, *p1, *p2, *p3, view, (blend), 0, 0, (r_), (g_), (b_), \
                   0.0f, 1.0f, 0.0f, 1.0f, (mode))
        scale = self->scale;
        if (scale == 101.0f) {
            GROUND_SQUARE(PRIM_BLEND_ADDITIVE, 0xff, 0, 0, PRIM_QUAD_PROJECTED);
            GROUND_SQUARE(PRIM_BLEND_ADDITIVE, 0xff, 0, 0, PRIM_QUAD_PROJECTED);
        } else if (scale == 102.0f) {
            GROUND_SQUARE(PRIM_BLEND_ADDITIVE, 0xff, 0xff, 0, PRIM_QUAD_PROJECTED);
        } else if (scale == 111.0f) {
            GROUND_SQUARE(PRIM_BLEND_ADDITIVE, 0x37, 0x37, 0xff, PRIM_QUAD_PROJECTED);
        } else if (scale == 112.0f) {
            GROUND_SQUARE(PRIM_BLEND_ADDITIVE, 0x69, 0xe1, 0x69, PRIM_QUAD_PROJECTED);
        } else if (scale == 100.0f) {
            GROUND_SQUARE(PRIM_BLEND_ADDITIVE, 0xff, 0xff, 0xff, PRIM_QUAD_PROJECTED);
            GROUND_SQUARE(PRIM_BLEND_ADDITIVE, 0xff, 0xff, 0xff, PRIM_QUAD_PROJECTED);
        } else if (scale == 110.0f) {
            GROUND_SQUARE(1, 0xff, 0xff, 0xff, PRIM_QUAD_PROJECTED);
        } else if (scale == 113.0f) {
            GROUND_SQUARE(PRIM_BLEND_ADDITIVE, 5, 5, 0xff, PRIM_QUAD_IN_FRONT);
        } else if (scale == 114.0f) {
            GROUND_SQUARE(PRIM_BLEND_ADDITIVE, 0x9b, 5, 5, PRIM_QUAD_IN_FRONT);
        } else if (scale == 15.0f) {
            GROUND_SQUARE(PRIM_BLEND_ADDITIVE, 0x23, 0xff, 0x23, PRIM_QUAD_PROJECTED);
        } else if (scale == 17.0f) {
            GROUND_SQUARE(PRIM_BLEND_ADDITIVE, 0x69, 0x69, 0xff, PRIM_QUAD_PROJECTED);
        } else if (scale == 18.0f) {
            GROUND_SQUARE(PRIM_BLEND_ADDITIVE, 0x23, 0x23, 0xff, PRIM_QUAD_PROJECTED);
        } else if (scale == 0.0f) {
            GROUND_SQUARE(1, 0xff, 0xff, 0xff, PRIM_QUAD_PROJECTED);
        } else {
            GROUND_SQUARE(PRIM_BLEND_ADDITIVE, 0xff, 0xff, 0xff, PRIM_QUAD_PROJECTED);
        }
#undef GROUND_SQUARE
    }
}

static int back_by(int deg, int step)
{
    deg -= step;
    if (deg < 0)
        deg += 360;
    return deg;
}

void prim_texture_quads_render(struct EffectPrim *self, const float *view)
{
    Vec3 a = {0.0f, 0.0f, 0.0f}, b = {0.0f, 0.0f, 0.0f}, c = {0.0f, 0.0f, 0.0f}, d = {0.0f, 0.0f, 0.0f};
    int count = 0;
    int slot;

    for (slot = 0; slot < 4; slot++) {
        struct PrimTexRecord *rec = &self->records.tex_records[slot];
        const Vec3 *at = &rec->point[0];
        float radius, shape;
        int spin, deg;
        signed char look;

        if (rec->active != 1)
            continue;
        if (rec->alpha <= 0)
            continue;
        spin = rec->spin;
        radius = g_sinTable[(int)rec->pulse] * rec->size * 0.05f + rec->size;
        a.y = 0.0f;
        a.x = g_cosTable[spin] * radius;
        a.z = g_sinTable[spin] * radius;
        shape = rec->shape;
        deg = spin;
        if (shape == 0.0f)
            deg = back_by(spin, 90);
        else if (shape == 1.0f)
            deg = back_by(spin, 60);
        else if (shape == 2.0f)
            deg = back_by(spin, 130);
        b.y = 0.0f;
        b.x = g_cosTable[deg] * radius;
        b.z = g_sinTable[deg] * radius;
        if (shape == 0.0f)
            deg = back_by(deg, 90);
        else if (shape == 1.0f)
            deg = back_by(deg, 120);
        else if (shape == 2.0f)
            deg = back_by(deg, 50);
        c.y = 0.0f;
        c.x = g_cosTable[deg] * radius;
        c.z = g_sinTable[deg] * radius;
        if (shape == 0.0f)
            deg = back_by(deg, 90);
        else if (shape == 1.0f)
            deg = back_by(deg, 60);
        else if (shape == 2.0f)
            deg = back_by(deg, 130);
        d.y = 0.0f;
        d.x = g_cosTable[deg] * radius;
        d.z = g_sinTable[deg] * radius;

        look = rec->look;
        if (look == 3) {
            a.x = a.x + 3.1f;
            b.x = b.x + 3.1f;
            d.x = d.x + 3.1f;
            c.x = c.x + 3.1f;
        }
        if (look == 3 || (unsigned char)(look - 17) <= 14) {
            d.z = d.z + 6.0f;
            c.z = c.z + 6.0f;
            b.z = b.z + 6.0f;
            a.z = a.z + 6.0f;
        }
        prim_quad_turn(&a, &b, &c, &d, self->camera_yaw, self->camera_pitch);
        a.x = at->x + a.x;
        a.y = at->y + a.y;
        a.z = at->z + a.z;
        b.x = at->x + b.x;
        b.y = at->y + b.y;
        b.z = at->z + b.z;
        c.x = at->x + c.x;
        c.y = at->y + c.y;
        c.z = at->z + c.z;
        d.x = at->x + d.x;
        d.y = at->y + d.y;
        d.z = at->z + d.z;

#define TEXTURE_QUAD(p0, p1, p2, p3, blend, r_, g_, b_, mode)                                         \
    prim_draw_quad(self, &count, rec->alpha, p0, p1, p2, p3, view, (blend), rec->texture, 1, (r_), (g_), \
                   (b_), 0.0f, 1.0f, 0.0f, 1.0f, (mode))
        switch (look) {
        case 0:
        case 3:
            TEXTURE_QUAD(a, b, c, d, 1, 0xff, 0xff, 0xff, PRIM_QUAD_PROJECTED);
            break;
        case 2:
            TEXTURE_QUAD(b, a, d, c, PRIM_BLEND_ADDITIVE, 0x82, 0x82, 0xfa, PRIM_QUAD_PROJECTED);
        case 4:
            TEXTURE_QUAD(b, a, d, c, PRIM_BLEND_ADDITIVE, 0xff, 0xff, 0xff, PRIM_QUAD_PROJECTED);
        case 5:
            TEXTURE_QUAD(b, a, d, c, PRIM_BLEND_ADDITIVE, 0x7d, 0xff, 0x7d, PRIM_QUAD_PROJECTED);
        case 10:
            TEXTURE_QUAD(b, a, d, c, PRIM_BLEND_ADDITIVE, 0x96, 0x96, 0x96, PRIM_QUAD_PROJECTED);
        case 12:
            TEXTURE_QUAD(b, a, d, c, PRIM_BLEND_ADDITIVE, 0x32, 0x32, 0x32, PRIM_QUAD_PROJECTED);
        case 15:
            TEXTURE_QUAD(b, a, d, c, PRIM_BLEND_ADDITIVE, 0x33, 0x99, 0x99, PRIM_QUAD_PROJECTED);
            break;
        case 16:
            TEXTURE_QUAD(b, a, d, c, 1, 0xff, 0xff, 0xff, PRIM_QUAD_UPRIGHT);
            break;
        case 17:
            TEXTURE_QUAD(b, a, d, c, PRIM_BLEND_ADDITIVE, 0xff, 0xff, 0xff, PRIM_QUAD_PROJECTED);
            break;
        case 18:
            TEXTURE_QUAD(b, a, d, c, PRIM_BLEND_ADDITIVE, 5, 5, 0xff, PRIM_QUAD_PROJECTED);
            TEXTURE_QUAD(b, a, d, c, PRIM_BLEND_ADDITIVE, 0x9b, 0x9b, 0xff, PRIM_QUAD_PROJECTED);
            break;
        case 19:
            TEXTURE_QUAD(b, a, d, c, PRIM_BLEND_ADDITIVE, 5, 0xff, 5, PRIM_QUAD_PROJECTED);
            TEXTURE_QUAD(b, a, d, c, PRIM_BLEND_ADDITIVE, 0x9b, 0xff, 0x9b, PRIM_QUAD_PROJECTED);
            break;
        case 20:
            TEXTURE_QUAD(b, a, d, c, PRIM_BLEND_ADDITIVE, 0xff, 5, 5, PRIM_QUAD_PROJECTED);
            TEXTURE_QUAD(b, a, d, c, PRIM_BLEND_ADDITIVE, 0xff, 0x9b, 0x9b, PRIM_QUAD_PROJECTED);
            break;
        case 32:
            TEXTURE_QUAD(b, a, d, c, PRIM_BLEND_ADDITIVE, 0xff, 5, 5, PRIM_QUAD_PROJECTED);
            break;
        default:
            break;
        }
#undef TEXTURE_QUAD
    }
}

void prim_ground_turn_process(struct EffectPrim *self)
{
    struct PrimTexRecord *rec = &self->records.tex_records[0];

    self->base.pos = self->master->base.pos;
    if (rec->ground_style == 14.0f) {
        rec->angle--;
        if (rec->angle < 0)
            rec->angle += 360;
    } else {
        rec->angle++;
        if (rec->angle >= 360)
            rec->angle -= 360;
    }
    prim_step_motion_counted(self);
}

void prim_ground_bob_process(struct EffectPrim *self)
{
    struct PrimTexRecord *recs = self->records.tex_records;

    self->base.pos = self->master->base.pos;
    recs[0].bob += 3;
    if (recs[0].bob >= 360)
        recs[0].bob -= 360;
    recs[1].bob += 3;
    if (recs[1].bob >= 360)
        recs[1].bob -= 360;
    prim_step_motion_counted(self);
}

void prim_ground_styled_process(struct EffectPrim *self)
{
    int i;

    self->base.pos = self->master->base.pos;
    for (i = 0; i < 2; i++) {
        struct PrimTexRecord *rec = &self->records.tex_records[i];
        float style;

        if (rec->active != 1)
            continue;
        style = rec->ground_style;
        if (style == 4.0f) {
            rec->angle += 6;
            if (rec->angle >= 360)
                rec->angle -= 360;
        } else if (style == 10.0f || style == 11.0f || style == 12.0f) {
            rec->angle += 3;
            if (rec->angle >= 360)
                rec->angle -= 360;
        }

        if (style == 2.0f) {
            rec->bob++;
        } else if (style == 10.0f) {
            if (rec->angle > 180)
                rec->size = rec->size + 0.3f;
            else
                rec->size = rec->size - 0.3f;
        } else if (style != 11.0f && style != 12.0f) {
            rec->bob += 3;
        }
        if (rec->bob >= 360)
            rec->bob -= 360;
    }
}

void prim_ground_fade_process(struct EffectPrim *self)
{
    struct PrimTexRecord *rec = &self->records.tex_records[0];

    self->base.pos = self->master->base.pos;
    rec->age++;
    if (rec->age <= 0)
        return;
    if (rec->age % 2 >= 1)
        rec->alpha--;
    if (rec->alpha < 0)
        rec->alpha = 0;
}

void prim_ground_spread_process(struct EffectPrim *self)
{
    int i;

    self->base.pos = self->master->base.pos;
    for (i = 0; i < 4; i++) {
        struct PrimTexRecord *rec = &self->records.tex_records[i];

        if ((signed char)rec->active <= 0)
            continue;
        rec->age++;
        if (rec->age <= 0)
            continue;
        if (rec->age <= 10)
            rec->alpha = (short)(rec->alpha + (self->scale == 113.0f ? 6 : 2));
        rec->size = rec->size * 1.07f;
        if (rec->age > 10) {
            rec->alpha--;
            if (rec->alpha < 0)
                rec->alpha = 0;
        }
    }
}

void prim_texture_fade_process(struct EffectPrim *self)
{
    struct PrimTexRecord *rec = &self->records.tex_records[0];

    keep_camera_angles(self);
    if (rec->active != 1)
        return;
    rec->age++;
    if (rec->pulse >= 360.0f)
        rec->pulse = rec->pulse - 360.0f;
    if (rec->age <= 30) {
        rec->alpha += 10;
        if (rec->alpha > 255)
            rec->alpha = 255;
    } else {
        rec->alpha -= 3;
        if (rec->alpha < 0)
            rec->alpha = 0;
    }
    rec->point[0].y = rec->point[2].y;
}

void prim_texture_cycle_process(struct EffectPrim *self)
{
    struct PrimTexRecord *rec = &self->records.tex_records[0];

    keep_camera_angles(self);
    self->base.pos = self->master->base.pos;
    if (rec->active != 1)
        return;
    rec->age++;
    if (rec->age % (rec->style == 0 ? 8 : 6) == 0) {
        rec->texture++;
        if (rec->texture >= self->motion_count)
            rec->texture = 0;
    }
    if (rec->alpha >= 255)
        rec->alpha = 255;
    else if (rec->alpha < 0)
        rec->alpha = 0;
    rec->point[0].y = rec->point[2].y;
    rec->point[0].x = self->base.pos.x;
    rec->point[0].z = self->base.pos.z;
}
