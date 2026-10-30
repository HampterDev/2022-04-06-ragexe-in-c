#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef PARTICLE_POOLS_REFERENCE
#else
#include "../src/effects/particle.c"
#include "../src/effects/sprite_particle.c"
#include "../src/effects/ez2str_particle.c"
#include "../src/effects/multitex_particle.c"
#include "../src/effects/particle_pool.c"
#endif

enum { PARTICLES = 130 };
static void *blocks[8192];
static unsigned block_count, live_count, unlocks;
static int fail_allocation;
static unsigned trace_hash = 2166136261u;
static FILE *trace;
static struct EzEffectRes resource;

void *mem_try_alloc(size_t bytes)
{
    void *memory;
    if (fail_allocation) return NULL;
    memory = malloc(bytes ? bytes : 1);
    assert(memory && block_count < 8192);
    memset(memory, 0xa5, bytes);
    blocks[block_count++] = memory;
    ++live_count;
    return memory;
}
void *mem_alloc(size_t bytes) { void *p = mem_try_alloc(bytes); assert(p); return p; }
void mem_free(void *memory)
{
    unsigned i;
    if (!memory) return;
    for (i = 0; i < block_count; ++i) {
        if (blocks[i] == memory) {
            blocks[i] = NULL;
            --live_count;
            free(memory);
            return;
        }
    }
    assert(!"free of unowned memory");
}
void res_unlock(struct Res *res) { assert(res == &resource.res); ++unlocks; }

static void record(unsigned value)
{
    unsigned shift;
    for (shift = 0; shift < 32; shift += 8) {
        unsigned char byte = (unsigned char)(value >> shift);
        trace_hash = (trace_hash ^ byte) * 16777619u;
        if (trace) assert(fwrite(&byte, 1, 1, trace) == 1);
    }
}

static struct Particle *take(int kind)
{
    const Vec3 pos = {12, 34, 56};
    switch (kind) {
    case PARTICLE_KIND_PLAIN: return particle_new(pos, 0x205);
    case PARTICLE_KIND_SPRITE: return (struct Particle *)sprite_particle_new(pos, 0x205);
    case PARTICLE_KIND_EZ2STR: return (struct Particle *)ez2str_particle_new(pos, 0x205);
    case PARTICLE_KIND_MULTITEX: return (struct Particle *)multitex_particle_new(pos, 0x205);
    }
    abort();
}

static void check_reset(struct Particle *p, int kind)
{
    assert(p->pool_kind == kind && p->list_flags == 0x205);
    assert(p->pos.x == 12 && p->pos.y == 34 && p->pos.z == 56);
    assert(p->r == 255 && p->g == 255 && p->b == 255 && p->alpha == 255);
    assert(p->scale == 1 && !p->texture && !p->flags && !p->age);
    assert(p->life_seconds == 0 && p->depth_bias == 0 && p->submit == particle_submit);
    assert(p->src_blend == GFX_BLEND_SRCALPHA && p->dest_blend == GFX_BLEND_ONE);
    assert(!memcmp(p->matrix, g_identityMatrix.m, sizeof p->matrix));
    assert(p->corners[0].x == -1 && p->corners[2].y == -1 && p->corners[3].z == 1);
    {
        static const unsigned char zero[sizeof p->effect_data];
        assert(!memcmp(p->effect_data, zero, sizeof zero));
    }
    if (kind == PARTICLE_KIND_SPRITE) {
        struct SpriteParticle *s = (struct SpriteParticle *)p;
        assert(!s->spr && !s->act && !s->frame_delay && !s->roll_step);
        assert(!s->action && !s->motion && s->loop == SPRITE_LOOP);
    } else if (kind == PARTICLE_KIND_EZ2STR) {
        struct Ez2StrParticle *s = (struct Ez2StrParticle *)p;
        assert(!s->res && !s->anim && !s->layers_done && !s->frame);
        assert(s->window_id == -1 && s->visible == 1 && s->zoom == 1 && s->alpha_scale == 1);
        assert(!s->state[0].anitype && !s->key_index[127] && !s->drawn[127]);
    } else if (kind == PARTICLE_KIND_MULTITEX) {
        struct MultiTexParticle *s = (struct MultiTexParticle *)p;
        assert(!s->texture0 && !s->texture1 && !s->u_scroll && !s->v_scroll);
    }
}

static void dirty(struct Particle *p, int kind, unsigned id)
{
    p->velocity.x = (float)id;
    p->alpha = 17; p->flags = 99; p->age = 10;
    memset(p->effect_data, 0x5a, sizeof p->effect_data);
    if (kind == PARTICLE_KIND_SPRITE) {
        struct SpriteParticle *s = (struct SpriteParticle *)p;
        s->spr = s->act = &resource; s->action = 9; s->loop = SPRITE_ONCE;
    } else if (kind == PARTICLE_KIND_EZ2STR) {
        struct Ez2StrParticle *s = (struct Ez2StrParticle *)p;
        const char *path = id % 2 ? "짧음" : "이펙트\\입자_재사용_긴_경로_시험.str";
        s->res = &resource;
        string_assign_cstr(&s->path, path);
        string_assign_cstr(&s->texture_set, path);
        s->window_id = 42; s->zoom = 7; s->drawn[127] = 1;
    } else if (kind == PARTICLE_KIND_MULTITEX) {
        struct MultiTexParticle *s = (struct MultiTexParticle *)p;
        s->texture0 = s->texture1 = (struct Texture *)&resource;
        s->u_scroll = s->v_scroll = 7;
    }
}

static void kind_case(int kind)
{
    struct Particle *all[PARTICLES], *held[64];
    unsigned i, before_unlocks = unlocks;
    for (i = 0; i < PARTICLES; ++i) {
        all[i] = take(kind); assert(all[i]);
        check_reset(all[i], kind);
        dirty(all[i], kind, i);
    }
    for (i = 0; i < 96; ++i) particle_free(all[i]);
    for (i = 0; i < 64; ++i) {
        held[i] = take(kind); assert(held[i] == all[i]);
        check_reset(held[i], kind);
    }
    for (i = 96; i < PARTICLES; ++i) particle_free(all[i]);
    for (i = 0; i < 64; ++i) particle_free(held[i]);
    for (i = 0; i < PARTICLES; ++i) {
        unsigned id = (i + 64) % PARTICLES;
        struct Particle *p = take(kind);
        assert(p == all[id]); check_reset(p, kind);
        assert(p->velocity.x == (float)id);
        if (kind == PARTICLE_KIND_EZ2STR) {
            struct Ez2StrParticle *s = (struct Ez2StrParticle *)p;
            const char *path = id % 2 ? "짧음" : "이펙트\\입자_재사용_긴_경로_시험.str";
            assert(s->path.size == strlen(path) && s->texture_set.size == strlen(path));
            assert(!strcmp(string_cstr(&s->path), path));
            assert(!strcmp(string_cstr(&s->texture_set), path));
            s->res = &resource;
        }
        record(kind); record(id);
    }
    assert(unlocks - before_unlocks == (kind == PARTICLE_KIND_EZ2STR ? PARTICLES : 0));
    for (i = 0; i < PARTICLES; ++i) particle_free(all[i]);
}

static void allocation_failure(void)
{
    int kind;
    for (kind = 0; kind < PARTICLE_KIND_COUNT; ++kind) {
        fail_allocation = 1;
#ifdef PARTICLE_POOLS_REFERENCE
        assert(!CEffectMemoryMgr_Alloc(g_effectMemoryMgr, kind));
#else
        assert(!take(kind));
#endif
        fail_allocation = 0;
    }
}

int main(int argc, char **argv)
{
    int kind;
    if (argc > 1) { trace = fopen(argv[1], "wb"); assert(trace); }
    effect_memory_destroy();
    assert(effect_memory_create() && !effect_memory_create());
#ifndef PARTICLE_POOLS_REFERENCE
    assert(block_count == 0);
#endif
    allocation_failure();
    for (kind = 0; kind < PARTICLE_KIND_COUNT; ++kind) kind_case(kind);
    effect_memory_destroy();
    assert(unlocks == PARTICLES * 2 && live_count == 0);
    record(unlocks);
    assert(trace_hash == 0xeca892c6u);
    effect_memory_destroy(); assert(live_count == 0);
    assert(effect_memory_create()); effect_memory_destroy(); assert(live_count == 0);
    if (trace) assert(fclose(trace) == 0);
    printf("Particle pools: four kinds, FIFO wrap/growth, resets, allocation failure and resource cleanup passed; digest %08x.\n", trace_hash);
    return 0;
}

#include "particle_pools_unreached.h"
