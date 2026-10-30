#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "effects/effect_controls.h"
#include "std/vec.h"

struct Effect;
struct RichsCoinEffect;
struct String;
void richs_coin_effect_init(struct Effect *self);
void richs_coin_effect_process(struct Effect *self);
void richs_coin_effect_render(struct Effect *self);
void richs_coin_effect_release(struct Effect *self);
static void richs_coin_effect_destroy(struct Effect *effect);

static const struct EffectOps richs_coin_effect_ops = {
    .set_submit = effect_set_submit,
    .init = richs_coin_effect_init,
    .update_range = effect_update_range,
    .process = richs_coin_effect_process,
    .render = richs_coin_effect_render,
    .release = richs_coin_effect_release,
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
    .destroy = richs_coin_effect_destroy,
    .fade_out = effect_fade_out,
};

struct RichsCoinParticle {
    float orbit;
    int bob;
};
PARTICLE_EFFECT_DATA_FITS(RichsCoinParticle);
static __inline struct RichsCoinParticle *richs_coin_particle(const struct Particle *p)
{
    return (struct RichsCoinParticle *)p->effect_data;
}

#define EFFECT_RICHS_COIN 0x46f
#define MAX_COINS 5
#define ORBIT_RADIUS 7.0f
#define ORBIT_HEIGHT 10.0f
#define ORBIT_STEP 0.015f
#define PARTICLE_LIST 5

struct RichsCoinEffect {
    struct Effect base;
    struct Vec coins;
    struct Vec lights;
    int coin_count;
};

void richs_coin_effect_set_coin_count(struct Effect *effect, int count)
{
    ((struct RichsCoinEffect *)effect)->coin_count = count;
}

static struct Particle *new_particle(struct RichsCoinEffect *self, const char *texture, size_t len)
{
    struct Particle *p = particle_new(self->base.pos, PARTICLE_LIST);
    struct String name;

    if (!p)
        return 0;
    string_init(&name);
    string_assign(&name, texture, len);
    particle_set_texture(p, name);
    return p;
}

static float start_angle(int deg)
{
    return (float)deg / 180.0f * 3.141592f - 10.0f;
}

struct RichsCoinEffect *richs_coin_effect_construct(struct RichsCoinEffect *self)
{
    effect_construct(&self->base, EFFECT_RICHS_COIN);
    self->base.ops = &richs_coin_effect_ops;
    self->coins.first = self->coins.last = self->coins.end = 0;
    self->lights.first = self->lights.last = self->lights.end = 0;
    self->coin_count = 0;
    return self;
}

struct Effect *richs_coin_effect_new(void)
{
    struct RichsCoinEffect *self = mem_try_alloc(sizeof *self);

    return self ? &richs_coin_effect_construct(self)->base : NULL;
}

void richs_coin_effect_destruct(struct RichsCoinEffect *self)
{
    self->base.ops = &richs_coin_effect_ops;
    vec_free(&self->lights);
    vec_free(&self->coins);
    effect_destruct(&self->base);
}

static void richs_coin_effect_destroy(struct Effect *effect)
{
    struct RichsCoinEffect *self = (struct RichsCoinEffect *)effect;
    richs_coin_effect_destruct(self);
    mem_free(self);
}

void richs_coin_effect_add_lights(struct RichsCoinEffect *self)
{
    int left, deg;

    if (self->coin_count <= MAX_COINS)
        return;
    for (left = self->coin_count - MAX_COINS, deg = 0; left > 0; left--, deg += 72) {
        struct Particle *p = new_particle(self, "effect\\rl_richs_coin_b\\light.bmp", 0x20);
        float m[12];

        if (!p)
            continue;
        richs_coin_particle(p)->orbit = start_angle(deg);
        richs_coin_particle(p)->bob = 0;
        p->scale = 1.5f;
        memcpy(m, g_identityMatrix.m, sizeof(m));
        mat_set_scale(m, 1.5f, 1.5f, 1.0f);
        memcpy(p->matrix, m, sizeof(m));
        p->alpha = 70;
        particle_quad_unit_corners(p->corners);
        vec_push(&self->lights, &p, sizeof p);
    }
}

void richs_coin_effect_init(struct Effect *effect)
{
    struct RichsCoinEffect *self = (struct RichsCoinEffect *)effect;
    int count = self->coin_count > MAX_COINS ? MAX_COINS : self->coin_count;
    int i;

    for (i = 0; i < count; i++) {
        struct Particle *p = new_particle(self, "effect\\rl_richs_coin_b\\rcoin1.bmp", 0x21);
        if (!p)
            continue;
        richs_coin_particle(p)->orbit = start_angle(360 / count * i);
        richs_coin_particle(p)->bob = 0;
        particle_quad_unit_corners(p->corners);
        vec_push(&self->coins, &p, sizeof p);
    }
    richs_coin_effect_add_lights(self);
}

void richs_coin_effect_orbit_light(struct RichsCoinEffect *self, struct Particle *p)
{
    float angle, s, c;

    if (!p)
        return;
    angle = richs_coin_particle(p)->orbit + ORBIT_STEP;
    richs_coin_particle(p)->orbit = angle;
    s = float_sin(angle) * ORBIT_RADIUS;
    c = float_cos(angle) * ORBIT_RADIUS;
    p->pos.x = s + self->base.pos.x;
    p->pos.y = self->base.pos.y - ORBIT_HEIGHT;
    p->pos.z = c + self->base.pos.z;
}

void richs_coin_effect_orbit_coin(struct RichsCoinEffect *self, struct Particle *p)
{
    float angle, s, c, deg;
    char texture[0x100];
    struct String name;

    if (!p)
        return;
    angle = richs_coin_particle(p)->orbit + ORBIT_STEP;
    richs_coin_particle(p)->orbit = angle;
    s = float_sin(angle) * ORBIT_RADIUS;
    c = float_cos(angle) * ORBIT_RADIUS;
    p->pos.x = self->base.pos.x + s;
    p->pos.y = self->base.pos.y - ORBIT_HEIGHT;
    p->pos.z = self->base.pos.z + c;

    deg = angle / 3.141592f * 180.0f - game_camera_heading();
    if (deg > 360.0f)
        deg = (float)((int)deg % 360);
    while (0.0f > deg)
        deg += 360.0f;
    sprintf(texture, "effect\\rl_richs_coin_b\\rcoin%d.bmp", (int)(deg / 36.0f) % 5 + 1);
    string_init(&name);
    string_assign_cstr(&name, texture);
    particle_set_texture(p, name);
}

void richs_coin_effect_process(struct Effect *effect)
{
    struct RichsCoinEffect *self = (struct RichsCoinEffect *)effect;
    struct Particle **it;

    for (it = self->coins.first; it != self->coins.last; it++) {
        (*it)->pos = self->base.pos;
        richs_coin_effect_orbit_coin(self, *it);
        (*it)->age++;
    }
    for (it = self->lights.first; it != self->lights.last; it++) {
        (*it)->pos = self->base.pos;
        richs_coin_effect_orbit_light(self, *it);
        (*it)->age++;
    }
}

void richs_coin_effect_draw_particle(struct Particle *p)
{
    if (p)
        particle_render(p, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
}

void richs_coin_effect_render(struct Effect *effect)
{
    struct RichsCoinEffect *self = (struct RichsCoinEffect *)effect;
    struct Particle **it;

    for (it = self->coins.first; it != self->coins.last; it++)
        richs_coin_effect_draw_particle(*it);
    for (it = self->lights.first; it != self->lights.last; it++)
        richs_coin_effect_draw_particle(*it);
}

void richs_coin_effect_release(struct Effect *effect)
{
    struct RichsCoinEffect *self = (struct RichsCoinEffect *)effect;
    struct Particle **it;

    for (it = self->coins.first; it != self->coins.last; it++)
        particle_free(*it);
    self->coins.last = self->coins.first;
    for (it = self->lights.first; it != self->lights.last; it++)
        particle_free(*it);
    self->lights.last = self->lights.first;
}
