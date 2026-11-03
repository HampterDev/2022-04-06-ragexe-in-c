#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/vec.h"

struct CommonFloorEffect;
struct Effect;
struct String;
void common_floor_effect_init(struct Effect *self);
void common_floor_effect_process(struct Effect *self);
void common_floor_effect_render(struct Effect *self);
void common_floor_effect_release(struct Effect *self);
void common_floor_effect_apply_settings_to_all(struct Effect *self);
void common_floor_effect_set_color(struct Effect *self, int r, int g, int b);
void common_floor_effect_set_texture(struct Effect *self, struct String name);
static void common_floor_effect_destroy(struct Effect *effect);

static const struct EffectOps common_floor_effect_ops = {
    .set_submit = effect_set_submit,
    .init = common_floor_effect_init,
    .update_range = effect_update_range,
    .process = common_floor_effect_process,
    .render = common_floor_effect_render,
    .release = common_floor_effect_release,
    .is_alive = effect_is_owner_alive,
    .restart = common_floor_effect_apply_settings_to_all,
    .set_color = common_floor_effect_set_color,
    .set_texture = common_floor_effect_set_texture,
    .after_init = effect_after_init,
    .skip = effect_skip,
    .set_stop_frame = effect_set_stop_frame,
    .set_start_delay = effect_set_start_delay,
    .set_spin = effect_set_spin,
    .set_str_name = effect_set_str_name,
    .set_list_flags = effect_set_list_flags,
    .set_zoom = effect_set_zoom,
    .set_alpha_scale = effect_set_alpha_scale,
    .destroy = common_floor_effect_destroy,
    .fade_out = effect_fade_out,
};

#define LIST_WORLD 5
#define TEXTURE "effect\\ra_unlimit.tga"
#define HALF_PI 1.5707962513f

struct CommonFloorParticle {
    int start_angle;
    float spin;
};
PARTICLE_EFFECT_DATA_FITS(CommonFloorParticle);
static __inline struct CommonFloorParticle *common_floor_particle(const struct Particle *p)
{
    return (struct CommonFloorParticle *)p->effect_data;
}
#define particle_owner_type(p) ((p)->effect_type)
#define particle_tag(p)        ((p)->owner_tag)
#define particle_index(p)      ((p)->index)
#define particle_row(p)        ((p)->group)
#define particle_start_angle(p) (common_floor_particle(p)->start_angle)

#define effect_particle_scale(e) ((e)->base.params.scale)
#define effect_particle_roll(e)  ((e)->base.params.roll)
#define effect_angle_step(e)     ((e)->base.params.rise_angle)
#define effect_particle_alpha(e) ((e)->base.params.alpha)

struct CommonFloorEffect {
    struct Effect base;
    struct Vec particles;
};

#define EACH_PARTICLE(self, it) \
    for (it = (struct Particle **)(self)->particles.first; it != (struct Particle **)(self)->particles.last; it++)

static void set_texture(struct Particle *p, const char *name)
{
    struct String s;
    string_init(&s);
    string_assign_cstr(&s, name);
    particle_set_texture(p, s);
}

struct CommonFloorEffect *common_floor_effect_construct(struct CommonFloorEffect *self)
{
    effect_construct(&self->base, 0x420);
    self->base.ops = &common_floor_effect_ops;
    self->particles.first = self->particles.last = self->particles.end = 0;
    return self;
}

struct Effect *common_floor_effect_new(void)
{
    struct CommonFloorEffect *self = mem_try_alloc(sizeof *self);

    return self ? &common_floor_effect_construct(self)->base : NULL;
}

/* Effect types created as CommonFloorEffect, registered at startup through g_effectTypes
   (effects/effect_registry.c). A type missing here is not created. */
static const struct EffectTypeRow common_floor_effect_type_rows[] = {
    EFFECT_TYPE(0x420, common_floor_effect_new),
};
const struct EffectTypeTable common_floor_effect_types = EFFECT_TYPE_TABLE(common_floor_effect_type_rows);

void common_floor_effect_destruct(struct CommonFloorEffect *self)
{
    self->base.ops = &common_floor_effect_ops;
    vec_free(&self->particles);
    effect_destruct(&self->base);
}

static void common_floor_effect_destroy(struct Effect *effect)
{
    struct CommonFloorEffect *self = (struct CommonFloorEffect *)effect;
    common_floor_effect_destruct(self);
    mem_free(self);
}

void common_floor_effect_init(struct Effect *effect)
{
    struct CommonFloorEffect *self = (struct CommonFloorEffect *)effect;
    int angle, index;

    for (angle = 0, index = 0; angle < 360; angle += 180, index++) {
        struct Particle *p = particle_new(self->base.pos, LIST_WORLD);
        if (!p)
            continue;
        set_texture(p, TEXTURE);
        particle_owner_type(p) = self->base.type;
        particle_index(p) = index;
        particle_row(p) = 0;
        particle_tag(p) = 1;
        p->alpha = 90;
        p->roll = (float)angle;
        particle_start_angle(p) = angle;
        common_floor_particle(p)->spin = 6.0f;
        p->scale = 15.0f;
        particle_quad_unit_corners(p->corners);
        vec_push(&self->particles, &p, sizeof p);
    }
}

void common_floor_effect_release(struct Effect *effect)
{
    struct CommonFloorEffect *self = (struct CommonFloorEffect *)effect;
    struct Particle **it;
    EACH_PARTICLE(self, it)
        particle_free(*it);
    self->particles.last = self->particles.first;
}

void common_floor_effect_apply_settings(struct CommonFloorEffect *self,
                                                  struct Particle *p)
{
    p->list_flags = LIST_WORLD;
    p->pos = self->base.pos;
    p->scale = effect_particle_scale(self);
    p->roll = (float)effect_particle_roll(self);
    particle_start_angle(p) = (int)((float)particle_row(p) * effect_angle_step(self));
    p->alpha = effect_particle_alpha(self);
    p->src_blend = 5;
    p->dest_blend = 7;
    particle_quad_unit_corners(p->corners);
    p->age = 0;
}

void common_floor_effect_apply_settings_to_all(struct Effect *effect)
{
    struct CommonFloorEffect *self = (struct CommonFloorEffect *)effect;
    struct Particle **it;
    EACH_PARTICLE(self, it)
        common_floor_effect_apply_settings(self, *it);
}

void common_floor_effect_set_color(struct Effect *effect, int r, int g, int b)
{
    struct CommonFloorEffect *self = (struct CommonFloorEffect *)effect;
    struct Particle **it;
    EACH_PARTICLE(self, it)
        particle_set_color(*it, (short)r, (short)g, (short)b);
}

void common_floor_effect_set_texture(struct Effect *effect,
                                               struct String name)
{
    struct CommonFloorEffect *self = (struct CommonFloorEffect *)effect;
    struct Particle **it;
    EACH_PARTICLE(self, it) {
        struct String copy;
        string_copy(&copy, &name);
        particle_set_texture(*it, copy);
    }
    string_free(&name);
}

void common_floor_effect_spin(struct Particle *p)
{
    float turn[12], scale[12];
    int angle;

    if (!p)
        return;
    angle = (int)(common_floor_particle(p)->spin + p->roll);
    if (angle >= 360)
        angle -= 360;
    else if (angle < 0)
        angle += 360;
    p->roll = (float)angle;
    mat_set_rotate_z(turn, p->roll / g_halfTurnDegrees * g_pi);
    mat_rotate_x(turn, HALF_PI);
    mat_set_scale(scale, p->scale, p->scale, 1.0f);
    mat_multiply(p->matrix, scale, turn);
}

void common_floor_effect_process(struct Effect *effect)
{
    struct CommonFloorEffect *self = (struct CommonFloorEffect *)effect;
    struct Particle **it;
    EACH_PARTICLE(self, it) {
        struct Particle *p = *it;
        p->pos = self->base.pos;
        common_floor_effect_spin(p);
        p->age++;
    }
}

void common_floor_render_particle(struct Particle *p)
{
    if (p)
        particle_render(p, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
}

void common_floor_effect_render(struct Effect *effect)
{
    struct CommonFloorEffect *self = (struct CommonFloorEffect *)effect;
    struct Particle **it;
    EACH_PARTICLE(self, it)
        common_floor_render_particle(*it);
}
