#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <stdlib.h>
#include <string.h>
#include "effects/particle.h"
#include "std/vec.h"

struct ScatterEffect {
    struct Effect base;
    struct Vec petals;
};

void scatter_effect_init(struct Effect *self);
void scatter_effect_process(struct Effect *self);
void scatter_effect_render(struct Effect *self);
void scatter_effect_release(struct Effect *self);
static void scatter_effect_destroy(struct Effect *effect);

static const struct EffectOps scatter_effect_ops = {
    .set_submit = effect_set_submit,
    .init = scatter_effect_init,
    .update_range = effect_update_range,
    .process = scatter_effect_process,
    .render = scatter_effect_render,
    .release = scatter_effect_release,
    .is_alive = effect_is_within_duration,
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
    .destroy = scatter_effect_destroy,
    .fade_out = effect_fade_out,
};

#define LIST_SPRITE 1
#define PETAL_COUNT 50
#define SAKURA_TYPE 0x3ea
#define DRIFT_SPEED 0.2f
#define FALL_HEIGHT 40.0f

struct ScatterParticle {
    int heading;
};
PARTICLE_EFFECT_DATA_FITS(ScatterParticle);
static __inline struct ScatterParticle *scatter_particle(struct SpriteParticle *p)
{
    return (struct ScatterParticle *)p->base.effect_data;
}
#define particle_owner_type(p) ((p)->base.effect_type)
#define particle_tag(p)        ((p)->base.owner_tag)
#define particle_index(p)      ((p)->base.index)
#define particle_row(p)        ((p)->base.group)
#define petal_heading(p)       (scatter_particle(p)->heading)
#define petal_drift_x(p)       ((p)->sprite_offset.x)
#define petal_fall(p)          ((p)->sprite_offset.y)
#define petal_drift_z(p)       ((p)->sprite_offset.z)

#define EACH_PETAL(self, it) \
    for (it = (struct SpriteParticle **)(self)->petals.first; it != (struct SpriteParticle **)(self)->petals.last; it++)

struct ScatterEffect *scatter_effect_construct(struct ScatterEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &scatter_effect_ops;
    self->petals.first = self->petals.last = self->petals.end = 0;
    return self;
}

struct Effect *scatter_effect_new(void)
{
    struct ScatterEffect *self = mem_try_alloc(sizeof *self);

    return self ? &scatter_effect_construct(self)->base : NULL;
}

/* Effect types created as ScatterEffect, registered at startup through g_effectTypes
   (effects/effect_registry.c). A type missing here is not created. */
static const struct EffectTypeRow scatter_effect_type_rows[] = {
    EFFECT_TYPE(0x3ea, scatter_effect_new),
};
const struct EffectTypeTable scatter_effect_types = EFFECT_TYPE_TABLE(scatter_effect_type_rows);

void scatter_effect_destruct(struct ScatterEffect *self)
{
    self->base.ops = &scatter_effect_ops;
    vec_free(&self->petals);
    effect_destruct(&self->base);
}

static void scatter_effect_destroy(struct Effect *effect)
{
    struct ScatterEffect *self = (struct ScatterEffect *)effect;
    scatter_effect_destruct(self);
    mem_free(self);
}

void scatter_effect_release(struct Effect *effect)
{
    struct ScatterEffect *self = (struct ScatterEffect *)effect;
    struct SpriteParticle **it;

    EACH_PETAL(self, it)
        particle_free(&(*it)->base);
    self->petals.last = self->petals.first;
}

void scatter_effect_setup_petal(struct ScatterEffect *self, struct SpriteParticle *p)
{
    int r;

    if (!p || self->base.type != SAKURA_TYPE)
        return;
    self->base.duration = 300;
    sprite_particle_set_spr(p, "이팩트\\sakura01.spr");
    sprite_particle_set_act(p, "이팩트\\sakura01.act");
    r = rand() % 6;
    p->base.scale = (float)((r < 0 ? 0 : r) + 22) / 70.0f;
    r = rand() % 3;
    p->frame_delay = (float)((r < 0 ? 0 : r) + 2) * 0.15f;
    r = rand() % 360;
    p->base.roll = (float)(r < 0 ? 0 : r);
    r = rand() % 360;
    petal_heading(p) = r < 0 ? 0 : r;
    petal_drift_x(p) = 0;
    petal_fall(p) = 0;
    petal_drift_z(p) = 0;
    p->base.alpha = 250;
    p->base.dest_blend = 6;
}

void scatter_effect_init(struct Effect *effect)
{
    struct ScatterEffect *self = (struct ScatterEffect *)effect;
    int i;

    for (i = 0; i < PETAL_COUNT; i++) {
        struct SpriteParticle *p = sprite_particle_new(self->base.pos, LIST_SPRITE);

        if (!p)
            continue;
        particle_owner_type(p) = self->base.type;
        particle_index(p) = i;
        particle_row(p) = 0;
        particle_tag(p) = 1;
        scatter_effect_setup_petal(self, p);
        vec_push(&self->petals, &p, sizeof(struct SpriteParticle *));
    }
}

void scatter_update_petal(struct SpriteParticle *p)
{
    float fall, x, z;
    int alpha, heading;

    if (!p)
        return;
    alpha = p->base.alpha - 3;
    p->base.alpha = alpha < 0 ? 0 : alpha;
    heading = petal_heading(p);
    fall = p->frame_delay + petal_fall(p);
    x = g_cosTable[heading] * DRIFT_SPEED + petal_drift_x(p);
    z = g_sinTable[heading] * DRIFT_SPEED + petal_drift_z(p);
    petal_fall(p) = fall;
    petal_drift_x(p) = x;
    petal_drift_z(p) = z;
    mat_set_translation(p->base.matrix, x + 0.0f, fall - FALL_HEIGHT, z + 0.0f);
}

void scatter_effect_process(struct Effect *effect)
{
    struct ScatterEffect *self = (struct ScatterEffect *)effect;
    struct SpriteParticle **it;

    EACH_PETAL(self, it) {
        struct SpriteParticle *p = *it;
        p->base.pos = self->base.pos;
        scatter_update_petal(p);
        p->base.age++;
    }
}

void scatter_effect_render(struct Effect *effect)
{
    struct ScatterEffect *self = (struct ScatterEffect *)effect;
    struct SpriteParticle **it;

    EACH_PETAL(self, it)
        if (*it)
            sprite_particle_render(*it);
}
