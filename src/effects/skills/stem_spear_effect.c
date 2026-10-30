#include "effects/effect_allocator.h"
#include <math.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/vec.h"

#define EFFECT_DIR "이팩트\\"
#define EFFECT_STEM_SPEAR 0x44c
#define SPEAR_COUNT 4

struct StemSpearEffect {
    struct Effect base;
    struct Vec spears;
};

struct StemSpearParticle {
    int heading;
};
PARTICLE_EFFECT_DATA_FITS(StemSpearParticle);
static __inline struct StemSpearParticle *stem_spear_particle(const void *p)
{
    return (struct StemSpearParticle *)((const struct Particle *)p)->effect_data;
}
#define SPRITE_OFFSET(sp) (&(sp)->sprite_offset)

void stem_spear_effect_init(struct Effect *self);
void stem_spear_effect_process(struct Effect *self);
void stem_spear_effect_render(struct Effect *self);
void stem_spear_effect_release(struct Effect *self);
unsigned char stem_spear_effect_is_alive(struct Effect *self);
static void stem_spear_effect_destroy(struct Effect *effect);

static const struct EffectOps stem_spear_effect_ops = {
    .set_submit = effect_set_submit,
    .init = stem_spear_effect_init,
    .update_range = effect_update_range,
    .process = stem_spear_effect_process,
    .render = stem_spear_effect_render,
    .release = stem_spear_effect_release,
    .is_alive = stem_spear_effect_is_alive,
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
    .destroy = stem_spear_effect_destroy,
    .fade_out = effect_fade_out,
};

struct StemSpearEffect *stem_spear_effect_construct(struct StemSpearEffect *self)
{
    effect_construct(&self->base, EFFECT_STEM_SPEAR);
    self->base.ops = &stem_spear_effect_ops;
    self->spears.first = self->spears.last = self->spears.end = 0;
    return self;
}

struct Effect *stem_spear_effect_new(void)
{
    struct StemSpearEffect *self = mem_try_alloc(sizeof *self);

    return self ? &stem_spear_effect_construct(self)->base : NULL;
}

void stem_spear_effect_destruct(struct StemSpearEffect *self)
{
    self->base.ops = &stem_spear_effect_ops;
    vec_free(&self->spears);
    effect_destruct(&self->base);
}

static void stem_spear_effect_destroy(struct Effect *effect)
{
    struct StemSpearEffect *self = (struct StemSpearEffect *)effect;
    stem_spear_effect_destruct(self);
    mem_free(self);
}

void stem_spear_effect_release(struct Effect *effect)
{
    struct StemSpearEffect *self = (struct StemSpearEffect *)effect;
    struct Particle **it;
    for (it = self->spears.first; it != (struct Particle **)self->spears.last; it++)
        particle_free(*it);
    self->spears.last = self->spears.first;
}

void stem_spear_effect_init(struct Effect *effect)
{
    struct StemSpearEffect *self = (struct StemSpearEffect *)effect;
    int i;

    for (i = 0; i < SPEAR_COUNT; i++) {
        struct SpriteParticle *sp = sprite_particle_new(self->base.pos, 0x101);
        struct Particle *head = (struct Particle *)sp;
        Vec3 here, target;
        float dx, dz, deg, heading;

        if (!sp)
            continue;
        head->effect_type = self->base.type;
        head->index = i;
        head->group = 0;
        head->owner_tag = 1;
        sprite_particle_set_spr(sp, EFFECT_DIR "sv_stemspear\\sv_stemspear.spr");
        sprite_particle_set_act(sp, EFFECT_DIR "sv_stemspear\\sv_stemspear.act");
        SPRITE_OFFSET(sp)->x = 0;
        SPRITE_OFFSET(sp)->y = 0;
        SPRITE_OFFSET(sp)->z = 0;
        here = self->base.pos;
        target = self->base.target_pos;
        dx = target.x - here.x;
        dz = target.z - here.z;
        self->base.duration = 16;
        deg = (float)atan2((double)dx, (double)-dz) / g_pi * g_halfTurnDegrees;
        if (0.0f > deg)
            deg += 360.0f;
        heading = g_halfTurnDegrees - deg;
        if (0.0f > heading)
            heading += 360.0f;
        stem_spear_particle(sp)->heading = (int)heading;
        sp->frame_delay = float_sqrt(dz * dz + dx * dx) / (float)self->base.duration;
        sp->base.age = -(self->base.duration / 4) * i;
        vec_push(&self->spears, &sp, sizeof(struct SpriteParticle *));
    }
}

unsigned char stem_spear_effect_is_alive(struct Effect *effect)
{
    struct StemSpearEffect *self = (struct StemSpearEffect *)effect;
    struct Particle **it;
    for (it = self->spears.first; it != (struct Particle **)self->spears.last; it++)
        if ((*it)->age < self->base.duration)
            return 1;
    return 0;
}

void stem_spear_effect_process(struct Effect *effect)
{
    struct StemSpearEffect *self = (struct StemSpearEffect *)effect;
    struct SpriteParticle **it;

    for (it = self->spears.first; it != (struct SpriteParticle **)self->spears.last; it++) {
        struct SpriteParticle *sp = *it;
        Vec3 here, target, step;
        float dx, dy, dz, inv_len, speed;
        int heading;

        if (sp->base.age >= 0) {
            effect_set_target(&self->base, self->base.target_aid);
            heading = stem_spear_particle(sp)->heading;
            sp->base.roll = (float)heading - game_camera_heading();
            here = self->base.pos;
            target = self->base.target_pos;
            dy = target.y - here.y;
            dx = target.x - here.x;
            dz = target.z - here.z;
            inv_len = (float)(1.0 / (double)float_sqrt(dy * dy + dx * dx + dz * dz));
            speed = sp->frame_delay;
            step.y = inv_len * dy * speed + SPRITE_OFFSET(sp)->y;
            step.x = inv_len * dx * speed + SPRITE_OFFSET(sp)->x;
            step.z = inv_len * dz * speed + SPRITE_OFFSET(sp)->z;
            SPRITE_OFFSET(sp)->y = step.y;
            SPRITE_OFFSET(sp)->x = step.x;
            SPRITE_OFFSET(sp)->z = step.z;
            here = self->base.pos;
            sp->base.pos.x = step.x + here.x;
            sp->base.pos.y = here.y - 10.0f;
            sp->base.pos.z = step.z + here.z;
            if (self->base.frame >= 10)
                sp->base.alpha -= 40;
        }
        sp->base.age++;
    }
}

void stem_spear_effect_render(struct Effect *effect)
{
    struct StemSpearEffect *self = (struct StemSpearEffect *)effect;
    struct SpriteParticle **it;
    for (it = self->spears.first; it != (struct SpriteParticle **)self->spears.last; it++) {
        int age = (*it)->base.age;
        if (age > 0 && age < self->base.duration)
            sprite_particle_render(*it);
    }
}
