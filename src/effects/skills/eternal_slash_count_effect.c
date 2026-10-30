#include "effects/effect_allocator.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "core/text_util.h"

struct EternalSlashCountEffect {
    struct Effect base;
    struct Ez2StrParticle *counter;
};

#define EFFECT_ETERNAL_SLASH_COUNT 0x779
#define ETERNAL_SLASH_MAX_COUNT 5

#define EFFECT_COUNT(e) ((e)->level)
#define PARTICLE_OFFSET(p) ((p)->base.offset)

struct String;
void eternal_slash_count_effect_init(struct Effect *self);
void eternal_slash_count_effect_process(struct Effect *self);
void eternal_slash_count_effect_render(struct Effect *self);
void eternal_slash_count_effect_release(struct Effect *self);
unsigned char eternal_slash_count_effect_is_alive(struct Effect *self);
static void eternal_slash_count_effect_destroy(struct Effect *effect);

static const struct EffectOps eternal_slash_count_effect_ops = {
    .set_submit = effect_set_submit,
    .init = eternal_slash_count_effect_init,
    .update_range = effect_update_range,
    .process = eternal_slash_count_effect_process,
    .render = eternal_slash_count_effect_render,
    .release = eternal_slash_count_effect_release,
    .is_alive = eternal_slash_count_effect_is_alive,
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
    .destroy = eternal_slash_count_effect_destroy,
    .fade_out = effect_fade_out,
};

struct EternalSlashCountEffect *
eternal_slash_count_effect_construct(struct EternalSlashCountEffect *self)
{
    effect_construct(&self->base, EFFECT_ETERNAL_SLASH_COUNT);
    self->base.ops = &eternal_slash_count_effect_ops;
    self->counter = 0;
    return self;
}

struct Effect *eternal_slash_count_effect_new(void)
{
    struct EternalSlashCountEffect *self = mem_try_alloc(sizeof *self);

    return self ? &eternal_slash_count_effect_construct(self)->base : NULL;
}

void eternal_slash_count_effect_destruct(struct EternalSlashCountEffect *self)
{
    self->base.ops = &eternal_slash_count_effect_ops;
    effect_destruct(&self->base);
}

static void eternal_slash_count_effect_destroy(struct Effect *effect)
{
    struct EternalSlashCountEffect *self = (struct EternalSlashCountEffect *)effect;
    eternal_slash_count_effect_destruct(self);
    mem_free(self);
}

void eternal_slash_count_effect_release(struct Effect *effect)
{
    struct EternalSlashCountEffect *self = (struct EternalSlashCountEffect *)effect;
    if (self->counter) {
        particle_free(&self->counter->base);
        self->counter = 0;
    }
}

void eternal_slash_count_effect_init(struct Effect *effect)
{
    struct EternalSlashCountEffect *self = (struct EternalSlashCountEffect *)effect;
    struct Ez2StrParticle *p;
    struct String formatted;
    const char *text;

    if ((unsigned)(EFFECT_COUNT(&self->base) - 1) > ETERNAL_SLASH_MAX_COUNT - 1)
        return;
    p = ez2str_particle_new(self->base.pos, 5);
    if (!p)
        return;
    string_init(&formatted);
    string_format(&formatted, "eternal_slash\\eternal_slash_count\\eternal_slash_count_%02d.str",
               EFFECT_COUNT(&self->base));
    text = string_cstr(&formatted);
    ez2_str_particle_open(p, text);
    string_free(&formatted);
    PARTICLE_OFFSET(p).x = 0.0f;
    PARTICLE_OFFSET(p).y = -25.0f;
    PARTICLE_OFFSET(p).z = 0.0f;
    self->counter = p;
}

unsigned char eternal_slash_count_effect_is_alive(struct Effect *effect)
{
    struct EternalSlashCountEffect *self = (struct EternalSlashCountEffect *)effect;
    struct Ez2StrParticle *p = self->counter;
    float life;

    if (!p)
        return 0;
    life = p->base.life_seconds;
    if (life == 0.0f)
        return 1;
    return life * 60.0f > (float)p->base.age;
}

void eternal_slash_count_effect_process(struct Effect *effect)
{
    struct EternalSlashCountEffect *self = (struct EternalSlashCountEffect *)effect;
    struct Ez2StrParticle *p = self->counter;
    Vec3 at, zero = { 0.0f, 0.0f, 0.0f };

    if (!p)
        return;
    at = self->base.pos;
    p->base.pos.x = PARTICLE_OFFSET(p).x + at.x;
    p->base.pos.y = PARTICLE_OFFSET(p).y + at.y;
    p->base.pos.z = PARTICLE_OFFSET(p).z + at.z;
    ez2_str_particle_process(self->counter, zero);
}

void eternal_slash_count_effect_render(struct Effect *effect)
{
    struct EternalSlashCountEffect *self = (struct EternalSlashCountEffect *)effect;

    if (!self->counter)
        return;
    ez2_str_particle_render_current(self->counter);
}
