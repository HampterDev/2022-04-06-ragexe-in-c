#include "effects/effect_allocator.h"
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/vec.h"

#define EFFECT_DIR "이팩트\\"
#define EFFECT_TUNA_PARTY 0x449

struct TunaPartyEffect {
    struct Effect base;
    struct Vec tunas;
};

struct TunaPartyParticle {
    float orbit;
};
PARTICLE_EFFECT_DATA_FITS(TunaPartyParticle);
static __inline struct TunaPartyParticle *tuna_party_particle(const void *p)
{
    return (struct TunaPartyParticle *)((const struct Particle *)p)->effect_data;
}

void tuna_party_effect_init(struct Effect *self);
void tuna_party_effect_process(struct Effect *self);
void tuna_party_effect_render(struct Effect *self);
void tuna_party_effect_release(struct Effect *self);
static void tuna_party_effect_destroy(struct Effect *effect);

static const struct EffectOps tuna_party_effect_ops = {
    .set_submit = effect_set_submit,
    .init = tuna_party_effect_init,
    .update_range = effect_update_range,
    .process = tuna_party_effect_process,
    .render = tuna_party_effect_render,
    .release = tuna_party_effect_release,
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
    .destroy = tuna_party_effect_destroy,
    .fade_out = effect_fade_out,
};

struct TunaPartyEffect *tuna_party_effect_construct(struct TunaPartyEffect *self)
{
    effect_construct(&self->base, EFFECT_TUNA_PARTY);
    self->base.ops = &tuna_party_effect_ops;
    self->tunas.first = self->tunas.last = self->tunas.end = 0;
    return self;
}

struct Effect *tuna_party_effect_new(void)
{
    struct TunaPartyEffect *self = mem_try_alloc(sizeof *self);

    return self ? &tuna_party_effect_construct(self)->base : NULL;
}

void tuna_party_effect_destruct(struct TunaPartyEffect *self)
{
    self->base.ops = &tuna_party_effect_ops;
    vec_free(&self->tunas);
    effect_destruct(&self->base);
}

static void tuna_party_effect_destroy(struct Effect *effect)
{
    struct TunaPartyEffect *self = (struct TunaPartyEffect *)effect;
    tuna_party_effect_destruct(self);
    mem_free(self);
}

void tuna_party_effect_release(struct Effect *effect)
{
    struct TunaPartyEffect *self = (struct TunaPartyEffect *)effect;
    struct Particle **it;
    for (it = self->tunas.first; it != (struct Particle **)self->tunas.last; it++)
        particle_free(*it);
    self->tunas.last = self->tunas.first;
}

void tuna_party_effect_init(struct Effect *effect)
{
    struct TunaPartyEffect *self = (struct TunaPartyEffect *)effect;
    struct SpriteParticle *sp = sprite_particle_new(self->base.pos, 0x101);
    struct Particle *head = (struct Particle *)sp;

    if (!sp)
        return;
    head->effect_type = self->base.type;
    head->index = 1;
    head->group = 0;
    head->owner_tag = 1;
    sprite_particle_set_spr(sp, EFFECT_DIR "su_tunaparty\\su_tunaparty.spr");
    sprite_particle_set_act(sp, EFFECT_DIR "su_tunaparty\\su_tunaparty.act");
    tuna_party_particle(sp)->orbit = 0;
    sp->loop = SPRITE_STILL;
    self->base.duration = 9999;
    vec_push(&self->tunas, &sp, sizeof(struct SpriteParticle *));
}

void tuna_party_effect_process(struct Effect *effect)
{
    struct TunaPartyEffect *self = (struct TunaPartyEffect *)effect;
    struct SpriteParticle **it;

    for (it = self->tunas.first; it != (struct SpriteParticle **)self->tunas.last; it++) {
        struct SpriteParticle *sp = *it;
        float orbit = tuna_party_particle(sp)->orbit + 0.05f;
        float dx, dz, deg, seen;
        Vec3 here;
        int view;

        tuna_party_particle(sp)->orbit = orbit;
        dx = (float)((double)float_sin(orbit) * 10.0);
        dz = (float)((double)float_cos(orbit) * 10.0);
        here = self->base.pos;
        sp->base.pos.y = here.y - 10.0f;
        sp->base.pos.x = dx + here.x;
        deg = orbit / g_pi * g_halfTurnDegrees;
        sp->base.pos.z = dz + here.z;
        if (deg > 360.0f)
            deg -= (float)((int)(deg / 360.0f) * 360);
        seen = deg - game_camera_heading();
        if (0.0f > seen)
            seen += 360.0f;
        view = (int)(((double)seen - 157.5) / 45.0);
        if (view < 0)
            view += 8;
        sp->base.age++;
        sp->motion = view;
    }
}

void tuna_party_effect_render(struct Effect *effect)
{
    struct TunaPartyEffect *self = (struct TunaPartyEffect *)effect;
    struct SpriteParticle **it;
    for (it = self->tunas.first; it != (struct SpriteParticle **)self->tunas.last; it++)
        sprite_particle_render(*it);
}
