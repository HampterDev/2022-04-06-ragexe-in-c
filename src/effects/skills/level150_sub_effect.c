#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <stdlib.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/vec.h"

struct Level150SubEffect {
    struct Effect base;
    struct Vec rings;
};

struct String;
void level150_sub_effect_init(struct Effect *self);
void level150_sub_effect_process(struct Effect *self);
void level150_sub_effect_render(struct Effect *self);
void level150_sub_effect_release(struct Effect *self);
void level150_sub_effect_set_color(struct Effect *self, int r, int g, int b);
static void level150_sub_effect_destroy(struct Effect *effect);

static const struct EffectOps level150_sub_effect_ops = {
    .set_submit = effect_set_submit,
    .init = level150_sub_effect_init,
    .update_range = effect_update_range,
    .process = level150_sub_effect_process,
    .render = level150_sub_effect_render,
    .release = level150_sub_effect_release,
    .is_alive = effect_is_owner_alive,
    .restart = effect_restart,
    .set_color = level150_sub_effect_set_color,
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
    .destroy = level150_sub_effect_destroy,
    .fade_out = effect_fade_out,
};

#define LEVEL150_RING_LIST 0xd

#define RING_EFFECT_TYPE(p) ((p)->effect_type)
struct Level150SubParticle {
    float ring_size;
    float ring_height;
};
PARTICLE_EFFECT_DATA_FITS(Level150SubParticle);
static __inline struct Level150SubParticle *level150_sub_particle(const struct Particle *p)
{
    return (struct Level150SubParticle *)p->effect_data;
}
#define RING_COLUMN(p)      ((p)->index)

static const int ring_colors_blue[4][3]   = { {105, 105, 255}, {5, 5, 255}, {55, 55, 255}, {155, 155, 255} };
static const int ring_colors_yellow[4][3] = { {255, 255, 105}, {255, 255, 5}, {255, 255, 55}, {255, 255, 155} };
static const int ring_colors_red[4][3]    = { {255, 105, 105}, {255, 5, 5}, {255, 55, 55}, {255, 155, 155} };

static float random_roll(void)
{
    int deg = rand() % 360;
    return (float)(deg < 0 ? 0 : deg);
}

void level150_ring_place(struct Particle *ring)
{
    float roll = ring->roll;
    float size = level150_sub_particle(ring)->ring_size;
    float height = level150_sub_particle(ring)->ring_height;
    float turn[12], scale[12], move[12], tmp[12], m[12];

    mat_set_rotate_y(turn, (float)(int)roll / 180.0f * 3.141592f
                        - (float)(int)game_camera_heading() / 180.0f * 3.141592f);
    mat_set_scale(scale, size * 0.5f, size * 0.5f, 1.0f);
    mat_set_translation(move, 0.0f, -height, 0.0f);
    mat_multiply(tmp, scale, turn);
    mat_multiply(m, tmp, move);
    memcpy(ring->matrix, m, sizeof(m));
}

void level150_ring_update(struct Particle *ring)
{
    float size;

    if (!ring || ring->age <= 0)
        return;
    size = (float)(level150_sub_particle(ring)->ring_size * 1.02);
    if (size > 20.0f) {
        int alpha = ring->alpha - 3;
        if (alpha <= 0) {
            alpha = 200;
            ring->roll = random_roll();
            size = 10.0f;
        }
        ring->alpha = alpha;
    }
    level150_sub_particle(ring)->ring_size = size;
    level150_ring_place(ring);
}

void level150_ring_render(struct Particle *ring)
{
    particle_render(ring, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
}

struct Level150SubEffect *level150_sub_effect_construct(struct Level150SubEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &level150_sub_effect_ops;
    self->rings.first = self->rings.last = self->rings.end = 0;
    return self;
}

struct Effect *level150_sub_effect_new(void)
{
    struct Level150SubEffect *self = mem_try_alloc(sizeof *self);

    return self ? &level150_sub_effect_construct(self)->base : NULL;
}

/* Effect types created as Level150SubEffect, registered at startup through g_effectTypes
   (effects/effect_registry.c). A type missing here is not created. */
static const struct EffectTypeRow level150_sub_effect_type_rows[] = {
    EFFECT_TYPE(0x3d3, level150_sub_effect_new),
    EFFECT_TYPE(0x3ff, level150_sub_effect_new),
    EFFECT_TYPE(0x4cb, level150_sub_effect_new),
    EFFECT_TYPE(0x8e4, level150_sub_effect_new),
};
const struct EffectTypeTable level150_sub_effect_types = EFFECT_TYPE_TABLE(level150_sub_effect_type_rows);

void level150_sub_effect_destruct(struct Level150SubEffect *self)
{
    self->base.ops = &level150_sub_effect_ops;
    vec_free(&self->rings);
    effect_destruct(&self->base);
}

static void level150_sub_effect_destroy(struct Effect *effect)
{
    struct Level150SubEffect *self = (struct Level150SubEffect *)effect;
    level150_sub_effect_destruct(self);
    mem_free(self);
}

void level150_sub_effect_init(struct Effect *effect)
{
    struct Level150SubEffect *self = (struct Level150SubEffect *)effect;
    int type = self->base.type;
    int row = 0, start_age = 0, column;

    do {
        for (column = 0; column < 2; column++) {
            struct Particle *ring = particle_new(self->base.pos, LEVEL150_RING_LIST);
            const int *rgb;
            struct String texture;

            if (!ring)
                continue;
            string_init(&texture);
            string_assign_cstr(&texture, "effect\\emp shock.tga");
            particle_set_texture(ring, texture);
            ring->roll = random_roll();
            ring->alpha = 200;
            level150_sub_particle(ring)->ring_size = 10.0f;
            level150_sub_particle(ring)->ring_height = 10.0f;
            RING_EFFECT_TYPE(ring) = self->base.type;
            RING_COLUMN(ring) = column;
            ring->group = row;
            ring->owner_tag = 1;
            ring->age = start_age;

            vec_push(&self->rings, &ring, sizeof(struct Particle *));

            if (type == 0x3d3)
                rgb = ring_colors_blue[row];
            else if (type == 0x3ff)
                rgb = ring_colors_yellow[row];
            else if (type == 0x4cb)
                rgb = ring_colors_red[row];
            else if (type == 0x8e4) {
                particle_set_color(ring, 255, 155, 0);
                continue;
            } else
                continue;
            particle_set_color(ring, (short)rgb[0], (short)rgb[1], (short)rgb[2]);
        }
        row++;
        start_age -= 20;
    } while (start_age > -80);
}

void level150_sub_effect_process(struct Effect *effect)
{
    struct Level150SubEffect *self = (struct Level150SubEffect *)effect;
    struct Particle **it;

    for (it = self->rings.first; it != (struct Particle **)self->rings.last; it++) {
        struct Particle *ring = *it;
        ring->pos = self->base.pos;
        level150_ring_update(ring);
        ring->age++;
    }
}

void level150_sub_effect_render(struct Effect *effect)
{
    struct Level150SubEffect *self = (struct Level150SubEffect *)effect;
    struct Particle **it;

    for (it = self->rings.first; it != (struct Particle **)self->rings.last; it++)
        level150_ring_render(*it);
}

void level150_sub_effect_release(struct Effect *effect)
{
    struct Level150SubEffect *self = (struct Level150SubEffect *)effect;
    struct Particle **it;

    for (it = self->rings.first; it != (struct Particle **)self->rings.last; it++)
        particle_free(*it);
    self->rings.last = self->rings.first;
}

void level150_sub_effect_set_color(struct Effect *effect, int r, int g, int b)
{
    struct Level150SubEffect *self = (struct Level150SubEffect *)effect;
    struct Particle **it;

    for (it = self->rings.first; it != (struct Particle **)self->rings.last; it++)
        particle_set_color(*it, (short)r, (short)g, (short)b);
}
