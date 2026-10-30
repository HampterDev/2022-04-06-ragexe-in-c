#include "effects/effect_allocator.h"
#include <stdlib.h>
#include <string.h>
#include "effects/skills/emitter_effect.h"
#include "std/string_vec.h"

#define ANIM_SETS 2
#define ANIM_FRAMES 5
#define ANIM_FRAME_TICKS 8

struct AnimatedEmitterEffect {
    struct EmitterEffect base;
    struct Vec sets[ANIM_SETS];
};

struct AnimatedEmitterParticle {
    struct EmitterParticle base;
    float anim_set;
};
PARTICLE_EFFECT_DATA_FITS(AnimatedEmitterParticle);
static __inline struct AnimatedEmitterParticle *animated_emitter_particle(const struct Particle *p)
{
    return (struct AnimatedEmitterParticle *)p->effect_data;
}

struct Effect;
struct Particle;
struct String;
void animated_emitter_effect_init(struct Effect *self);
static void animated_emitter_effect_destroy(struct Effect *effect);
void animated_emitter_effect_setup_particle(struct EmitterEffect *self, struct Particle *p);
void animated_emitter_effect_step_particle(struct EmitterEffect *self, struct Particle *p);

static const struct EmitterEffectOps animated_emitter_effect_ops = {
    .base = {
        .set_submit = effect_set_submit,
        .init = animated_emitter_effect_init,
        .update_range = emitter_effect_update_range,
        .process = emitter_effect_process,
        .render = emitter_effect_render,
        .release = emitter_effect_release,
        .is_alive = emitter_effect_is_alive,
        .restart = emitter_effect_restart,
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
        .destroy = animated_emitter_effect_destroy,
        .fade_out = effect_fade_out,
    },
    .init_particle = animated_emitter_effect_setup_particle,
    .process_particle = animated_emitter_effect_step_particle,
};

static void push_name(struct Vec *v, const char *name)
{
    string_vec_push_text(v, name, strlen(name));
}

struct AnimatedEmitterEffect *animated_emitter_effect_construct(
    struct AnimatedEmitterEffect *self)
{
    struct EmitterEffect *e = &self->base;
    int i;

    emitter_effect_construct(e);
    e->base.ops = &animated_emitter_effect_ops.base;
    for (i = 0; i < ANIM_SETS; i++)
        self->sets[i].first = self->sets[i].last = self->sets[i].end = 0;
    e->acceleration.x = e->acceleration.y = e->acceleration.z = 0;
    e->velocity_min.x = -3.0f; e->velocity_min.y = -1.0f; e->velocity_min.z = -3.0f;
    e->velocity_max.x = 3.0f;  e->velocity_max.y = 1.0f;  e->velocity_max.z = 3.0f;
    e->spawn_box.x = e->spawn_box.y = e->spawn_box.z = 0;
    e->size_min = 10.0f;
    e->size_max = 20.0f;
    e->life_min = 3.0f;
    e->life_max = 4.0f;
    e->rate_min = 1.0f;
    e->rate_max = 3.0f;
    e->count = 4;
    return self;
}

struct Effect *animated_emitter_effect_new(void)
{
    struct AnimatedEmitterEffect *self = mem_try_alloc(sizeof *self);

    return self ? &animated_emitter_effect_construct(self)->base.base : NULL;
}

void animated_emitter_effect_destruct(struct AnimatedEmitterEffect *self)
{
    int i;
    self->base.base.ops = &animated_emitter_effect_ops.base;
    for (i = ANIM_SETS - 1; i >= 0; i--)
        string_vec_free(&self->sets[i]);
    emitter_effect_destruct(&self->base);
}

static void animated_emitter_effect_destroy(struct Effect *effect)
{
    struct AnimatedEmitterEffect *self = (struct AnimatedEmitterEffect *)effect;
    animated_emitter_effect_destruct(self);
    mem_free(self);
}

void animated_emitter_effect_init(struct Effect *effect)
{
    struct AnimatedEmitterEffect *self = (struct AnimatedEmitterEffect *)effect;
    push_name(&self->sets[0], "EFFECT\\shockwave_a.bmp");
    push_name(&self->sets[0], "EFFECT\\shockwave_b.bmp");
    push_name(&self->sets[0], "EFFECT\\shockwave_c.bmp");
    push_name(&self->sets[0], "EFFECT\\shockwave_d.bmp");
    push_name(&self->sets[0], "EFFECT\\shockwave_e.bmp");
    push_name(&self->sets[1], "EFFECT\\plazma_a.bmp");
    push_name(&self->sets[1], "EFFECT\\plazma_b.bmp");
    push_name(&self->sets[1], "EFFECT\\plazma_c.bmp");
    push_name(&self->sets[1], "EFFECT\\plazma_a.bmp");
    push_name(&self->sets[1], "EFFECT\\plazma_b.bmp");
    emitter_effect_init(&self->base.base);
}

void animated_emitter_effect_setup_particle(struct EmitterEffect *effect,
                                                      struct Particle *p)
{
    animated_emitter_particle(p)->anim_set = (float)(rand() % ANIM_SETS);
    emitter_effect_init_particle(effect, p);
}

void animated_emitter_effect_step_particle(struct EmitterEffect *effect,
                                                     struct Particle *p)
{
    struct AnimatedEmitterEffect *self = (struct AnimatedEmitterEffect *)effect;
    const struct String *names = self->sets[(int)animated_emitter_particle(p)->anim_set].first;
    struct String name;

    string_copy(&name, &names[p->age / ANIM_FRAME_TICKS % ANIM_FRAMES]);
    particle_set_texture(p, name);
    emitter_effect_process_particle(&self->base, p);
}
