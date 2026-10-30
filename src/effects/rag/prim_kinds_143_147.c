#include "session/state.h"
#include <stdlib.h>
#include "effects/rag/rag_effect.h"

#define BLEND_ALPHA 1

void prim_wave_ring_process(struct EffectPrim *self)
{
    struct PrimWaveRing *ring = self->records.wave_rings;
    int i, j;

    self->base.pos = self->master->base.pos;
    for (i = 0; i < 4; i++, ring++) {
        unsigned from;

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
        if (ring->radius >= 4.0f) {
            ring->alpha = (unsigned short)(from - 3);
            if ((short)ring->alpha <= 0) {
                ring->alpha = 0;
                if (ring->frame < self->duration - 30) {
                    ring->radius = 3.37f;
                    ring->tilt = 74;
                }
            }
        } else {
            ring->alpha = (unsigned short)(from + 10);
        }
        for (j = 0; j < WAVE_RING_POINTS; j++) {
            int phase = i * 45 < 90 ? ring->frame + i * 90 : (ring->frame + i * 45) * 2;
            float base = ring->base_height;
            ring->heights[j] = g_sinTable[j * 9] * base * 0.3f * g_sinTable[phase % 360] + base;
        }
    }
}

void prim_wave_ring_render(struct EffectPrim *self, const float *view)
{
    struct PrimWaveRing *ring = self->records.wave_rings;
    Vec3 low = {0.0f, 0.0f, 0.0f}, high = {0.0f, 0.0f, 0.0f};
    Vec3 prev_low = {0.0f, 0.0f, 0.0f}, prev_high = {0.0f, 0.0f, 0.0f};
    int count = 0;
    int i;

    for (i = 0; i < 4; i++, ring++) {
        int step, along, point;

        if (ring->active != 1)
            continue;
        step = ring->arc / 20;
        point = 0;
        if (ring->arc < 0)
            continue;
        for (along = 0; along <= ring->arc; along += step) {
            int deg = (along + ring->arc_start) % 360;
            float cos_d, height, out, up;

            if ((unsigned short)ring->arc == 360 && point == 20)
                deg = ring->arc_start;
            cos_d = g_cosTable[deg];
            low.z = ring->radius * g_sinTable[deg];
            low.x = ring->radius * cos_d;
            low.y = ring->base_height == 6.001f ? -2.0f : ring->base_height == 24.001f ? -1.0f : 0.0f;

            height = ring->heights[point];
            out = g_cosTable[ring->tilt] * height;
            up = g_sinTable[ring->tilt] * height;
            high.z = out * g_sinTable[deg] + low.z;
            high.x = out * cos_d + low.x;
            if (ring->base_height == 6.001f)
                up = up + 2.0f;
            else if (ring->base_height == 24.001f)
                up = up + 1.0f;
            high.y = -up;

            if (point > 0)
                prim_draw_strip(self, &count, point - 1, (short)ring->alpha, prev_low, low, high,
                                prev_high, view, (char)(ring->frame % self->motion_count));
            if (++point >= 21)
                point = 20;
            prev_low = low;
            prev_high = high;
        }
    }
}

static int quarter_turn(int deg)
{
    return deg < 270 ? deg + 90 : deg - 270;
}

unsigned char prim_spin_square_process(struct EffectPrim *self)
{
    struct PrimSpinSquare *sq = self->records.spin_squares;
    int i;

    self->base.pos = self->master->base.pos;
    for (i = 0; i < 4; i++, sq++) {
        int frame;
        float tex, frames;

        if (sq->active != 1)
            continue;
        frame = ++sq->frame;
        if (sq->still == 0.0f) {
            sq->angle += 6;
            if (sq->angle >= 360)
                sq->angle -= 360;
            sq->pulse += 3;
            if (sq->pulse >= 360)
                sq->pulse -= 360;
        }
        tex = sq->texture;
        if (frame % 4 == 0) {
            tex = tex + 1.0f;
            sq->texture = tex;
        }
        frames = (float)self->motion_count;
        if (tex >= frames)
            sq->texture = tex - frames;
    }
    return 1;
}

void prim_spin_square_corners(struct PrimSpinSquare *sq)
{
    double radius = (double)((g_sinTable[sq->pulse] + 1.0f) * (sq->size * 0.1f)) + (double)sq->size * 0.8;
    int deg0 = sq->angle;
    int deg1 = quarter_turn(deg0);
    int deg2 = quarter_turn(deg1);
    int deg3 = quarter_turn(deg2);

    sq->corners[2].x = (float)((double)g_cosTable[deg0] * radius);
    sq->corners[2].z = (float)((double)g_sinTable[deg0] * radius);
    sq->corners[2].y = sq->y;
    sq->corners[0].x = (float)((double)g_cosTable[deg1] * radius);
    sq->corners[0].z = (float)((double)g_sinTable[deg1] * radius);
    sq->corners[0].y = sq->y;
    sq->corners[1].y = sq->y;
    sq->corners[1].x = (float)((double)g_cosTable[deg2] * radius);
    sq->corners[1].z = (float)((double)g_sinTable[deg2] * radius);
    sq->corners[3].x = (float)((double)g_cosTable[deg3] * radius);
    sq->corners[3].z = (float)((double)g_sinTable[deg3] * radius);
    sq->corners[3].y = sq->y;
}

void prim_spin_square_render(struct EffectPrim *self, const float *view)
{
    struct PrimSpinSquare *sq = self->records.spin_squares;
    int count = 0;
    int i;

    for (i = 0; i < 4; i++, sq++) {
        if (sq->active != 1)
            continue;
        prim_spin_square_corners(sq);
        prim_draw_quad(self, &count, (short)sq->alpha, sq->corners[2], sq->corners[0], sq->corners[1],
                       sq->corners[3], view, sq->still == 1.0f ? BLEND_ALPHA : PRIM_BLEND_ADDITIVE,
                       (char)(int)sq->texture, 0, 0xff, 0xff, 0xff, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_PROJECTED);
    }
}

unsigned char prim_rising_bars_process(struct EffectPrim *self)
{
    struct PrimHalo *rec = self->records.halos;
    int i, k;

    prim_face_camera(self);
    if (rec[0].style == 1)
        self->base.pos = self->master->base.pos;

    for (i = 0; i < 4; i++, rec++) {
        int frame;

        if (rec->active != 1)
            continue;
        frame = ++rec->age;
        if (frame <= 0)
            continue;
        if (frame < 20) {
            for (k = 0; k < HALO_LAYERS; k++) {
                float cap = (float)(70 + 20 * k);
                rec->layer_alpha[k] = rec->layer_alpha[k] + 10.0f;
                if (rec->layer_alpha[k] >= cap)
                    rec->layer_alpha[k] = cap;
            }
        } else {
            for (k = 0; k < 9; k++) {
                rec->layer_alpha[k] = rec->layer_alpha[k] - 4.0f;
                if (0.0f > rec->layer_alpha[k])
                    rec->layer_alpha[k] = 0.0f;
            }
            if ((rec->style == 6 || rec->style == 7) && rec->age > 90) {
                float last = rec->layer_alpha[9] - 5.0f;
                rec->layer_alpha[9] = last;
                if (last > 0.0f) {
                    rec->size = rec->size * 1.01f;
                    last = rec->layer_alpha[9];
                }
                if (0.0f > last)
                    rec->layer_alpha[9] = 0.0f;
            }
        }
        if (rec->age < 50)
            rec->size = rec->size - 0.2f;
    }
    return 1;
}

void prim_drift_angles(float (*pairs)[2], int count)
{
    int k;

    for (k = 0; k < count; k++) {
        float cur = pairs[k][0], target = pairs[k][1];

        if (target > cur) {
            cur = cur + 1.0f;
            if (cur > 359.0f)
                cur = 359.0f;
            pairs[k][0] = cur;
            if (cur > target)
                pairs[k][1] = (float)prim_rand_below((int)cur);
        } else {
            cur = cur - 1.0f;
            if (0.0f > cur)
                cur = 0.0f;
            pairs[k][0] = cur;
            if (target >= cur)
                pairs[k][1] = (float)(359 - prim_rand_below((int)cur));
        }
    }
}

static void drift_orb_circle(struct PrimDriftOrb *orb, float radius)
{
    orb->offset.x = g_sinTable[(int)orb->spin] * radius + orb->offset.x;
    orb->offset.z = g_cosTable[(int)orb->spin] * radius + orb->offset.z;
}

static void drift_orb_offset(struct PrimDriftOrb *orb, signed char style)
{
    float y;

    if (style == 0) {
        orb->offset.x = g_sinTable[(int)orb->drift[0][0]];
        orb->offset.z = g_sinTable[(int)orb->drift[1][0]];
        y = g_sinTable[(int)orb->drift[2][0]];
    } else {
        orb->offset.x = g_sinTable[(int)orb->drift[0][0]] * 0.5f;
        orb->offset.z = g_sinTable[(int)orb->drift[1][0]] * 0.5f;
        y = g_sinTable[(int)orb->drift[2][0]] * 0.5f;
    }
    orb->offset.y = y;
    drift_orb_circle(orb, 7.0f);
}

static void drift_orb_spin(struct PrimDriftOrb *orb)
{
    float spin = orb->spin + 1.0f;

    orb->spin = spin;
    if (spin >= 360.0f)
        orb->spin = spin - 360.0f;
}

static void drift_orb_shimmer(struct PrimDriftOrb *orb)
{
    int roll = rand() % 5;
    float shimmer = (float)((roll < 0 ? 0 : roll) + 5) + orb->shimmer;

    orb->shimmer = shimmer;
    if (shimmer >= 360.0f)
        orb->shimmer = shimmer - 360.0f;
}

unsigned char prim_drift_orb_process(struct EffectPrim *self)
{
    struct PrimDriftOrb *orb = self->records.drift_orbs;
    signed char style;

    prim_face_camera(self);
    self->base.pos = self->master->base.pos;
    if (orb->active != 1)
        return 1;

    if (orb->frame++ < 100) {
        orb->alpha += 2;
        if (orb->alpha > 255)
            orb->alpha = 255;
    }
    prim_drift_angles(orb->drift, DRIFT_ORB_AXES);

    style = orb->style;
    if (style == 0 || (style < 10 ? (orb->frame & 1) == 0 : orb->frame % (style - 9) == 0))
        drift_orb_spin(orb);

    if (orb->wide == 1.0f) {
        orb->offset.x = g_sinTable[(int)orb->drift[0][0]] * 0.7f;
        orb->offset.z = g_sinTable[(int)orb->drift[1][0]] * 0.7f;
        orb->offset.y = g_sinTable[(int)orb->drift[2][0]] * 0.7f;
        drift_orb_circle(orb, 5.5f);
    } else {
        drift_orb_offset(orb, style);
    }
    orb->offset.y = orb->lift + orb->offset.y;
    drift_orb_shimmer(orb);
    return 1;
}

static void square_corners(Vec3 corner[4], int deg, float size)
{
    int i;

    deg %= 360;
    for (i = 0; i < 4; i++) {
        corner[i].x = g_cosTable[deg] * size;
        corner[i].y = 0.0f;
        corner[i].z = g_sinTable[deg] * size;
        deg = (deg + 90) % 360;
    }
}

static void drift_orb_colour(signed char style, int halo, unsigned char *r, unsigned char *g, unsigned char *b)
{
    switch (style) {
    case 10:
        if (halo) {
            *r = 0xff, *g = 0x14, *b = 0x14;
        } else {
            *r = 0xff, *g = 0xff, *b = 0xff;
        }
        break;
    case 11: *r = 0xc8, *g = 0xc8, *b = 0x32; break;
    case 12: *r = 0x32, *g = 0x32, *b = 0xff; break;
    case 13: *r = 0x64, *g = 0x96, *b = 0x64; break;
    default:
        if (halo) {
            *r = 0xff, *g = 0xff, *b = 0;
        } else {
            *r = 0, *g = 0, *b = 0xff;
        }
        break;
    }
}

void prim_drift_orb_render(struct EffectPrim *self, const float *view)
{
    struct PrimDriftOrb *orb = self->records.drift_orbs;
    short pitch = self->camera_pitch;
    int count = 0;
    int halo, i;

    if (orb->active != 1 || orb->alpha <= 0)
        return;
    for (halo = 0; halo < 2; halo++) {
        Vec3 corner[4];
        float size = orb->size;
        float shimmer = g_sinTable[(int)orb->shimmer] * size * 0.05f;
        unsigned char r, g, b;

        if (!halo) {
            square_corners(corner, orb->frame * 2, shimmer + size);
        } else {
            float pulse = g_sinTable[orb->frame * 5 % 90];
            square_corners(corner, orb->frame * 2, shimmer + (pulse + pulse + 1.0f) * size);
        }
        prim_quad_turn(&corner[0], &corner[1], &corner[2], &corner[3], self->camera_yaw, pitch);
        for (i = 0; i < 4; i++) {
            corner[i].x = orb->offset.x + corner[i].x;
            corner[i].y = orb->offset.y + corner[i].y;
            corner[i].z = orb->offset.z + corner[i].z;
        }
        drift_orb_colour(orb->style, halo, &r, &g, &b);
        prim_draw_quad(self, &count, (unsigned short)orb->alpha, corner[0], corner[1], corner[2], corner[3], view,
                       orb->style == 10 && !halo ? BLEND_ALPHA : PRIM_BLEND_ADDITIVE, orb->style == 10 && halo ? 1 : 0,
                       0, r, g, b, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_PROJECTED);
    }
}

unsigned char prim_orb_blades_process(struct EffectPrim *self)
{
    struct PrimDriftOrb *orb = &self->records.drift_orbs[0];
    struct PrimDriftOrb *blade = self->records.drift_orbs;
    int i;

    prim_face_camera(self);
    self->base.pos = self->master->base.pos;
    if (orb->active != 1)
        return 1;

    if (orb->frame++ < 200)
        orb->alpha++;
    prim_drift_angles(orb->drift, DRIFT_ORB_AXES);
    if ((orb->frame & 1) == 0)
        drift_orb_spin(orb);
    drift_orb_offset(orb, orb->style);
    orb->offset.y = orb->lift + orb->offset.y;
    drift_orb_shimmer(orb);

    for (i = 0; i < 4; i++, blade++) {
        short sway;
        float grow, height;

        if (i > 0)
            blade->frame++;
        if (blade->frame <= 0)
            continue;
        sway = (short)(blade->sway + 1);
        blade->sway = sway;
        if (sway >= 360) {
            sway = (short)(sway - 360);
            blade->sway = sway;
        }
        grow = blade->grow * 1.02f;
        blade->grow = grow;
        if (grow > 6.0f) {
            blade->blade_alpha += -3;
            if (blade->blade_alpha <= 0) {
                int roll;

                blade->grow = 3.0f;
                roll = rand() % 360;
                grow = blade->grow;
                blade->angle = (short)(roll < 0 ? 0 : (unsigned short)roll);
                blade->blade_alpha = 200;
            }
        }
        height = g_sinTable[sway] * grow * 0.1f;
        height = blade->grow * 0.9f + height;
        blade->height = height;
        if (0.0f > height)
            blade->height = 0.0f;
    }
    return 1;
}

static void orb_square(struct EffectPrim *self, const struct PrimDriftOrb *orb, float size, Vec3 corner[4])
{
    int i;

    square_corners(corner, 0, size);
    prim_quad_turn(&corner[0], &corner[1], &corner[2], &corner[3], self->camera_yaw, self->camera_pitch);
    for (i = 0; i < 4; i++) {
        corner[i].x = orb->offset.x + corner[i].x;
        corner[i].y = orb->offset.y + corner[i].y;
        corner[i].z = orb->offset.z + corner[i].z;
    }
}

void prim_orb_blades_render(struct EffectPrim *self, const float *view)
{
    static const unsigned char blade_shade[4] = {0x69, 0x23, 0x4b, 0x05};
    struct PrimDriftOrb *orb = &self->records.drift_orbs[0];
    struct PrimDriftOrb *blade;
    Vec3 corner[4];
    int count = 0;
    int i;

    if (orb->active != 1 || orb->alpha <= 0)
        return;

    orb_square(self, orb, g_sinTable[(int)orb->shimmer] * orb->size * 0.05f + orb->size, corner);
    prim_draw_quad(self, &count, orb->alpha, corner[0], corner[1], corner[2], corner[3], view, PRIM_BLEND_ADDITIVE,
                   0, 0, 0x44, 0x2a, 0xff, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_PROJECTED);
    orb_square(self, orb, g_sinTable[(int)orb->shimmer] * orb->size * 0.05f + orb->size * 0.5f, corner);
    prim_draw_quad(self, &count, orb->alpha, corner[0], corner[1], corner[2], corner[3], view, PRIM_BLEND_ADDITIVE,
                   0, 0, 0x9b, 0x9b, 0xff, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_PROJECTED);

    blade = self->records.drift_orbs;
    for (i = 0; i < 4; i++, blade++) {
        float half = blade->height * 0.5f;
        float height;
        int deg = blade->angle, back_deg;
        Vec3 top_near, top_far;

        blade->foot_near.x = g_cosTable[deg] * half - g_sinTable[deg] * 0.0f;
        blade->foot_near.z = g_sinTable[deg] * half + g_cosTable[deg] * 0.0f;
        blade->foot_near.y = blade->height * 0.5f;
        back_deg = deg + 180;
        if (deg >= 180)
            back_deg = deg - 180;
        blade->foot_far.x = g_cosTable[back_deg] * half - g_sinTable[back_deg] * 0.0f;
        blade->foot_far.z = g_sinTable[back_deg] * half + g_cosTable[back_deg] * 0.0f;
        blade->foot_far.y = blade->height * 0.5f;

        blade->foot_near.x = orb->offset.x + blade->foot_near.x;
        blade->foot_near.y = orb->offset.y + blade->foot_near.y;
        blade->foot_near.z = orb->offset.z + blade->foot_near.z;
        blade->foot_far.x = orb->offset.x + blade->foot_far.x;
        blade->foot_far.y = orb->offset.y + blade->foot_far.y;
        blade->foot_far.z = orb->offset.z + blade->foot_far.z;

        height = blade->height;
        top_near = blade->foot_near;
        top_near.y = blade->foot_near.y - height;
        top_far = blade->foot_far;
        top_far.y = blade->foot_far.y - height;
        if (height > 0.0f)
            prim_draw_quad(self, &count, blade->blade_alpha, blade->foot_near, blade->foot_far, top_far, top_near, view,
                           PRIM_BLEND_ADDITIVE, 1, 0, blade_shade[i], blade_shade[i], 0xff, 0.0f, 1.0f, 0.0f, 1.0f,
                           PRIM_QUAD_PROJECTED);
    }
}

static const float walking_ring_taper[WAVE_RING_POINTS] = {
    0.4f, 0.5f, 0.6f, 0.70000005f, 0.8f, 0.9f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f, 0.9f, 0.8f, 0.70000005f, 0.6f, 0.5f, 0.4f,
};

unsigned char prim_walking_ring_process(struct EffectPrim *self)
{
    struct PrimWaveRing *ring = self->records.wave_rings;
    int i, j;

    for (i = 0; i < 4; i++, ring++) {
        unsigned short heading;
        short dir, alpha;
        int frame;

        if (ring->active != 1)
            continue;
        frame = ++ring->frame;
        if (frame <= 0)
            continue;

        heading = (unsigned short)ring->arc_start;
        dir = (short)(heading + 345);
        if (dir >= 360)
            dir = (short)(dir - 360);
        ring->walked.x = g_cosTable[dir] + ring->walked.x;
        ring->walked.z = g_sinTable[dir] + ring->walked.z;
        if ((short)heading >= 360)
            ring->arc_start = (short)(heading - 360);

        if (frame < 10) {
            int cap = g_session1.map_rule_flags[0] == 1 ? 125 : 250;
            alpha = (short)(ring->alpha + 25);
            ring->alpha = (unsigned short)alpha;
            if (alpha > cap)
                ring->alpha = (unsigned short)cap;
        } else {
            alpha = (short)(ring->alpha - 10);
            ring->alpha = (unsigned short)alpha;
            if (alpha < 0) {
                ring->tilt += -20;
                ring->radius = ring->radius - 0.4f;
                ring->alpha = 0;
                ring->frame = 0;
                if (ring->tilt <= 40) {
                    ring->active = 0;
                    ring->tilt = 40;
                }
            }
        }

        for (j = 0; j < WAVE_RING_POINTS; j++) {
            float height = ring->base_height * walking_ring_taper[j];
            ring->heights[j] = height;
            if (ring->frame <= 15) {
                height = height * g_sinTable[ring->frame * 6];
                ring->heights[j] = height;
            }
            if (0.0f > height)
                ring->heights[j] = 0.0f;
        }
    }
    return 1;
}

void prim_walking_ring_render(struct EffectPrim *self, const float *view)
{
    struct PrimWaveRing *ring = self->records.wave_rings;
    const struct PrimWaveRing *first = ring;
    Vec3 low = {0.0f, 0.0f, 0.0f}, high = {0.0f, 0.0f, 0.0f};
    Vec3 prev_low = {0.0f, 0.0f, 0.0f}, prev_high = {0.0f, 0.0f, 0.0f};
    int count = 0;
    int i;

    for (i = 0; i < 4; i++, ring++) {
        int step, along, point;
        char texture;

        if (ring->active != 1 || ring->frame <= 0)
            continue;
        texture = (char)(ring->frame % self->motion_count);
        step = ring->arc / 20;
        point = 0;
        if (ring->arc < 0)
            continue;
        low.y = 0.0f;
        for (along = 0; along <= ring->arc; along += step) {
            int deg = (along + ring->arc_start) % 360;
            float cos_d, sin_d, ring_x, ring_z, height, out;

            if ((unsigned short)ring->arc == 360 && point == 20)
                deg = ring->arc_start;
            cos_d = g_cosTable[deg];
            ring_z = ring->radius * g_sinTable[deg];
            height = ring->heights[point];
            out = g_cosTable[ring->tilt] * height;
            high.y = -(g_sinTable[ring->tilt] * height);
            ring_x = ring->radius * cos_d;
            low.x = ring_x - first->walked.x * 0.75f;
            low.z = ring_z - first->walked.z * 0.75f;
            sin_d = g_sinTable[deg];
            high.z = out * sin_d + ring_z - first->walked.z;
            high.x = out * cos_d + ring_x - first->walked.x;
            if (point > 0) {
                self->scale = ring->strip_style;
                prim_draw_strip(self, &count, point - 1, (short)ring->alpha, prev_low, low, high,
                                prev_high, view, texture);
            }
            if (++point >= 21)
                point = 20;
            prev_low = low;
            prev_low.y = 0.0f;
            prev_high = high;
        }
    }
}
