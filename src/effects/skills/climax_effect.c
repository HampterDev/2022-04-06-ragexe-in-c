#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"

struct ClimaxEffect {
    struct Effect base;
    struct Ez2StrParticle *parts[2];
};

#define CLIMAX_AURA 0
#define CLIMAX_RING 1

#define EFFECT_LEVEL(e) ((e)->level)

void climax_effect_init(struct Effect *self);
void climax_effect_process(struct Effect *self);
void climax_effect_render(struct Effect *self);
void climax_effect_release(struct Effect *self);
static void climax_effect_destroy(struct Effect *effect);

static const struct EffectOps climax_effect_ops = {
    .set_submit = effect_set_submit,
    .init = climax_effect_init,
    .update_range = effect_update_range,
    .process = climax_effect_process,
    .render = climax_effect_render,
    .release = climax_effect_release,
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
    .destroy = climax_effect_destroy,
    .fade_out = effect_fade_out,
};

static Vec3 transform(const float *m, Vec3 v)
{
    Vec3 out;
    out.x = v.x * m[0] + v.y * m[3] + v.z * m[6] + m[9];
    out.y = v.x * m[1] + v.y * m[4] + v.z * m[7] + m[10];
    out.z = v.x * m[2] + v.y * m[5] + v.z * m[8] + m[11];
    return out;
}

static void open_str(struct Ez2StrParticle *p, const char *path)
{
    ez2_str_particle_open(p, path);
}

struct ClimaxEffect *climax_effect_construct(struct ClimaxEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &climax_effect_ops;
    self->parts[CLIMAX_AURA] = 0;
    self->parts[CLIMAX_RING] = 0;
    return self;
}

struct Effect *climax_effect_new(void)
{
    struct ClimaxEffect *self = mem_try_alloc(sizeof *self);

    return self ? &climax_effect_construct(self)->base : NULL;
}

void climax_effect_destruct(struct ClimaxEffect *self)
{
    self->base.ops = &climax_effect_ops;
    effect_destruct(&self->base);
}

static void climax_effect_destroy(struct Effect *effect)
{
    struct ClimaxEffect *self = (struct ClimaxEffect *)effect;
    climax_effect_destruct(self);
    mem_free(self);
}

void climax_effect_release(struct Effect *effect)
{
    struct ClimaxEffect *self = (struct ClimaxEffect *)effect;
    int i;
    for (i = 0; i < 2; i++)
        if (self->parts[i]) {
            particle_free(&self->parts[i]->base);
            self->parts[i] = 0;
        }
}

void climax_effect_raise_on_screen(struct ClimaxEffect *self, struct Ez2StrParticle *p)
{
    float view[12], from_view[12];
    Vec3 seen;

    if (!effect_find_actor(self->base.owner_aid))
        return;
    if (!game_copy_view_matrix(view))
        return;
    mat_invert(from_view, view);
    seen = transform(view, p->base.pos);
    seen.x = seen.x + 0.0f;
    seen.y = seen.y - 10.0f;
    p->base.pos = transform(from_view, seen);
}

void climax_effect_init(struct Effect *effect)
{
    struct ClimaxEffect *self = (struct ClimaxEffect *)effect;
    struct Ez2StrParticle *p;

    self->base.has_instance_key = 1;
    p = ez2str_particle_new(self->base.pos, 5);
    open_str(p, "climax\\climax\\climax.str");
    p->loop_forever = 1;
    self->parts[CLIMAX_AURA] = p;

    p = ez2str_particle_new(self->base.pos, 13);
    switch (EFFECT_LEVEL(&self->base)) {
    case 1: open_str(p, "climax\\climax_bottom\\climax_bottom_1.str"); break;
    case 2: open_str(p, "climax\\climax_bottom\\climax_bottom_2.str"); break;
    case 3: open_str(p, "climax\\climax_bottom\\climax_bottom_3.str"); break;
    case 4: open_str(p, "climax\\climax_bottom\\climax_bottom_4.str"); break;
    case 5: open_str(p, "climax\\climax_bottom\\climax_bottom_5.str"); break;
    }
    p->loop_forever = 1;
    self->parts[CLIMAX_RING] = p;
    effect_add_sound_cue(&self->base, SOUND_CUE_WAVE, "effect\\AG_CLIMAX.wav", 0, 0.0f);
}

void climax_effect_process(struct Effect *effect)
{
    struct ClimaxEffect *self = (struct ClimaxEffect *)effect;
    Vec3 zero = { 0, 0, 0 };
    int i;

    for (i = 0; i < 2; i++) {
        struct Ez2StrParticle *p = self->parts[i];
        if (!p)
            continue;
        p->base.pos = self->base.pos;
        climax_effect_raise_on_screen(self, p);
        ez2_str_particle_process(p, zero);
        p->base.age++;
    }
}

void climax_effect_render(struct Effect *effect)
{
    struct ClimaxEffect *self = (struct ClimaxEffect *)effect;
    int i;

    for (i = 0; i < 2; i++)
        if (self->parts[i]) {
            ez2_str_particle_render_current(self->parts[i]);
        }
}
