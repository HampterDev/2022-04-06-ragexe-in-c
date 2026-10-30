#include "effects/effect_allocator.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "game/camera.h"

#define EFFECT_FALLEN_ANGEL 0x474

struct FallenAngelEffect {
    struct Effect base;
    struct Ez2StrParticle *anim;
    Vec3 start;
};

void fallen_angel_effect_init(struct Effect *self);
void fallen_angel_effect_process(struct Effect *self);
void fallen_angel_effect_render(struct Effect *self);
void fallen_angel_effect_release(struct Effect *self);
unsigned char fallen_angel_effect_is_alive(struct Effect *self);
static void fallen_angel_effect_destroy(struct Effect *effect);

static const struct EffectOps fallen_angel_effect_ops = {
    .set_submit = effect_set_submit,
    .init = fallen_angel_effect_init,
    .update_range = effect_update_range,
    .process = fallen_angel_effect_process,
    .render = fallen_angel_effect_render,
    .release = fallen_angel_effect_release,
    .is_alive = fallen_angel_effect_is_alive,
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
    .destroy = fallen_angel_effect_destroy,
    .fade_out = effect_fade_out,
};

static Vec3 anim_lift(void)
{
    Vec3 lift;
    lift.x = 0;
    lift.y = -80.0f;
    lift.z = 0;
    return lift;
}

struct FallenAngelEffect *fallen_angel_effect_construct(struct FallenAngelEffect *self)
{
    effect_construct(&self->base, EFFECT_FALLEN_ANGEL);
    self->base.ops = &fallen_angel_effect_ops;
    self->anim = 0;
    self->start.x = self->start.y = self->start.z = 0;
    return self;
}

struct Effect *fallen_angel_effect_new(void)
{
    struct FallenAngelEffect *self = mem_try_alloc(sizeof *self);

    return self ? &fallen_angel_effect_construct(self)->base : NULL;
}

void fallen_angel_effect_destruct(struct FallenAngelEffect *self)
{
    self->base.ops = &fallen_angel_effect_ops;
    effect_destruct(&self->base);
}

static void fallen_angel_effect_destroy(struct Effect *effect)
{
    struct FallenAngelEffect *self = (struct FallenAngelEffect *)effect;
    fallen_angel_effect_destruct(self);
    mem_free(self);
}

void fallen_angel_effect_release(struct Effect *effect)
{
    struct FallenAngelEffect *self = (struct FallenAngelEffect *)effect;
    particle_free(&self->anim->base);
}

void fallen_angel_effect_init(struct Effect *effect)
{
    struct FallenAngelEffect *self = (struct FallenAngelEffect *)effect;
    Vec3 here;

    if (!self->anim) {
        self->anim = ez2str_particle_new(self->base.pos, 5);
        ez2_str_particle_load(self->anim, "RL_fallen_angel\\Penne.str");
    } else {
        ez2_str_particle_rewind(self->anim);
    }
    particle_quad_unit_corners(self->anim->base.corners);
    here = self->base.pos;
    self->start = here;
}

unsigned char fallen_angel_effect_is_alive(struct Effect *effect)
{
    struct FallenAngelEffect *self = (struct FallenAngelEffect *)effect;
    float life = self->anim->base.life_seconds;
    if (life == 0.0f)
        return 1;
    return life * 60.0f > (float)self->anim->base.age;
}

void fallen_angel_effect_process(struct Effect *effect)
{
    struct FallenAngelEffect *self = (struct FallenAngelEffect *)effect;
    Vec3 at = self->anim->base.pos, here = self->base.pos;
    float dx = self->start.x - here.x;
    float dz = self->start.z - here.z;
    float inv_len = (float)(1.0 / (double)float_sqrt(dx * dx + 0.0f + dz * dz));

    self->anim->base.pos.x = inv_len * dx * 0.09f + at.x;
    self->anim->base.pos.z = inv_len * dz * 0.09f + at.z;
    self->anim->base.pos.y = inv_len * 0.0f + at.y;
    ez2_str_particle_process(self->anim, anim_lift());
    self->anim->base.age++;
}

void fallen_angel_effect_render(struct Effect *effect)
{
    struct FallenAngelEffect *self = (struct FallenAngelEffect *)effect;
    struct Ez2StrParticle *anim = self->anim;

    if (!anim)
        return;
    ez2_str_particle_render_current(anim);
}

void fallen_angel_effect_set_position(struct FallenAngelEffect *self, Vec3 pos)
{
    float y = (g_viewDistance - g_viewDistanceMin) * 0.0625f + 10.0f + pos.y;

    self->base.pos.x = pos.x;
    self->base.pos.z = pos.z;
    self->base.pos.y = y;
    self->anim->base.pos.x = pos.x;
    self->anim->base.pos.y = y;
    self->anim->base.pos.z = pos.z;
}
