#ifndef EFFECTS_STL_PARTICLE_SLOTS_H
#define EFFECTS_STL_PARTICLE_SLOTS_H

#include "base.h"
#include "std/container_types.h"

struct Particle;

struct ParticleSlots {
    struct Tree map;
    unsigned in_use;
};

void particle_slots_construct(struct ParticleSlots *s);
void particle_slots_destroy(struct ParticleSlots *s);
void particle_slots_clear(struct ParticleSlots *s);
struct Particle *particle_slots_acquire(struct ParticleSlots *s);
void particle_slots_release(struct ParticleSlots *s, struct Particle *p);
void particle_slots_release_all(struct ParticleSlots *s);
unsigned char particle_slots_is_used(struct ParticleSlots *s, struct Particle *p);
void particle_slots_use_all(struct ParticleSlots *s);
void particle_slots_add(struct ParticleSlots *s, struct Particle *p);

#endif
