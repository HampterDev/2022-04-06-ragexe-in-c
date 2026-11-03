#include "effects/effect_allocator.h"
#include "resources/attr_queries.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"

struct Effect;
struct GravityControlEffect;
void gravity_control_effect_init(struct Effect *self);
void gravity_control_effect_process(struct Effect *self);
void gravity_control_effect_render(struct Effect *self);
void gravity_control_effect_release(struct Effect *self);
static void gravity_control_effect_destroy(struct Effect *effect);

static const struct EffectOps gravity_control_effect_ops = {
    .set_submit = effect_set_submit,
    .init = gravity_control_effect_init,
    .update_range = effect_update_range,
    .process = gravity_control_effect_process,
    .render = gravity_control_effect_render,
    .release = gravity_control_effect_release,
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
    .destroy = gravity_control_effect_destroy,
    .fade_out = effect_fade_out,
};

struct GravityControlParticle {
    float lift;
};
PARTICLE_EFFECT_DATA_FITS(GravityControlParticle);
static __inline struct GravityControlParticle *gravity_control_particle(const void *p)
{
    return (struct GravityControlParticle *)((const struct Particle *)p)->effect_data;
}

#define EFFECT_GRAVITY_CONTROL 0x4b1
#define LIFT_STEP 0.5f

struct GravityControlEffect {
    struct Effect base;
    struct Ez2StrParticle *particle;
    float lift_limit;
};

static int ground_height_under(const void *actor, float *height)
{
    void *ground = game_current_map_attr();
    if (!actor || !ground)
        return 0;
    *height = c3d_attr_get_height(ground, ACTOR_POS(actor)->x, ACTOR_POS(actor)->z);
    return 1;
}

struct GravityControlEffect *gravity_control_effect_construct(struct GravityControlEffect *self)
{
    effect_construct(&self->base, EFFECT_GRAVITY_CONTROL);
    self->base.ops = &gravity_control_effect_ops;
    self->particle = 0;
    self->lift_limit = -20.0f;
    return self;
}

struct Effect *gravity_control_effect_new(void)
{
    struct GravityControlEffect *self = mem_try_alloc(sizeof *self);

    return self ? &gravity_control_effect_construct(self)->base : NULL;
}

/* Effect types created as GravityControlEffect, registered at startup through g_effectTypes
   (effects/effect_registry.c). A type missing here is not created. */
static const struct EffectTypeRow gravity_control_effect_type_rows[] = {
    EFFECT_TYPE(0x4b1, gravity_control_effect_new),
};
const struct EffectTypeTable gravity_control_effect_types = EFFECT_TYPE_TABLE(gravity_control_effect_type_rows);

static void gravity_control_effect_destroy(struct Effect *effect)
{
    struct GravityControlEffect *self = (struct GravityControlEffect *)effect;
    self->base.ops = &gravity_control_effect_ops;
    effect_destruct(&self->base);
    mem_free(self);
}

void gravity_control_effect_init(struct Effect *effect)
{
    struct GravityControlEffect *self = (struct GravityControlEffect *)effect;
    struct Ez2StrParticle *p = ez2str_particle_new(self->base.pos, 5);

    if (!p)
        return;
    p->loop_forever = 1;
    ez2_str_particle_load(p, "WL_TELEKINESIS_INTENSE.str");
    gravity_control_particle(p)->lift = 0;
    particle_quad_unit_corners(p->base.corners);
    self->particle = p;
}

void gravity_control_effect_lift_owner(struct GravityControlEffect *self,
                                                 struct Ez2StrParticle *p)
{
    void *owner;
    float lift, ground_height;
    Vec3 no_offset;

    if (!p)
        return;
    owner = effect_find_actor(self->base.owner_aid);
    if (!ground_height_under(owner, &ground_height))
        return;
    lift = gravity_control_particle(p)->lift - LIFT_STEP;
    lift = self->lift_limit > lift ? self->lift_limit : lift;
    gravity_control_particle(p)->lift = lift;
    ((struct RenderObject *)owner)->pos.y = ground_height + lift;
    no_offset.x = no_offset.y = no_offset.z = 0;
    ez2_str_particle_process(p, no_offset);
    p->base.pos = self->base.pos;
}

void gravity_control_effect_process(struct Effect *effect)
{
    struct GravityControlEffect *self = (struct GravityControlEffect *)effect;
    if (self->particle) {
        gravity_control_effect_lift_owner(self, self->particle);
        self->particle->base.age++;
    }
}

void gravity_control_effect_render_particle(struct GravityControlEffect *self,
                                                      struct Ez2StrParticle *p)
{
    if (!p)
        return;
    ez2_str_particle_render_current(p);
}

void gravity_control_effect_render(struct Effect *effect)
{
    struct GravityControlEffect *self = (struct GravityControlEffect *)effect;
    gravity_control_effect_render_particle(self, self->particle);
}

void gravity_control_effect_release(struct Effect *effect)
{
    struct GravityControlEffect *self = (struct GravityControlEffect *)effect;
    void *owner;
    float ground_height;

    owner = effect_find_actor(self->base.owner_aid);
    if (ground_height_under(owner, &ground_height)) {
        ((struct RenderObject *)owner)->pos.y = ground_height;
        effect_play_wave_at("effect\\t_공격력.wav", &self->base.pos, 0.0f);
    }
    if (self->particle) {
        particle_free(&self->particle->base);
        self->particle = NULL;
    }
}
