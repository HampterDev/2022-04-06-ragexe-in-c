#include "effects/effect_allocator.h"
#include <stdlib.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/vec.h"
#include <stdint.h>

#define ASTRAL_STRIKE_TYPE 0x6e8
#define STRIKE_LIST 5
#define MARK_LIST 0xd
#define LEVELS 10

struct AstralStrikeEffect {
    struct Effect base;
    struct Vec strike;
    struct Vec sub_strikes;
};

#define EFFECT_LEVEL(e) ((e)->level)
#define PARTICLE_LIFE(p) ((p)->base.life_seconds)

void astral_strike_effect_init(struct Effect *self);
void astral_strike_effect_process(struct Effect *self);
void astral_strike_effect_render(struct Effect *self);
void astral_strike_effect_release(struct Effect *self);
static void astral_strike_effect_destroy(struct Effect *effect);

static const struct EffectOps astral_strike_effect_ops = {
    .set_submit = effect_set_submit,
    .init = astral_strike_effect_init,
    .update_range = effect_update_range,
    .process = astral_strike_effect_process,
    .render = astral_strike_effect_render,
    .release = astral_strike_effect_release,
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
    .destroy = astral_strike_effect_destroy,
    .fade_out = effect_fade_out,
};

static const int sub_area_cells[LEVELS] = { 5, 5, 5, 7, 7, 7, 9, 9, 9, 11 };
static const int sub_interval[LEVELS] = { 11, 11, 11, 9, 9, 9, 7, 7, 7, 5 };

static const char *const sub_strike_str[3] = {
    "astralstrike\\astralstrike_sub_1\\astralstrike_sub_1.str",
    "astralstrike\\astralstrike_sub_2\\astralstrike_sub_2.str",
    "astralstrike\\astralstrike_sub_3\\astralstrike_sub_3.str",
};
static const char *const sub_mark_str[3] = {
    "astralstrike\\astralstrike_sub_1_bottom\\astralstrike_sub_1_bottom.str",
    "astralstrike\\astralstrike_sub_2_bottom\\astralstrike_sub_2_bottom.str",
    "astralstrike\\astralstrike_sub_3_bottom\\astralstrike_sub_3_bottom.str",
};

static int64_t level_entry(const struct AstralStrikeEffect *self, const int *table)
{
    int64_t by_level[LEVELS + 1];
    unsigned level = (unsigned)EFFECT_LEVEL(&self->base), i;
    for (i = 0; i < LEVELS; i++)
        by_level[i + 1] = table[i];
    return by_level[level < LEVELS ? level : LEVELS];
}

static struct Ez2StrParticle *str_particle(Vec3 pos, int list_flags, const char *str)
{
    struct Ez2StrParticle *p = ez2str_particle_new(pos, list_flags);
    ez2_str_particle_open(p, str);
    return p;
}

static void particles_free_all(struct Vec *v)
{
    struct Ez2StrParticle **it;
    for (it = v->first; it != (struct Ez2StrParticle **)v->last; it++)
        particle_free(&(*it)->base);
    v->last = v->first;
}

static void particles_step(struct Vec *v)
{
    static const Vec3 no_offset = { 0.0f, 0.0f, 0.0f };
    struct Ez2StrParticle **it = v->first;

    while (it != (struct Ez2StrParticle **)v->last) {
        struct Ez2StrParticle *p = *it;
        if (PARTICLE_LIFE(p) == 0.0f || PARTICLE_LIFE(p) * 60.0f > (float)p->base.age) {
            if (p)
                ez2_str_particle_process(p, no_offset);
            it++;
        } else {
            particle_free(&p->base);
            memmove(it, it + 1, (char *)v->last - (char *)(it + 1));
            v->last = (struct Ez2StrParticle **)v->last - 1;
        }
    }
}

static void particles_render(struct Vec *v)
{
    struct Ez2StrParticle **it;
    for (it = v->first; it != (struct Ez2StrParticle **)v->last; it++) {
        struct Ez2StrParticle *p = *it;
        if (p) {
            ez2_str_particle_render_current(p);
        }
    }
}

struct AstralStrikeEffect *astral_strike_effect_construct(struct AstralStrikeEffect *self)
{
    effect_construct(&self->base, ASTRAL_STRIKE_TYPE);
    self->base.ops = &astral_strike_effect_ops;
    self->strike.first = self->strike.last = self->strike.end = 0;
    self->sub_strikes.first = self->sub_strikes.last = self->sub_strikes.end = 0;
    return self;
}

struct Effect *astral_strike_effect_new(void)
{
    struct AstralStrikeEffect *self = mem_try_alloc(sizeof *self);

    return self ? &astral_strike_effect_construct(self)->base : NULL;
}

void astral_strike_effect_destruct(struct AstralStrikeEffect *self)
{
    self->base.ops = &astral_strike_effect_ops;
    vec_free(&self->sub_strikes);
    vec_free(&self->strike);
    effect_destruct(&self->base);
}

static void astral_strike_effect_destroy(struct Effect *effect)
{
    struct AstralStrikeEffect *self = (struct AstralStrikeEffect *)effect;
    astral_strike_effect_destruct(self);
    mem_free(self);
}

void astral_strike_effect_spawn_sub_strike(struct AstralStrikeEffect *self)
{
    int64_t width = level_entry(self, sub_area_cells) * 50;
    Vec3 pos = self->base.pos;
    int r, variant;
    float half;

    r = rand() % ((int)width + 1);
    if (r < 0)
        r = 0;
    half = (float)(double)width * 0.5f;
    pos.x = ((float)r - half) * 0.1f + pos.x;
    r = rand() % ((int)width + 1);
    if (r < 0)
        r = 0;
    pos.z = ((float)r - half) * 0.1f + pos.z;

    variant = rand() % 3;
    if (variant < 0)
        variant = 0;
    if (variant <= 2) {
        struct Ez2StrParticle *p;

        p = str_particle(pos, MARK_LIST, sub_mark_str[variant]);
        vec_push(&self->sub_strikes, &p, sizeof p);
        p = str_particle(pos, STRIKE_LIST, sub_strike_str[variant]);
        vec_push(&self->sub_strikes, &p, sizeof p);
    }
    effect_add_sound_cue(&self->base, SOUND_CUE_WAVE, "effect\\AG_ASTRAL_STRIKE2.wav", self->base.frame + 1, 0.0f);
}

void astral_strike_effect_init(struct Effect *effect)
{
    struct AstralStrikeEffect *self = (struct AstralStrikeEffect *)effect;
    struct Ez2StrParticle *p;

    p = str_particle(self->base.pos, MARK_LIST, "astralstrike\\astralstrike_bottom\\astralstrike_bottom.str");
    vec_push(&self->strike, &p, sizeof p);
    effect_add_sound_cue(&self->base, SOUND_CUE_WAVE, "effect\\AG_ASTRAL_STRIKE1.wav", 0, 0.0f);
    p = str_particle(self->base.pos, STRIKE_LIST, "astralstrike\\astralstrike\\astralstrike.str");
    vec_push(&self->strike, &p, sizeof p);
}

void astral_strike_effect_process(struct Effect *effect)
{
    struct AstralStrikeEffect *self = (struct AstralStrikeEffect *)effect;
    particles_step(&self->strike);
    if (effect_is_owner_alive(&self->base) == 1 && self->strike.first == self->strike.last) {
        int64_t interval = level_entry(self, sub_interval);
        if ((int64_t)self->base.frame % interval == 0)
            astral_strike_effect_spawn_sub_strike(self);
    }
    particles_step(&self->sub_strikes);
}

void astral_strike_effect_render(struct Effect *effect)
{
    struct AstralStrikeEffect *self = (struct AstralStrikeEffect *)effect;
    particles_render(&self->strike);
    particles_render(&self->sub_strikes);
}

void astral_strike_effect_release(struct Effect *effect)
{
    struct AstralStrikeEffect *self = (struct AstralStrikeEffect *)effect;
    particles_free_all(&self->strike);
    particles_free_all(&self->sub_strikes);
}
