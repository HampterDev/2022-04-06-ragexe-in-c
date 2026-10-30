#include "effects/effect_allocator.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/vec.h"

#define EFFECT_DANCING_KNIFE 0x725

struct DancingKnifeEffect {
    struct Effect base;
    struct Vec opening;
    struct Vec looping;
    unsigned char loop_started;
};

#define OPENING_BOTTOM "dancing_knife\\dancing_knife_bottom\\dancing_knife_skill_bottom_01.str"
#define OPENING_KNIVES "dancing_knife\\dancing_knife\\dancing_knife_skill_01.str"
#define LOOPING_BOTTOM "dancing_knife\\dancing_knife_bottom\\dancing_knife_skill_bottom_02.str"
#define LOOPING_KNIVES "dancing_knife\\dancing_knife\\dancing_knife_skill_02.str"

void dancing_knife_effect_init(struct Effect *self);
void dancing_knife_effect_process(struct Effect *self);
void dancing_knife_effect_render(struct Effect *self);
void dancing_knife_effect_release(struct Effect *self);
static void dancing_knife_effect_destroy(struct Effect *effect);

static const struct EffectOps dancing_knife_effect_ops = {
    .set_submit = effect_set_submit,
    .init = dancing_knife_effect_init,
    .update_range = effect_update_range,
    .process = dancing_knife_effect_process,
    .render = dancing_knife_effect_render,
    .release = dancing_knife_effect_release,
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
    .destroy = dancing_knife_effect_destroy,
    .fade_out = effect_fade_out,
};

static int still_playing(const struct Ez2StrParticle *p)
{
    return p->base.life_seconds == 0.0f || p->base.life_seconds * 60.0f > (float)p->base.age;
}

static struct Ez2StrParticle *open_str(struct DancingKnifeEffect *self, int list_flags, const char *path)
{
    struct Ez2StrParticle *p = ez2str_particle_new(self->base.pos, list_flags);

    ez2_str_particle_open(p, path);
    return p;
}

void dancing_knife_effect_start_loop(struct DancingKnifeEffect *self)
{
    struct Ez2StrParticle *p;

    self->loop_started = 1;
    p = open_str(self, 13, LOOPING_BOTTOM);
    p->loop_forever = 1;
    p->actor_aid = self->base.owner_aid;
    vec_push(&self->looping, &p, sizeof p);
    p = open_str(self, 5, LOOPING_KNIVES);
    p->loop_forever = 1;
    p->actor_aid = self->base.owner_aid;
    vec_push(&self->looping, &p, sizeof p);
}

void dancing_knife_effect_start_opening(struct DancingKnifeEffect *self)
{
    struct Ez2StrParticle *p;

    p = open_str(self, 13, OPENING_BOTTOM);
    vec_push(&self->opening, &p, sizeof p);
    p = open_str(self, 5, OPENING_KNIVES);
    vec_push(&self->opening, &p, sizeof p);
}

void dancing_knife_effect_step_part(struct DancingKnifeEffect *self, struct Ez2StrParticle *p)
{
    Vec3 zero = { 0, 0, 0 }, here;

    if (!p)
        return;
    here = self->base.pos;
    p->base.pos.x = here.x + 0.0f;
    p->base.pos.y = here.y - 12.0f;
    p->base.pos.z = here.z + 0.0f;
    ez2_str_particle_process(p, zero);
}

struct DancingKnifeEffect *dancing_knife_effect_construct(struct DancingKnifeEffect *self)
{
    effect_construct(&self->base, EFFECT_DANCING_KNIFE);
    self->base.ops = &dancing_knife_effect_ops;
    self->opening.first = self->opening.last = self->opening.end = 0;
    self->looping.first = self->looping.last = self->looping.end = 0;
    return self;
}

struct Effect *dancing_knife_effect_new(void)
{
    struct DancingKnifeEffect *self = mem_try_alloc(sizeof *self);

    return self ? &dancing_knife_effect_construct(self)->base : NULL;
}

void dancing_knife_effect_destruct(struct DancingKnifeEffect *self)
{
    self->base.ops = &dancing_knife_effect_ops;
    vec_free(&self->looping);
    vec_free(&self->opening);
    effect_destruct(&self->base);
}

static void dancing_knife_effect_destroy(struct Effect *effect)
{
    struct DancingKnifeEffect *self = (struct DancingKnifeEffect *)effect;
    dancing_knife_effect_destruct(self);
    mem_free(self);
}

void dancing_knife_effect_release(struct Effect *effect)
{
    struct DancingKnifeEffect *self = (struct DancingKnifeEffect *)effect;
    struct Ez2StrParticle **it;

    for (it = self->opening.first; it != (struct Ez2StrParticle **)self->opening.last; it++)
        particle_free(&(*it)->base);
    self->opening.last = self->opening.first;
    for (it = self->looping.first; it != (struct Ez2StrParticle **)self->looping.last; it++)
        particle_free(&(*it)->base);
    self->looping.last = self->looping.first;
}

void dancing_knife_effect_init(struct Effect *effect)
{
    struct DancingKnifeEffect *self = (struct DancingKnifeEffect *)effect;
    self->loop_started = 0;
    dancing_knife_effect_start_opening(self);
    effect_add_sound_cue(&self->base, SOUND_CUE_WAVE, "effect\\SHC_DANCING_KNIFE.wav", 0, 0.0f);
}

void dancing_knife_effect_process(struct Effect *effect)
{
    struct DancingKnifeEffect *self = (struct DancingKnifeEffect *)effect;
    struct Ez2StrParticle **it = self->opening.first;

    while (it != (struct Ez2StrParticle **)self->opening.last) {
        struct Ez2StrParticle *p = *it;
        if (!still_playing(p)) {
            particle_free(&p->base);
            memmove(it, it + 1, (char *)self->opening.last - (char *)(it + 1));
            self->opening.last = (struct Ez2StrParticle **)self->opening.last - 1;
            continue;
        }
        dancing_knife_effect_step_part(self, p);
        it++;
    }
    if (self->opening.first == self->opening.last && !self->loop_started)
        dancing_knife_effect_start_loop(self);
    for (it = self->looping.first; it != (struct Ez2StrParticle **)self->looping.last; it++)
        dancing_knife_effect_step_part(self, *it);
}

void dancing_knife_effect_render_part(struct DancingKnifeEffect *self, struct Ez2StrParticle *p)
{
    if (!p)
        return;
    ez2_str_particle_render_current(p);
}

void dancing_knife_effect_render(struct Effect *effect)
{
    struct DancingKnifeEffect *self = (struct DancingKnifeEffect *)effect;
    struct Ez2StrParticle **it;

    for (it = self->opening.first; it != (struct Ez2StrParticle **)self->opening.last; it++)
        dancing_knife_effect_render_part(self, *it);
    for (it = self->looping.first; it != (struct Ez2StrParticle **)self->looping.last; it++)
        dancing_knife_effect_render_part(self, *it);
}
