#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "effect_factory_classes.h"
#ifdef EFFECT_FACTORIES_REFERENCE
#else
#include "../src/effects/effect_mgr.c"
#include "../src/effects/effect_registry.c"
#endif
#include "std/std.h"

static void *construct_effect(void *memory)
{
    memset(memory, 0, sizeof(struct Effect));
    return memory;
}
#ifdef EFFECT_FACTORIES_REFERENCE
#define TEST_FACTORY(type, bytes, ctor, create) void *ctor(void *memory) { return construct_effect(memory); }
#else
static const char *created_type;
static struct Effect *new_effect(size_t bytes)
{
    void *memory = mem_try_alloc(bytes);
    return memory ? construct_effect(memory) : NULL;
}
#define TEST_FACTORY(type, bytes, ctor, create) \
    struct Effect *create(void) { created_type = #type; return new_effect(bytes); }
#endif
TEST_EFFECT_FACTORIES(TEST_FACTORY)
#undef TEST_FACTORY

#ifdef EFFECT_FACTORIES_REFERENCE
#define TEST_FACTORY(type, bytes, ctor, create) &type##_allocator,
static const struct EffectAllocatorClass *const factories[] = {
#else
#define TEST_FACTORY(type, bytes, ctor, create) create,
static struct Effect *(*const factories[])(void) = {
#endif
    TEST_EFFECT_FACTORIES(TEST_FACTORY)
};
#undef TEST_FACTORY

static void *allocations[8192];
static unsigned allocation_count, live_count;
static int fail_allocation, hat_count, lua_calls;
static int extra_ids[8], extra_count;
static unsigned trace_hash = 2166136261u;
static FILE *trace;

void *mem_try_alloc(size_t bytes)
{
    void *memory;
    if (fail_allocation) return NULL;
    memory = malloc(bytes ? bytes : 1);
    assert(memory && allocation_count < 8192);
    allocations[allocation_count++] = memory;
    ++live_count;
    return memory;
}
void *mem_alloc(size_t bytes) { void *p = mem_try_alloc(bytes); assert(p); return p; }
void mem_free(void *memory)
{
    unsigned i;
    if (!memory) return;
    for (i = 0; i < allocation_count; ++i) {
        if (allocations[i] == memory) {
            allocations[i] = NULL;
            --live_count;
            free(memory);
            return;
        }
    }
    assert(!"free of unowned memory");
}

static void record(unsigned value)
{
    unsigned shift;
    for (shift = 0; shift < 32; shift += 8) {
        unsigned char byte = (unsigned char)(value >> shift);
        trace_hash = (trace_hash ^ byte) * 16777619u;
        if (trace) assert(fwrite(&byte, 1, 1, trace) == 1);
    }
}

static unsigned factory_index(const struct EffectFactory *node)
{
    unsigned i;
    for (i = 0; i < sizeof factories / sizeof *factories; ++i) {
#ifdef EFFECT_FACTORIES_REFERENCE
        if (node->factory->ops == factories[i]->ops) return i;
#else
        if (node->create == factories[i]) return i;
#endif
    }
    assert(!"unknown factory");
    return 0;
}

static void reset_allocations(void)
{
    unsigned i;
#ifndef EFFECT_FACTORIES_REFERENCE
    assert(live_count == 0);
#endif
    for (i = 0; i < allocation_count; ++i) free(allocations[i]);
    allocation_count = live_count = 0;
}

static void registry_case(int which)
{
    struct EffectMgr manager;
    struct TreeNode *node;
    struct Effect *effect;
    unsigned before, registered;
    hat_count = which == 1 ? 3 : 0;
    extra_count = which == 1 ? 3 : which == 2 ? 1 : 0;
    extra_ids[0] = which == 2 ? 0x943 : 0xc8;
    extra_ids[1] = extra_ids[2] = 45000;
    lua_calls = 0;
    effect_mgr_construct(&manager);
    assert(manager.ready == (which != 2));
    assert(lua_calls == 1);
    record(which);
    record(manager.ready);
    registered = manager.factories.size;
    record(registered);
    for (node = tree_first(&manager.factories); node != manager.factories.head; node = tree_next(node)) {
        record(TREE_VALUE(node, struct EffectFactory)->type);
        record(factory_index(TREE_VALUE(node, struct EffectFactory)));
    }
    assert(effect_mgr_is_registered(&manager, 0xc8));
    assert(!effect_mgr_create_of_type(&manager, -1));
    before = live_count;
    effect = effect_mgr_create_of_type(&manager, 0xc8);
    assert(effect && live_count == before + 1);
#ifndef EFFECT_FACTORIES_REFERENCE
    assert(!strcmp(created_type, "Level99Effect"));
#endif
    mem_free(effect);
    fail_allocation = 1;
    assert(!effect_mgr_create_of_type(&manager, 0xc8));
    fail_allocation = 0;
    assert(live_count == before);
    effect_mgr_clear(&manager);
    assert(manager.factories.size == 0);
    assert(!effect_mgr_create_of_type(&manager, 0xc8));
    effect_mgr_destruct(&manager);
    reset_allocations();
}

static void shown_effect_map_case(void)
{
    struct Tree map;
    struct IntIntPair *pair;
    int keys[] = {7, -2, 100};
    unsigned i;
    tree_init(&map);
    for (i = 0; i < 3; ++i) {
        pair = tree_get(&map, &keys[i], sizeof keys[i], sizeof *pair, tree_less_int);
        assert(pair->value == 0);
        pair->value = keys[i] + 1;
    }
    assert(map.size == 3);
    for (i = 0; i < 3; ++i) {
        pair = tree_get(&map, &keys[i], sizeof keys[i], sizeof *pair, tree_less_int);
        assert(pair->value == keys[i] + 1);
    }
    assert(map.size == 3);
    tree_clear(&map, NULL);
    assert(map.size == 0 && live_count == 1);
    tree_free(&map, NULL);
    reset_allocations();
}

static void effect_maps_case(void)
{
    struct EffectMgr manager;
    struct Effect tracked;
    struct List *list;
    unsigned count;

    hat_count = extra_count = 0;
    effect_mgr_construct(&manager);
    list = owner_effects(&manager, -7);
    assert(manager.by_owner.size == 1 && list->size == 0);
    assert(list->head->next == list->head && list->head->prev == list->head);
    count = live_count;
    assert(owner_effects(&manager, -7) == list && live_count == count);
    memset(&tracked, 0, sizeof tracked);
    tracked.type = 0x3ce;
    effect_mgr_track(&manager, &tracked);
    effect_mgr_track(&manager, &tracked);
    assert(manager.tracked.size == 1);
    assert(effect_mgr_tracked_at(&manager, 0x3ce, 1) == &tracked && !effect_mgr_tracked_at(&manager, 0x3ce, 2));
    assert(!effect_mgr_tracked_at(&manager, 0x401, 0));
    effect_mgr_destruct(&manager);
    reset_allocations();
}

static void singleton_lifetime_cases(void)
{
    struct EffectMgr *saved;
    unsigned before;
    hat_count = extra_count = 0;
    effect_mgr_destroy_instance();
    assert(!g_effectMgr && !live_count);
    fail_allocation = 1;
    assert(!effect_mgr_create() && !g_effectMgr && !live_count);
    fail_allocation = 0;
    assert(effect_mgr_create() && g_effectMgr->ready);
    saved = g_effectMgr;
    before = live_count;
    assert(!effect_mgr_create() && g_effectMgr == saved && live_count == before);
    effect_mgr_destroy_instance();
    assert(!g_effectMgr && !live_count);
    effect_mgr_destroy_instance();
    assert(!live_count);
    reset_allocations();

    extra_count = 1;
    extra_ids[0] = 0x943;
    assert(!effect_mgr_create() && !g_effectMgr);
    reset_allocations();
    extra_count = 0;
    assert(effect_mgr_create());
    effect_mgr_destroy_instance();
    assert(!g_effectMgr && !live_count);
    reset_allocations();
}

int main(int argc, char **argv)
{
    int i;
    if (argc > 1) { trace = fopen(argv[1], "wb"); assert(trace); }
    for (i = 0; i < 3; ++i) registry_case(i);
    assert(trace_hash == 0xd3673c01u);
    shown_effect_map_case();
    effect_maps_case();
    singleton_lifetime_cases();
    if (trace) assert(fclose(trace) == 0);
    printf("Effect factories: registration, duplicates, creation, allocation failure and cleanup passed; digest %08x.\n", trace_hash);
    return 0;
}

#include "effect_factories_boundaries.h"
