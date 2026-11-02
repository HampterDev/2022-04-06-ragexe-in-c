#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <stdio.h>
#include "effects/durations.h"
#include "session/state.h"
#include <stdlib.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/list.h"
#include <stdint.h>

enum {
    EFFECT_SEVERE_RAINSTORM = 0x4fc,
    EFFECT_RAIN_OF_CRYSTAL = 0x6d8,
    EFFECT_DIAMOND_STORM = 0x7ea,
    EFFECT_CROSS_RAIN = 0x875,
    EFFECT_TWINKLING_GALAXY = 0x92c,
    EFFECT_TWINKLING_GALAXY_SCATTERED = 0x92d,
    EFFECT_STAR_CANNON = 0x92e,
    EFFECT_GRENADES_DROPPING = 0x92f,
    EFFECT_MISSION_BOMBARD = 0x932,
};

struct SevereRainStorm {
    struct Effect base;
    struct List particles;
    int ticks;
};

#define EFFECT_LEVEL(e) ((e)->level)

void severe_rain_storm_init(struct Effect *self);
void severe_rain_storm_process(struct Effect *self);
void severe_rain_storm_render(struct Effect *self);
void severe_rain_storm_release(struct Effect *self);
unsigned char severe_rain_storm_is_alive(struct Effect *self);
static void severe_rain_storm_destroy(struct Effect *effect);

static const struct EffectOps severe_rain_storm_ops = {
    .set_submit = effect_set_submit,
    .init = severe_rain_storm_init,
    .update_range = effect_update_range,
    .process = severe_rain_storm_process,
    .render = severe_rain_storm_render,
    .release = severe_rain_storm_release,
    .is_alive = severe_rain_storm_is_alive,
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
    .destroy = severe_rain_storm_destroy,
    .fade_out = effect_fade_out,
};

static int random_below(int span)
{
    int r = rand() % span;
    return r < 0 ? 0 : r;
}

static int64_t level_width(const struct SevereRainStorm *self, const int *table, unsigned count)
{
    int64_t widths[11];
    unsigned level = (unsigned)EFFECT_LEVEL(&self->base), i;
    for (i = 0; i < count; i++)
        widths[i + 1] = table[i];
    return widths[level < count ? level : count] * 50;
}

struct Scatter {
    int span;
    float half;
};
static struct Scatter scatter_for(int64_t width)
{
    struct Scatter s;
    s.span = (int)width + 1;
    s.half = (float)(double)width * 0.5f;
    return s;
}
static float scatter(const struct Scatter *s, float centre)
{
    return ((float)random_below(s->span) - s->half) * 0.1f + centre;
}

static struct Ez2StrParticle *ez2str_load(Vec3 pos, int list_flags, const char *name)
{
    struct Ez2StrParticle *p = ez2str_particle_new(pos, list_flags);
    ez2_str_particle_open(p, name);
    return p;
}

static void ez2str_load_by_value(struct Ez2StrParticle *p, const char *name)
{
    ez2_str_particle_load(p, name);
}

static void place_and_list(struct SevereRainStorm *self, struct Ez2StrParticle *p, const char *name, Vec3 at)
{
    if (!p)
        return;
    ez2_str_particle_open(p, name);
    p->base.pos = at;
    list_push_back(&self->particles, &p, sizeof p);
}

static void star_cannon_sound(struct SevereRainStorm *self)
{
    effect_add_sound_cue(&self->base, SOUND_CUE_WAVE, "effect\\SKY_EMPEROR\\ske_star_cannon.wav",
                         self->base.frame + 1, 0.0f);
}

struct SevereRainStorm *severe_rain_storm_construct(struct SevereRainStorm *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &severe_rain_storm_ops;
    list_init(&self->particles);
    self->base.params.rise_angle = 0;
    return self;
}

struct Effect *severe_rain_storm_new(void)
{
    struct SevereRainStorm *self = mem_try_alloc(sizeof *self);

    return self ? &severe_rain_storm_construct(self)->base : NULL;
}

void severe_rain_storm_destruct(struct SevereRainStorm *self)
{
    self->base.ops = &severe_rain_storm_ops;
    list_free(&self->particles, NULL);
    effect_destruct(&self->base);
}

static void severe_rain_storm_destroy(struct Effect *effect)
{
    struct SevereRainStorm *self = (struct SevereRainStorm *)effect;
    severe_rain_storm_destruct(self);
    mem_free(self);
}

void severe_rain_storm_spawn_severe_rainstorm(struct SevereRainStorm *self)
{
    Vec3 at = self->base.pos;
    float z = at.z + 10.0f;
    struct Ez2StrParticle *p;

    if (random_below(10) >= 5)
        at.x = (float)random_below(251) * 0.1f + at.x;
    else
        at.x = at.x - (float)random_below(251) * 0.1f;
    if (random_below(10) >= 5)
        at.z = (float)random_below(251) * 0.1f + z;
    else
        at.z = z - (float)random_below(251) * 0.1f;

    p = ez2str_particle_new(at, 1);
    if (g_session1.map_rule_flags[0] == 1)
        ez2str_load_by_value(p, "mineffect\\new_severerainstorm\\new_severerainstorm\\new_severerainstorm.str");
    else
        ez2str_load_by_value(p, "new_severerainstorm\\new_severerainstorm\\new_severerainstorm.str");
    p->base.list_flags = 5;
    list_push_back(&self->particles, &p, sizeof p);
}

void severe_rain_storm_spawn_rain_of_crystal(struct SevereRainStorm *self)
{
    static const int width_by_level[5] = { 11, 13, 15, 17, 19 };
    struct Scatter s = scatter_for(level_width(self, width_by_level, 5));
    Vec3 at = self->base.pos;
    struct Ez2StrParticle *p;

    at.x = scatter(&s, at.x);
    at.z = scatter(&s, at.z);
    p = ez2str_load(at, 13, "rain_of_crystal\\rain_of_crystal_attack_bottom\\rain_of_crystal_attack_bottom.str");
    list_push_back(&self->particles, &p, sizeof p);
    p = ez2str_load(at, 5, "rain_of_crystal\\rain_of_crystal_attack\\rain_of_crystal_attack.str");
    list_push_back(&self->particles, &p, sizeof p);
    if (rand() % 7 == 0)
        effect_add_sound_cue(&self->base, SOUND_CUE_WAVE, "effect\\AG_RAIN_OF_CRYSTAL.wav",
                             self->base.frame + 1, 0.0f);
}

void severe_rain_storm_spawn_diamond_storm(struct SevereRainStorm *self)
{
    Vec3 at = self->base.pos;
    struct Ez2StrParticle *p;

    at.x = ((float)random_below(451) - 225.0f) * 0.1f + at.x;
    at.z = ((float)random_below(451) - 225.0f) * 0.1f + at.z;
    p = ez2str_particle_new(at, 5);
    ez2str_load_by_value(p, "diamond_storm\\diamond_storm\\diamond_storm.str");
    list_push_back(&self->particles, &p, sizeof p);
}

void severe_rain_storm_spawn_cross_rain(struct SevereRainStorm *self)
{
    static const int width_by_level[10] = { 3, 3, 3, 5, 5, 5, 7, 7, 7, 9 };
    struct Scatter s = scatter_for(level_width(self, width_by_level, 10));
    Vec3 at = self->base.pos;
    struct Ez2StrParticle *p;

    at.x = scatter(&s, at.x);
    at.z = scatter(&s, at.z);
    p = ez2str_load(at, 5, "crossrain\\cross_rain\\cross_rain.str");
    list_push_back(&self->particles, &p, sizeof p);
}

void severe_rain_storm_spawn_twinkling_galaxy(struct SevereRainStorm *self, int index)
{
    struct Ez2StrParticle *p;
    char name[256];

    p = ez2str_particle_new(self->base.pos, 13);
    if (p) {
        ez2_str_particle_open(p, "SKY_EMPEROR\\SKE_TWINKLING_GALAXY\\twinkling_galaxy_bottom\\ske_twinkling_galaxy_bottom.str");
        p->base.offset.x = 0.2f;
        p->base.offset.y = -13.5f;
        p->base.offset.z = 0.0f;
        list_push_back(&self->particles, &p, sizeof p);
    }
    p = ez2str_particle_new(self->base.pos, 5);
    if (p) {
        snprintf(name, sizeof name, "SKY_EMPEROR\\SKE_TWINKLING_GALAXY\\twinkling_galaxy\\ske_twinkling_galaxy%02d.str", index);
        ez2_str_particle_open(p, name);
        p->base.offset.x = 0.2f;
        p->base.offset.y = -13.5f;
        p->base.offset.z = 0.0f;
        list_push_back(&self->particles, &p, sizeof p);
    }
    star_cannon_sound(self);
}

static void spawn_sky_emperor_hit(struct SevereRainStorm *self, const char *bottom, const char *top)
{
    static const int width_by_level[5] = { 13, 13, 11, 11, 9 };
    struct Scatter s = scatter_for(level_width(self, width_by_level, 5));
    Vec3 at;

    at.y = self->base.pos.y - 15.0f;
    at.x = scatter(&s, self->base.pos.x);
    at.z = scatter(&s, self->base.pos.z);
    place_and_list(self, ez2str_particle_new(self->base.pos, 13), bottom, at);
    place_and_list(self, ez2str_particle_new(self->base.pos, 5), top, at);
    star_cannon_sound(self);
}

void severe_rain_storm_spawn_star_cannon(struct SevereRainStorm *self)
{
    spawn_sky_emperor_hit(self, "SKY_EMPEROR\\SKE_STAR_CANNON\\star_cannon_bottom\\ske_star_cannon_bottom.str",
                          "SKY_EMPEROR\\SKE_STAR_CANNON\\star_cannon\\ske_star_cannon.str");
}

void severe_rain_storm_spawn_galaxy_hit(struct SevereRainStorm *self)
{
    spawn_sky_emperor_hit(self, "SKY_EMPEROR\\SKE_TWINKLING_GALAXY\\twinkling_galaxy_bottom\\ske_twinkling_galaxy_bottom.str",
                          "SKY_EMPEROR\\SKE_TWINKLING_GALAXY\\twinkling_galaxy\\ske_twinkling_galaxy03.str");
}

void severe_rain_storm_spawn_grenades(struct SevereRainStorm *self)
{
    static const int width_by_level[5] = { 11, 11, 11, 9, 9 };
    struct Scatter s = scatter_for(level_width(self, width_by_level, 5));
    int i;

    for (i = 0; i < (EFFECT_LEVEL(&self->base) + 1) / 2; i++) {
        Vec3 at;
        at.y = self->base.pos.y - 15.0f;
        at.x = scatter(&s, self->base.pos.x);
        at.z = scatter(&s, self->base.pos.z);
        place_and_list(self, ez2str_particle_new(self->base.pos, 13),
                       "NIGHT_WATCH\\NW_GRENADES_DROPPING\\grenades_dropping_bottom\\grenades_dropping_bottom.str", at);
        place_and_list(self, ez2str_particle_new(self->base.pos, 5),
                       "NIGHT_WATCH\\NW_GRENADES_DROPPING\\grenades_dropping\\grenades_dropping.str", at);
    }
}

void severe_rain_storm_spawn_mission_bombard(struct SevereRainStorm *self)
{
    int i;

    for (i = 0; i < 3; i++) {
        Vec3 at;
        struct Ez2StrParticle *p;

        at.y = self->base.pos.y - 15.0f;
        at.x = ((float)random_below(451) - 225.0f) * 0.1f + self->base.pos.x;
        at.z = ((float)random_below(451) - 225.0f) * 0.1f + self->base.pos.z;
        p = ez2str_particle_new(self->base.pos, 5);
        if (!p)
            continue;
        ez2_str_particle_open(p, "NIGHT_WATCH\\NW_MISSION_BOMBARD\\mission_bombard\\mission_bombard_hit.str");
        p->base.pos = at;
        p->zoom = (float)(random_below(51) + 50) * 0.01f;
        list_push_back(&self->particles, &p, sizeof p);
    }
}

void severe_rain_storm_spawn(struct SevereRainStorm *self)
{
    int level = EFFECT_LEVEL(&self->base);

    switch (self->base.type) {
    case EFFECT_RAIN_OF_CRYSTAL:
        if (self->ticks % (4 - (level + 1) / 2) == 0)
            severe_rain_storm_spawn_rain_of_crystal(self);
        break;
    case EFFECT_SEVERE_RAINSTORM:
        if (self->ticks % 3 == 0)
            severe_rain_storm_spawn_severe_rainstorm(self);
        break;
    case EFFECT_DIAMOND_STORM:
        if (self->ticks % 5 == 0)
            severe_rain_storm_spawn_diamond_storm(self);
        break;
    case EFFECT_CROSS_RAIN:
        if (self->ticks % (9 - level / 3) == 0)
            severe_rain_storm_spawn_cross_rain(self);
        break;
    case EFFECT_TWINKLING_GALAXY:
        if (effect_is_owner_alive(&self->base) && self->ticks % 15 == 0)
            severe_rain_storm_spawn_twinkling_galaxy(self, self->ticks / 15 % 4);
        break;
    case EFFECT_TWINKLING_GALAXY_SCATTERED:
        if (effect_is_owner_alive(&self->base) && self->ticks % 18 == 0)
            severe_rain_storm_spawn_galaxy_hit(self);
        break;
    case EFFECT_STAR_CANNON:
        if (effect_is_owner_alive(&self->base) && self->ticks % 18 == 0)
            severe_rain_storm_spawn_star_cannon(self);
        break;
    case EFFECT_GRENADES_DROPPING:
        if (effect_is_owner_alive(&self->base) && self->ticks % 15 == 0)
            severe_rain_storm_spawn_grenades(self);
        break;
    case EFFECT_MISSION_BOMBARD:
        if (effect_is_owner_alive(&self->base) && self->ticks % 15 == 0)
            severe_rain_storm_spawn_mission_bombard(self);
        break;
    }
}

static Vec3 transform(const float *m, Vec3 v)
{
    Vec3 out;
    out.x = v.x * m[0] + v.y * m[3] + v.z * m[6] + m[9];
    out.y = v.x * m[1] + v.y * m[4] + v.z * m[7] + m[10];
    out.z = v.x * m[2] + v.y * m[5] + v.z * m[8] + m[11];
    return out;
}

void severe_rain_storm_place_in_view(struct SevereRainStorm *self, struct Ez2StrParticle *p)
{
    float view[12], from_view[12];
    Vec3 seen, zero = { 0, 0, 0 };

    if (!p)
        return;
    if (!game_copy_view_matrix(view))
        return;
    mat_invert(from_view, view);
    seen = transform(view, self->base.pos);
    seen.x = seen.x + p->base.offset.x;
    seen.y = seen.y + p->base.offset.y;
    seen.z = seen.z + p->base.offset.z;
    p->base.pos = transform(from_view, seen);
    ez2_str_particle_process(p, zero);
}

void severe_rain_storm_init(struct Effect *effect)
{
    struct SevereRainStorm *self = (struct SevereRainStorm *)effect;
    self->ticks = 0;
    if (self->base.type == EFFECT_TWINKLING_GALAXY)
        self->base.has_instance_key = 1;
}

void severe_rain_storm_process(struct Effect *effect)
{
    struct SevereRainStorm *self = (struct SevereRainStorm *)effect;
    struct ListNode *node;
    Vec3 zero = { 0, 0, 0 };

    severe_rain_storm_spawn(self);
    node = self->particles.head->next;
    while (node != self->particles.head) {
        struct Ez2StrParticle *p = *LIST_VALUE(node, struct Ez2StrParticle *);
        if (!(p->base.life_seconds >= 0.0f)) {
            particle_free(&p->base);
            node = list_erase(&self->particles, node, NULL);
            continue;
        }
        if (self->base.type == EFFECT_TWINKLING_GALAXY && p)
            severe_rain_storm_place_in_view(self, p);
        p = *LIST_VALUE(node, struct Ez2StrParticle *);
        if (p)
            ez2_str_particle_process(p, zero);
        node = node->next;
    }
    self->ticks++;
}

void severe_rain_storm_render(struct Effect *effect)
{
    struct SevereRainStorm *self = (struct SevereRainStorm *)effect;
    struct ListNode *node;
    for (node = self->particles.head->next; node != self->particles.head; node = node->next) {
        struct Ez2StrParticle *p = *LIST_VALUE(node, struct Ez2StrParticle *);
        if (p)
            ez2_str_particle_render_current(p);
    }
}

void severe_rain_storm_release(struct Effect *effect)
{
    struct SevereRainStorm *self = (struct SevereRainStorm *)effect;
    struct ListNode *node;

    for (node = self->particles.head->next; node != self->particles.head; node = node->next)
        particle_free(&(*LIST_VALUE(node, struct Ez2StrParticle *))->base);
    list_clear(&self->particles, NULL);
}

unsigned char severe_rain_storm_is_alive(struct Effect *effect)
{
    struct SevereRainStorm *self = (struct SevereRainStorm *)effect;
    switch (self->base.type) {
    case EFFECT_RAIN_OF_CRYSTAL:
    case EFFECT_CROSS_RAIN:
        return effect_is_owner_alive(&self->base);
    case EFFECT_SEVERE_RAINSTORM:
        return self->ticks > 240 ? 0 : 1;
    case EFFECT_DIAMOND_STORM:
        return g_effectDurations[EFFECT_DIAMOND_STORM] < self->ticks ? 0 : 1;
    case EFFECT_TWINKLING_GALAXY:
    case EFFECT_TWINKLING_GALAXY_SCATTERED:
    case EFFECT_STAR_CANNON:
    case EFFECT_GRENADES_DROPPING:
    case EFFECT_MISSION_BOMBARD:
        if (effect_is_owner_alive(&self->base) == 1)
            return 1;
        return self->particles.size != 0;
    }
    return 1;
}
