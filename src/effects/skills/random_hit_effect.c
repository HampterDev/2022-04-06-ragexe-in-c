#include "effects/effect_allocator.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"

#define EFFECT_SECOND_FAITH_HIT 0x820
#define EFFECT_SECOND_JUDGEMENT 0x822
#define LIST_WORLD 5
#define HIT_SPOTS 3

struct RandomHitEffect {
    struct Effect base;
    struct Ez2StrParticle *hit;
};

#define HIT_OFFSET(p) (&(p)->base.offset)

void random_hit_effect_init(struct Effect *self);
void random_hit_effect_process(struct Effect *self);
void random_hit_effect_render(struct Effect *self);
void random_hit_effect_release(struct Effect *self);
static void random_hit_effect_destroy(struct Effect *effect);

static const struct EffectOps random_hit_effect_ops = {
    .set_submit = effect_set_submit,
    .init = random_hit_effect_init,
    .update_range = effect_update_range,
    .process = random_hit_effect_process,
    .render = random_hit_effect_render,
    .release = random_hit_effect_release,
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
    .destroy = random_hit_effect_destroy,
    .fade_out = effect_fade_out,
};
__int64 g_randomHitSpot;

static const Vec3 no_offset = { 0, 0, 0 };

static const double spot_x[HIT_SPOTS] = { 0.0, 4.0, -4.0 };
static const double spot_y[HIT_SPOTS] = { -7.0, -6.0, -5.0 };
static const double spot_spin[HIT_SPOTS] = { 0.0, 320.0, 40.0 };

struct RandomHitEffect *random_hit_effect_construct(struct RandomHitEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &random_hit_effect_ops;
    return self;
}

struct Effect *random_hit_effect_new(void)
{
    struct RandomHitEffect *self = mem_try_alloc(sizeof *self);

    return self ? &random_hit_effect_construct(self)->base : NULL;
}

void random_hit_effect_destruct(struct RandomHitEffect *self)
{
    self->base.ops = &random_hit_effect_ops;
    effect_destruct(&self->base);
}

static void random_hit_effect_destroy(struct Effect *effect)
{
    struct RandomHitEffect *self = (struct RandomHitEffect *)effect;
    random_hit_effect_destruct(self);
    mem_free(self);
}

void random_hit_effect_release(struct Effect *effect)
{
    struct RandomHitEffect *self = (struct RandomHitEffect *)effect;
    if (self->hit) {
        particle_free(&self->hit->base);
        self->hit = 0;
    }
}

void random_hit_effect_init(struct Effect *effect)
{
    struct RandomHitEffect *self = (struct RandomHitEffect *)effect;
    const char *name = 0;

    self->hit = ez2str_particle_new(self->base.pos, LIST_WORLD);
    if (self->hit) {
        switch (self->base.type) {
        case EFFECT_SECOND_FAITH_HIT:
            name = "4ig_secondfalth\\secondfalth_hit\\secondfalth_hit.str";
            break;
        case EFFECT_SECOND_JUDGEMENT:
            name = "4iq_secondjudgement\\secondjudgement\\secondjudgement.str";
            break;
        }
    }
    if (name) {
        int n = (int)g_randomHitSpot;

        ez2_str_particle_open(self->hit, name);
        self->hit->spin_degrees = (float)spot_spin[n];
        HIT_OFFSET(self->hit)->x = (float)spot_x[n];
        HIT_OFFSET(self->hit)->y = (float)spot_y[n];
        HIT_OFFSET(self->hit)->z = 0;
        self->hit->start_delay = n * 200;
    }
    if (g_randomHitSpot < 2)
        g_randomHitSpot++;
    else
        g_randomHitSpot = 0;
}

void random_hit_effect_process(struct Effect *effect)
{
    struct RandomHitEffect *self = (struct RandomHitEffect *)effect;
    struct Ez2StrParticle *hit = self->hit;
    Vec3 here;

    if (!hit)
        return;
    here = self->base.pos;
    hit->base.pos.x = HIT_OFFSET(hit)->x + here.x;
    hit->base.pos.y = HIT_OFFSET(hit)->y + here.y;
    hit->base.pos.z = HIT_OFFSET(hit)->z + here.z;
    ez2_str_particle_process(self->hit, no_offset);
    ez2_str_particle_process(self->hit, no_offset);
}

void random_hit_effect_render(struct Effect *effect)
{
    struct RandomHitEffect *self = (struct RandomHitEffect *)effect;

    if (self->hit) {
        ez2_str_particle_render_current(self->hit);
    }
}
