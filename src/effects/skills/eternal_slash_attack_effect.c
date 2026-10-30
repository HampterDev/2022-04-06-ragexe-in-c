#include "effects/effect_allocator.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/vec.h"

struct EternalSlashAttackEffect {
    struct Effect base;
    struct Vec parts;
};

#define PARTICLE_OFFSET(p) ((p)->base.offset)

void eternal_slash_attack_effect_init(struct Effect *self);
void eternal_slash_attack_effect_process(struct Effect *self);
void eternal_slash_attack_effect_render(struct Effect *self);
void eternal_slash_attack_effect_release(struct Effect *self);
unsigned char eternal_slash_attack_effect_is_alive(struct Effect *self);
static void eternal_slash_attack_effect_destroy(struct Effect *effect);

static const struct EffectOps eternal_slash_attack_effect_ops = {
    .set_submit = effect_set_submit,
    .init = eternal_slash_attack_effect_init,
    .update_range = effect_update_range,
    .process = eternal_slash_attack_effect_process,
    .render = eternal_slash_attack_effect_render,
    .release = eternal_slash_attack_effect_release,
    .is_alive = eternal_slash_attack_effect_is_alive,
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
    .destroy = eternal_slash_attack_effect_destroy,
    .fade_out = effect_fade_out,
};

static int particle_alive(const struct Ez2StrParticle *p)
{
    float life = p->base.life_seconds;
    if (life == 0.0f)
        return 1;
    return life * 60.0f > (float)p->base.age;
}

static void add_part(struct EternalSlashAttackEffect *self, int list_flags, const char *name)
{
    struct Ez2StrParticle *p = ez2str_particle_new(self->base.pos, list_flags);

    if (!p)
        return;
    ez2_str_particle_open(p, name);
    PARTICLE_OFFSET(p).x = 0.0f;
    PARTICLE_OFFSET(p).y = -13.0f;
    PARTICLE_OFFSET(p).z = 0.0f;
    vec_push(&self->parts, &p, sizeof(struct Ez2StrParticle *));
}

static void step_part(const struct EternalSlashAttackEffect *self, struct Ez2StrParticle *p)
{
    Vec3 at = self->base.pos, zero = { 0.0f, 0.0f, 0.0f };

    p->base.pos.x = PARTICLE_OFFSET(p).x + at.x;
    p->base.pos.y = PARTICLE_OFFSET(p).y + at.y;
    p->base.pos.z = PARTICLE_OFFSET(p).z + at.z;
    ez2_str_particle_process(p, zero);
}

struct EternalSlashAttackEffect *
eternal_slash_attack_effect_construct(struct EternalSlashAttackEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &eternal_slash_attack_effect_ops;
    self->parts.first = self->parts.last = self->parts.end = 0;
    return self;
}

struct Effect *eternal_slash_attack_effect_new(void)
{
    struct EternalSlashAttackEffect *self = mem_try_alloc(sizeof *self);

    return self ? &eternal_slash_attack_effect_construct(self)->base : NULL;
}

void eternal_slash_attack_effect_destruct(struct EternalSlashAttackEffect *self)
{
    self->base.ops = &eternal_slash_attack_effect_ops;
    vec_free(&self->parts);
    effect_destruct(&self->base);
}

static void eternal_slash_attack_effect_destroy(struct Effect *effect)
{
    struct EternalSlashAttackEffect *self = (struct EternalSlashAttackEffect *)effect;
    eternal_slash_attack_effect_destruct(self);
    mem_free(self);
}

void eternal_slash_attack_effect_release(struct Effect *effect)
{
    struct EternalSlashAttackEffect *self = (struct EternalSlashAttackEffect *)effect;
    struct Particle **it;

    for (it = self->parts.first; it != (struct Particle **)self->parts.last; it++)
        particle_free(*it);
    self->parts.last = self->parts.first;
}

void eternal_slash_attack_effect_init(struct Effect *effect)
{
    struct EternalSlashAttackEffect *self = (struct EternalSlashAttackEffect *)effect;
    add_part(self, 13, "eternal_slash\\eternal_slash_bottom\\eternal_slash_bottom.str");
    add_part(self, 5, "eternal_slash\\eternal_slash\\eternal_slash.str");
}

unsigned char eternal_slash_attack_effect_is_alive(struct Effect *effect)
{
    struct EternalSlashAttackEffect *self = (struct EternalSlashAttackEffect *)effect;
    struct Ez2StrParticle **it;

    for (it = self->parts.first; it != (struct Ez2StrParticle **)self->parts.last; it++)
        if (*it && particle_alive(*it))
            return 1;
    return 0;
}

void eternal_slash_attack_effect_process(struct Effect *effect)
{
    struct EternalSlashAttackEffect *self = (struct EternalSlashAttackEffect *)effect;
    struct Ez2StrParticle **it;

    for (it = self->parts.first; it != (struct Ez2StrParticle **)self->parts.last; it++)
        if (*it)
            step_part(self, *it);
}

void eternal_slash_attack_effect_render(struct Effect *effect)
{
    struct EternalSlashAttackEffect *self = (struct EternalSlashAttackEffect *)effect;
    struct Ez2StrParticle **it;

    for (it = self->parts.first; it != (struct Ez2StrParticle **)self->parts.last; it++)
        if (*it) {
            ez2_str_particle_render_current(*it);
        }
}
