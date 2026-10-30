#include "effects/effect_allocator.h"
#include "render/projection.h"
#include <math.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"

#define EFFECT_SAVAGE_IMPACT 0x72d
#define LIST_WORLD 5
#define LIFT 12.0f

struct SavageImpactEffect {
    struct Effect base;
    struct Ez2StrParticle *slash;
    Vec3 to_target;
};

void savage_impact_effect_init(struct Effect *self);
void savage_impact_effect_process(struct Effect *self);
void savage_impact_effect_render(struct Effect *self);
void savage_impact_effect_release(struct Effect *self);
unsigned char savage_impact_effect_is_alive(struct Effect *self);
static void savage_impact_effect_destroy(struct Effect *effect);

static const struct EffectOps savage_impact_effect_ops = {
    .set_submit = effect_set_submit,
    .init = savage_impact_effect_init,
    .update_range = effect_update_range,
    .process = savage_impact_effect_process,
    .render = savage_impact_effect_render,
    .release = savage_impact_effect_release,
    .is_alive = savage_impact_effect_is_alive,
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
    .destroy = savage_impact_effect_destroy,
    .fade_out = effect_fade_out,
};

static const Vec3 no_offset = { 0, 0, 0 };

struct SavageImpactEffect *savage_impact_effect_construct(struct SavageImpactEffect *self)
{
    effect_construct(&self->base, EFFECT_SAVAGE_IMPACT);
    self->base.ops = &savage_impact_effect_ops;
    self->slash = NULL;
    self->to_target.x = self->to_target.y = self->to_target.z = 0;
    return self;
}

struct Effect *savage_impact_effect_new(void)
{
    struct SavageImpactEffect *self = mem_try_alloc(sizeof *self);

    return self ? &savage_impact_effect_construct(self)->base : NULL;
}

void savage_impact_effect_destruct(struct SavageImpactEffect *self)
{
    self->base.ops = &savage_impact_effect_ops;
    effect_destruct(&self->base);
}

static void savage_impact_effect_destroy(struct Effect *effect)
{
    struct SavageImpactEffect *self = (struct SavageImpactEffect *)effect;
    savage_impact_effect_destruct(self);
    mem_free(self);
}

void savage_impact_effect_release(struct Effect *effect)
{
    struct SavageImpactEffect *self = (struct SavageImpactEffect *)effect;
    if (self->slash) {
        particle_free(&self->slash->base);
        self->slash = NULL;
    }
}

void savage_impact_effect_init(struct Effect *effect)
{
    struct SavageImpactEffect *self = (struct SavageImpactEffect *)effect;
    float view[12];
    float from_x, from_y, to_x, to_y, rhw_from = 0, rhw_to = 0;
    float dx, dy, len, deg;
    double angle;
    Vec3 at;
    const char *name = "savage_impact\\savage_impact\\savage_impact.str";

    self->slash = ez2str_particle_new(self->base.pos, LIST_WORLD);
    if (!self->slash)
        return;
    self->to_target.x = self->base.target_pos.x - self->base.pos.x;
    self->to_target.y = self->base.target_pos.y - self->base.pos.y;
    self->to_target.z = self->base.target_pos.z - self->base.pos.z;

    angle = 0.0;
    if (game_copy_view_matrix(view)) {
        at = self->base.pos;
        renderer_project_point(g_renderer, &at, view, &from_x, &from_y, &rhw_from);
        at = self->base.target_pos;
        renderer_project_point(g_renderer, &at, view, &to_x, &to_y, &rhw_to);

        dx = to_x - from_x;
        dy = to_y - from_y;
        len = float_sqrt(dy * dy + dx * dx);
        deg = (float)acos((double)(dx / len)) / 3.141592f * 180.0f;
        angle = (double)deg;
        if (from_y > to_y)
            angle = 360.0 - angle;
    }

    ez2_str_particle_open(self->slash, name);
    self->slash->spin_degrees = (float)angle;
    effect_add_sound_cue(&self->base, SOUND_CUE_WAVE, "effect\\SHC_SAVAGE_IMPACT.wav", 0, 0.0f);
}

unsigned char savage_impact_effect_is_alive(struct Effect *effect)
{
    struct SavageImpactEffect *self = (struct SavageImpactEffect *)effect;
    struct Ez2StrParticle *slash = self->slash;
    float life;

    if (!slash)
        return 0;
    life = slash->base.life_seconds;
    if (life == 0.0f)
        return 1;
    return life * 60.0f > (float)slash->base.age;
}

void savage_impact_effect_process_part(struct SavageImpactEffect *self,
                                                 struct Ez2StrParticle *p)
{
    Vec3 here = self->base.pos;
    if (!p)
        return;
    p->base.pos.x = here.x + 0.0f;
    p->base.pos.y = here.y - LIFT;
    p->base.pos.z = here.z + 0.0f;
    ez2_str_particle_process(p, no_offset);
}

void savage_impact_effect_process(struct Effect *effect)
{
    struct SavageImpactEffect *self = (struct SavageImpactEffect *)effect;
    savage_impact_effect_process_part(self, self->slash);
}

void savage_impact_effect_render(struct Effect *effect)
{
    struct SavageImpactEffect *self = (struct SavageImpactEffect *)effect;
    struct Ez2StrParticle *slash = self->slash;

    ez2_str_particle_render_current(slash);
}
