#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/vec.h"

#define EFFECT_BIND_TRAP_SMALL 1069

struct BindTrapEffect {
    struct Effect base;
    struct Vec quads;
};

struct String;
void bind_trap_effect_init(struct Effect *self);
void bind_trap_effect_process(struct Effect *self);
void bind_trap_effect_render(struct Effect *self);
void bind_trap_effect_release(struct Effect *self);
static void bind_trap_effect_destroy(struct Effect *effect);

static const struct EffectOps bind_trap_effect_ops = {
    .set_submit = effect_set_submit,
    .init = bind_trap_effect_init,
    .update_range = effect_update_range,
    .process = bind_trap_effect_process,
    .render = bind_trap_effect_render,
    .release = bind_trap_effect_release,
    .is_alive = effect_is_owner_alive,
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
    .destroy = bind_trap_effect_destroy,
    .fade_out = effect_fade_out,
};

struct BindTrapEffect *bind_trap_effect_construct(struct BindTrapEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &bind_trap_effect_ops;
    self->quads.first = self->quads.last = self->quads.end = 0;
    self->base.duration = 8000;
    return self;
}

struct Effect *bind_trap_effect_new(void)
{
    struct BindTrapEffect *self = mem_try_alloc(sizeof *self);

    return self ? &bind_trap_effect_construct(self)->base : NULL;
}

void bind_trap_effect_destruct(struct BindTrapEffect *self)
{
    self->base.ops = &bind_trap_effect_ops;
    vec_free(&self->quads);
    effect_destruct(&self->base);
}

static void bind_trap_effect_destroy(struct Effect *effect)
{
    struct BindTrapEffect *self = (struct BindTrapEffect *)effect;
    bind_trap_effect_destruct(self);
    mem_free(self);
}

void bind_trap_effect_add_quad(struct BindTrapEffect *self, int level)
{
    struct String texture;
    struct Particle *p = particle_new(self->base.pos, 5);

    if (!p)
        return;
    string_init(&texture);
    string_assign_cstr(&texture, "effect\\hero_011.tga");
    particle_set_texture(p, texture);
    p->effect_type = self->base.type;
    p->index = 0;
    p->group = 0;
    p->owner_tag = 1;
    p->alpha = 255;
    p->roll = 0;
    p->scale = (float)level * 6.0f + 9.0f;
    particle_quad_unit_corners(p->corners);

    vec_push(&self->quads, &p, sizeof(struct Particle *));
}

void bind_trap_effect_release(struct Effect *effect)
{
    struct BindTrapEffect *self = (struct BindTrapEffect *)effect;
    struct Particle **it;
    for (it = self->quads.first; it != (struct Particle **)self->quads.last; it++)
        particle_free(*it);
    self->quads.last = self->quads.first;
}

void bind_trap_effect_init(struct Effect *effect)
{
    struct BindTrapEffect *self = (struct BindTrapEffect *)effect;
    switch (self->base.type) {
    case EFFECT_BIND_TRAP_SMALL:     bind_trap_effect_add_quad(self, 0); break;
    case EFFECT_BIND_TRAP_SMALL + 1: bind_trap_effect_add_quad(self, 1); break;
    case EFFECT_BIND_TRAP_SMALL + 2: bind_trap_effect_add_quad(self, 2); break;
    }
}

static void lay_flat(struct Particle *p)
{
    float turn[12], scale[12], m[12];
    float camera = game_camera_heading();

    mat_set_rotate_x(turn, 3.141592f / 2.0f);
    mat_rotate_y(turn, camera / -180.0f * 3.141592f);
    mat_set_scale(scale, p->scale, p->scale, 1.0f);
    mat_multiply(m, scale, turn);
    memcpy(p->matrix, m, sizeof m);
}

void bind_trap_effect_process(struct Effect *effect)
{
    struct BindTrapEffect *self = (struct BindTrapEffect *)effect;
    struct Particle **it;
    for (it = self->quads.first; it != (struct Particle **)self->quads.last; it++) {
        struct Particle *p = *it;
        p->pos = self->base.pos;
        if (p)
            lay_flat(p);
        p->age++;
    }
}

void bind_trap_effect_render(struct Effect *effect)
{
    struct BindTrapEffect *self = (struct BindTrapEffect *)effect;
    struct Particle **it;
    for (it = self->quads.first; it != (struct Particle **)self->quads.last; it++)
        if (*it)
            particle_render(*it, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
}
