#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <stdlib.h>
#include <string.h>
#include "effects/particle.h"
#include "std/vec.h"

struct RotateLineEffect {
    struct Effect base;
    float ring_radius;
    int ring_angle;
    int ring_span;
    int ring_alpha;
    unsigned char walking;
    Vec3 prev_low;
    Vec3 prev_high;
    struct Vec particles;
};

struct String;
void rotate_line_effect_init(struct Effect *self);
void rotate_line_effect_process(struct Effect *self);
void rotate_line_effect_render(struct Effect *self);
void rotate_line_effect_release(struct Effect *self);
static void rotate_line_effect_destroy(struct Effect *effect);

static const struct EffectOps rotate_line_effect_ops = {
    .set_submit = effect_set_submit,
    .init = rotate_line_effect_init,
    .update_range = effect_update_range,
    .process = rotate_line_effect_process,
    .render = rotate_line_effect_render,
    .release = rotate_line_effect_release,
    .is_alive = effect_is_within_duration,
    .restart = effect_restart,
    .set_color = effect_set_color,
    .set_texture = effect_set_texture,
    .after_init = effect_after_init,
    .skip = effect_skip,
    .set_stop_frame = effect_set_stop_frame,
    .set_start_delay = effect_set_start_delay,
    .set_spin = effect_set_spin,
    .set_str_name = effect_set_str_name,
    .set_list_flags = effect_set_list_flags,
    .set_zoom = effect_set_zoom,
    .set_alpha_scale = effect_set_alpha_scale,
    .destroy = rotate_line_effect_destroy,
    .fade_out = effect_fade_out,
};

#define LIST_WORLD 5
#define SEGMENTS 21
#define DARK_TYPE 0x3da
#define BLUE_TYPE 0x3e8

struct RotateLineParticle {
    int tilt;
    int span;
    float height;
    float band;
};
PARTICLE_EFFECT_DATA_FITS(RotateLineParticle);
static __inline struct RotateLineParticle *rotate_line_particle(struct Particle *p)
{
    return (struct RotateLineParticle *)p->effect_data;
}
#define particle_owner_type(p) ((p)->effect_type)
#define particle_tag(p)        ((p)->owner_tag)
#define particle_column(p)     ((p)->index)
#define particle_ring(p)       ((p)->group)
#define particle_tilt(p)       (rotate_line_particle(p)->tilt)
#define particle_span(p)       (rotate_line_particle(p)->span)
#define particle_height(p)     (rotate_line_particle(p)->height)
#define particle_band(p)       (rotate_line_particle(p)->band)

#define EACH_PARTICLE(self, it) \
    for (it = (struct Particle **)(self)->particles.first; it != (struct Particle **)(self)->particles.last; it++)

struct RotateLineEffect *rotate_line_effect_construct(struct RotateLineEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &rotate_line_effect_ops;
    self->ring_radius = 0;
    self->ring_angle = 0;
    self->ring_span = 0;
    self->ring_alpha = 0;
    self->walking = 0;
    self->prev_low.x = self->prev_low.y = self->prev_low.z = 0;
    self->prev_high.x = self->prev_high.y = self->prev_high.z = 0;
    self->particles.first = self->particles.last = self->particles.end = 0;
    return self;
}

struct Effect *rotate_line_effect_new(void)
{
    struct RotateLineEffect *self = mem_try_alloc(sizeof *self);

    return self ? &rotate_line_effect_construct(self)->base : NULL;
}

/* Effect types created as RotateLineEffect, registered at startup through g_effectTypes
   (effects/effect_registry.c). A type missing here is not created. */
static const struct EffectTypeRow rotate_line_effect_type_rows[] = {
    EFFECT_TYPE(0x3da, rotate_line_effect_new),
    EFFECT_TYPE(0x3e8, rotate_line_effect_new),
};
const struct EffectTypeTable rotate_line_effect_types = EFFECT_TYPE_TABLE(rotate_line_effect_type_rows);

void rotate_line_effect_destruct(struct RotateLineEffect *self)
{
    self->base.ops = &rotate_line_effect_ops;
    vec_free(&self->particles);
    effect_destruct(&self->base);
}

static void rotate_line_effect_destroy(struct Effect *effect)
{
    struct RotateLineEffect *self = (struct RotateLineEffect *)effect;
    rotate_line_effect_destruct(self);
    mem_free(self);
}

void rotate_line_effect_release(struct Effect *effect)
{
    struct RotateLineEffect *self = (struct RotateLineEffect *)effect;
    struct Particle **it;

    EACH_PARTICLE(self, it)
        particle_free(*it);
    self->particles.last = self->particles.first;
}

void rotate_line_effect_tint(struct RotateLineEffect *self, struct Particle *p)
{
    if (!p)
        return;
    if (self->base.type == DARK_TYPE) {
        particle_set_color(p, 50, 50, 50);
        p->dest_blend = 6;
    } else if (self->base.type == BLUE_TYPE) {
        particle_set_color(p, 100, 100, 255);
        p->scale = p->scale + 3.0f;
    }
}

void rotate_line_effect_init(struct Effect *effect)
{
    struct RotateLineEffect *self = (struct RotateLineEffect *)effect;
    int ring = 0, start, column, tilt, r;

    for (start = 0; start < 360; start += 90, ring++) {
        r = rand() % 21;
        tilt = (r < 0 ? 0 : r) + 80;
        for (column = 0; column < SEGMENTS; column++) {
            struct Particle *p = particle_new(self->base.pos, LIST_WORLD);
            struct String texture;

            if (!p)
                continue;
            particle_owner_type(p) = self->base.type;
            particle_column(p) = column;
            particle_ring(p) = ring;
            particle_tag(p) = 1;
            string_init(&texture);
            string_assign_cstr(&texture, "effect\\cloud11.tga");
            particle_set_texture(p, texture);
            particle_span(p) = 30;
            p->roll = (float)start;
            p->alpha = 50;
            particle_tilt(p) = tilt;
            p->scale = 7.0f - (float)ring;
            particle_height(p) = 28.0f - (float)ring * 5.0f;
            particle_band(p) = 3.0f;
            rotate_line_effect_tint(self, p);
            vec_push(&self->particles, &p, sizeof(struct Particle *));
        }
    }
}

void rotate_line_effect_shape(struct RotateLineEffect *self, struct Particle *p)
{
    int span, column, deg, tilt;
    float s, c, low_x, low_y, low_z, high_x, high_y, high_z;

    if (!p)
        return;
    span = particle_span(p);
    column = particle_column(p);
    deg = (int)((float)((int)((float)span / 21.0f) * column) + p->roll) % 360;
    if (span == 360 && column == 20)
        deg = (int)p->roll;
    tilt = particle_tilt(p);
    s = g_sinTable[deg];
    c = g_cosTable[deg];

    low_x = c * p->scale;
    low_y = -particle_height(p);
    low_z = s * p->scale;
    high_x = g_cosTable[tilt] * c + low_x;
    high_y = (particle_band(p) - g_sinTable[tilt]) + low_y;
    high_z = g_cosTable[tilt] * s + low_z;
    p->corners[1].x = high_x;
    p->corners[1].y = high_y;
    p->corners[1].z = high_z;
    p->corners[3].x = low_x;
    p->corners[3].y = low_y;
    p->corners[3].z = low_z;
    if (column != 0) {
        p->corners[0].x = self->prev_high.x;
        p->corners[0].y = self->prev_high.y;
        p->corners[0].z = self->prev_high.z;
        p->corners[2].x = self->prev_low.x;
        p->corners[2].y = self->prev_low.y;
        p->corners[2].z = self->prev_low.z;
    }
    self->prev_low.x = low_x;
    self->prev_low.y = low_y;
    self->prev_low.z = low_z;
    self->prev_high.x = high_x;
    self->prev_high.y = high_y;
    self->prev_high.z = high_z;
}

void rotate_line_effect_update_particle(struct RotateLineEffect *self, struct Particle *p)
{
    if (!p)
        return;
    if (particle_column(p) == 0) {
        self->ring_angle = (int)p->roll;
        self->ring_span = particle_span(p);
        self->ring_alpha = p->alpha;
        self->ring_radius = p->scale;
        self->walking = 1;
    } else if (!self->walking) {
        return;
    }

    self->ring_angle += 5;
    p->roll = (float)(self->ring_angle % 360);
    self->ring_span += 3;
    if (self->ring_span > 180)
        self->ring_span = 180;
    particle_span(p) = self->ring_span;
    if (p->age <= 12) {
        p->alpha = ++self->ring_alpha;
    } else {
        self->ring_radius = self->ring_radius + 0.1f;
        p->scale = self->ring_radius;
        if (p->age > 50) {
            if (--self->ring_alpha < 0)
                self->ring_alpha = 0;
            p->alpha = self->ring_alpha;
        }
    }
    rotate_line_effect_shape(self, p);
}

void rotate_line_effect_process(struct Effect *effect)
{
    struct RotateLineEffect *self = (struct RotateLineEffect *)effect;
    struct Particle **it;

    EACH_PARTICLE(self, it) {
        struct Particle *p = *it;
        p->pos = self->base.pos;
        rotate_line_effect_update_particle(self, p);
        p->age++;
    }
}

void rotate_line_render_segment(struct Particle *p)
{
    struct ParticleRenderArgs args;
    int column;

    if (!p)
        return;
    column = particle_column(p);
    args.facing_fixed = 0;
    args.v_top = 0.0f;
    args.v_bottom = 1.0f;
    args.u_left = (float)(column - 1) / 21.0f;
    args.u_right = (float)column / 21.0f;
    args.map_chain_flag = 0;
    args.with_depth = 1;
    particle_render_with(p, &args);
}

void rotate_line_effect_render(struct Effect *effect)
{
    struct RotateLineEffect *self = (struct RotateLineEffect *)effect;
    struct Particle **it;

    EACH_PARTICLE(self, it)
        rotate_line_render_segment(*it);
}
