#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/fatal.h"

#ifdef EFFECT_OPERATIONS_REFERENCE
#define BASE_OPERATIONS(name) ((const struct EffectOps *)(name))
#define EMITTER_OPERATIONS(name) ((const struct EffectOps *)(name))
#else
#include "../src/effects/effect.c"
#include "../src/effects/skills/emitter_effect.c"
#include "../src/effects/skills/animated_emitter_effect.c"
#define BASE_OPERATIONS(name) (&(name))
#define EMITTER_OPERATIONS(name) (&(name).base)
#endif

static unsigned live_blocks, texture_calls, slot_calls;
static struct ParticleSlots *expected_slots;
static struct Particle *expected_particle;
static const char *expected_texture;
static uint32_t trace_hash = UINT32_C(2166136261);
static FILE *trace_file;

static void trace_bytes(const void *bytes, size_t count)
{
    const unsigned char *p = bytes;
    if (trace_file) assert(fwrite(bytes, 1, count, trace_file) == count);
    while (count--) trace_hash = (trace_hash ^ *p++) * UINT32_C(16777619);
}

void *mem_alloc(size_t size)
{
    void *block = malloc(size ? size : 1);
    assert(block);
    ++live_blocks;
    return block;
}

void mem_free(void *block)
{
    if (block) {
        assert(live_blocks);
        --live_blocks;
        free(block);
    }
}

void *mem_try_alloc(size_t size)
{
    return mem_alloc(size);
}

void *effect_find_actor(int aid)
{
    assert(aid == 42 || aid == -42);
    return aid == 42 ? expected_particle : NULL;
}

float effect_random_tenths(float low, float high)
{
    assert(low <= high);
    return low;
}

unsigned char particle_slots_is_used(struct ParticleSlots *slots, struct Particle *particle)
{
    assert(slots == expected_slots && particle == expected_particle);
    ++slot_calls;
    return 0;
}

void particle_set_texture(struct Particle *particle, struct String name)
{
    const char *text = string_cstr(&name);
    assert(particle == expected_particle && expected_texture);
    assert(!strcmp(text, expected_texture));
    assert(text != expected_texture);
    trace_bytes(text, name.size + 1);
    ++texture_calls;
    string_free(&name);
}

static void base_operations(void)
{
    struct Effect effect = {0};
    struct String texture;
    const char *name = "effect\\공유하지_않는_텍스처.bmp";
    effect.ops = BASE_OPERATIONS(effect_ops);
    string_init(&texture);
    string_assign_cstr(&texture, name);
    assert(live_blocks == 1);
    effect.ops->set_texture(&effect, texture);
    assert(!live_blocks);
    effect.owner_aid = 42;
    assert(effect.ops->is_alive(&effect) == 1);
    effect.owner_aid = -42;
    assert(effect.ops->is_alive(&effect) == 0);
}

static void duration_boundaries(void)
{
    static const int values[] = {INT_MIN, -1, 0, 1, INT_MAX};
    struct Effect effect = {0};
    unsigned frame, duration;
    for (frame = 0; frame < 5; ++frame) {
        for (duration = 0; duration < 5; ++duration) {
            unsigned char alive;
            effect.frame = values[frame];
            effect.duration = values[duration];
#ifdef EFFECT_OPERATIONS_REFERENCE
            alive = !effect_is_finished(&effect);
#else
            alive = effect_is_within_duration(&effect);
#endif
            assert(alive == (frame < duration));
            trace_bytes(&alive, sizeof alive);
        }
    }
}

static void emitter_operations(int animated)
{
    struct AnimatedEmitterEffect effect = {0};
    struct EmitterEffect *emitter = &effect.base;
    struct Effect *base = &emitter->base;
    struct Particle particle = {0};
    struct Particle *particles[] = {&particle};
    struct String names[2][5];
    const char *textures[2][5] = {
        {"a.bmp", "b.bmp", "c.bmp", "d.bmp", "e.bmp"},
        {"effect\\한글_애니메이션_0.bmp", "effect\\한글_애니메이션_1.bmp",
         "effect\\한글_애니메이션_2.bmp", "effect\\한글_애니메이션_3.bmp",
         "effect\\한글_애니메이션_4.bmp"}
    };
    int set, frame;
    base->ops = animated ? EMITTER_OPERATIONS(animated_emitter_effect_ops)
                          : EMITTER_OPERATIONS(emitter_effect_ops);
    base->pos.x = -2.5f; base->pos.y = 3.25f; base->pos.z = 9.0f;
    base->paused = 1;
    emitter->particles.first = particles;
    emitter->particles.last = emitter->particles.end = particles + 1;
    emitter->life_min = emitter->life_max = 1.5f;
    emitter->size_min = emitter->size_max = 2.5f;
    emitter->velocity_min.x = emitter->velocity_max.x = -4.0f;
    emitter->src_blend = -1; emitter->dest_blend = 7;
    particle.src_blend = 3;
    expected_particle = &particle;
    expected_slots = &emitter->slots;
    string_init(&emitter->texture);
    expected_texture = "effect\\방출_텍스처.bmp";
    string_assign_cstr(&emitter->texture, expected_texture);
    base->ops->restart(base);
    assert(particle.age == 0 && particle.alpha == 0);
    assert(particle.life_seconds == 1.5f && particle.scale == 2.5f);
    assert(particle.velocity.x == -4.0f && particle.offset.y == -10.0f);
    assert(particle.src_blend == 3 && particle.dest_blend == 7);
    assert(!memcmp(&particle.pos, &base->pos, sizeof particle.pos));

    for (set = 0; set < 2; ++set) {
        for (frame = 0; frame < 5; ++frame) {
            string_init(&names[set][frame]);
            string_assign_cstr(&names[set][frame], textures[set][frame]);
        }
        effect.sets[set].first = names[set];
        effect.sets[set].last = effect.sets[set].end = names[set] + 5;
    }
    for (set = 0; set < 2; ++set) {
        animated_emitter_particle(&particle)->anim_set = (float)set;
        for (frame = 0; frame < 80; ++frame) {
            particle.age = frame;
            expected_texture = textures[set][frame / 8 % 5];
            base->ops->process(base);
            assert(particle.age == frame + 1);
            assert(!memcmp(&particle.pos, &base->pos, sizeof particle.pos));
            trace_bytes(&particle.age, sizeof particle.age);
            trace_bytes(&particle.pos, sizeof particle.pos);
        }
    }
    emitter->max_frames = 1;
    base->frame = 1;
    assert(base->ops->is_alive(base));
    base->frame = 2;
    assert(!base->ops->is_alive(base));
    emitter->max_frames = INT_MAX;
    base->frame = INT_MAX;
    assert(base->ops->is_alive(base));
    for (set = 0; set < 2; ++set)
        for (frame = 0; frame < 5; ++frame) string_free(&names[set][frame]);
    string_free(&emitter->texture);
    assert(!live_blocks);
}

int main(int argc, char **argv)
{
    struct Particle owner;
    if (argc > 1) { trace_file = fopen(argv[1], "wb"); assert(trace_file); }
    expected_particle = &owner;
    base_operations();
    duration_boundaries();
    emitter_operations(0);
    emitter_operations(1);
    assert(slot_calls == 320 && texture_calls == 162);
    if (trace_file) assert(!fclose(trace_file));
    printf("Effect operations: base ownership, inherited emitter dispatch and animation frames passed; digest %08x.\n", (unsigned)trace_hash);
    return 0;
}

#include "effect_operations_unreached.h"
