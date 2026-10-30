#include "effects/effect_allocator.h"
#include <stdlib.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/vec.h"

#define EFFECT_DIR "이팩트\\"

enum {
    EFFECT_LUNATIC_CARROT_BEAT = 0x44b,
    EFFECT_CN_METEOR = 0x452,
    EFFECT_LUNATIC_CARROT_BEAT_2 = 0x457,
    EFFECT_CN_METEOR_2 = 0x458,
};

#define GAME_MSG_QUAKE 0x7a

struct MeteorEffect {
    struct Effect base;
    struct Vec sprites;
    int count;
    int hit_frame;
};

static void particle_tag(void *p, int effect_type, int index)
{
    struct Particle *head = p;
    head->effect_type = effect_type;
    head->index = index;
    head->group = 0;
    head->owner_tag = 1;
}
#define SPRITE_OFFSET(sp) (&(sp)->sprite_offset)

void meteor_effect_init(struct Effect *self);
void meteor_effect_process(struct Effect *self);
void meteor_effect_render(struct Effect *self);
void meteor_effect_release(struct Effect *self);
unsigned char meteor_effect_is_alive(struct Effect *self);
static void meteor_effect_destroy(struct Effect *effect);

static const struct EffectOps meteor_effect_ops = {
    .set_submit = effect_set_submit,
    .init = meteor_effect_init,
    .update_range = effect_update_range,
    .process = meteor_effect_process,
    .render = meteor_effect_render,
    .release = meteor_effect_release,
    .is_alive = meteor_effect_is_alive,
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
    .destroy = meteor_effect_destroy,
    .fade_out = effect_fade_out,
};

static void send_game_message(int msg)
{
    game_send_message(msg, 0, 0, 0, 0);
}

static int random_below(int range)
{
    int r;
    if (range == 0)
        return 0;
    r = rand() % range;
    return r < 0 ? 0 : r;
}

struct MeteorEffect *meteor_effect_construct(struct MeteorEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &meteor_effect_ops;
    self->sprites.first = self->sprites.last = self->sprites.end = 0;
    self->count = 1;
    self->hit_frame = 0;
    return self;
}

struct Effect *meteor_effect_new(void)
{
    struct MeteorEffect *self = mem_try_alloc(sizeof *self);

    return self ? &meteor_effect_construct(self)->base : NULL;
}

void meteor_effect_destruct(struct MeteorEffect *self)
{
    self->base.ops = &meteor_effect_ops;
    vec_free(&self->sprites);
    effect_destruct(&self->base);
}

static void meteor_effect_destroy(struct Effect *effect)
{
    struct MeteorEffect *self = (struct MeteorEffect *)effect;
    meteor_effect_destruct(self);
    mem_free(self);
}

void meteor_effect_release(struct Effect *effect)
{
    struct MeteorEffect *self = (struct MeteorEffect *)effect;
    struct SpriteParticle **it;
    for (it = self->sprites.first; it != (struct SpriteParticle **)self->sprites.last; it++)
        particle_free(&(*it)->base);
    self->sprites.last = self->sprites.first;
}

void meteor_effect_add_meteor(struct MeteorEffect *self)
{
    struct SpriteParticle *sp = sprite_particle_new(self->base.pos, 1);

    if (sp) {
        particle_tag(sp, self->base.type, 0);
        sprite_particle_set_spr(sp, EFFECT_DIR "su_cn_meteor\\su_cn_meteor.spr");
        sprite_particle_set_act(sp, EFFECT_DIR "su_cn_meteor\\su_cn_meteor.act");
        sp->base.flags |= PARTICLE_TINT_BY_CLIP;
        sp->frame_delay = 4.0f;
        sp->base.depth_bias = 0.00015f;
        sp->loop = SPRITE_ONCE;
        if (self->base.type == EFFECT_CN_METEOR)
            sp->base.alpha = 100;
        vec_push(&self->sprites, &sp, sizeof sp);
    }
    self->hit_frame = 48;
}

void meteor_effect_add_carrots(struct MeteorEffect *self)
{
    int total, i;

    switch (self->base.level) {
    case 1: case 2: self->count = 3; break;
    case 3: case 4: self->count = 5; break;
    case 5: self->count = 7; break;
    }
    total = self->count * 2;
    for (i = 0; i < total; i++) {
        struct SpriteParticle *sp = sprite_particle_new(self->base.pos, 1);
        int x, z;

        if (!sp)
            continue;
        particle_tag(sp, self->base.type, i);
        sprite_particle_set_spr(sp, EFFECT_DIR "su_lunaticcarrotbeat\\su_lunaticcarrotbeat.spr");
        sprite_particle_set_act(sp, EFFECT_DIR "su_lunaticcarrotbeat\\su_lunaticcarrotbeat.act");
        x = random_below(self->count * 5) - self->count * 5 / 2;
        z = random_below(self->count * 5) - self->count * 5 / 2;
        SPRITE_OFFSET(sp)->y = 0;
        SPRITE_OFFSET(sp)->x = (float)x;
        SPRITE_OFFSET(sp)->z = (float)z;
        sp->base.age = -105 / total * i;
        sp->base.flags |= PARTICLE_TINT_BY_CLIP;
        sp->frame_delay = 4.0f;
        sp->base.depth_bias = 0.00015f;
        sp->loop = SPRITE_ONCE;
        if (self->base.type == EFFECT_LUNATIC_CARROT_BEAT)
            sp->base.alpha = 100;
        vec_push(&self->sprites, &sp, sizeof sp);
    }
    self->hit_frame = 48;
}

void meteor_effect_init(struct Effect *effect)
{
    struct MeteorEffect *self = (struct MeteorEffect *)effect;
    switch (self->base.type) {
    case EFFECT_LUNATIC_CARROT_BEAT:
    case EFFECT_LUNATIC_CARROT_BEAT_2:
        meteor_effect_add_carrots(self);
        break;
    case EFFECT_CN_METEOR:
    case EFFECT_CN_METEOR_2:
        meteor_effect_add_meteor(self);
        break;
    }
    self->base.duration = 9999;
}

unsigned char meteor_effect_is_alive(struct Effect *effect)
{
    struct MeteorEffect *self = (struct MeteorEffect *)effect;
    struct SpriteParticle **it;
    for (it = self->sprites.first; it != (struct SpriteParticle **)self->sprites.last; it++) {
        float life = (*it)->base.life_seconds;
        if (life == 0.0f)
            return 1;
        if (life * 60.0f > (float)(*it)->base.age)
            return 1;
    }
    return 0;
}

void meteor_effect_process(struct Effect *effect)
{
    struct MeteorEffect *self = (struct MeteorEffect *)effect;
    struct SpriteParticle **it;
    for (it = self->sprites.first; it != (struct SpriteParticle **)self->sprites.last; it++) {
        struct SpriteParticle *sp = *it;
        if (sp->base.age >= 0) {
            Vec3 here = self->base.pos;
            sp->base.pos.x = SPRITE_OFFSET(sp)->x + here.x;
            sp->base.pos.y = SPRITE_OFFSET(sp)->y + here.y;
            sp->base.pos.z = SPRITE_OFFSET(sp)->z + here.z;
            if (self->hit_frame == sp->base.age)
                send_game_message(GAME_MSG_QUAKE);
        }
        sp->base.age++;
    }
}

void meteor_effect_render(struct Effect *effect)
{
    struct MeteorEffect *self = (struct MeteorEffect *)effect;
    struct SpriteParticle **it;
    for (it = self->sprites.first; it != (struct SpriteParticle **)self->sprites.last; it++)
        if ((*it)->base.age >= 0)
            sprite_particle_render(*it);
}
