#include <stdlib.h>
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"

#define COS_DEG(d) g_cosTable[d]
#define SIN_DEG(d) g_sinTable[d]

#define COLUMN_PROFILE(j) SIN_DEG((j) * 9)

#define RING_HEIGHT_SUNK_2 6.001f
#define RING_HEIGHT_SUNK_1 24.001f

#define SCALE_FOURTH_RING_ONLY 3.0f

#define RING_CAMERA_YAW(p) ((p)->records.rings[2].mode)

void prim_rings_render(struct EffectPrim *self, const float *view)
{
    struct PrimRing *rings = self->records.rings;
    Vec3 prev_ground = {0.0f, 0.0f, 0.0f};
    Vec3 prev_top = {0.0f, 0.0f, 0.0f};
    int count = 0;
    int i;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRing *ring = &rings[i];
        int step, deg, seg;

        if (ring->active != 1)
            continue;
        step = ring->span / 20;
        seg = 0;
        for (deg = 0; deg <= ring->span; deg += step) {
            Vec3 ground, top;
            float c, s, along, up;
            int at = (deg + ring->turn) % 360;

            if (ring->span == 360 && seg == 20)
                at = ring->turn;
            c = COS_DEG(at);
            s = SIN_DEG(at);
            ground.x = ring->radius * c;
            ground.z = ring->radius * s;
            if (ring->height == RING_HEIGHT_SUNK_2)
                ground.y = -2.0f;
            else if (ring->height == RING_HEIGHT_SUNK_1)
                ground.y = -1.0f;
            else
                ground.y = 0.0f;

            along = COS_DEG(ring->tilt) * ring->column[seg];
            up = SIN_DEG(ring->tilt) * ring->column[seg];
            top.z = along * s + ground.z;
            top.x = along * c + ground.x;
            if (ring->height == RING_HEIGHT_SUNK_2)
                up += 2.0f;
            else if (ring->height == RING_HEIGHT_SUNK_1)
                up += 1.0f;
            top.y = -up;

            if (seg > 0)
                prim_draw_strip(self, &count, seg - 1, ring->alpha, prev_ground,
                                ground, top, prev_top, view, 0);
            prev_ground = ground;
            prev_top = top;
            seg++;
            if (seg >= PRIM_RING_COLUMNS)
                seg = PRIM_RING_COLUMNS - 1;
        }
    }
}

void prim_rings_render_offset(struct EffectPrim *self, const float *view)
{
    struct PrimRing *rings = self->records.rings;
    Vec3 prev_ground = {0.0f, 0.0f, 0.0f};
    Vec3 prev_top = {0.0f, 0.0f, 0.0f};
    int count = 0;
    int i;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRing *ring = &rings[i];
        int step, deg, seg;

        if (ring->active != 1)
            continue;
        step = ring->span / 10;
        seg = 0;
        for (deg = 0; deg <= ring->span; deg += step) {
            Vec3 ground, top;
            float c, s, along;
            int at = (deg + ring->turn) % 360;

            if (ring->span == 360 && seg == 10)
                at = ring->turn;
            c = COS_DEG(at);
            s = SIN_DEG(at);
            ground.x = ring->radius * c;
            ground.y = 0.0f;
            ground.z = ring->radius * s;
            along = COS_DEG(ring->tilt) * ring->column[seg];
            top.x = along * c + ground.x;
            top.y = -(SIN_DEG(ring->tilt) * ring->column[seg]);
            top.z = along * s + ground.z;

            if (ring->offset_flag == PRIM_RING_OFFSET_ON) {
                short yaw = RING_CAMERA_YAW(self);
                float ox = COS_DEG(yaw) * ring->offset_x - SIN_DEG(yaw) * ring->offset_z;
                float oz = SIN_DEG(yaw) * ring->offset_x + COS_DEG(yaw) * ring->offset_z;
                top.x = ox + top.x;
                ground.x = ground.x + ox;
                top.z = oz + top.z;
                ground.z = ground.z + oz;
            }

            if (seg > 0)
                prim_draw_strip(self, &count, seg - 1, ring->alpha, prev_ground,
                                ground, top, prev_top, view, 0);
            prev_ground = ground;
            prev_top = top;
            seg++;
            if (seg > 10)
                seg = 10;
        }
    }
}

static void follow_master(struct EffectPrim *self)
{
    self->base.pos = self->master->base.pos;
}

static void ring_turn(struct PrimRing *ring, int step)
{
    ring->turn += step;
    if (ring->turn >= 360)
        ring->turn -= 360;
}

static void ring_rise_columns(struct PrimRing *ring)
{
    int j;

    for (j = 0; j < PRIM_RING_COLUMNS; j++) {
        float full;

        if (ring->column_flag[j])
            continue;
        if (ring->age <= 90)
            ring->column[j] = ring->height * COLUMN_PROFILE(j) * SIN_DEG(ring->age);
        full = ring->height * COLUMN_PROFILE(j);
        if (ring->column[j] > full) {
            ring->column[j] = full;
            ring->column_flag[j] = 1;
        }
        if (0.0f > ring->column[j])
            ring->column[j] = 0.0f;
    }
}

#define RISE_NO_FADE_IN 1
unsigned char prim_rings_rise_process(struct EffectPrim *self)
{
    int i;

    follow_master(self);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRing *ring;

        if (self->scale == SCALE_FOURTH_RING_ONLY && i < 3)
            i = 3;
        ring = &self->records.rings[i];
        if (ring->active != 1)
            continue;
        ring->age++;
        if (i < 3)
            ring->turn += i + 3;
        if (ring->turn >= 360)
            ring->turn -= 360;

        if (ring->age >= self->duration - 40) {
            ring->alpha += i < 3 ? -5 : -2;
            if (ring->alpha < 0)
                ring->alpha = 0;
        } else if (ring->age < 20 && ring->mode != RISE_NO_FADE_IN) {
            if (i == 3) {
                ring->alpha += 8;
                if (ring->alpha > 60)
                    ring->alpha = 60;
            } else {
                ring->alpha += 10;
                if (ring->alpha > 180)
                    ring->alpha = 180;
            }
        }
        ring_rise_columns(ring);
    }
    return 1;
}

unsigned char prim_rings_rise_sway_process(struct EffectPrim *self)
{
    int i;

    follow_master(self);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRing *ring;
        int j;

        if (self->scale == SCALE_FOURTH_RING_ONLY && i < 3)
            i = 3;
        ring = &self->records.rings[i];
        if (ring->active != 1)
            continue;
        ring->age++;
        if (i < 3)
            ring->turn += i + 3;
        if (ring->turn >= 360)
            ring->turn -= 360;

        if (ring->age >= self->duration - 40) {
            ring->alpha += i < 3 ? -5 : -3;
            if (ring->alpha < 0)
                ring->alpha = 0;
        } else if (ring->age < 20) {
            if (i == 3) {
                ring->alpha += 8;
                if (ring->alpha > 100)
                    ring->alpha = 100;
            } else {
                ring->alpha += 10;
                if (ring->alpha > 180)
                    ring->alpha = 180;
            }
        }

        if (i < 3) {
            ring_rise_columns(ring);
            continue;
        }
        for (j = 0; j < PRIM_RING_COLUMNS; j++) {
            if (!ring->column_flag[j]) {
                float v;
                if (ring->age > 90) {
                    ring->column_flag[j] = 1;
                    v = ring->column[j];
                } else {
                    v = SIN_DEG(ring->age) * (COLUMN_PROFILE(j) * ring->height);
                    ring->column[j] = v;
                }
                if (v > COLUMN_PROFILE(j) * ring->height)
                    ring->column_flag[j] = 1;
                if (0.0f > ring->column[j])
                    ring->column[j] = 0.0f;
            } else {
                ring->column[j] = COLUMN_PROFILE(j) * ring->height * 0.55f;
                ring->column[j] = COLUMN_PROFILE(j) * ring->height * 0.45f * SIN_DEG(ring->age % 360) +
                                  ring->column[j];
            }
        }
    }
    return 1;
}

unsigned char prim_rings_grow_process(struct EffectPrim *self)
{
    int i;

    follow_master(self);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRing *ring = &self->records.rings[i];
        int j, step;

        if (ring->active != 1)
            continue;
        ring->age++;
        step = rand();
        if (i == 3) {
            step %= 2;
            ring->turn = (short)((step < 0 ? 0 : step) + 2 + ring->turn);
        } else {
            step %= 3;
            ring->turn = (short)((step < 0 ? 0 : step) + 4 + ring->turn);
        }
        if (ring->turn >= 360)
            ring->turn -= 360;

        if (ring->age >= 40) {
            ring->alpha -= 15;
            if (ring->alpha < 0)
                ring->alpha = 0;
        } else if (ring->age < 20) {
            ring->alpha += 5;
            if (ring->alpha > 80)
                ring->alpha = 80;
        }
        ring->height = ring->height * (i == 3 ? 1.025f : 1.005f);

        for (j = 0; j < PRIM_RING_COLUMNS; j++) {
            float v;
            if (ring->age <= 90) {
                v = ring->height * COLUMN_PROFILE(j) * SIN_DEG(ring->age);
                ring->column[j] = v;
            } else {
                ring->column_flag[j] = 1;
                v = ring->column[j];
            }
            if (v > ring->height * COLUMN_PROFILE(j)) {
                ring->column_flag[j] = 1;
                v = ring->column[j];
            }
            if (0.0f > v)
                ring->column[j] = 0.0f;
        }
    }
    return 1;
}

#define SPREAD_SOFT 22

static void ring_spread(struct EffectPrim *self, struct PrimRing *ring, int i)
{
    int j, phase;

    ring->radius = ring->radius + 0.07f;
    ring->tilt--;
    if (ring->tilt < 10) {
        ring->tilt = 10;
        ring->alpha = 0;
    }
    if (ring->radius >= 4.0f) {
        ring->alpha -= 3;
        if (ring->alpha <= 0) {
            ring->alpha = 0;
            if (ring->age < self->duration - 30) {
                ring->radius = 3.37f;
                ring->tilt = ring->mode == SPREAD_SOFT ? 64 : 74;
            }
        }
    } else {
        ring->alpha += ring->mode == SPREAD_SOFT ? 5 : 10;
    }

    for (j = 0; j < PRIM_RING_COLUMNS; j++) {
        if (i < 2)
            phase = ring->age + i * 90;
        else
            phase = (ring->age + i * 45) * 2;
        ring->column[j] = ring->height;
        ring->column[j] = COLUMN_PROFILE(j) * ring->height * 0.3f * SIN_DEG(phase % 360) + ring->height;
    }
}

void prim_rings_spread_process(struct EffectPrim *self)
{
    int i;

    follow_master(self);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRing *ring = &self->records.rings[i];

        if (ring->active != 1)
            continue;
        ring->age++;
        if (ring->age <= 0)
            continue;
        ring_spread(self, ring, i);
    }
}

void prim_rings_spread_colors_process(struct EffectPrim *self)
{
    int i;

    follow_master(self);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRing *ring = &self->records.rings[i];

        if (ring->active != 1)
            continue;
        ring->age++;
        switch (ring->age % 100) {
        case 0:  self->scale = 4.0f; break;
        case 25: self->scale = 6.0f; break;
        case 50: self->scale = 7.0f; break;
        case 75: self->scale = 8.0f; break;
        }
        if (ring->age <= 0)
            continue;
        ring_spread(self, ring, i);
    }
}

#define WIDEN_SLOW(p) ((p)->accel == 99.0f)

void prim_rings_widen_process(struct EffectPrim *self)
{
    int i;

    follow_master(self);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRing *ring = &self->records.rings[i];
        int j;

        if (ring->active != 1)
            continue;
        ring->age++;
        ring_turn(ring, WIDEN_SLOW(self) ? 3 : 5);
        ring->radius = ring->radius + (WIDEN_SLOW(self) ? 0.1f : 0.3f);
        ring->tilt--;
        if (WIDEN_SLOW(self)) {
            if (ring->tilt < 30)
                ring->tilt = 30;
        } else {
            if (ring->tilt < 40)
                ring->tilt = 40;
        }

        if (ring->age > 10) {
            switch (ring->mode) {
            case 2:  ring->alpha -= 8; break;
            case 3:  ring->alpha -= 5; break;
            case 4:  ring->alpha -= 5; break;
            case 5:  ring->alpha -= 4; break;
            default: ring->alpha -= 2; break;
            }
            if (ring->alpha <= 0)
                ring->alpha = 0;
        } else {
            ring->alpha += ring->mode == 1 ? 10 : 20;
        }

        for (j = 0; j < PRIM_RING_COLUMNS; j++) {
            int phase = (ring->age + i * 90) % 360;
            ring->column[j] = ring->height * COLUMN_PROFILE(j);
            ring->column[j] = ring->height * COLUMN_PROFILE(j) * 0.15f * SIN_DEG(phase) + ring->column[j];
        }
    }
}

#define STYLE_WAVE        0x00
#define STYLE_RISE        1
#define STYLE_FLASH       2
#define STYLE_RISE_SLOW   3
#define STYLE_WAVE_FACING 0x21

unsigned char prim_rings_styled_process(struct EffectPrim *self)
{
    struct PrimRing *rings = self->records.rings;
    int i;

    if (rings[0].column_flag[0] == STYLE_WAVE_FACING) {
        RING_CAMERA_YAW(self) = (short)(360 - (int)game_camera_heading());
    }
    follow_master(self);

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRing *ring = &rings[i];
        unsigned char style;
        int j;

        if (ring->active != 1)
            continue;
        ring->age++;
        style = ring->column_flag[0];
        if (style == STYLE_RISE)
            ring->turn += i + 8;
        else if (style == STYLE_FLASH)
            ring->turn += (i < 2) * 2 + 2 + i;
        else if (style == STYLE_RISE_SLOW)
            ring->turn += i + 6;
        if (ring->turn >= 360)
            ring->turn -= 360;

        if (ring->age >= ring->mode && ring->mode >= 0) {
            ring->alpha -= 2;
            if (ring->alpha < 0)
                ring->alpha = 0;
        } else if (ring->age < 16) {
            style = ring->column_flag[0];
            if (style == STYLE_FLASH)
                ring->alpha += 3;
            else if (style == STYLE_RISE_SLOW)
                ring->alpha += 2;
            else
                ring->alpha += 5;
            if (ring->alpha > 180)
                ring->alpha = 180;
        }

        for (j = 0; j < PRIM_RING_COLUMNS; j++) {
            unsigned char col_style = ring->column_flag[j];

            if (col_style == STYLE_WAVE || col_style == STYLE_WAVE_FACING) {
                int phase;
                float v;
                if (i == 0)
                    phase = j * 34 + ring->age * 4;
                else if (i == 1)
                    phase = j * 34 + ring->age * 3;
                else
                    phase = (ring->age + j * 17) * 2;
                v = ring->height * 0.25f * SIN_DEG(phase % 360) + ring->height * 0.75f;
                ring->column[j] = v;
                if (ring->age <= 90) {
                    v = v * SIN_DEG(ring->age);
                    ring->column[j] = v;
                }
                if (0.0f > v)
                    ring->column[j] = 0.0f;
            } else if (col_style == STYLE_RISE || col_style == STYLE_RISE_SLOW) {
                ring->column[j] = ring->height;
                if (ring->age <= 90)
                    ring->column[j] = SIN_DEG(ring->age) * ring->height;
            } else if (col_style == STYLE_FLASH) {
                ring->column[j] = ring->height;
                if (ring->age > 45)
                    ring->column[j] = 0.0f;
                else
                    ring->column[j] = SIN_DEG(ring->age * 4) * ring->height;
            }
        }
    }
    return 1;
}

#define PULSE_BRIEF  1
#define PULSE_INWARD 3
#define PULSE_FAINT  4
#define PULSE_DIM    5

unsigned char prim_rings_pulse_process(struct EffectPrim *self)
{
    struct PrimRing *rings = self->records.rings;
    struct PrimRing *control = &rings[3];
    int i;

    follow_master(self);
    for (i = 0; i < 3; i++) {
        struct PrimRing *ring = &rings[i];
        int j;

        if (ring->active != 1)
            continue;
        ring->age++;
        if (i == 0)
            control->age++;
        if (ring->age <= 0)
            continue;

        if (control->mode == PULSE_INWARD) {
            ring->radius = ring->radius - 0.25f;
            if (7.0f > ring->radius) {
                if (0.0f > ring->radius)
                    ring->radius = 0.0f;
                ring->alpha -= 7;
                if (ring->alpha < 0) {
                    ring->alpha = 0;
                    if (control->age > 1400) {
                        ring->active = 0;
                    } else {
                        ring->age = 0;
                        ring->radius = 14.0f;
                    }
                }
            }
        } else {
            ring->radius = ring->radius + 0.5f;
            if (ring->radius > 7.0f) {
                ring->alpha += control->mode == PULSE_FAINT ? -5 : control->mode == PULSE_DIM ? -8 : -15;
                if (ring->alpha < 0) {
                    ring->alpha = 0;
                    if ((control->age > 120 && (control->mode == PULSE_BRIEF || control->mode == PULSE_FAINT ||
                                                control->mode == PULSE_DIM)) ||
                        control->age > 1400) {
                        ring->active = 0;
                    } else {
                        ring->age = 0;
                        ring->radius = 0.0f;
                    }
                }
            }
        }

        if (control->mode == PULSE_INWARD) {
            if (ring->age < 20) {
                ring->alpha += 12;
                if (ring->alpha > 240)
                    ring->alpha = 240;
            }
        } else if (ring->age < 10) {
            if (control->mode == PULSE_FAINT) {
                ring->alpha += 4;
                if (ring->alpha > 40)
                    ring->alpha = 40;
            } else {
                ring->alpha += 24;
                if (control->mode == PULSE_DIM) {
                    if (ring->alpha > 120)
                        ring->alpha = 120;
                } else if (ring->alpha > 240) {
                    ring->alpha = 240;
                }
            }
        }

        for (j = 0; j < PRIM_RING_COLUMNS; j++) {
            ring->column[j] = ring->height;
            if (ring->age <= 10)
                ring->column[j] = SIN_DEG(ring->age * 9) * ring->height;
        }
    }
    return 1;
}

static int ring_wave_phase(const struct PrimRing *ring, int i)
{
    if (i < 2)
        return (ring->age + i * 90) % 360;
    return ((ring->age + i * 45) * 2) % 360;
}

static void ring_wave_profile_columns(struct PrimRing *ring, int i)
{
    int phase = ring_wave_phase(ring, i);
    int j;

    for (j = 0; j < PRIM_RING_COLUMNS; j++) {
        float base = ring->height * COLUMN_PROFILE(j);
        ring->column[j] = base;
        ring->column[j] = ring->height * COLUMN_PROFILE(j) * 0.3f * SIN_DEG(phase) + base;
    }
}

static void ring_wave_columns(struct PrimRing *ring, int i)
{
    int phase = ring_wave_phase(ring, i);
    int j;

    for (j = 0; j < PRIM_RING_COLUMNS; j++) {
        float height = ring->height;
        ring->column[j] = height;
        ring->column[j] = COLUMN_PROFILE(j) * ring->height * 0.3f * SIN_DEG(phase) + height;
    }
}

static void ring_flat_columns(struct PrimRing *ring)
{
    int j;

    for (j = 0; j < PRIM_RING_COLUMNS; j++) {
        if (ring->column_flag[j])
            continue;
        ring->column[j] = ring->height;
        if (0.0f > ring->column[j])
            ring->column[j] = 0.0f;
    }
}

void prim_rings_wave_process(struct EffectPrim *self)
{
    int i;

    follow_master(self);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRing *ring = &self->records.rings[i];

        if (ring->active != 1)
            continue;
        ring->age++;
        ring_turn(ring, i == 0 || i == 2 ? 1 : 2);
        if (ring->age >= self->duration - 20) {
            ring->alpha -= 5;
            if (ring->alpha < 0)
                ring->alpha = 0;
        } else if (ring->age < 100) {
            ring->alpha++;
            if (ring->alpha > 80)
                ring->alpha = 80;
        }
        ring_wave_profile_columns(ring, i);
    }
}

void prim_rings_ripple_process(struct EffectPrim *self)
{
    int i;

    follow_master(self);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRing *ring = &self->records.rings[i];

        if (ring->active != 1)
            continue;
        ring->age++;
        ring->radius = ring->radius + 0.04f;
        ring->tilt--;
        if (ring->tilt < 10) {
            ring->tilt = 10;
            ring->alpha = 0;
        }
        if (ring->radius >= 2.5f) {
            ring->alpha -= 2;
            if (ring->alpha <= 0) {
                ring->alpha = 0;
                ring->radius = 2.1399999f;
                ring->tilt = 74;
            }
        } else {
            ring->alpha += 20;
        }
        ring_wave_columns(ring, i);
    }
}

void prim_rings_ripple_from_process(struct EffectPrim *self)
{
    int i;

    follow_master(self);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRing *ring = &self->records.rings[i];
        float start;

        if (ring->active != 1)
            continue;
        ring->age++;
        ring->radius = ring->radius + 0.04f;
        ring->tilt--;
        if (ring->tilt < 10) {
            ring->tilt = 10;
            ring->alpha = 0;
        }
        start = ring->ripple_start;
        if (ring->radius >= start + 1.0f) {
            ring->alpha -= 2;
            if (ring->alpha <= 0) {
                ring->alpha = 0;
                ring->radius = ring->ripple_start;
                ring->tilt = 74;
            }
        } else {
            ring->alpha += 10;
            if (ring->alpha > 200)
                ring->alpha = 200;
        }
        ring_wave_columns(ring, i);
    }
}

#define LATE_RISE_FADING 1
void prim_rings_late_rise_process(struct EffectPrim *self)
{
    int i;

    follow_master(self);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRing *ring = &self->records.rings[i];
        int j;

        if (ring->active != 1)
            continue;
        ring->age++;
        ring_turn(ring, 1);
        if (ring->mode == LATE_RISE_FADING && ring->age > 250) {
            ring->alpha -= 1;
            if (ring->alpha < 0)
                ring->alpha = 0;
        }
        if (ring->age >= 800) {
            ring->height = ring->height - 1.0f;
            if (0.0f > ring->height)
                ring->height = 0.0f;
        }
        for (j = 0; j < PRIM_RING_COLUMNS; j++) {
            float v;
            if (ring->age < 200)
                v = 0.0f;
            else if (ring->age <= 290)
                v = SIN_DEG(ring->age - 200) * ring->height;
            else
                v = ring->height;
            ring->column[j] = v;
            if (0.0f > v)
                ring->column[j] = 0.0f;
        }
    }
}

void prim_rings_breathe_process(struct EffectPrim *self)
{
    int i;

    follow_master(self);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRing *ring = &self->records.rings[i];

        if (ring->active != 1)
            continue;
        ring->age++;
        ring->mode++;
        if (ring->mode >= 360)
            ring->mode -= 360;
        ring->radius = SIN_DEG(ring->mode) + 3.0f;
        ring_turn(ring, 3);
        ring_wave_profile_columns(ring, i);
    }
}

void prim_rings_swell_process(struct EffectPrim *self)
{
    int i;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRing *ring = &self->records.rings[i];
        int j, phase;

        if (ring->active != 1)
            continue;
        ring->age++;
        ring_turn(ring, 5 - i);
        if (ring->age <= 0)
            continue;

        if (ring->column_flag[0] == 0) {
            if (ring->age * 2 >= 180) {
                ring->column_flag[0] = 1;
            } else if (ring->age * 2 >= 135) {
                ring->alpha -= 1;
                if (ring->alpha < 0)
                    ring->alpha = 0;
            }
            ring->height = (float)(signed char)ring->column_flag[1] * SIN_DEG(ring->age * 2);
        } else {
            ring->alpha -= 10;
            if (ring->alpha < 0)
                ring->alpha = 0;
        }

        if (i < 2)
            phase = (ring->age + i * 90) % 360;
        else
            phase = ((ring->age + i * 30) * 3) % 360;
        for (j = 0; j < PRIM_RING_COLUMNS; j++) {
            float height = ring->height;
            ring->column[j] = height;
            ring->column[j] = COLUMN_PROFILE(j) * ring->height * 0.3f * SIN_DEG(phase) + height;
        }
    }
}

void prim_rings_close_process(struct EffectPrim *self)
{
    int i;

    follow_master(self);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRing *ring = &self->records.rings[i];

        if (ring->active != 1)
            continue;
        ring->age++;
        ring->radius = ring->radius - 0.05f;
        if (!(0.0f < ring->radius)) {
            ring->radius = 10.0f;
            ring->alpha = 0;
        }
        ring->tilt = (short)(90.0f - ring->radius * 9.0f);
        ring->height = ring->radius;
        if (ring->age >= self->duration - 40) {
            ring->alpha -= 3;
            if (ring->alpha < 0)
                ring->alpha = 0;
        } else if (ring->alpha < 70) {
            ring->alpha++;
        }
        ring_flat_columns(ring);
    }
}

void prim_rings_bloom_process(struct EffectPrim *self)
{
    int i;

    follow_master(self);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRing *ring = &self->records.rings[i];

        if (ring->active != 1)
            continue;
        ring->age++;
        ring->radius = ring->radius + 0.1f;
        if (!(ring->radius < 13.0f)) {
            ring->radius = 3.0f;
            ring->alpha = 0;
            if (ring->age > 50)
                ring->active = 0;
        }
        ring->height = ring->radius - 3.0f;
        if (ring->height > 5.0f) {
            ring->height = 10.0f - ring->height;
            if (ring->alpha > 0)
                ring->alpha -= 2;
        } else {
            ring->alpha += 2;
        }
        ring->height = ring->height + ring->height;
        ring->tilt = (short)(90.0f - (ring->radius - 3.0f) * 5.0f);
        ring_flat_columns(ring);
    }
}

void prim_rings_throb_process(struct EffectPrim *self)
{
    int i;

    follow_master(self);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRing *ring = &self->records.rings[i];
        int j;

        if (ring->active != 1)
            continue;
        ring->age++;
        if (ring->turn >= 360)
            ring->turn -= 360;
        ring->height = SIN_DEG(ring->age % 90 * 4) * 2.5f + ring->throb_base;
        if (ring->tilt >= 360)
            ring->tilt -= 360;
        if (ring->tilt < 0)
            ring->tilt += 360;
        if (ring->age > 110) {
            ring->alpha -= 1;
            if (ring->alpha < 0)
                ring->alpha = 0;
        } else if (ring->age < 20) {
            ring->alpha += 5;
            if (ring->alpha > 180)
                ring->alpha = 180;
        }
        for (j = 0; j < PRIM_RING_COLUMNS; j++) {
            float full, v;

            if (ring->column_flag[j])
                continue;
            full = ring->height * COLUMN_PROFILE(j);
            v = ring->age <= 90 ? SIN_DEG(ring->age) * full : full;
            ring->column[j] = v;
            full = COLUMN_PROFILE(j) * ring->height;
            if (v > full) {
                ring->column[j] = full;
                ring->column_flag[j] = 1;
            }
            if (0.0f > ring->column[j])
                ring->column[j] = 0.0f;
        }
    }
}

void prim_rings_converge_process(struct EffectPrim *self)
{
    int i;

    follow_master(self);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRing *ring = &self->records.rings[i];

        if (ring->active != 1)
            continue;
        ring->age++;
        ring_turn(ring, 4);
        if (ring->age <= 30) {
            ring->alpha += 8;
            if (ring->alpha > 160)
                ring->alpha = 160;
        }
        ring->radius = ring->radius - 0.2f;
        ring_rise_columns(ring);
    }
}
