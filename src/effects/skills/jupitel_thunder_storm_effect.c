#include "effects/effect_allocator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "effects/skills/ez2str_effect_ex.h"
#include "std/vec.h"

#define EFFECT_JUPITEL_THUNDER_STORM 0x93e

struct JupitelThunderStormEffect {
    struct Effect base;
    struct Ez2StrParticle *ball;
    struct Vec bolts;
    Vec3 step;
    int travel_frames;
};

#define EFFECT_LEVEL(e) ((e)->level)

void jupitel_thunder_storm_effect_init(struct Effect *self);
void jupitel_thunder_storm_effect_process(struct Effect *self);
void jupitel_thunder_storm_effect_render(struct Effect *self);
void jupitel_thunder_storm_effect_release(struct Effect *self);
unsigned char jupitel_thunder_storm_effect_is_alive(struct Effect *self);
static void jupitel_thunder_storm_effect_destroy(struct Effect *effect);

static const struct EffectOps jupitel_thunder_storm_effect_ops = {
    .set_submit = effect_set_submit,
    .init = jupitel_thunder_storm_effect_init,
    .update_range = effect_update_range,
    .process = jupitel_thunder_storm_effect_process,
    .render = jupitel_thunder_storm_effect_render,
    .release = jupitel_thunder_storm_effect_release,
    .is_alive = jupitel_thunder_storm_effect_is_alive,
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
    .destroy = jupitel_thunder_storm_effect_destroy,
    .fade_out = effect_fade_out,
};

static int still_playing(const struct Ez2StrParticle *p)
{
    return p->base.life_seconds == 0.0f || p->base.life_seconds * 60.0f > (float)p->base.age;
}

static int random_below(int span)
{
    int r;
    if (span == 0)
        return 0;
    r = rand() % span;
    return r < 0 ? 0 : r;
}

struct JupitelThunderStormEffect *
jupitel_thunder_storm_effect_construct(struct JupitelThunderStormEffect *self)
{
    effect_construct(&self->base, EFFECT_JUPITEL_THUNDER_STORM);
    self->base.ops = &jupitel_thunder_storm_effect_ops;
    self->ball = 0;
    self->bolts.first = self->bolts.last = self->bolts.end = 0;
    self->step.x = 0;
    self->step.y = 0;
    self->step.z = 0;
    self->travel_frames = 10;
    return self;
}

struct Effect *jupitel_thunder_storm_effect_new(void)
{
    struct JupitelThunderStormEffect *self = mem_try_alloc(sizeof *self);

    return self ? &jupitel_thunder_storm_effect_construct(self)->base : NULL;
}

void jupitel_thunder_storm_effect_destruct(struct JupitelThunderStormEffect *self)
{
    self->base.ops = &jupitel_thunder_storm_effect_ops;
    vec_free(&self->bolts);
    effect_destruct(&self->base);
}

static void jupitel_thunder_storm_effect_destroy(struct Effect *effect)
{
    struct JupitelThunderStormEffect *self = (struct JupitelThunderStormEffect *)effect;
    jupitel_thunder_storm_effect_destruct(self);
    mem_free(self);
}

void jupitel_thunder_storm_effect_release(struct Effect *effect)
{
    struct JupitelThunderStormEffect *self = (struct JupitelThunderStormEffect *)effect;
    struct Ez2StrParticle **it;

    if (self->ball) {
        particle_free(&self->ball->base);
        self->ball = 0;
    }
    for (it = self->bolts.first; it != (struct Ez2StrParticle **)self->bolts.last; it++)
        if (*it)
            particle_free(&(*it)->base);
    self->bolts.last = self->bolts.first;
}

void jupitel_thunder_storm_effect_init(struct Effect *effect)
{
    struct JupitelThunderStormEffect *self = (struct JupitelThunderStormEffect *)effect;
    static const int quadrant[4][2] = { { -1, 1 }, { 1, 1 }, { -1, -1 }, { 1, -1 } };
    Vec3 from = self->base.pos, to = self->base.target_pos;
    float speed, inv, angle, view[12];
    struct Ez2StrParticle *p;
    int spread, last_delay, delay, i;

    speed = float_sqrt((to.y - from.y) * (to.y - from.y) + (to.x - from.x) * (to.x - from.x) +
                  (to.z - from.z) * (to.z - from.z)) / (float)self->travel_frames;
    self->step.x = to.x - from.x;
    self->step.y = to.y - from.y;
    self->step.z = to.z - from.z;
    inv = 1.0f / float_sqrt(self->step.x * self->step.x + self->step.y * self->step.y + self->step.z * self->step.z);
    self->step.x = inv * self->step.x * speed;
    self->step.y = inv * self->step.y * speed;
    self->step.z = self->step.z * inv * speed;
    angle = 0.0f;
    if (game_copy_view_matrix(view))
        angle = effect_screen_angle(self->base.pos, self->base.target_pos, view);

    p = self->ball = ez2str_particle_new(self->base.pos, 5);
    if (p) {
        const char *path = "HYPER_NOVICE\\HN_JUPITEL_THUNDER_STORM\\jupitel_thunder_storm_ball\\"
                           "jupitel_thunder_storm_ball.str";
        ez2_str_particle_open(p, path);
        self->ball->spin_degrees = angle;
    }

    spread = (EFFECT_LEVEL(&self->base) - 1) / 4 * 10 + 25;
    last_delay = ((EFFECT_LEVEL(&self->base) - 1) / 2 + 6) * 150;
    for (i = 0, delay = 80; delay <= last_delay; i++, delay += 80) {
        char name[256];
        int dz, dx, q;
        float ox, oz;

        p = ez2str_particle_new(self->base.pos, 5);
        if (!p)
            continue;
        dz = random_below(spread / 2);
        dx = random_below(spread / 2);
        q = i % 4;
        ox = (float)quadrant[q][0] * ((float)dx + 0.5f);
        oz = (float)quadrant[q][1] * ((float)dz + 0.5f);
        snprintf(name, sizeof name, "HYPER_NOVICE\\HN_JUPITEL_THUNDER_STORM\\jupitel_thunder_storm\\jupitel_thunder_storm_%02d.str",
                   random_below(3));
        ez2_str_particle_open(p, name);
        to = self->base.target_pos;
        p->base.pos.x = to.x + ox;
        p->base.pos.y = to.y - 15.0f;
        p->base.pos.z = to.z + oz;
        p->start_delay = delay;
        vec_push(&self->bolts, &p, sizeof p);
        if (q == 0)
            effect_add_sound_cue(&self->base, SOUND_CUE_WAVE, "effect\\HYPER_NOVICE\\hu_jupitel_thunder_storm.wav",
                                 delay / 16 + 1, 0.0f);
    }
}

unsigned char jupitel_thunder_storm_effect_is_alive(struct Effect *effect)
{
    struct JupitelThunderStormEffect *self = (struct JupitelThunderStormEffect *)effect;
    struct Ez2StrParticle **it;

    if (self->ball && still_playing(self->ball))
        return 1;
    for (it = self->bolts.first; it != (struct Ez2StrParticle **)self->bolts.last; it++)
        if (*it && still_playing(*it))
            break;
    return it != (struct Ez2StrParticle **)self->bolts.last;
}

void jupitel_thunder_storm_effect_process(struct Effect *effect)
{
    struct JupitelThunderStormEffect *self = (struct JupitelThunderStormEffect *)effect;
    struct Ez2StrParticle *ball = self->ball, **it;
    Vec3 zero = { 0, 0, 0 };

    if (ball) {
        if (!still_playing(ball)) {
            particle_free(&ball->base);
            self->ball = 0;
        } else {
            Vec3 at;
            if (self->base.frame < self->travel_frames) {
                Vec3 here = ball->base.pos;
                at.x = self->step.x + here.x;
                at.y = self->step.y + here.y;
                at.z = self->step.z + here.z;
            } else {
                at = self->base.target_pos;
            }
            ball->base.pos = at;
            ez2_str_particle_process(self->ball, zero);
        }
    }
    for (it = self->bolts.first; it != (struct Ez2StrParticle **)self->bolts.last; it++)
        if (*it)
            ez2_str_particle_process(*it, zero);
}

void jupitel_thunder_storm_effect_render(struct Effect *effect)
{
    struct JupitelThunderStormEffect *self = (struct JupitelThunderStormEffect *)effect;
    struct Ez2StrParticle **it;

    if (self->ball) {
        ez2_str_particle_render_current(self->ball);
    }
    for (it = self->bolts.first; it != (struct Ez2StrParticle **)self->bolts.last; it++)
        if (*it) {
            ez2_str_particle_render_current(*it);
        }
}
