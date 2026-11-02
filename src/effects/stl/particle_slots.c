#include "effects/stl/particle_slots.h"
#include "std/tree.h"
#include "std/vec.h"

struct ParticleSlot {
    struct Particle *particle;
    unsigned char in_use;
};

#define SLOT_COUNT(s) VEC_COUNT(&(s)->order, struct ParticleSlot *)
#define SLOT_AT(s, i) VEC_AT(&(s)->order, struct ParticleSlot *, i)

void particle_slots_release_all(struct ParticleSlots *s)
{
    size_t i;

    for (i = 0; i < SLOT_COUNT(s); i++)
        SLOT_AT(s, i)->in_use = 0;
    s->in_use = 0;
}

void particle_slots_construct(struct ParticleSlots *s)
{
    tree_init(&s->map);
    vec_init(&s->order);
    s->in_use = 0;
}

void particle_slots_destroy(struct ParticleSlots *s)
{
    tree_clear(&s->map, NULL);
    tree_free(&s->map, NULL);
    vec_free(&s->order);
    s->in_use = 0;
}

void particle_slots_clear(struct ParticleSlots *s)
{
    tree_clear(&s->map, NULL);
    vec_clear(&s->order);
    s->in_use = 0;
}

struct Particle *particle_slots_acquire(struct ParticleSlots *s)
{
    size_t i;

    if (!(s->in_use < SLOT_COUNT(s)))
        return 0;
    for (i = 0; i < SLOT_COUNT(s); i++) {
        struct ParticleSlot *slot = SLOT_AT(s, i);
        if (!slot->in_use) {
            slot->in_use = 1;
            s->in_use++;
            return slot->particle;
        }
    }
    return 0;
}

void particle_slots_release(struct ParticleSlots *s, struct Particle *p)
{
    struct ParticleSlot *slot = tree_find(&s->map, &p, tree_less_ptr);

    if (slot && slot->in_use == 1) {
        s->in_use--;
        slot->in_use = 0;
    }
}

unsigned char particle_slots_is_used(struct ParticleSlots *s, struct Particle *p)
{
    struct ParticleSlot *slot = tree_find(&s->map, &p, tree_less_ptr);

    return slot ? slot->in_use : 0;
}

void particle_slots_use_all(struct ParticleSlots *s)
{
    size_t i;

    for (i = 0; i < SLOT_COUNT(s); i++)
        SLOT_AT(s, i)->in_use = 1;
    s->in_use = (unsigned)SLOT_COUNT(s);
}

void particle_slots_add(struct ParticleSlots *s, struct Particle *p)
{
    size_t before = s->map.size;
    struct ParticleSlot *slot = tree_get(&s->map, &p, sizeof p, sizeof *slot, tree_less_ptr);

    if (slot && s->map.size != before)
        vec_push(&s->order, &slot, sizeof slot);
}
