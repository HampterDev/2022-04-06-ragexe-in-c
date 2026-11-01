#include "render/rp_pool.h"
#include "resources/state_icon_sheet.h"
#include "game/own_states.h"
#include <stdlib.h>
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"
#include "actor/attr_state.h"

#define STATE_END_NEVER 999999
#define STATE_SECONDS_NONE 999

static void update_state_seconds(struct PrimStateIcon *icon)
{
    const struct OwnStateEntry *t = own_states_find(icon->state_id);
    unsigned end, now;

    icon->seconds_left = STATE_SECONDS_NONE;
    if (!t)
        return;
    end = t->end_tick;
    now = timeGetTime();
    if (end <= now)
        return;
    if (end == 0 || end == STATE_END_NEVER) {
        icon->seconds_left = STATE_SECONDS_NONE;
        return;
    }
    icon->seconds_left = (int)((end - now) / 1000);
}

static void draw_state_timers(struct EffectPrim *self, const struct PrimStateIcon *icon)
{
    size_t i;

    for (i = 0; i < own_states_count(); ++i) {
        const struct OwnStateEntry *t = own_states_at(i);
        if (t->index != icon->state_id)
            continue;
        if (!(t->end_tick > timeGetTime() && t->end_tick != STATE_END_NEVER && t->end_tick != 0)) {
            continue;
        }
        icon_sheet_queue_state(g_stateIconSheet, (unsigned)t->index,
                               (int)self->base.pos.x, (int)self->base.pos.y);
    }
    if (g_playerBlind == 1)
        icon_sheet_clear(g_stateIconSheet);
    else
        icon_sheet_flush(g_stateIconSheet);
}

#define PATTERN_SPRITE_ABOVE      0x001
#define PATTERN_SCREEN_AT_DELTA2  0x004
#define PATTERN_STOP_SIZE_AT      0x002
#define PATTERN_WANDER            0x008
#define PATTERN_RAY_STOP_AT_ZERO  0x001
#define PATTERN_RING_FULL_WIDTH   0x001

unsigned char prim_screen_sprite_process(struct EffectPrim *self)
{
    float step_x, step_y;
    int turn;

    if (self->variant == PRIM_VARIANT_STATE_ICON)
        update_state_seconds(&self->records.state_icon);

    if (self->change_frame == self->frame) {
        if (self->next_size1_speed != PRIM_UNSET)
            self->size[1].speed = self->next_size1_speed;
        if (self->next_size2_speed != PRIM_UNSET)
            self->size[2].speed = self->next_size2_speed;
        if (self->next_speed != PRIM_UNSET)
            self->speed = self->next_speed;
        if (self->next_size1_accel != PRIM_UNSET)
            self->size[1].accel = self->next_size1_accel;
        if (self->next_size2_accel != PRIM_UNSET)
            self->size[2].accel = self->next_size2_accel;
        if (self->next_accel != PRIM_UNSET)
            self->accel = self->next_accel;
    }

    if (self->pattern & PATTERN_SCREEN_AT_DELTA2) {
        self->base.pos.x = self->delta2.x;
        self->base.pos.y = self->delta2.y;
    } else {
        self->base.pos.x = self->master->screen_pos.x + self->delta2.x;
        self->base.pos.y = self->master->screen_pos.y + self->delta2.y;
    }

    self->speed += self->accel;
    self->angle_speed[2] += self->angle_accel[2];
    self->size[1].speed += self->size[1].accel;
    self->size[2].speed += self->size[2].accel;
    if ((self->pattern & PATTERN_STOP_SIZE_AT) && self->frame >= self->pattern_arg) {
        self->size[1].accel = 0.0f;
        self->size[1].speed = 0.0f;
        self->size[2].accel = 0.0f;
        self->size[2].speed = 0.0f;
    }

    if (self->pattern & PRIM_PATTERN_ALPHA_BOUNCE) {
        if (0.0f > self->alpha_speed) {
            if (self->alpha <= self->alpha_min)
                self->alpha_speed = self->alpha_speed * -1.0f;
        } else if (self->alpha >= self->alpha_max) {
            self->alpha_speed = self->alpha_speed * -1.0f;
        }
    }

    self->angle[2] += self->angle_speed[2];
    self->size[1].value += self->size[1].speed;
    self->size[2].value += self->size[2].speed;

    step_x = get_sin_deg((int)self->angle[0]) * self->speed;
    self->delta.x += step_x;
    step_y = get_cos_deg((int)self->angle[0]) * self->speed;
    self->r += self->r_speed;
    self->g += self->g_speed;
    self->b += self->b_speed;
    self->base.pos.x += self->delta.x;
    self->delta.y -= step_y;
    self->base.pos.y = self->delta.y + self->base.pos.y;

    if (self->r > 255)
        self->r = 0;
    else if (self->r < 0)
        self->r = 255;
    if (self->g > 255)
        self->g = 255;
    else if (self->g < 0)
        self->g = 0;
    if (self->b > 255)
        self->b = 255;
    else if (self->b < 0)
        self->b = 0;

    if ((self->pattern & PATTERN_WANDER) && self->frame % self->pattern_arg == 0) {
        turn = rand() % 16;
        self->angle[0] = (float)turn + (self->angle[0] - 8.0f);
        self->pattern_arg = rand() % 10 + 10;
    }

    prim_update_alpha(self);
    return prim_step_motion_counted(self);
}

void prim_screen_sprite_render(struct EffectPrim *self)
{
    struct PrimTriangle *tri;
    float s, c, w, top, bottom;
    float tl_x, tl_y, tr_x, tr_y, bl_x, bl_y, br_x, br_y;
    float px, py;

    if (self->variant == PRIM_VARIANT_STATE_ICON)
        self->base.motion = 0;
    s = get_sin_deg((int)self->angle[2]);
    c = get_cos_deg((int)self->angle[2]);
    w = self->size[1].value;
    if (self->pattern & PATTERN_SPRITE_ABOVE) {
        top = self->size[2].value * -2.0f;
        bottom = 0.0f;
    } else {
        top = -self->size[2].value;
        bottom = self->size[2].value;
    }

    tl_x = -w * c - top * s;
    tl_y = -w * s + top * c;
    tr_x = w * c - top * s;
    tr_y = w * s + top * c;
    bl_x = -w * c - bottom * s;
    bl_y = -w * s + bottom * c;
    br_x = w * c - bottom * s;
    br_y = w * s + bottom * c;

    px = self->base.pos.x;
    py = self->base.pos.y;
    tri = &self->triangles[0];
    prim_set_screen_vertex(&tri->v[0], bl_x + px - 0.5f, bl_y + py - 0.5f, self->argb, 0.0f, 1.0f);
    prim_set_screen_vertex(&tri->v[1], px + tl_x - 0.5f, py + tl_y - 0.5f, self->argb,
                           0.0f, 0.0f);
    prim_set_screen_vertex(&tri->v[2], px + br_x - 0.5f, py + br_y - 0.5f, self->argb,
                           1.0f, 1.0f);
    tri->texture = self->textures[self->base.motion];
    renderer_add_triangle(g_renderer, tri, self->use_list_flags ? self->list_flags : 1);

    tri = &self->triangles[1];
    prim_set_screen_vertex(&tri->v[0], px + br_x - 0.5f, py + br_y - 0.5f, self->argb,
                           1.0f, 1.0f);
    prim_set_screen_vertex(&tri->v[1], px + tl_x - 0.5f, py + tl_y - 0.5f, self->argb,
                           0.0f, 0.0f);
    prim_set_screen_vertex(&tri->v[2], px + tr_x - 0.5f, py + tr_y - 0.5f, self->argb,
                           1.0f, 0.0f);
    tri->texture = self->textures[self->base.motion];
    renderer_add_triangle(g_renderer, tri, self->use_list_flags ? self->list_flags : 1);

    if (self->variant == PRIM_VARIANT_STATE_ICON)
        draw_state_timers(self, &self->records.state_icon);
}

static void screen_turn(struct TexDrawVertex *v, float s, float c, const Vec3 *at)
{
    float x = v->x;
    float y = v->y;

    v->x = x * c - y * s + at->x;
    v->y = y * c + x * s + at->y;
}

void prim_screen_spike_render(struct EffectPrim *self)
{
    struct PrimTriangle *tri;
    float s = get_sin_deg((int)self->angle[2]);
    float c = get_cos_deg((int)self->angle[2]);
    float half_h = self->size[2].value * 0.5f;
    float half_w = self->size[1].value * 0.5f;
    float tip = half_h / 1.2f;
    int i;

    tri = &self->triangles[0];
    prim_set_screen_vertex(&tri->v[0], -half_w, tip - half_h, self->argb, 0.0f, 0.5f);
    prim_set_screen_vertex(&tri->v[1], 0.0f, half_h, 0xffffffff, 0.5f, 1.0f);
    prim_set_screen_vertex(&tri->v[2], 0.0f, -half_h, self->argb, 0.5f, 0.0f);
    tri->texture = self->textures[self->base.motion];
    for (i = 0; i < 3; i++)
        screen_turn(&tri->v[i], s, c, &self->base.pos);
    renderer_add_triangle(g_renderer, tri, 1);

    tri = &self->triangles[1];
    prim_set_screen_vertex(&tri->v[0], half_w, half_h - tip, self->argb, 1.0f, 0.5f);
    prim_set_screen_vertex(&tri->v[1], 0.0f, half_h, 0xffffffff, 0.5f, 1.0f);
    prim_set_screen_vertex(&tri->v[2], 0.0f, -half_h, self->argb, 0.5f, 0.0f);
    tri->texture = self->textures[self->base.motion];
    for (i = 0; i < 3; i++)
        screen_turn(&tri->v[i], s, c, &self->base.pos);
    renderer_add_triangle(g_renderer, tri, 1);
}

void prim_screen_ring_render(struct EffectPrim *self)
{
    struct PrimTriangle *tri = self->triangles;
    float limit = 360.0f - self->arc_degrees;
    float inner = self->size[0].value - self->scale;
    float u = 0.0f;
    float a = 0.0f;

    if (!(limit >= 0.0f))
        return;
    do {
        float b = self->arc_degrees + a;
        float c0 = g_cosTable[(int)a], s0 = g_sinTable[(int)a];
        float c1 = g_cosTable[(int)b], s1 = g_sinTable[(int)b];
        float in_x = c0 * inner;
        float in_y = s0 * inner;
        float u1 = u + 0.25f;

        prim_set_screen_vertex(&tri->v[0], in_x + self->base.pos.x, self->base.pos.y - in_y, self->argb, u, 1.0f);
        prim_set_screen_vertex(&tri->v[1], c1 * self->size[0].value + self->base.pos.x,
                               self->base.pos.y - s1 * self->size[0].value, self->argb, u1, 0.0f);
        prim_set_screen_vertex(&tri->v[2], c0 * self->size[0].value + self->base.pos.x,
                               self->base.pos.y - s0 * self->size[0].value, self->argb, u, 0.0f);
        tri->texture = self->textures[0];
        renderer_add_triangle(g_renderer, tri, 1);
        tri++;

        prim_set_screen_vertex(&tri->v[0], in_x + self->base.pos.x, self->base.pos.y - in_y, self->argb, u, 1.0f);
        prim_set_screen_vertex(&tri->v[1], c1 * inner + self->base.pos.x, self->base.pos.y - s1 * inner, self->argb,
                               u1, 1.0f);
        prim_set_screen_vertex(&tri->v[2], c1 * self->size[0].value + self->base.pos.x,
                               self->base.pos.y - s1 * self->size[0].value, self->argb, u1, 0.0f);
        tri->texture = self->textures[0];
        renderer_add_triangle(g_renderer, tri, 1);
        tri++;

        u = u1 >= 1.0f ? 0.0f : u1;
        a = self->arc_degrees + a;
    } while (limit >= a);
}

unsigned char prim_screen_orbit_process(struct EffectPrim *self)
{
    self->master_pos.x = self->delta2.x + self->master->screen_pos.x;
    self->master_pos.y = self->delta2.y + self->master->screen_pos.y;
    self->size[0].speed += self->size[0].accel;
    self->angle_speed[0] += self->angle_accel[0];
    self->size[1].speed += self->size[1].accel;
    self->size[2].speed += self->size[2].accel;
    self->angle_speed[2] += self->angle_accel[2];
    self->size[0].value += self->size[0].speed;
    self->angle[0] += self->angle_speed[0];
    self->angle[2] += self->angle_speed[2];
    self->size[1].value += self->size[1].speed;
    self->size[2].value += self->size[2].speed;
    self->base.pos.x = get_sin_deg((int)self->angle[0]) * self->size[0].value + self->master_pos.x;
    self->base.pos.y =
        (float)((double)self->master_pos.y - (double)get_cos_deg((int)self->angle[0]) * self->size[0].value);
    prim_update_alpha(self);
    return prim_step_motion_counted(self);
}

void prim_screen_ray_process(struct EffectPrim *self)
{
    self->base.pos.x = self->delta2.x + self->master->screen_pos.x;
    self->base.pos.y = self->delta2.y + self->master->screen_pos.y;
    if ((self->pattern & PATTERN_RAY_STOP_AT_ZERO) && 0.0f >= self->size[2].value) {
        self->size[2].speed = 0.0f;
        self->size[2].accel = 0.0f;
    }
    if (self->frame == self->change_frame) {
        self->size[2].speed = self->next_size2_speed;
        self->angle_speed[0] = self->next_speed;
        self->angle_accel[0] = self->next_accel;
    }
    if (0.0f > self->size[2].value) {
        self->size[2].value = 0.0f;
        self->size[2].speed = self->size[2].speed * -1.0f;
    }
    self->angle_speed[0] += self->angle_accel[0];
    self->size[2].speed += self->size[2].accel;
    self->angle[0] += self->angle_speed[0];
    self->size[2].value += self->size[2].speed;
    self->arc_degrees += self->size[1].speed;
    prim_update_alpha(self);
}

void prim_screen_ray_render(struct EffectPrim *self)
{
    struct PrimTriangle *tri = &self->triangles[0];
    float c1 = get_cos_deg((int)(self->angle[0] - self->arc_degrees));
    float s1 = get_sin_deg((int)(self->angle[0] - self->arc_degrees));
    float c2 = get_cos_deg((int)(self->angle[0] + self->arc_degrees));
    float s2 = get_sin_deg((int)(self->angle[0] + self->arc_degrees));

    prim_set_screen_vertex(&tri->v[0], self->base.pos.x, self->base.pos.y, self->argb, 0.5f, 0.0f);
    prim_set_screen_vertex(&tri->v[1], self->size[2].value * c1 + self->base.pos.x,
                           self->base.pos.y - self->size[2].value * s1, self->argb, 1.0f, 1.0f);
    prim_set_screen_vertex(&tri->v[2], self->size[2].value * c2 + self->base.pos.x,
                           self->base.pos.y - self->size[2].value * s2, self->argb, 0.0f, 1.0f);
    tri->texture = self->textures[0];
    renderer_add_triangle(g_renderer, tri, 1);
}

void prim_screen_ring_process(struct EffectPrim *self)
{
    float radius;

    self->base.pos.x = self->delta2.x + self->master->screen_pos.x;
    self->base.pos.y = self->delta2.y + self->master->screen_pos.y;
    self->size[0].speed += self->size[0].accel;
    radius = self->size[0].value + self->size[0].speed;
    self->size[0].value = radius;
    if ((self->pattern & PATTERN_RING_FULL_WIDTH) || self->radius[0] > radius)
        self->scale = radius;
    else
        self->scale = self->radius[0];
    prim_update_alpha(self);
}
