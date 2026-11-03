#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <string.h>
#include "effects/particle.h"
#include "std/vec.h"

struct SquareRangeEffect {
    struct Effect base;
    float size;
    unsigned char size_changed;
    struct Vec squares;
};

struct String;
void square_range_effect_init(struct Effect *self);
void square_range_effect_process(struct Effect *self);
void square_range_effect_render(struct Effect *self);
void square_range_effect_release(struct Effect *self);
unsigned char square_range_effect_is_alive(struct Effect *self);
static void square_range_effect_destroy(struct Effect *effect);

static const struct EffectOps square_range_effect_ops = {
    .set_submit = effect_set_submit,
    .init = square_range_effect_init,
    .update_range = effect_update_range,
    .process = square_range_effect_process,
    .render = square_range_effect_render,
    .release = square_range_effect_release,
    .is_alive = square_range_effect_is_alive,
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
    .destroy = square_range_effect_destroy,
    .fade_out = effect_fade_out,
};

#define SQUARE_RANGE_LIST 5
#define EFFECT_GREY_SQUARE 0x3df

void square_range_effect_make_grey(struct SquareRangeEffect *self, struct Particle *square)
{
    if (self->base.type != EFFECT_GREY_SQUARE)
        return;
    self->base.duration = 300;
    particle_set_color(square, 92, 92, 92);
    square->dest_blend = 6;
}

void square_range_effect_init_size(struct SquareRangeEffect *self)
{
    if (self->base.type == EFFECT_GREY_SQUARE)
        self->size = 7.0f;
}

void square_range_effect_update_square(struct SquareRangeEffect *self, struct Particle *square)
{
    float turn[12], scale[12], m[12];

    if (!square)
        return;
    if (self->size_changed) {
        square->scale = self->size * 2.5f;
        self->size_changed = 0;
    }
    mat_set_rotate_x(turn, 1.570796f);
    mat_set_scale(scale, square->scale, square->scale, 1.0f);
    mat_multiply(m, scale, turn);
    memcpy(square->matrix, m, sizeof(m));
}

void square_range_render(struct Particle *square)
{
    if (square)
        particle_render(square, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
}

struct SquareRangeEffect *square_range_effect_construct(struct SquareRangeEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &square_range_effect_ops;
    self->size = 1.0f;
    self->size_changed = 0;
    self->squares.first = self->squares.last = self->squares.end = 0;
    return self;
}

struct Effect *square_range_effect_new(void)
{
    struct SquareRangeEffect *self = mem_try_alloc(sizeof *self);

    return self ? &square_range_effect_construct(self)->base : NULL;
}

/* Effect types created as SquareRangeEffect, registered at startup through g_effectTypes
   (effects/effect_registry.c). A type missing here is not created. */
static const struct EffectTypeRow square_range_effect_type_rows[] = {
    EFFECT_TYPE(0x3df, square_range_effect_new),
};
const struct EffectTypeTable square_range_effect_types = EFFECT_TYPE_TABLE(square_range_effect_type_rows);

void square_range_effect_destruct(struct SquareRangeEffect *self)
{
    self->base.ops = &square_range_effect_ops;
    vec_free(&self->squares);
    effect_destruct(&self->base);
}

static void square_range_effect_destroy(struct Effect *effect)
{
    struct SquareRangeEffect *self = (struct SquareRangeEffect *)effect;
    square_range_effect_destruct(self);
    mem_free(self);
}

void square_range_effect_init(struct Effect *effect)
{
    struct SquareRangeEffect *self = (struct SquareRangeEffect *)effect;
    struct Particle *square;
    struct String texture;

    square_range_effect_init_size(self);
    square = particle_new(self->base.pos, SQUARE_RANGE_LIST);
    if (!square)
        return;
    square->alpha = 255;
    particle_set_color(square, 255, 255, 255);
    string_init(&texture);
    string_assign_cstr(&texture, "effect\\SquareRange.tga");
    particle_set_texture(square, texture);
    square->scale = self->size * 2.5f;
    square_range_effect_make_grey(self, square);

    vec_push(&self->squares, &square, sizeof(struct Particle *));
}

unsigned char square_range_effect_is_alive(struct Effect *effect)
{
    if (effect->type == EFFECT_GREY_SQUARE)
        return !effect_is_finished(effect);
    return 1;
}

void square_range_effect_process(struct Effect *effect)
{
    struct SquareRangeEffect *self = (struct SquareRangeEffect *)effect;
    struct Particle **it;

    for (it = self->squares.first; it != (struct Particle **)self->squares.last; it++) {
        struct Particle *square = *it;
        square->pos = self->base.pos;
        square_range_effect_update_square(self, square);
        square->age++;
    }
}

void square_range_effect_render(struct Effect *effect)
{
    struct SquareRangeEffect *self = (struct SquareRangeEffect *)effect;
    struct Particle **it;

    for (it = self->squares.first; it != (struct Particle **)self->squares.last; it++)
        square_range_render(*it);
}

void square_range_effect_release(struct Effect *effect)
{
    struct SquareRangeEffect *self = (struct SquareRangeEffect *)effect;
    struct Particle **it;

    for (it = self->squares.first; it != (struct Particle **)self->squares.last; it++)
        particle_free(*it);
    self->squares.last = self->squares.first;
}
