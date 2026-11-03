#include "effects/effect_allocator.h"
#include "session/state.h"
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/vec.h"

#define EFFECT_DIR "이팩트\\"
#define EFFECT_SPRITE_MABLE 0x448
#define MARBLE_COUNT 3

struct SpriteMableEffect {
    struct Effect base;
    struct Vec marbles;
};

struct SpriteMableParticle {
    float orbit;
    float bob;
};
PARTICLE_EFFECT_DATA_FITS(SpriteMableParticle);
static __inline struct SpriteMableParticle *sprite_mable_particle(const void *p)
{
    return (struct SpriteMableParticle *)((const struct Particle *)p)->effect_data;
}

void sprite_mable_effect_init(struct Effect *self);
void sprite_mable_effect_process(struct Effect *self);
void sprite_mable_effect_render(struct Effect *self);
void sprite_mable_effect_release(struct Effect *self);
static void sprite_mable_effect_destroy(struct Effect *effect);

static const struct EffectOps sprite_mable_effect_ops = {
    .set_submit = effect_set_submit,
    .init = sprite_mable_effect_init,
    .update_range = effect_update_range,
    .process = sprite_mable_effect_process,
    .render = sprite_mable_effect_render,
    .release = sprite_mable_effect_release,
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
    .destroy = sprite_mable_effect_destroy,
    .fade_out = effect_fade_out,
};

struct SpriteMableEffect *sprite_mable_effect_construct(struct SpriteMableEffect *self)
{
    effect_construct(&self->base, EFFECT_SPRITE_MABLE);
    self->base.ops = &sprite_mable_effect_ops;
    self->marbles.first = self->marbles.last = self->marbles.end = 0;
    return self;
}

struct Effect *sprite_mable_effect_new(void)
{
    struct SpriteMableEffect *self = mem_try_alloc(sizeof *self);

    return self ? &sprite_mable_effect_construct(self)->base : NULL;
}

/* Effect types created as SpriteMableEffect, registered at startup through g_effectTypes
   (effects/effect_registry.c). A type missing here is not created. */
static const struct EffectTypeRow sprite_mable_effect_type_rows[] = {
    EFFECT_TYPE(0x448, sprite_mable_effect_new),
};
const struct EffectTypeTable sprite_mable_effect_types = EFFECT_TYPE_TABLE(sprite_mable_effect_type_rows);

void sprite_mable_effect_destruct(struct SpriteMableEffect *self)
{
    self->base.ops = &sprite_mable_effect_ops;
    vec_free(&self->marbles);
    effect_destruct(&self->base);
}

static void sprite_mable_effect_destroy(struct Effect *effect)
{
    struct SpriteMableEffect *self = (struct SpriteMableEffect *)effect;
    sprite_mable_effect_destruct(self);
    mem_free(self);
}

void sprite_mable_effect_release(struct Effect *effect)
{
    struct SpriteMableEffect *self = (struct SpriteMableEffect *)effect;
    struct Particle **it;
    for (it = self->marbles.first; it != (struct Particle **)self->marbles.last; it++)
        particle_free(*it);
    self->marbles.last = self->marbles.first;
}

void sprite_mable_effect_init(struct Effect *effect)
{
    struct SpriteMableEffect *self = (struct SpriteMableEffect *)effect;
    int i;

    for (i = 0; i < MARBLE_COUNT; i++) {
        struct SpriteParticle *sp = sprite_particle_new(self->base.pos, 1);
        struct Particle *head = (struct Particle *)sp;

        if (!sp)
            continue;
        head->effect_type = self->base.type;
        head->index = i;
        head->group = 0;
        head->owner_tag = 1;
        sp->base.scale = 0.3f;
        sp->base.alpha = 200;
        sprite_mable_particle(sp)->orbit = (float)i * 2.0943947f;
        sprite_mable_particle(sp)->bob = 0;
        sp->loop = SPRITE_STILL;
        switch (i) {
        case 0:
            sprite_particle_set_spr(sp, EFFECT_DIR "su_spritemable\\su_spritemable_green.spr");
            sprite_particle_set_act(sp, EFFECT_DIR "su_spritemable\\su_spritemable_green.act");
            break;
        case 1:
            sprite_particle_set_spr(sp, EFFECT_DIR "su_spritemable\\su_spritemable_red.spr");
            sprite_particle_set_act(sp, EFFECT_DIR "su_spritemable\\su_spritemable_red.act");
            break;
        case 2:
            sprite_particle_set_spr(sp, EFFECT_DIR "su_spritemable\\su_spritemable_blue.spr");
            sprite_particle_set_act(sp, EFFECT_DIR "su_spritemable\\su_spritemable_blue.act");
            break;
        }
        self->base.duration = 9999;
        vec_push(&self->marbles, &sp, sizeof(struct SpriteParticle *));
    }
}

void sprite_mable_effect_process(struct Effect *effect)
{
    struct SpriteMableEffect *self = (struct SpriteMableEffect *)effect;
    struct SpriteParticle **it;

    for (it = self->marbles.first; it != (struct SpriteParticle **)self->marbles.last; it++) {
        struct SpriteParticle *sp = *it;
        float orbit = sprite_mable_particle(sp)->orbit + 0.02f;
        float bob = sprite_mable_particle(sp)->bob + 0.05f;
        float dx, dy, dz;
        Vec3 here;
        int deg, view;

        sprite_mable_particle(sp)->orbit = orbit;
        sprite_mable_particle(sp)->bob = bob;
        dx = (float)((double)float_sin(orbit) * 6.0);
        dy = (float)((double)float_sin(bob) * 3.0 - 6.0);
        dz = (float)((double)float_cos(orbit) * 6.0);
        here = self->base.pos;
        sp->base.pos.x = here.x + dx;
        sp->base.pos.y = dy + here.y;
        sp->base.pos.z = dz + here.z;
        deg = (int)(orbit / g_pi * g_halfTurnDegrees) % 360;
        deg = (int)((float)deg - game_camera_heading());
        if (deg < 0)
            deg += 360;
        view = (deg - 75) / 30;
        if (view < 0)
            view += 12;
        sp->base.age++;
        sp->motion = view;
    }
}

void sprite_mable_effect_render(struct Effect *effect)
{
    struct SpriteMableEffect *self = (struct SpriteMableEffect *)effect;
    struct SpriteParticle **it;

    if (!g_session1.showSpiritMarbles)
        return;
    for (it = self->marbles.first; it != (struct SpriteParticle **)self->marbles.last; it++)
        sprite_particle_render(*it);
}
