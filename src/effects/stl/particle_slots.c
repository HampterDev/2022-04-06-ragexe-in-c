#include "effects/stl/particle_slots.h"
#include "std/tree.h"

struct ParticleSlot {
    struct Particle *particle;
    unsigned char in_use;
};

void particle_slots_release_all(struct ParticleSlots *s)
{
    struct TreeNode *node;

    for (node = tree_first(&s->map); node != s->map.head; node = tree_next(node))
        TREE_VALUE(node, struct ParticleSlot)->in_use = 0;
    s->in_use = 0;
}

void particle_slots_construct(struct ParticleSlots *s)
{
    tree_init(&s->map);
    particle_slots_release_all(s);
}

void particle_slots_destroy(struct ParticleSlots *s)
{
    tree_clear(&s->map, NULL);
    s->in_use = 0;
    tree_free(&s->map, NULL);
}

void particle_slots_clear(struct ParticleSlots *s)
{
    tree_clear(&s->map, NULL);
    s->in_use = 0;
}

struct Particle *particle_slots_acquire(struct ParticleSlots *s)
{
    struct TreeNode *node;
    struct ParticleSlot *slot;

    if (!(s->in_use < s->map.size))
        return 0;
    node = tree_first(&s->map);
    while (node != s->map.head && TREE_VALUE(node, struct ParticleSlot)->in_use != 0)
        node = tree_next(node);
    if (node == s->map.head)
        return 0;
    slot = TREE_VALUE(node, struct ParticleSlot);
    slot->in_use = 1;
    s->in_use++;
    return slot->particle;
}

void particle_slots_release(struct ParticleSlots *s, struct Particle *p)
{
    struct ParticleSlot *slot = tree_find(&s->map, &p, tree_less_uint);

    if (slot && slot->in_use == 1) {
        s->in_use--;
        slot->in_use = 0;
    }
}

unsigned char particle_slots_is_used(struct ParticleSlots *s, struct Particle *p)
{
    struct ParticleSlot *slot = tree_find(&s->map, &p, tree_less_uint);

    return slot ? slot->in_use : 0;
}

void particle_slots_use_all(struct ParticleSlots *s)
{
    struct TreeNode *node;

    for (node = tree_first(&s->map); node != s->map.head; node = tree_next(node))
        TREE_VALUE(node, struct ParticleSlot)->in_use = 1;
    s->in_use = s->map.size;
}

void particle_slots_add(struct ParticleSlots *s, struct Particle *p)
{
    tree_get(&s->map, &p, sizeof p, sizeof(struct ParticleSlot), tree_less_uint);
}
