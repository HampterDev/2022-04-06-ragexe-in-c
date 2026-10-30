#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <stdlib.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/vec.h"

struct FloralFlareRoad {
    struct Effect base;
    struct Ez2StrParticle *ground;
    struct Vec bursts;
};

#define EFFECT_FLORAL_FLARE_ROAD 0x70c
#define BURST_SPACING_MS 300
#define BURST_LAST_MS 4500
#define MS_PER_FRAME 16
#define BURST_RADIUS 20

void floral_flare_road_init(struct Effect *self);
void floral_flare_road_process(struct Effect *self);
void floral_flare_road_render(struct Effect *self);
void floral_flare_road_release(struct Effect *self);
static void floral_flare_road_destroy(struct Effect *effect);

static const struct EffectOps floral_flare_road_ops = {
    .set_submit = effect_set_submit,
    .init = floral_flare_road_init,
    .update_range = effect_update_range,
    .process = floral_flare_road_process,
    .render = floral_flare_road_render,
    .release = floral_flare_road_release,
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
    .destroy = floral_flare_road_destroy,
    .fade_out = effect_fade_out,
};

static void render_part(struct Ez2StrParticle *p)
{
    ez2_str_particle_render_current(p);
}

static void step_part(struct Ez2StrParticle *p)
{
    Vec3 lift = { 0.0f, -80.0f, 0.0f };
    ez2_str_particle_process(p, lift);
}

static int random_below(int span)
{
    int r = rand() % span;
    return r < 0 ? 0 : r;
}

struct FloralFlareRoad *floral_flare_road_construct(struct FloralFlareRoad *self)
{
    effect_construct(&self->base, EFFECT_FLORAL_FLARE_ROAD);
    self->base.ops = &floral_flare_road_ops;
    self->bursts.first = self->bursts.last = self->bursts.end = 0;
    return self;
}

struct Effect *floral_flare_road_new(void)
{
    struct FloralFlareRoad *self = mem_try_alloc(sizeof *self);

    return self ? &floral_flare_road_construct(self)->base : NULL;
}

void floral_flare_road_destruct(struct FloralFlareRoad *self)
{
    self->base.ops = &floral_flare_road_ops;
    vec_free(&self->bursts);
    effect_destruct(&self->base);
}

static void floral_flare_road_destroy(struct Effect *effect)
{
    struct FloralFlareRoad *self = (struct FloralFlareRoad *)effect;
    floral_flare_road_destruct(self);
    mem_free(self);
}

void floral_flare_road_release(struct Effect *effect)
{
    struct FloralFlareRoad *self = (struct FloralFlareRoad *)effect;
    struct Particle **it;

    particle_free(&self->ground->base);
    for (it = self->bursts.first; it != (struct Particle **)self->bursts.last; it++)
        particle_free(*it);
    self->bursts.last = self->bursts.first;
}

void floral_flare_road_init(struct Effect *effect)
{
    struct FloralFlareRoad *self = (struct FloralFlareRoad *)effect;
    int delay;

    self->ground = ez2str_particle_new(self->base.pos, 13);
    ez2_str_particle_open(self->ground, "froral_flareroad\\froral_flareroad_bottom\\froral_flareroad_bottom.str");

    for (delay = 0; delay < BURST_LAST_MS; delay += BURST_SPACING_MS) {
        Vec3 at = self->base.pos;
        struct Ez2StrParticle *burst;
        int radius, angle;

        radius = random_below(BURST_RADIUS);
        angle = random_below(360);
        at.x = (float)((double)get_sin_deg(angle) * (double)(float)radius + (double)at.x);
        radius = random_below(BURST_RADIUS);
        angle = random_below(360);
        at.z = (float)((double)get_cos_deg(angle) * (double)(float)radius + (double)at.z);

        burst = ez2str_particle_new(at, 5);
        ez2_str_particle_open(burst, "froral_flareroad\\froral_flareroad\\froral_flareroad.str");
        burst->start_delay = delay;
        vec_push(&self->bursts, &burst, sizeof(struct Ez2StrParticle *));
        effect_add_sound_cue(&self->base, SOUND_CUE_WAVE, "effect\\AG_ALL_BLOOM.wav", delay / MS_PER_FRAME, 0.0f);
    }
}

void floral_flare_road_process(struct Effect *effect)
{
    struct FloralFlareRoad *self = (struct FloralFlareRoad *)effect;
    struct Ez2StrParticle **it;

    if (self->ground)
        step_part(self->ground);
    it = self->bursts.first;
    while (it != (struct Ez2StrParticle **)self->bursts.last) {
        struct Ez2StrParticle *burst = *it;
        float life = burst->base.life_seconds;

        if (life == 0.0f || life * 60.0f > (float)burst->base.age) {
            if (burst)
                step_part(burst);
            it++;
        } else {
            particle_free(&burst->base);
            memmove(it, it + 1, (unsigned char *)self->bursts.last - (unsigned char *)(it + 1));
            self->bursts.last = (struct Ez2StrParticle **)self->bursts.last - 1;
        }
    }
}

void floral_flare_road_render(struct Effect *effect)
{
    struct FloralFlareRoad *self = (struct FloralFlareRoad *)effect;
    struct Ez2StrParticle **it;

    if (self->ground)
        render_part(self->ground);
    for (it = self->bursts.first; it != (struct Ez2StrParticle **)self->bursts.last; it++)
        if (*it)
            render_part(*it);
}
