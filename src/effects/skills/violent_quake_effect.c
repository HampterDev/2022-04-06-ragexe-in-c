#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <stdlib.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "core/text_util.h"

#define EFFECT_VIOLENT_QUAKE 0x6e0
#define EFFECT_ALL_BLOOM_BOMB 0x706
#define LIST_WORLD 5
#define LIST_GROUND 0xd

struct ViolentQuakeEffect {
    struct Effect base;
    struct Vec parts;
};

#define PART_OFFSET(p) (&(p)->base.offset)

struct String;
void violent_quake_effect_init(struct Effect *self);
void violent_quake_effect_process(struct Effect *self);
void violent_quake_effect_render(struct Effect *self);
void violent_quake_effect_release(struct Effect *self);
unsigned char violent_quake_effect_is_alive(struct Effect *self);
static void violent_quake_effect_destroy(struct Effect *effect);

static const struct EffectOps violent_quake_effect_ops = {
    .set_submit = effect_set_submit,
    .init = violent_quake_effect_init,
    .update_range = effect_update_range,
    .process = violent_quake_effect_process,
    .render = violent_quake_effect_render,
    .release = violent_quake_effect_release,
    .is_alive = violent_quake_effect_is_alive,
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
    .destroy = violent_quake_effect_destroy,
    .fade_out = effect_fade_out,
};

static const Vec3 no_offset = { 0, 0, 0 };

static void transform(Vec3 *out, const float *m, float x, float y, float z)
{
    out->x = y * m[3] + x * m[0] + z * m[6] + m[9];
    out->y = y * m[4] + x * m[1] + z * m[7] + m[10];
    out->z = y * m[5] + x * m[2] + z * m[8] + m[11];
}

static struct Ez2StrParticle *add_part(struct ViolentQuakeEffect *self, int list_flags, const char *name)
{
    struct Ez2StrParticle *p = ez2str_particle_new(self->base.pos, list_flags);

    ez2_str_particle_open(p, name);
    vec_push(&self->parts, &p, sizeof p);
    return p;
}

struct ViolentQuakeEffect *violent_quake_effect_construct(struct ViolentQuakeEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &violent_quake_effect_ops;
    self->parts.first = self->parts.last = self->parts.end = 0;
    return self;
}

struct Effect *violent_quake_effect_new(void)
{
    struct ViolentQuakeEffect *self = mem_try_alloc(sizeof *self);

    return self ? &violent_quake_effect_construct(self)->base : NULL;
}

void violent_quake_effect_destruct(struct ViolentQuakeEffect *self)
{
    self->base.ops = &violent_quake_effect_ops;
    vec_free(&self->parts);
    effect_destruct(&self->base);
}

static void violent_quake_effect_destroy(struct Effect *effect)
{
    struct ViolentQuakeEffect *self = (struct ViolentQuakeEffect *)effect;
    violent_quake_effect_destruct(self);
    mem_free(self);
}

void violent_quake_effect_release(struct Effect *effect)
{
    struct ViolentQuakeEffect *self = (struct ViolentQuakeEffect *)effect;
    struct Ez2StrParticle **it;
    for (it = self->parts.first; it != (struct Ez2StrParticle **)self->parts.last; it++)
        particle_free(&(*it)->base);
    self->parts.last = self->parts.first;
}

void violent_quake_effect_add_bloom_bomb(struct ViolentQuakeEffect *self)
{
    int which = rand() % 2;

    if (which < 0)
        which = 0;
    switch (which) {
    case 0:
        add_part(self, LIST_GROUND, "allbloom\\allbloom_bomb\\allbloom_bomb01_bottom.str");
        add_part(self, LIST_WORLD, "allbloom\\allbloom_bomb\\allbloom_bomb01.str");
        break;
    case 1:
        add_part(self, LIST_GROUND, "allbloom\\allbloom_bomb\\allbloom_bomb02_bottom.str");
        add_part(self, LIST_WORLD, "allbloom\\allbloom_bomb\\allbloom_bomb02.str");
        break;
    }
    effect_add_sound_cue(&self->base, SOUND_CUE_WAVE, "effect\\AG_ALL_BLOOM.wav", 0, 0.0f);
}

void violent_quake_effect_add_quake(struct ViolentQuakeEffect *self)
{
    struct Ez2StrParticle *p = ez2str_particle_new(self->base.pos, LIST_WORLD);
    struct String name;
    const char *text;
    int look = rand() % 3;

    string_init(&name);
    string_format(&name, "violentquake\\violentquake\\violentquake_%02d.str", (look < 0 ? 0 : look) + 1);
    text = string_cstr(&name);
    ez2_str_particle_open(p, text);
    string_free(&name);
    PART_OFFSET(p)->x = 0;
    PART_OFFSET(p)->y = -8.0f;
    PART_OFFSET(p)->z = 0;
    vec_push(&self->parts, &p, sizeof p);
    effect_add_sound_cue(&self->base, SOUND_CUE_WAVE, "effect\\AG_VIOLENT_QUAKE.wav", 0, 0.0f);
}

void violent_quake_effect_init(struct Effect *effect)
{
    struct ViolentQuakeEffect *self = (struct ViolentQuakeEffect *)effect;
    switch (self->base.type) {
    case EFFECT_VIOLENT_QUAKE:
        violent_quake_effect_add_quake(self);
        break;
    case EFFECT_ALL_BLOOM_BOMB:
        violent_quake_effect_add_bloom_bomb(self);
        break;
    }
}

unsigned char violent_quake_effect_is_alive(struct Effect *effect)
{
    struct ViolentQuakeEffect *self = (struct ViolentQuakeEffect *)effect;
    struct Ez2StrParticle **it;
    for (it = self->parts.first; it != (struct Ez2StrParticle **)self->parts.last; it++) {
        float life = (*it)->base.life_seconds;
        if (life == 0.0f)
            return 1;
        if (life * 60.0f > (float)(*it)->base.age)
            return 1;
    }
    return 0;
}

void violent_quake_effect_place_part(struct ViolentQuakeEffect *self,
                                               struct Ez2StrParticle *p)
{
    float view[12], to_world[12];
    Vec3 here, seen;

    if (!p)
        return;
    here = self->base.pos;
    if (!game_copy_view_matrix(view))
        return;
    transform(&seen, view, here.x, here.y, here.z);
    mat_invert(to_world, view);
    transform(&p->base.pos, to_world, PART_OFFSET(p)->x + seen.x, PART_OFFSET(p)->y + seen.y,
              PART_OFFSET(p)->z + seen.z);
    ez2_str_particle_process(p, no_offset);
}

void violent_quake_effect_process(struct Effect *effect)
{
    struct ViolentQuakeEffect *self = (struct ViolentQuakeEffect *)effect;
    struct Ez2StrParticle **it;
    for (it = self->parts.first; it != (struct Ez2StrParticle **)self->parts.last; it++)
        violent_quake_effect_place_part(self, *it);
}

void violent_quake_effect_render(struct Effect *effect)
{
    struct ViolentQuakeEffect *self = (struct ViolentQuakeEffect *)effect;
    struct Ez2StrParticle **it;

    for (it = self->parts.first; it != (struct Ez2StrParticle **)self->parts.last; it++) {
        if (*it) {
            ez2_str_particle_render_current(*it);
        }
    }
}
