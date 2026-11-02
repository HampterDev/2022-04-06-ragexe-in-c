#include "effects/effect_allocator.h"
#include "session/state.h"
#include <stdlib.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/list.h"

enum {
    EFFECT_SEVERE_RAINSTORM = 0x4fc,
};

struct SevereRainStorm {
    struct Effect base;
    struct List particles;
    int ticks;
};

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

static void ez2str_load_by_value(struct Ez2StrParticle *p, const char *name)
{
    ez2_str_particle_load(p, name);
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

void severe_rain_storm_spawn(struct SevereRainStorm *self)
{
    if (self->base.type == EFFECT_SEVERE_RAINSTORM && self->ticks % 3 == 0)
        severe_rain_storm_spawn_severe_rainstorm(self);
}

void severe_rain_storm_init(struct Effect *effect)
{
    struct SevereRainStorm *self = (struct SevereRainStorm *)effect;
    self->ticks = 0;
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
    if (self->base.type == EFFECT_SEVERE_RAINSTORM)
        return self->ticks > 240 ? 0 : 1;
    return 1;
}
