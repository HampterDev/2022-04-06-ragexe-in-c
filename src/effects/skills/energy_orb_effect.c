#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <stdlib.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "effects/effect_controls.h"
#include "std/vec.h"

enum EnergyOrbType {
    ENERGY_ORB_TWO_TONE = 0xe4,
    ENERGY_ORB_FIRE = 0x2df,
    ENERGY_ORB_LIGHTNING = 0x2e0,
    ENERGY_ORB_WATER = 0x2e1,
    ENERGY_ORB_STONE = 0x2e2,
    ENERGY_ORB_FOG = 0x3ed,
};

#define ORB_FADE_IN_FRAMES 200

struct EnergyOrbEffect {
    struct Effect base;
    int ring_count;
    int orbs_per_ring;
    struct Vec orbs;
};

struct EnergyOrbParticle {
    int orb_phase;
    float orb_radius;
    float orb_bob;
    float orb_height;
    float orb_speed;
};
PARTICLE_EFFECT_DATA_FITS(EnergyOrbParticle);
static __inline struct EnergyOrbParticle *energy_orb_particle(const struct Particle *p)
{
    return (struct EnergyOrbParticle *)p->effect_data;
}
#define ORB_ANGLE(p) ((p)->roll)

struct String;
void energy_orb_effect_init(struct Effect *self);
void energy_orb_effect_process(struct Effect *self);
void energy_orb_effect_render(struct Effect *self);
void energy_orb_effect_release(struct Effect *self);
static void energy_orb_effect_destroy(struct Effect *effect);

static const struct EffectOps energy_orb_effect_ops = {
    .set_submit = effect_set_submit,
    .init = energy_orb_effect_init,
    .update_range = effect_update_range,
    .process = energy_orb_effect_process,
    .render = energy_orb_effect_render,
    .release = energy_orb_effect_release,
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
    .destroy = energy_orb_effect_destroy,
    .fade_out = effect_fade_out,
};

#define MATRIX_BYTES (12 * sizeof(float))

static int rand_mod(int n)
{
    int r = rand() % n;
    return r < 0 ? 0 : r;
}

static void particle_texture(struct Particle *p, const char *name)
{
    struct String s;
    string_init(&s);
    string_assign_cstr(&s, name);
    particle_set_texture(p, s);
}

struct EnergyOrbEffect *energy_orb_effect_construct(struct EnergyOrbEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &energy_orb_effect_ops;
    self->ring_count = 0;
    self->orbs_per_ring = 2;
    self->orbs.first = self->orbs.last = self->orbs.end = 0;
    return self;
}

struct Effect *energy_orb_effect_new(void)
{
    struct EnergyOrbEffect *self = mem_try_alloc(sizeof *self);

    return self ? &energy_orb_effect_construct(self)->base : NULL;
}

/* Effect types created as EnergyOrbEffect, registered at startup through g_effectTypes
   (effects/effect_registry.c). A type missing here is not created. */
static const struct EffectTypeRow energy_orb_effect_type_rows[] = {
    EFFECT_TYPE(0x0e4, energy_orb_effect_new),
    EFFECT_TYPES(0x2e1, 0x2e2, energy_orb_effect_new),
    EFFECT_TYPES(0x2df, 0x2e0, energy_orb_effect_new),
    EFFECT_TYPE(0x3ed, energy_orb_effect_new),
};
const struct EffectTypeTable energy_orb_effect_types = EFFECT_TYPE_TABLE(energy_orb_effect_type_rows);

void energy_orb_effect_destruct(struct EnergyOrbEffect *self)
{
    self->base.ops = &energy_orb_effect_ops;
    vec_free(&self->orbs);
    effect_destruct(&self->base);
}

static void energy_orb_effect_destroy(struct Effect *effect)
{
    struct EnergyOrbEffect *self = (struct EnergyOrbEffect *)effect;
    energy_orb_effect_destruct(self);
    mem_free(self);
}

void energy_orb_effect_dress(struct EnergyOrbEffect *self, struct Particle *p)
{
    if (!p)
        return;
    switch (self->base.type) {
    case ENERGY_ORB_TWO_TONE:
        if (p->index == 0)
            particle_set_color(p, 0, 0, 0xff);
        else
            particle_set_color(p, 0xff, 0xff, 0);
        break;
    case ENERGY_ORB_FIRE:
        particle_texture(p, "effect\\FireOrb.bmp");
        p->scale = 2.3f;
        break;
    case ENERGY_ORB_LIGHTNING:
        particle_texture(p, "effect\\LightningOrb.bmp");
        p->scale = 2.3f;
        break;
    case ENERGY_ORB_WATER:
        particle_texture(p, "effect\\WaterOrb.bmp");
        p->scale = 1.3f;
        break;
    case ENERGY_ORB_STONE:
        particle_texture(p, "effect\\StoneOrb.bmp");
        p->scale = 2.3f;
        break;
    case ENERGY_ORB_FOG:
        particle_texture(p, "effect\\fog1.tga");
        p->scale = 6.0f;
        energy_orb_particle(p)->orb_speed = 0.05f;
        energy_orb_particle(p)->orb_radius = 4.0f;
        energy_orb_particle(p)->orb_bob = 0.0f;
        energy_orb_particle(p)->orb_height = -8.0f;
        p->dest_blend = 6;
        break;
    }
}

void energy_orb_effect_update_orb(struct EnergyOrbEffect *self, struct Particle *p)
{
    float turn[12], scale[12], move[12], tmp[12], m[12];
    float angle, size, pulse, lift, radius, y;
    int alpha, phase, a;

    if (!p)
        return;
    alpha = p->alpha;
    if (p->age <= ORB_FADE_IN_FRAMES)
        p->alpha = ++alpha;
    angle = energy_orb_particle(p)->orb_speed * 5.0f + ORB_ANGLE(p);
    if (angle > 360.0f)
        angle = angle - 360.0f;
    ORB_ANGLE(p) = angle;
    if (alpha <= 0)
        return;

    memcpy(turn, g_identityMatrix.m, MATRIX_BYTES);
    mat_set_rotate_x(turn, game_camera_pitch() / 180.0f * 3.141592f);
    mat_rotate_x(turn, 3.141592f);

    phase = (int)((float)energy_orb_particle(p)->orb_phase + energy_orb_particle(p)->orb_bob) % 360;
    energy_orb_particle(p)->orb_phase = phase;
    size = p->scale;
    pulse = size * 0.1f * g_sinTable[phase];
    if (self->orbs_per_ring > 1 && p->index == 0)
        pulse = (float)((double)size * 0.1 + (double)pulse);
    memcpy(scale, g_identityMatrix.m, MATRIX_BYTES);
    mat_set_scale(scale, size + pulse, size + pulse, size + pulse);

    lift = g_sinTable[energy_orb_particle(p)->orb_phase] * 0.6f;
    radius = energy_orb_particle(p)->orb_radius + lift;
    y = energy_orb_particle(p)->orb_bob * lift + energy_orb_particle(p)->orb_height;
    a = (int)ORB_ANGLE(p);
    memcpy(move, g_identityMatrix.m, MATRIX_BYTES);
    mat_set_translation(move, g_cosTable[a] * radius, y, g_sinTable[a] * radius);

    mat_multiply(tmp, scale, turn);
    mat_multiply(m, tmp, move);
    memcpy(p->matrix, m, MATRIX_BYTES);
}

void energy_orb_effect_init(struct Effect *effect)
{
    struct EnergyOrbEffect *self = (struct EnergyOrbEffect *)effect;
    int ring, i;

    for (ring = 0; ring < self->ring_count; ring++) {
        int phase = rand_mod(360);
        int bob = rand_mod(3);
        for (i = 0; i < self->orbs_per_ring; i++) {
            struct Particle *p = particle_new(self->base.pos, 5);
            if (!p)
                continue;
            p->effect_type = self->base.type;
            p->group = ring;
            p->index = i;
            p->owner_tag = 1;
            ORB_ANGLE(p) = (float)(360 / self->ring_count * ring);
            particle_texture(p, "effect\\thunder_center.bmp");
            p->alpha = 0;
            p->scale = 0.8f;
            energy_orb_particle(p)->orb_speed = 0.15f;
            energy_orb_particle(p)->orb_radius = 8.0f;
            energy_orb_particle(p)->orb_phase = phase;
            energy_orb_particle(p)->orb_bob = (float)bob;
            energy_orb_particle(p)->orb_height = -12.0f;
            if (i == 0)
                p->dest_blend = 6;
            energy_orb_effect_dress(self, p);
            vec_push(&self->orbs, &p, sizeof p);
        }
    }
}

void energy_orb_effect_process(struct Effect *effect)
{
    struct EnergyOrbEffect *self = (struct EnergyOrbEffect *)effect;
    struct Particle **it;
    for (it = self->orbs.first; it != (struct Particle **)self->orbs.last; it++) {
        struct Particle *p = *it;
        p->pos = self->base.pos;
        energy_orb_effect_update_orb(self, p);
        p->age++;
    }
}

void energy_orb_render(struct Particle *p)
{
    if (p)
        particle_render(p, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
}

void energy_orb_effect_render(struct Effect *effect)
{
    struct EnergyOrbEffect *self = (struct EnergyOrbEffect *)effect;
    struct Particle **it;
    for (it = self->orbs.first; it != (struct Particle **)self->orbs.last; it++)
        energy_orb_render(*it);
}

void energy_orb_effect_release(struct Effect *effect)
{
    struct EnergyOrbEffect *self = (struct EnergyOrbEffect *)effect;
    struct Particle **it;
    for (it = self->orbs.first; it != (struct Particle **)self->orbs.last; it++)
        particle_free(*it);
    self->orbs.last = self->orbs.first;
}

void energy_orb_effect_set_ring_count(struct Effect *effect, int count)
{
    struct EnergyOrbEffect *self = (struct EnergyOrbEffect *)effect;
    if (self->ring_count != count) {
        self->ring_count = count;
        energy_orb_effect_init(&self->base);
    }
}
