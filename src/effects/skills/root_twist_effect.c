#include "effects/effect_allocator.h"
#include "effects/particle.h"
#include "std/vec.h"

#define EFFECT_DIR "이팩트\\"
#define EFFECT_ROOT_TWIST 0x456

struct RootTwistEffect {
    struct Effect base;
    struct Vec sprites;
};

void root_twist_effect_init(struct Effect *self);
void root_twist_effect_process(struct Effect *self);
void root_twist_effect_render(struct Effect *self);
void root_twist_effect_release(struct Effect *self);
void root_twist_effect_after_init(struct Effect *self);
static void root_twist_effect_destroy(struct Effect *effect);

static const struct EffectOps root_twist_effect_ops = {
    .set_submit = effect_set_submit,
    .init = root_twist_effect_init,
    .update_range = effect_update_range,
    .process = root_twist_effect_process,
    .render = root_twist_effect_render,
    .release = root_twist_effect_release,
    .is_alive = effect_is_owner_alive,
    .restart = effect_restart,
    .set_color = effect_set_color,
    .set_texture = effect_set_texture,
    .after_init = root_twist_effect_after_init,
    .skip = effect_skip,
    .set_stop_frame = effect_set_stop_frame,
    .set_start_delay = effect_set_start_delay,
    .set_spin = effect_set_spin,
    .set_str_name = effect_set_str_name,
    .set_list_flags = effect_set_list_flags,
    .set_zoom = effect_set_zoom,
    .set_alpha_scale = effect_set_alpha_scale,
    .destroy = root_twist_effect_destroy,
    .fade_out = effect_fade_out,
};

struct RootTwistEffect *root_twist_effect_construct(struct RootTwistEffect *self)
{
    effect_construct(&self->base, EFFECT_ROOT_TWIST);
    self->base.ops = &root_twist_effect_ops;
    self->sprites.first = self->sprites.last = self->sprites.end = 0;
    return self;
}

struct Effect *root_twist_effect_new(void)
{
    struct RootTwistEffect *self = mem_try_alloc(sizeof *self);

    return self ? &root_twist_effect_construct(self)->base : NULL;
}

/* Effect types created as RootTwistEffect, registered at startup through g_effectTypes
   (effects/effect_registry.c). A type missing here is not created. */
static const struct EffectTypeRow root_twist_effect_type_rows[] = {
    EFFECT_TYPE(0x456, root_twist_effect_new),
};
const struct EffectTypeTable root_twist_effect_types = EFFECT_TYPE_TABLE(root_twist_effect_type_rows);

void root_twist_effect_destruct(struct RootTwistEffect *self)
{
    self->base.ops = &root_twist_effect_ops;
    vec_free(&self->sprites);
    effect_destruct(&self->base);
}

static void root_twist_effect_destroy(struct Effect *effect)
{
    struct RootTwistEffect *self = (struct RootTwistEffect *)effect;
    root_twist_effect_destruct(self);
    mem_free(self);
}

void root_twist_effect_release(struct Effect *effect)
{
    struct RootTwistEffect *self = (struct RootTwistEffect *)effect;
    struct Particle **it;
    for (it = self->sprites.first; it != (struct Particle **)self->sprites.last; it++)
        particle_free(*it);
    self->sprites.last = self->sprites.first;
}

void root_twist_effect_init(struct Effect *effect)
{
    struct RootTwistEffect *self = (struct RootTwistEffect *)effect;
    struct SpriteParticle *sp = sprite_particle_new(self->base.pos, 0x101);

    if (sp) {
        struct Particle *head = (struct Particle *)sp;
        head->effect_type = self->base.type;
        head->index = 0;
        head->group = 0;
        head->owner_tag = 1;
        sprite_particle_set_spr(sp, EFFECT_DIR "su_sv_roottwist\\su_sv_roottwist.spr");
        sprite_particle_set_act(sp, EFFECT_DIR "su_sv_roottwist\\su_sv_roottwist.act");
        sp->base.flags |= PARTICLE_TINT_BY_CLIP;
        sp->frame_delay = 8.0f;
        sp->base.depth_bias = 0.00015f;
        vec_push(&self->sprites, &sp, sizeof(struct SpriteParticle *));
    }
    self->base.duration = 9999;
}

void root_twist_effect_after_init(struct Effect *effect)
{
    if (effect->type == EFFECT_ROOT_TWIST)
        effect_add_sound_cue(effect, SOUND_CUE_WAVE, "effect\\su_roottwist.wav", 0, 0.0f);
}

void root_twist_effect_process(struct Effect *effect)
{
    struct RootTwistEffect *self = (struct RootTwistEffect *)effect;
    struct Particle **it;
    for (it = self->sprites.first; it != (struct Particle **)self->sprites.last; it++) {
        struct Particle *p = *it;
        Vec3 here = self->base.pos;
        p->pos.x = here.x + 0.0f;
        p->pos.y = here.y + 3.0f;
        p->pos.z = here.z + 0.0f;
        p->age++;
    }
}

void root_twist_effect_render(struct Effect *effect)
{
    struct RootTwistEffect *self = (struct RootTwistEffect *)effect;
    struct SpriteParticle **it;
    for (it = self->sprites.first; it != (struct SpriteParticle **)self->sprites.last; it++)
        sprite_particle_render(*it);
}
