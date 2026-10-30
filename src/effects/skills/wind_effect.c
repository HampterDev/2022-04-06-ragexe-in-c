#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <stdlib.h>
#include <string.h>
#include "effects/skills/wind_effect.h"
#include "std/vec.h"

#define EFFECT_WIND 0x927
#define LIST_WORLD 5
#define SEGMENTS 21
#define KEEP_BLEND (-1)

struct WindParticle {
    int segment_arc;
    float segment_height;
    signed char segment_dimming;
    float segment_spin;
};
PARTICLE_EFFECT_DATA_FITS(WindParticle);
static __inline struct WindParticle *wind_particle(const struct Particle *p)
{
    return (struct WindParticle *)p->effect_data;
}

struct Effect;
struct WindEffect;
struct String;
void wind_effect_init(struct Effect *self);
void wind_effect_process(struct Effect *self);
void wind_effect_render(struct Effect *self);
void wind_effect_release(struct Effect *self);
void wind_effect_restart(struct Effect *self);
static void wind_effect_destroy(struct Effect *effect);

static const struct EffectOps wind_effect_ops = {
    .set_submit = effect_set_submit,
    .init = wind_effect_init,
    .update_range = effect_update_range,
    .process = wind_effect_process,
    .render = wind_effect_render,
    .release = wind_effect_release,
    .is_alive = effect_is_owner_alive,
    .restart = wind_effect_restart,
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
    .destroy = wind_effect_destroy,
    .fade_out = effect_fade_out,
};

static float to_radians(float deg)
{
    return deg / 180.0f * 3.141592f;
}

static void transform(Vec3 *out, const float *m, float x, float y, float z)
{
    out->x = y * m[3] + x * m[0] + z * m[6] + m[9];
    out->y = y * m[4] + x * m[1] + z * m[7] + m[10];
    out->z = y * m[5] + x * m[2] + z * m[8] + m[11];
}

static int random_below(int n)
{
    int r = rand() % n;
    return r < 0 ? 0 : r;
}

struct WindEffect *wind_effect_construct(struct WindEffect *self)
{
    effect_construct(&self->base, EFFECT_WIND);
    self->base.ops = &wind_effect_ops;
    self->rings = 4;
    self->max_alpha = 200;
    self->r = 255;
    self->g = 100;
    self->b = 100;
    self->radius = 12.0f;
    self->band_width = 0.5f;
    self->height = 50.0f;
    self->spin = 5.0f;
    self->arc = 200;
    self->tilt_x = 0;
    self->tilt_y = 0;
    self->tilt_z = 0;
    self->src_blend = 5;
    self->dest_blend = 7;
    string_init(&self->texture);
    string_assign_cstr(&self->texture, "effect\\cloud11.tga");
    self->prev_bottom.x = self->prev_bottom.y = self->prev_bottom.z = 0;
    self->prev_top.x = self->prev_top.y = self->prev_top.z = 0;
    self->segments.first = self->segments.last = self->segments.end = 0;
    return self;
}

struct Effect *wind_effect_new(void)
{
    struct WindEffect *self = mem_try_alloc(sizeof *self);

    return self ? &wind_effect_construct(self)->base : NULL;
}

void wind_effect_destruct(struct WindEffect *self)
{
    self->base.ops = &wind_effect_ops;
    vec_free(&self->segments);
    string_free(&self->texture);
    effect_destruct(&self->base);
}

static void wind_effect_destroy(struct Effect *effect)
{
    struct WindEffect *self = (struct WindEffect *)effect;
    wind_effect_destruct(self);
    mem_free(self);
}

void wind_effect_release(struct Effect *effect)
{
    struct WindEffect *self = (struct WindEffect *)effect;
    struct Particle **it;
    for (it = self->segments.first; it != (struct Particle **)self->segments.last; it++)
        particle_free(*it);
    self->segments.last = self->segments.first;
}

void wind_effect_init(struct Effect *effect)
{
    struct WindEffect *self = (struct WindEffect *)effect;
    float step = (float)self->arc / 20.0f;
    int ring;

    for (ring = 0; ring < self->rings; ring++) {
        int start_angle = random_below(360);
        int start_alpha = self->max_alpha == 0 ? 0 : random_below(self->max_alpha);
        int seg;

        for (seg = 0; seg < SEGMENTS; seg++) {
            struct Particle *p = particle_new(self->base.pos, LIST_WORLD);
            struct String texture;

            if (!p)
                continue;
            p->alpha = start_alpha;
            particle_set_color(p, self->r, self->g, self->b);
            p->scale = self->radius;
            wind_particle(p)->segment_height = self->height / (float)(self->rings - 1) * (float)ring;
            wind_particle(p)->segment_spin = self->spin;
            wind_particle(p)->segment_arc = self->arc;
            if (self->src_blend != KEEP_BLEND)
                p->src_blend = self->src_blend;
            if (self->dest_blend != KEEP_BLEND)
                p->dest_blend = self->dest_blend;
            string_copy(&texture, &self->texture);
            particle_set_texture(p, texture);
            p->roll = (float)seg * step + (float)start_angle;
            wind_particle(p)->segment_dimming = 0;
            particle_quad_unit_corners(p->corners);
            p->effect_type = self->base.type;
            p->index = seg;
            p->group = ring;
            p->owner_tag = 1;
            vec_push(&self->segments, &p, sizeof(struct Particle *));
        }
    }
}

void wind_effect_restart(struct Effect *effect)
{
    wind_effect_release(effect);
    wind_effect_init(effect);
}

void wind_effect_pulse(struct WindEffect *self, struct Particle *p)
{
    int alpha;

    if (!p)
        return;
    switch (wind_particle(p)->segment_dimming) {
    case 0:
        alpha = p->alpha + 1;
        if (!(self->max_alpha > alpha)) {
            alpha = self->max_alpha;
            wind_particle(p)->segment_dimming = 1;
        }
        p->alpha = alpha;
        break;
    case 1:
        alpha = p->alpha - 1;
        if (!(alpha > 0)) {
            alpha = 0;
            wind_particle(p)->segment_dimming = 0;
        }
        p->alpha = alpha;
        break;
    }
}

void wind_effect_update_segment(struct WindEffect *self, struct Particle *p)
{
    float m[12];
    float angle, c, s, grow, radius, bx, by, bz, tx, ty, tz;
    Vec3 here, bottom, top;

    if (!p)
        return;
    wind_effect_pulse(self, p);

    angle = wind_particle(p)->segment_spin + p->roll;
    if (angle > 360.0f)
        angle -= 360.0f;
    p->roll = angle;
    c = float_cos(to_radians(angle));
    s = float_sin(to_radians(p->roll));

    grow = self->band_width;
    radius = p->scale;
    bx = c * radius + 0.0f;
    by = radius * 0.0f - wind_particle(p)->segment_height;
    bz = s * radius + 0.0f;

    mat_set_rotate_x(m, to_radians(self->tilt_x));
    mat_rotate_z(m, to_radians(self->tilt_z));
    here = self->base.pos;
    mat_translate(m, &here);

    transform(&bottom, m, bx, by, bz);
    ty = by + grow;
    tx = (c + 0.0f) * grow + bx;
    tz = (s + 0.0f) * grow + bz;
    transform(&top, m, tx, ty, tz);

    p->corners[1].x = top.x;
    p->corners[1].y = top.y;
    p->corners[1].z = top.z;
    p->corners[3].x = bottom.x;
    p->corners[3].y = bottom.y;
    p->corners[3].z = bottom.z;
    if (p->index != 0) {
        p->corners[0].x = self->prev_top.x;
        p->corners[0].y = self->prev_top.y;
        p->corners[0].z = self->prev_top.z;
        p->corners[2].x = self->prev_bottom.x;
        p->corners[2].y = self->prev_bottom.y;
        p->corners[2].z = self->prev_bottom.z;
    }
    self->prev_bottom = bottom;
    self->prev_top = top;
}

void wind_effect_process(struct Effect *effect)
{
    struct WindEffect *self = (struct WindEffect *)effect;
    struct Particle **it;
    for (it = self->segments.first; it != (struct Particle **)self->segments.last; it++)
        wind_effect_update_segment(self, *it);
}

void wind_effect_render_segment(struct WindEffect *self, struct Particle *p)
{
    struct ParticleRenderArgs args;
    int seg;

    if (!p)
        return;
    seg = p->index;
    if (seg <= 0)
        return;
    args.facing_fixed = 1;
    args.v_top = 0.0f;
    args.v_bottom = 1.0f;
    args.u_left = (float)(seg - 1) / 21.0f;
    args.u_right = (float)seg / 21.0f;
    args.map_chain_flag = 0;
    args.with_depth = 1;
    particle_render_with(p, &args);
}

void wind_effect_render(struct Effect *effect)
{
    struct WindEffect *self = (struct WindEffect *)effect;
    struct Particle **it;
    for (it = self->segments.first; it != (struct Particle **)self->segments.last; it++)
        wind_effect_render_segment(self, *it);
}
