#ifndef EFFECTS_SKILLS_EMITTER_EFFECT_H
#define EFFECTS_SKILLS_EMITTER_EFFECT_H

#include "effects/particle.h"
#include "effects/stl/particle_slots.h"

struct EmitterParticle {
    float speed;
};

struct EmitterEffect;
struct EmitterEffectOps {
    struct EffectOps base;
    void (*init_particle)(struct EmitterEffect *, struct Particle *);
    void (*process_particle)(struct EmitterEffect *, struct Particle *);
};

struct EmitterEffect {
    struct Effect base;
    Vec3 velocity_min;
    Vec3 velocity_max;
    Vec3 acceleration;
    Vec3 spawn_box;
    float emit_carry;
    float frame_time;
    float size_min, size_max;
    float life_min, life_max;
    float rate_min, rate_max;
    float scale_x, scale_y;
    int r, g, b;
    int alpha;
    struct String texture;
    int src_blend, dest_blend;
    int count;
    float particle_speed;
    unsigned char with_depth;
    unsigned char fixed_facing;
    Vec3 turn;
    unsigned char keep_particles;
    struct Vec particles;
    struct ParticleSlots slots;
    int max_frames;
};

struct EmitterEffect *emitter_effect_construct(struct EmitterEffect *self);
void emitter_effect_destruct(struct EmitterEffect *self);
void emitter_effect_init(struct Effect *self);
void emitter_effect_update_range(struct Effect *self);
void emitter_effect_process(struct Effect *self);
void emitter_effect_render(struct Effect *self);
void emitter_effect_release(struct Effect *self);
unsigned char emitter_effect_is_alive(struct Effect *self);
void emitter_effect_restart(struct Effect *self);
void emitter_effect_init_particle(struct EmitterEffect *self, struct Particle *p);
void emitter_effect_process_particle(struct EmitterEffect *self, struct Particle *p);

void emitter_effect_set_count(struct EmitterEffect *self, int count);
void emitter_effect_set_particle_speed(struct EmitterEffect *self, float speed);

#endif
