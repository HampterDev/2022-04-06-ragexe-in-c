#include "effects/effect_allocator.h"
#include <stdlib.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/vec.h"
#include <stdint.h>

struct GaleStorm {
    struct Effect base;
    struct Vec anims;
    int64_t side;
};

#define EFFECT_GALE_STORM 0x7b5
#define CELL_SIZE 5
#define GUST_PERIOD 100
#define ARROW_PERIOD 30

#define EFFECT_LEVEL(e) ((e)->level)

void gale_storm_init(struct Effect *self);
void gale_storm_process(struct Effect *self);
void gale_storm_render(struct Effect *self);
void gale_storm_release(struct Effect *self);
unsigned char gale_storm_is_alive(struct Effect *self);
static void gale_storm_destroy(struct Effect *effect);

static const struct EffectOps gale_storm_ops = {
    .set_submit = effect_set_submit,
    .init = gale_storm_init,
    .update_range = effect_update_range,
    .process = gale_storm_process,
    .render = gale_storm_render,
    .release = gale_storm_release,
    .is_alive = gale_storm_is_alive,
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
    .destroy = gale_storm_destroy,
    .fade_out = effect_fade_out,
};

static void render_anim(struct Ez2StrParticle *p)
{
    ez2_str_particle_render_current(p);
}

static void step_anim(struct Ez2StrParticle *p)
{
    Vec3 zero = { 0.0f, 0.0f, 0.0f };
    ez2_str_particle_process(p, zero);
}

static float scatter(const struct GaleStorm *self, float centre)
{
    int span = (int)((unsigned)self->side * CELL_SIZE + 1u);
    int r = 0;
    float half;

    if (span != 0) {
        r = rand() % span;
        if (r < 0)
            r = 0;
    }
    half = (float)(double)(self->side * CELL_SIZE) * 0.5f;
    return ((float)r - half) + centre;
}

static void spawn_anim(struct GaleStorm *self, const char *name)
{
    Vec3 at = self->base.pos;
    struct Ez2StrParticle *p;

    at.x = scatter(self, at.x);
    at.z = scatter(self, at.z);
    p = ez2str_particle_new(at, 5);
    if (!p)
        return;
    ez2_str_particle_open(p, name);
    vec_push(&self->anims, &p, sizeof(struct Ez2StrParticle *));
}

struct GaleStorm *gale_storm_construct(struct GaleStorm *self)
{
    effect_construct(&self->base, EFFECT_GALE_STORM);
    self->base.ops = &gale_storm_ops;
    self->anims.first = self->anims.last = self->anims.end = 0;
    self->side = 5;
    return self;
}

struct Effect *gale_storm_new(void)
{
    struct GaleStorm *self = mem_try_alloc(sizeof *self);

    return self ? &gale_storm_construct(self)->base : NULL;
}

void gale_storm_destruct(struct GaleStorm *self)
{
    self->base.ops = &gale_storm_ops;
    vec_free(&self->anims);
    effect_destruct(&self->base);
}

static void gale_storm_destroy(struct Effect *effect)
{
    struct GaleStorm *self = (struct GaleStorm *)effect;
    gale_storm_destruct(self);
    mem_free(self);
}

void gale_storm_spawn_arrow(struct GaleStorm *self)
{
    spawn_anim(self, "galestorm\\galestorm_arrow\\galestorm_arrow.str");
    if (rand() % 7 == 0)
        effect_add_sound_cue(&self->base, SOUND_CUE_WAVE, "effect\\WH_GALESTORM.wav", self->base.frame + 1, 0.0f);
}

void gale_storm_spawn_gust(struct GaleStorm *self)
{
    spawn_anim(self, "galestorm\\galestorm\\galestorm.str");
}

void gale_storm_spawn(struct GaleStorm *self)
{
    int64_t frame;

    if (self->base.frame > self->base.duration)
        return;
    frame = self->base.frame;
    if (frame % (GUST_PERIOD / self->side) == 0)
        gale_storm_spawn_gust(self);
    if (frame % (ARROW_PERIOD / self->side) == 0)
        gale_storm_spawn_arrow(self);
}

void gale_storm_release(struct Effect *effect)
{
    struct GaleStorm *self = (struct GaleStorm *)effect;
    struct Particle **it;

    for (it = self->anims.first; it != (struct Particle **)self->anims.last; it++)
        particle_free(*it);
    self->anims.last = self->anims.first;
}

void gale_storm_init(struct Effect *effect)
{
    struct GaleStorm *self = (struct GaleStorm *)effect;
    static const int sides[10] = { 5, 5, 5, 5, 7, 7, 7, 9, 9, 11 };
    int64_t side_by_level[11];
    unsigned level = (unsigned)EFFECT_LEVEL(&self->base), i;

    for (i = 0; i < 10; i++)
        side_by_level[i + 1] = sides[i];
    self->side = side_by_level[level < 10 ? level : 10];
}

unsigned char gale_storm_is_alive(struct Effect *effect)
{
    struct GaleStorm *self = (struct GaleStorm *)effect;
    if (self->base.frame > self->base.duration && self->anims.first == self->anims.last)
        return 0;
    return effect_is_owner_alive(&self->base);
}

void gale_storm_process(struct Effect *effect)
{
    struct GaleStorm *self = (struct GaleStorm *)effect;
    struct Ez2StrParticle **it;

    gale_storm_spawn(self);
    it = self->anims.first;
    while (it != (struct Ez2StrParticle **)self->anims.last) {
        struct Ez2StrParticle *p = *it;
        float life = p->base.life_seconds;

        if (life == 0.0f || life * 60.0f > (float)p->base.age) {
            if (p)
                step_anim(p);
            it++;
        } else {
            particle_free(&p->base);
            memmove(it, it + 1, (unsigned char *)self->anims.last - (unsigned char *)(it + 1));
            self->anims.last = (struct Ez2StrParticle **)self->anims.last - 1;
        }
    }
}

void gale_storm_render(struct Effect *effect)
{
    struct GaleStorm *self = (struct GaleStorm *)effect;
    struct Ez2StrParticle **it;

    for (it = self->anims.first; it != (struct Ez2StrParticle **)self->anims.last; it++)
        if (*it)
            render_anim(*it);
}
