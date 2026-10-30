#include "effects/effect_allocator.h"
#include "render/projection.h"
#include <math.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"

enum {
    EFFECT_CRIMSON_ARROW = 0x6dc,
    EFFECT_RHYTHM_SHOOTING = 0x81a,
    EFFECT_FRENZY_SHOT = 0x842,
    EFFECT_UNLUCKY_RUSH = 0x86a,
    EFFECT_JUDGEMENT_CROSS = 0x893,
};

struct CrimsonArrowEffect {
    struct Effect base;
    struct Ez2StrParticle *arrow;
};

#define ARROW_OFFSET(p) ((p)->base.offset)

void crimson_arrow_effect_init(struct Effect *self);
void crimson_arrow_effect_process(struct Effect *self);
void crimson_arrow_effect_render(struct Effect *self);
void crimson_arrow_effect_release(struct Effect *self);
unsigned char crimson_arrow_effect_is_alive(struct Effect *self);
static void crimson_arrow_effect_destroy(struct Effect *effect);

static const struct EffectOps crimson_arrow_effect_ops = {
    .set_submit = effect_set_submit,
    .init = crimson_arrow_effect_init,
    .update_range = effect_update_range,
    .process = crimson_arrow_effect_process,
    .render = crimson_arrow_effect_render,
    .release = crimson_arrow_effect_release,
    .is_alive = crimson_arrow_effect_is_alive,
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
    .destroy = crimson_arrow_effect_destroy,
    .fade_out = effect_fade_out,
};

static void open_str(struct Ez2StrParticle *p, const char *path)
{
    ez2_str_particle_open(p, path);
}

static void set_offset_y(struct Ez2StrParticle *p, float y)
{
    ARROW_OFFSET(p).x = 0.0f;
    ARROW_OFFSET(p).y = y;
    ARROW_OFFSET(p).z = 0.0f;
}

struct CrimsonArrowEffect *crimson_arrow_effect_construct(struct CrimsonArrowEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &crimson_arrow_effect_ops;
    self->arrow = 0;
    return self;
}

struct Effect *crimson_arrow_effect_new(void)
{
    struct CrimsonArrowEffect *self = mem_try_alloc(sizeof *self);

    return self ? &crimson_arrow_effect_construct(self)->base : NULL;
}

void crimson_arrow_effect_destruct(struct CrimsonArrowEffect *self)
{
    self->base.ops = &crimson_arrow_effect_ops;
    effect_destruct(&self->base);
}

static void crimson_arrow_effect_destroy(struct Effect *effect)
{
    struct CrimsonArrowEffect *self = (struct CrimsonArrowEffect *)effect;
    crimson_arrow_effect_destruct(self);
    mem_free(self);
}

void crimson_arrow_effect_release(struct Effect *effect)
{
    struct CrimsonArrowEffect *self = (struct CrimsonArrowEffect *)effect;
    if (self->arrow) {
        particle_free(&self->arrow->base);
        self->arrow = 0;
    }
}

void crimson_arrow_effect_init(struct Effect *effect)
{
    struct CrimsonArrowEffect *self = (struct CrimsonArrowEffect *)effect;
    float from_x, from_y, from_rhw = 0, to_x, to_y, to_rhw = 0;
    float view[12], dx, dy, deg;
    double angle;
    Vec3 from, to;
    struct Ez2StrParticle *p;

    p = ez2str_particle_new(self->base.pos, 5);
    if (!p)
        return;
    to = self->base.target_pos;
    from = self->base.pos;
    angle = 0.0;
    if (game_copy_view_matrix(view)) {
        renderer_project_point(g_renderer, &from, view, &from_x, &from_y, &from_rhw);
        renderer_project_point(g_renderer, &to, view, &to_x, &to_y, &to_rhw);

        dx = to_x - from_x;
        dy = to_y - from_y;
        deg = (float)acos((double)(dx / float_sqrt(dy * dy + dx * dx))) / g_pi * g_halfTurnDegrees;
        if (from_y > to_y)
            angle = 360.0 - (double)deg;
        else
            angle = deg;
    }

    switch (self->base.type) {
    case EFFECT_CRIMSON_ARROW:
        open_str(p, "crimsonarrow\\crimsonarrow\\crimsonarrow.str");
        set_offset_y(p, -13.0f);
        p->spin_degrees = (float)angle;
        effect_add_sound_cue(&self->base, SOUND_CUE_WAVE, "effect\\AG_CRIMSON_ARROW1.wav", 0, 0.0f);
        break;
    case EFFECT_RHYTHM_SHOOTING:
        open_str(p, "rhythmshooting\\rhythmshooting_shot\\rhythmshooting_shot.str");
        set_offset_y(p, -8.0f);
        p->spin_degrees = (float)(angle + 180.0);
        effect_add_sound_cue(&self->base, SOUND_CUE_WAVE, "effect\\TR_RHYTHMSHOOTING.wav", 0, 0.0f);
        break;
    case EFFECT_FRENZY_SHOT:
        open_str(p, "frenzy_shot\\frenzy_shot\\frenzy_shot.str");
        set_offset_y(p, -4.0f);
        p->spin_degrees = (float)angle;
        effect_add_sound_cue(&self->base, SOUND_CUE_WAVE, "effect\\ABC_FRENZY_SHOT.wav", 0, 0.0f);
        break;
    case EFFECT_UNLUCKY_RUSH:
        open_str(p, "unlucky_rush\\unlucky_rush\\unlucky_rush.str");
        p->spin_degrees = (float)angle;
        break;
    case EFFECT_JUDGEMENT_CROSS:
        open_str(p, "judgement_cross\\judgement_cross_shot\\judgement_cross_shot.str");
        p->spin_degrees = (float)(angle + 180.0);
        break;
    }
    self->arrow = p;
}

unsigned char crimson_arrow_effect_is_alive(struct Effect *effect)
{
    struct CrimsonArrowEffect *self = (struct CrimsonArrowEffect *)effect;
    struct Ez2StrParticle *p = self->arrow;

    if (!p)
        return 0;
    if (p->base.life_seconds == 0.0f)
        return 1;
    return p->base.life_seconds * 60.0f > (float)p->base.age;
}

void crimson_arrow_effect_process(struct Effect *effect)
{
    struct CrimsonArrowEffect *self = (struct CrimsonArrowEffect *)effect;
    Vec3 zero = { 0, 0, 0 }, here;
    struct Ez2StrParticle *p = self->arrow;

    if (!p)
        return;
    here = self->base.pos;
    p->base.pos.x = ARROW_OFFSET(p).x + here.x;
    p->base.pos.y = ARROW_OFFSET(p).y + here.y;
    p->base.pos.z = ARROW_OFFSET(p).z + here.z;
    ez2_str_particle_process(self->arrow, zero);
}

void crimson_arrow_effect_render(struct Effect *effect)
{
    struct CrimsonArrowEffect *self = (struct CrimsonArrowEffect *)effect;

    if (!self->arrow)
        return;
    ez2_str_particle_render_current(self->arrow);
}
