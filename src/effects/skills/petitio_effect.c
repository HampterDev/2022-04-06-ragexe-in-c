#include "effects/effect_allocator.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/vec.h"

#define EFFECT_PETITIO 0x777
#define LIST_WORLD 5
#define LIST_GROUND 0xd
#define HIT_FRAME 6
#define LIFT 12.0f

struct PetitioEffect {
    struct Effect base;
    struct Vec parts;
};

void petitio_effect_init(struct Effect *self);
void petitio_effect_process(struct Effect *self);
void petitio_effect_render(struct Effect *self);
void petitio_effect_release(struct Effect *self);
unsigned char petitio_effect_is_alive(struct Effect *self);
static void petitio_effect_destroy(struct Effect *effect);

static const struct EffectOps petitio_effect_ops = {
    .set_submit = effect_set_submit,
    .init = petitio_effect_init,
    .update_range = effect_update_range,
    .process = petitio_effect_process,
    .render = petitio_effect_render,
    .release = petitio_effect_release,
    .is_alive = petitio_effect_is_alive,
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
    .destroy = petitio_effect_destroy,
    .fade_out = effect_fade_out,
};

static const Vec3 no_offset = { 0, 0, 0 };

static void render_with_view(struct Ez2StrParticle *p)
{
    ez2_str_particle_render_current(p);
}

static struct Ez2StrParticle *open_part(struct PetitioEffect *self, int list_flags, const char *name)
{
    struct Ez2StrParticle *p = ez2str_particle_new(self->base.pos, list_flags);

    ez2_str_particle_open(p, name);
    vec_push(&self->parts, &p, sizeof(struct Ez2StrParticle *));
    return p;
}

struct PetitioEffect *petitio_effect_construct(struct PetitioEffect *self)
{
    effect_construct(&self->base, EFFECT_PETITIO);
    self->base.ops = &petitio_effect_ops;
    self->parts.first = self->parts.last = self->parts.end = 0;
    return self;
}

struct Effect *petitio_effect_new(void)
{
    struct PetitioEffect *self = mem_try_alloc(sizeof *self);

    return self ? &petitio_effect_construct(self)->base : NULL;
}

void petitio_effect_destruct(struct PetitioEffect *self)
{
    self->base.ops = &petitio_effect_ops;
    vec_free(&self->parts);
    effect_destruct(&self->base);
}

static void petitio_effect_destroy(struct Effect *effect)
{
    struct PetitioEffect *self = (struct PetitioEffect *)effect;
    petitio_effect_destruct(self);
    mem_free(self);
}

void petitio_effect_release(struct Effect *effect)
{
    struct PetitioEffect *self = (struct PetitioEffect *)effect;
    struct Ez2StrParticle **first = self->parts.first;

    while (first != (struct Ez2StrParticle **)self->parts.last) {
        particle_free(&(*first)->base);
        memmove(first, first + 1, (char *)self->parts.last - (char *)(first + 1));
        self->parts.last = (struct Ez2StrParticle **)self->parts.last - 1;
    }
}

void petitio_effect_init(struct Effect *effect)
{
    struct PetitioEffect *self = (struct PetitioEffect *)effect;
    open_part(self, LIST_GROUND, "petitio\\petitio_bottom\\petitio_bottom.str");
    open_part(self, LIST_WORLD, "petitio\\petitio\\petitio.str");
    effect_add_sound_cue(&self->base, SOUND_CUE_GAME_MESSAGE, 0, HIT_FRAME, 0.0f);
    effect_add_sound_cue(&self->base, SOUND_CUE_WAVE, "effect\\CD_PETITIO.wav", 0, 0.0f);
}

unsigned char petitio_effect_is_alive(struct Effect *effect)
{
    struct PetitioEffect *self = (struct PetitioEffect *)effect;
    struct Ez2StrParticle **it;
    for (it = self->parts.first; it != (struct Ez2StrParticle **)self->parts.last; it++) {
        float life = (*it)->base.life_seconds;
        if (life == 0.0f)
            return 1;
        if (life * 60.0f > (float)(*it)->base.age)
            return 1;
    }
    return 0;
}

void petitio_effect_process_part(struct PetitioEffect *self, struct Ez2StrParticle *p)
{
    if (p) {
        Vec3 here = self->base.pos;
        p->base.pos.x = here.x + 0.0f;
        p->base.pos.y = here.y - LIFT;
        p->base.pos.z = here.z + 0.0f;
        ez2_str_particle_process(p, no_offset);
    }
}

void petitio_effect_process(struct Effect *effect)
{
    struct PetitioEffect *self = (struct PetitioEffect *)effect;
    struct Ez2StrParticle **it;
    for (it = self->parts.first; it != (struct Ez2StrParticle **)self->parts.last; it++)
        petitio_effect_process_part(self, *it);
}

void petitio_effect_render(struct Effect *effect)
{
    struct PetitioEffect *self = (struct PetitioEffect *)effect;
    struct Ez2StrParticle **it;
    for (it = self->parts.first; it != (struct Ez2StrParticle **)self->parts.last; it++)
        if (*it)
            render_with_view(*it);
}
