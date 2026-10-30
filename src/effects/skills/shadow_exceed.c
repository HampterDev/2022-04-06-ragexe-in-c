#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "effects/render_object.h"
#include "actor/actor_common.h"

#define EFFECT_SHADOW_EXCEED 0x74b
#define EFFECT_CASTER_AURA 0x3ec
#define ACTOR_MSG_REMOVE_EFFECT 0x6d
#define LIST_WORLD 5
#define LIST_GROUND 0xd

struct ShadowExceed {
    struct Effect base;
    struct Vec parts;
    unsigned char aura_restored;
};

#define PART_OFFSET(p) (&(p)->base.offset)

#define ACTOR_DIRECTION(actor) ((const struct RenderObject *)(actor))->action
#define ACTOR_ANGLE(actor) ((const struct RenderObject *)(actor))->roty

void shadow_exceed_init(struct Effect *self);
void shadow_exceed_process(struct Effect *self);
void shadow_exceed_render(struct Effect *self);
void shadow_exceed_release(struct Effect *self);
static void shadow_exceed_destroy(struct Effect *effect);

static const struct EffectOps shadow_exceed_ops = {
    .set_submit = effect_set_submit,
    .init = shadow_exceed_init,
    .update_range = effect_update_range,
    .process = shadow_exceed_process,
    .render = shadow_exceed_render,
    .release = shadow_exceed_release,
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
    .destroy = shadow_exceed_destroy,
    .fade_out = effect_fade_out,
};

#include "actor/render_ops.h"

static const Vec3 no_offset = { 0, 0, 0 };

static void transform(Vec3 *out, const float *m, float x, float y, float z)
{
    out->x = y * m[3] + x * m[0] + z * m[6] + m[9];
    out->y = y * m[4] + x * m[1] + z * m[7] + m[10];
    out->z = y * m[5] + x * m[2] + z * m[8] + m[11];
}

static struct Ez2StrParticle *open_part(struct ShadowExceed *self, int list_flags, const char *name)
{
    struct Ez2StrParticle *p = ez2str_particle_new(self->base.pos, list_flags);

    ez2_str_particle_open(p, name);
    return p;
}

static void clear_caster_aura(void *actor)
{
    while (able_to_make_effect_is_effect_exist(actor, EFFECT_CASTER_AURA) == 1) {
        render_object_send(actor, NULL, ACTOR_MSG_REMOVE_EFFECT, EFFECT_CASTER_AURA, 0, 0, 0);
    }
}

struct ShadowExceed *shadow_exceed_construct(struct ShadowExceed *self)
{
    effect_construct(&self->base, EFFECT_SHADOW_EXCEED);
    self->base.ops = &shadow_exceed_ops;
    self->parts.first = self->parts.last = self->parts.end = 0;
    self->aura_restored = 0;
    return self;
}

struct Effect *shadow_exceed_new(void)
{
    struct ShadowExceed *self = mem_try_alloc(sizeof *self);

    return self ? &shadow_exceed_construct(self)->base : NULL;
}

void shadow_exceed_destruct(struct ShadowExceed *self)
{
    self->base.ops = &shadow_exceed_ops;
    vec_free(&self->parts);
    effect_destruct(&self->base);
}

static void shadow_exceed_destroy(struct Effect *effect)
{
    struct ShadowExceed *self = (struct ShadowExceed *)effect;
    shadow_exceed_destruct(self);
    mem_free(self);
}

void shadow_exceed_release(struct Effect *effect)
{
    struct ShadowExceed *self = (struct ShadowExceed *)effect;
    struct Ez2StrParticle **it;
    void *caster;

    for (it = self->parts.first; it != (struct Ez2StrParticle **)self->parts.last; it++)
        particle_free(&(*it)->base);
    self->parts.last = self->parts.first;
    caster = effect_find_actor(self->base.owner_aid);
    if (caster)
        clear_caster_aura(caster);
}

void shadow_exceed_init(struct Effect *effect)
{
    struct ShadowExceed *self = (struct ShadowExceed *)effect;
    void *caster;
    int dir;
    struct Ez2StrParticle *p;
    float shift_x, shift_y;
    size_t i, count;

    self->aura_restored = 0;
    caster = effect_find_actor(self->base.owner_aid);
    if (!caster)
        return;
    dir = ACTOR_DIRECTION(caster) % 8;
    if ((unsigned)(dir - 2) <= 3) {
        p = open_part(self, LIST_WORLD, "shadow_exceed\\shadow_exceed\\shadow_exceed_b_shadow.str");
        if (dir == 4 || dir == 5)
            p->flip_v = 1;
        vec_push(&self->parts, &p, sizeof p);
        p = open_part(self, LIST_WORLD, "shadow_exceed\\shadow_exceed\\shadow_exceed_b_shadow_bottom.str");
    } else {
        p = open_part(self, LIST_GROUND, "shadow_exceed\\shadow_exceed\\shadow_exceed_f_shadow.str");
        if (dir == 0 || dir == 1)
            p->flip_v = 1;
        vec_push(&self->parts, &p, sizeof p);
        p = open_part(self, LIST_GROUND, "shadow_exceed\\shadow_exceed\\shadow_exceed_f_shadow_bottom.str");
    }
    vec_push(&self->parts, &p, sizeof p);

    shift_x = (float)((double)get_sin_deg((int)ACTOR_ANGLE(caster)) * -5.0);
    shift_x = shift_x + 0.3f;
    shift_y = (float)((double)-7.3f - (double)get_cos_deg((int)ACTOR_ANGLE(caster)) * 3.5);

    count = VEC_COUNT(&self->parts, struct Ez2StrParticle *);
    for (i = 0; i < count; i++) {
        p = ((struct Ez2StrParticle **)self->parts.first)[i];
        if (p) {
            PART_OFFSET(p)->x = shift_x;
            PART_OFFSET(p)->y = shift_y;
            PART_OFFSET(p)->z = 0;
        }
    }
    effect_add_sound_cue(&self->base, SOUND_CUE_WAVE, "effect\\SHC_SHADOW_EXCEED.wav", 0, 0.0f);
}

void shadow_exceed_place_part(struct ShadowExceed *self, struct Ez2StrParticle *p)
{
    float view[12], to_world[12];
    Vec3 here, seen;

    if (!p)
        return;
    here = self->base.pos;
    if (!game_copy_view_matrix(view))
        return;
    transform(&seen, view, here.x, here.y, here.z);
    mat_invert(to_world, view);
    transform(&p->base.pos, to_world, PART_OFFSET(p)->x + seen.x, PART_OFFSET(p)->y + seen.y,
              PART_OFFSET(p)->z + seen.z);
    ez2_str_particle_process(p, no_offset);
}

void shadow_exceed_process(struct Effect *effect)
{
    struct ShadowExceed *self = (struct ShadowExceed *)effect;
    struct Ez2StrParticle **it = self->parts.first;
    void *caster;

    while (it != (struct Ez2StrParticle **)self->parts.last) {
        struct Ez2StrParticle *p = *it;
        float life = p->base.life_seconds;

        if (!(life == 0.0f) && !(life * 60.0f > (float)p->base.age)) {
            particle_free(&p->base);
            memmove(it, it + 1, (char *)self->parts.last - (char *)(it + 1));
            self->parts.last = (struct Ez2StrParticle **)self->parts.last - 1;
            continue;
        }
        shadow_exceed_place_part(self, p);
        it++;
    }

    if (self->parts.first != self->parts.last || self->aura_restored)
        return;
    self->aura_restored = 1;
    caster = effect_find_actor(self->base.owner_aid);
    if (caster) {
        Vec3 at = { 0, 0, 0 };
        clear_caster_aura(caster);
        game_actor_launch_effect(caster, EFFECT_CASTER_AURA, at, 0);
    }
}

void shadow_exceed_render(struct Effect *effect)
{
    struct ShadowExceed *self = (struct ShadowExceed *)effect;
    struct Ez2StrParticle **it;

    for (it = self->parts.first; it != (struct Ez2StrParticle **)self->parts.last; it++) {
        if (*it) {
            ez2_str_particle_render_current(*it);
        }
    }
}
