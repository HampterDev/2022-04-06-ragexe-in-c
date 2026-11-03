#include "effects/effect_allocator.h"
#include <stdlib.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/effect_math.h"
#include "std/vec.h"

struct SmokeEffect {
    struct Effect base;
    int visible_count;
    struct Vec puffs;
};

void smoke_effect_init(struct Effect *self);
void smoke_effect_process(struct Effect *self);
void smoke_effect_render(struct Effect *self);
void smoke_effect_release(struct Effect *self);
static void smoke_effect_destroy(struct Effect *effect);

static const struct EffectOps smoke_effect_ops = {
    .set_submit = effect_set_submit,
    .init = smoke_effect_init,
    .update_range = effect_update_range,
    .process = smoke_effect_process,
    .render = smoke_effect_render,
    .release = smoke_effect_release,
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
    .destroy = smoke_effect_destroy,
    .fade_out = effect_fade_out,
};

#define LIST_SPRITE 1
#define PUFF_COUNT 4
#define PUFF_LIFE 50
#define FADE_FRAME 34
#define SMOKE_HEIGHT 9.0f

#define particle_owner_type(p) ((p)->base.effect_type)
#define particle_tag(p)        ((p)->base.owner_tag)
#define particle_index(p)      ((p)->base.index)
#define particle_row(p)        ((p)->base.group)
#define puff_course(p)         ((p)->base.orientation)
#define puff_offset_x(p)       ((p)->sprite_offset.x)
#define puff_offset_y(p)       ((p)->sprite_offset.y)
#define puff_offset_z(p)       ((p)->sprite_offset.z)
#define puff_spin(p)           ((p)->roll_step)

#define EACH_PUFF(self, it) \
    for (it = (struct SpriteParticle **)(self)->puffs.first; it != (struct SpriteParticle **)(self)->puffs.last; it++)

struct SmokeEffect *smoke_effect_construct(struct SmokeEffect *self)
{
    effect_construct(&self->base, 0x2c);
    self->base.ops = &smoke_effect_ops;
    self->visible_count = 1;
    self->puffs.first = self->puffs.last = self->puffs.end = 0;
    return self;
}

struct Effect *smoke_effect_new(void)
{
    struct SmokeEffect *self = mem_try_alloc(sizeof *self);

    return self ? &smoke_effect_construct(self)->base : NULL;
}

/* Effect types created as SmokeEffect, registered at startup through g_effectTypes
   (effects/effect_registry.c). A type missing here is not created. */
static const struct EffectTypeRow smoke_effect_type_rows[] = {
    EFFECT_TYPE(0x02c, smoke_effect_new),
};
const struct EffectTypeTable smoke_effect_types = EFFECT_TYPE_TABLE(smoke_effect_type_rows);

void smoke_effect_destruct(struct SmokeEffect *self)
{
    self->base.ops = &smoke_effect_ops;
    vec_free(&self->puffs);
    effect_destruct(&self->base);
}

static void smoke_effect_destroy(struct Effect *effect)
{
    struct SmokeEffect *self = (struct SmokeEffect *)effect;
    smoke_effect_destruct(self);
    mem_free(self);
}

void smoke_effect_release(struct Effect *effect)
{
    struct SmokeEffect *self = (struct SmokeEffect *)effect;
    struct SpriteParticle **it;

    EACH_PUFF(self, it)
        particle_free(&(*it)->base);
    self->puffs.last = self->puffs.first;
}

void smoke_effect_init(struct Effect *effect)
{
    struct SmokeEffect *self = (struct SmokeEffect *)effect;
    int i;

    for (i = 0; i < PUFF_COUNT; i++) {
        struct SpriteParticle *p = sprite_particle_new(self->base.pos, LIST_SPRITE);

        if (!p)
            continue;
        particle_owner_type(p) = self->base.type;
        particle_index(p) = i;
        particle_row(p) = 0;
        particle_tag(p) = 1;
        vec_push(&self->puffs, &p, sizeof(struct SpriteParticle *));
    }
}

void smoke_start_puff(struct SpriteParticle *p)
{
    float *course;
    int r;

    if (!p)
        return;
    sprite_particle_set_spr(p, "이팩트\\굴뚝연기.spr");
    sprite_particle_set_act(p, "이팩트\\굴뚝연기.act");
    p->base.scale = 1.5f;
    puff_spin(p) = 0.75f;
    r = rand() % 3;
    p->base.age = 0;
    p->frame_delay = (float)((r < 0 ? 0 : r) + 3) / 10.0f;
    puff_offset_x(p) = 0;
    puff_offset_y(p) = 0;
    puff_offset_z(p) = 0;
    p->base.list_flags = 1;
    p->base.alpha = 255;
    course = puff_course(p);
    mat_is_identity(course);
    r = rand() % 30;
    mat_set_rotate_x(course, (float)((r < 0 ? 0 : r) + 75) / 180.0f * 3.141592f);
    r = rand() % 360;
    mat_rotate_y(course, (float)(r < 0 ? 0 : r) / 180.0f * 3.141592f);
}

void smoke_effect_update_puff(struct SmokeEffect *self, struct SpriteParticle *p)
{
    if (!p)
        return;
    if (p->base.age < PUFF_LIFE) {
        const float *m = puff_course(p);
        float rise = p->frame_delay;
        float x = 0.0f * m[0] + 0.0f * m[3] + rise * m[6] + m[9] + puff_offset_x(p);
        float y = 0.0f * m[1] + 0.0f * m[4] + rise * m[7] + m[10] + puff_offset_y(p);
        float z = 0.0f * m[2] + 0.0f * m[5] + rise * m[8] + m[11] + puff_offset_z(p);

        puff_offset_x(p) = x;
        puff_offset_y(p) = y;
        puff_offset_z(p) = z;
        p->base.roll = puff_spin(p) + p->base.roll;
        if (p->base.age > FADE_FRAME) {
            int alpha = p->base.alpha - 15;
            p->base.alpha = alpha < 0 ? 0 : alpha;
        }
    } else {
        unsigned count;

        smoke_start_puff(p);
        count = VEC_COUNT(&self->puffs, struct SpriteParticle *);
        self->visible_count = (int)effect_random_tenths(1.0f, (float)count);
    }
    mat_set_translation(p->base.matrix, puff_offset_x(p) + 0.0f, puff_offset_y(p) - SMOKE_HEIGHT, puff_offset_z(p) + 0.0f);
}

void smoke_effect_process(struct Effect *effect)
{
    struct SmokeEffect *self = (struct SmokeEffect *)effect;
    struct SpriteParticle **it;

    EACH_PUFF(self, it) {
        struct SpriteParticle *p = *it;
        p->base.pos = self->base.pos;
        smoke_effect_update_puff(self, p);
        p->base.age++;
    }
}

void smoke_effect_render_puff(struct SmokeEffect *self, struct SpriteParticle *p)
{
    if (p && particle_index(p) < self->visible_count)
        sprite_particle_render(p);
}

void smoke_effect_render(struct Effect *effect)
{
    struct SmokeEffect *self = (struct SmokeEffect *)effect;
    struct SpriteParticle **it;

    EACH_PUFF(self, it)
        smoke_effect_render_puff(self, *it);
}
