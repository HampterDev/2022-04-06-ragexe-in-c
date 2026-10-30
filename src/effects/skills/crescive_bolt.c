#include "effects/effect_allocator.h"
#include "render/projection.h"
#include <math.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"

#define EFFECT_CRESCIVE_BOLT 0x79f

struct CresciveBolt {
    struct Effect base;
    struct Ez2StrParticle *bolt;
};

void crescive_bolt_init(struct Effect *self);
void crescive_bolt_process(struct Effect *self);
void crescive_bolt_render(struct Effect *self);
void crescive_bolt_release(struct Effect *self);
unsigned char crescive_bolt_is_alive(struct Effect *self);
static void crescive_bolt_destroy(struct Effect *effect);

static const struct EffectOps crescive_bolt_ops = {
    .set_submit = effect_set_submit,
    .init = crescive_bolt_init,
    .update_range = effect_update_range,
    .process = crescive_bolt_process,
    .render = crescive_bolt_render,
    .release = crescive_bolt_release,
    .is_alive = crescive_bolt_is_alive,
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
    .destroy = crescive_bolt_destroy,
    .fade_out = effect_fade_out,
};

struct CresciveBolt *crescive_bolt_construct(struct CresciveBolt *self)
{
    effect_construct(&self->base, EFFECT_CRESCIVE_BOLT);
    self->base.ops = &crescive_bolt_ops;
    self->bolt = 0;
    return self;
}

struct Effect *crescive_bolt_new(void)
{
    struct CresciveBolt *self = mem_try_alloc(sizeof *self);

    return self ? &crescive_bolt_construct(self)->base : NULL;
}

void crescive_bolt_destruct(struct CresciveBolt *self)
{
    self->base.ops = &crescive_bolt_ops;
    effect_destruct(&self->base);
}

static void crescive_bolt_destroy(struct Effect *effect)
{
    struct CresciveBolt *self = (struct CresciveBolt *)effect;
    crescive_bolt_destruct(self);
    mem_free(self);
}

void crescive_bolt_release(struct Effect *effect)
{
    struct CresciveBolt *self = (struct CresciveBolt *)effect;
    if (self->bolt) {
        particle_free(&self->bolt->base);
        self->bolt = 0;
    }
}

void crescive_bolt_init(struct Effect *effect)
{
    struct CresciveBolt *self = (struct CresciveBolt *)effect;
    float from_x, from_y, from_rhw = 0, to_x, to_y, to_rhw = 0;
    float view[12], dx, dy, deg;
    double angle;
    Vec3 v;
    struct Ez2StrParticle *p;

    angle = 0.0;
    if (game_copy_view_matrix(view)) {
        v = self->base.pos;
        renderer_project_point(g_renderer, &v, view, &from_x, &from_y, &from_rhw);
        v = self->base.target_pos;
        renderer_project_point(g_renderer, &v, view, &to_x, &to_y, &to_rhw);

        dx = to_x - from_x;
        dy = to_y - from_y;
        deg = (float)acos((double)-(dx / float_sqrt(dy * dy + dx * dx))) / g_pi * g_halfTurnDegrees;
        angle = deg;
        if (to_y > from_y)
            angle = 360.0 - angle;
    }

    p = ez2str_particle_new(self->base.pos, 5);
    if (p) {
        const char *path = "crescivebolt\\crescivebolt\\crescivebolt.str";
        ez2_str_particle_open(p, path);
        p->spin_degrees = (float)angle;
        self->bolt = p;
    }
    effect_add_sound_cue(&self->base, SOUND_CUE_WAVE, "effect\\WH_CRESCIVE_BOLT.wav", 0, 0.0f);
}

unsigned char crescive_bolt_is_alive(struct Effect *effect)
{
    struct CresciveBolt *self = (struct CresciveBolt *)effect;
    struct Ez2StrParticle *p = self->bolt;

    if (!p)
        return 0;
    if (p->base.life_seconds == 0.0f)
        return 1;
    return p->base.life_seconds * 60.0f > (float)p->base.age;
}

void crescive_bolt_process(struct Effect *effect)
{
    struct CresciveBolt *self = (struct CresciveBolt *)effect;
    Vec3 zero = { 0, 0, 0 }, here;
    struct Ez2StrParticle *p = self->bolt;

    if (!p)
        return;
    here = self->base.pos;
    p->base.pos.x = here.x + 0.0f;
    p->base.pos.y = here.y - 3.0f;
    p->base.pos.z = here.z + 0.0f;
    ez2_str_particle_process(self->bolt, zero);
}

void crescive_bolt_render(struct Effect *effect)
{
    struct CresciveBolt *self = (struct CresciveBolt *)effect;

    if (!self->bolt)
        return;
    ez2_str_particle_render_current(self->bolt);
}
