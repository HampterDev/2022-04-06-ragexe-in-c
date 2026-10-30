#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "core/fatal.h"
#include "core/memory.h"
#include "effects/particle.h"

struct ParticleQueue {
    struct Particle **items;
    size_t head, count, capacity;
};

static struct ParticleQueue pools[PARTICLE_KIND_COUNT];
static unsigned char initialized;

static void queue_push(struct ParticleQueue *queue, struct Particle *particle)
{
    if (queue->count == queue->capacity) {
        size_t capacity, i;
        struct Particle **items;
        if (queue->capacity > SIZE_MAX / sizeof(*items) / 2)
            fatal_error("Particle queue too large");
        capacity = queue->capacity ? queue->capacity * 2 : 32;
        items = mem_alloc(capacity * sizeof(*items));
        for (i = 0; i < queue->count; ++i)
            items[i] = queue->items[(queue->head + i) % queue->capacity];
        mem_free(queue->items);
        queue->items = items;
        queue->head = 0;
        queue->capacity = capacity;
    }
    queue->items[(queue->head + queue->count) % queue->capacity] = particle;
    ++queue->count;
}

static struct Particle *queue_pop(struct ParticleQueue *queue)
{
    struct Particle *particle;
    if (!queue->count)
        return NULL;
    particle = queue->items[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    if (!--queue->count)
        queue->head = 0;
    return particle;
}

unsigned char effect_memory_create(void)
{
    if (initialized)
        return 0;
    memset(pools, 0, sizeof pools);
    initialized = 1;
    return 1;
}

void effect_memory_destroy(void)
{
    int kind;
    for (kind = 0; kind < PARTICLE_KIND_COUNT; ++kind) {
        struct Particle *particle;
        while ((particle = queue_pop(&pools[kind])) != NULL) {
            if (kind == PARTICLE_KIND_EZ2STR)
                ez2_str_particle_destruct((struct Ez2StrParticle *)particle);
            mem_free(particle);
        }
        mem_free(pools[kind].items);
    }
    memset(pools, 0, sizeof pools);
    initialized = 0;
}

static struct Particle *particle_take(enum ParticleKind kind, Vec3 pos, int list_flags)
{
    static const Vec3 origin = {0, 0, 0};
    struct Particle *particle;
    assert(initialized);
    particle = queue_pop(&pools[kind]);
    switch (kind) {
    case PARTICLE_KIND_PLAIN:
        if (particle)
            particle_reset(particle);
        else {
            particle = mem_try_alloc(sizeof *particle);
            if (particle) particle_construct(particle, origin, -1);
        }
        break;
    case PARTICLE_KIND_SPRITE: {
        struct SpriteParticle *sprite = (struct SpriteParticle *)particle;
        if (sprite)
            sprite_particle_reset(sprite);
        else {
            sprite = mem_try_alloc(sizeof *sprite);
            if (sprite) sprite_particle_construct(sprite, origin, -1);
        }
        particle = sprite ? &sprite->base : NULL;
        break;
    }
    case PARTICLE_KIND_EZ2STR: {
        struct Ez2StrParticle *animation = (struct Ez2StrParticle *)particle;
        if (animation)
            ez2_str_particle_reset(animation);
        else {
            animation = mem_try_alloc(sizeof *animation);
            if (animation) ez2_str_particle_construct(animation, origin, -1);
        }
        particle = animation ? &animation->base : NULL;
        break;
    }
    case PARTICLE_KIND_MULTITEX: {
        struct MultiTexParticle *quad = (struct MultiTexParticle *)particle;
        if (quad)
            multi_tex_particle_reset(quad);
        else {
            quad = mem_try_alloc(sizeof *quad);
            if (quad) multi_tex_particle_construct(quad, origin, -1);
        }
        particle = quad ? &quad->base : NULL;
        break;
    }
    default:
        fatal_error("Invalid particle kind");
    }
    if (particle) {
        particle->pos = pos;
        particle->list_flags = list_flags;
    }
    return particle;
}

struct Particle *particle_new(Vec3 pos, int list_flags)
{
    return particle_take(PARTICLE_KIND_PLAIN, pos, list_flags);
}

struct SpriteParticle *sprite_particle_new(Vec3 pos, int list_flags)
{
    return (struct SpriteParticle *)particle_take(PARTICLE_KIND_SPRITE, pos, list_flags);
}

struct Ez2StrParticle *ez2str_particle_new(Vec3 pos, int list_flags)
{
    return (struct Ez2StrParticle *)particle_take(PARTICLE_KIND_EZ2STR, pos, list_flags);
}

struct MultiTexParticle *multitex_particle_new(Vec3 pos, int list_flags)
{
    return (struct MultiTexParticle *)particle_take(PARTICLE_KIND_MULTITEX, pos, list_flags);
}

void particle_free(struct Particle *particle)
{
    assert(initialized && particle);
    assert((unsigned)particle->pool_kind < PARTICLE_KIND_COUNT);
    if (particle->pool_kind == PARTICLE_KIND_EZ2STR)
        ez2_str_particle_reset((struct Ez2StrParticle *)particle);
    queue_push(&pools[particle->pool_kind], particle);
}
