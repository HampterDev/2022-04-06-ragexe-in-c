#include "actor/map_type_table.h"
#include "skills/skill_action_registry.h"
#ifndef ACTOR_HELPER_REFERENCE
#include "skills/skill_action.h"
#endif
#include "std/std.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct Block { unsigned char *raw; size_t size; } blocks[4096];
static unsigned live_blocks, allocations, try_calls, factory_calls;
static int fail_next;
static unsigned trace_hash = 2166136261u;
static FILE *trace;

static void record(unsigned tag, int value)
{
    unsigned values[] = {tag, (unsigned)value};
    const unsigned char *bytes = (const unsigned char *)values;
    size_t i;
    if (trace) assert(fwrite(values, sizeof values, 1, trace) == 1);
    for (i = 0; i < sizeof values; ++i) trace_hash = (trace_hash ^ bytes[i]) * 16777619u;
}

void *mem_alloc(size_t size)
{
    unsigned i;
    for (i = 0; i < 4096 && blocks[i].raw; ++i) {}
    assert(i < 4096 && size <= SIZE_MAX - 32);
    blocks[i].raw = malloc(size + 32);
    assert(blocks[i].raw);
    blocks[i].size = size;
    memset(blocks[i].raw, 0xa5, size + 32);
    ++live_blocks; ++allocations;
    return blocks[i].raw + 16;
}

void *mem_try_alloc(size_t size)
{
    ++try_calls;
    if (fail_next) { fail_next = 0; return NULL; }
    return mem_alloc(size);
}

void mem_free(void *memory)
{
    unsigned i, j;
    if (!memory) return;
    for (i = 0; i < 4096 && (!blocks[i].raw || blocks[i].raw + 16 != memory); ++i) {}
    assert(i < 4096 && live_blocks);
    for (j = 0; j < 16; ++j) {
        assert(blocks[i].raw[j] == 0xa5);
        assert(blocks[i].raw[16 + blocks[i].size + j] == 0xa5);
    }
    free(blocks[i].raw); blocks[i].raw = NULL; --live_blocks;
}

void fatal_error(const char *message) { fprintf(stderr, "%s\n", message); abort(); }

void string_free(struct String *value) { abort(); }
int string_compare(const struct String *a, const struct String *b) { abort(); }

#ifdef ACTOR_HELPER_REFERENCE
const void *CSkillActionMgr_ops[] = {CSkillActionMgr_Delete};
const void *SkillActionMgrSingleton_ops[] = {SkillActionMgrSingleton_Delete};
#endif

static void map_properties(void)
{
    const int edge[] = {INT_MIN, INT_MAX};
    int cycle, i, j;
    unsigned before;
    fail_next = 1;
    assert(!map_property_mgr_create() && !g_mapFlags && !live_blocks);
    map_property_mgr_clear(NULL); map_property_mgr_release();
    for (cycle = 0; cycle < 3; ++cycle) {
        assert(map_property_mgr_create() && g_mapFlags);
        before = allocations;
        assert(!map_property_mgr_create() && allocations == before);
        assert(map_property_mgr_get(g_mapFlags, 0, 0) == 0);
        assert(allocations == before);
        for (i = 0; i < 97; ++i) {
            int map = (i * 37) % 97 - 48;
            for (j = -6; j <= 6; ++j) map_property_mgr_set(g_mapFlags, map, j, map * 100 + j);
        }
        before = allocations;
        for (i = -48; i <= 48; ++i)
            for (j = -6; j <= 6; ++j) {
                int value = map_property_mgr_get(g_mapFlags, i, j);
                assert(value == i * 100 + j); record(1, value);
                map_property_mgr_set(g_mapFlags, i, j, -value);
                assert(map_property_mgr_get(g_mapFlags, i, j) == -value);
            }
        assert(allocations == before);
        for (i = 0; i < 2; ++i) {
            map_property_mgr_set(g_mapFlags, edge[i], edge[1-i], edge[i]);
            assert(map_property_mgr_get(g_mapFlags, edge[i], edge[1-i]) == edge[i]);
            record(2, edge[i]);
        }
        assert(!map_property_mgr_get(g_mapFlags, 0, 7));
        assert(!map_property_mgr_get(g_mapFlags, 100, 0));
        before = allocations;
        map_property_mgr_clear(g_mapFlags);
        assert(!map_property_mgr_get(g_mapFlags, 1, 1));
        assert(!map_property_mgr_get(g_mapFlags, INT_MIN, INT_MAX));
        assert(allocations == before && live_blocks == 2);
        map_property_mgr_clear(g_mapFlags);
        map_property_mgr_set(g_mapFlags, 5, 6, 7);
        assert(map_property_mgr_get(g_mapFlags, 5, 6) == 7);
        map_property_mgr_release(); map_property_mgr_release();
        assert(!g_mapFlags && !live_blocks);
    }
}

#ifdef ACTOR_HELPER_REFERENCE
static void *owned_action(void)
{
    int *value = mem_alloc(sizeof *value);
    *value = 42; ++factory_calls;
    return value;
}

static void *empty_action(void) { ++factory_calls; return NULL; }

static void registry(void)
{
    const int ids[] = {INT_MIN, -1, 0, 1, INT_MAX};
    int cycle;
    unsigned i, before;
    fail_next = 1;
    assert(!skill_action_mgr_instance() && !live_blocks);
    for (cycle = 0; cycle < 3; ++cycle) {
        struct SkillActionMgr *manager = skill_action_mgr_instance();
        void *retained;
        assert(manager && live_blocks == 4);
        before = allocations;
        assert(skill_action_mgr_instance() == manager && allocations == before);
        assert(!CSkillActionMgr_NewSkillAction(manager, 22));
        assert(!CSkillActionMgr_NewEfstAction(manager, 22));
        assert(!CSkillActionMgr_NewObjectAction(manager, 22));
        assert(!skill_action_mgr_has_efst_action(manager, 22));
        for (i = 0; i < sizeof ids / sizeof ids[0]; ++i) {
            skill_action_mgr_add_skill(manager, ids[i], owned_action);
            skill_action_mgr_add_efst(manager, ids[i], empty_action);
            skill_action_mgr_add_object_action(manager, ids[i], owned_action);
            before = allocations;
            skill_action_mgr_add_skill(manager, ids[i], empty_action);
            skill_action_mgr_add_efst(manager, ids[i], owned_action);
            skill_action_mgr_add_object_action(manager, ids[i], empty_action);
            assert(allocations == before);
        }
        for (i = 0; i < sizeof ids / sizeof ids[0]; ++i) {
            int *value = (int *)CSkillActionMgr_NewSkillAction(manager, ids[i]);
            assert(value && *value == 42); record(3, *value); mem_free(value);
            assert(skill_action_mgr_has_efst_action(manager, ids[i]));
            assert(!CSkillActionMgr_NewEfstAction(manager, ids[i]));
            value = (int *)CSkillActionMgr_NewObjectAction(manager, ids[i]);
            assert(value && *value == 42); record(4, *value); mem_free(value);
        }
        retained = CSkillActionMgr_NewSkillAction(manager, 0);
        skill_action_mgr_destroy_instance();
        assert(live_blocks == 1 && *(int *)retained == 42);
        mem_free(retained); assert(!live_blocks);
    }
    assert(factory_calls == 48);
}

#else
static const struct StatusAction shared_status = {0};

static void registry(void)
{
    const int ids[] = {INT_MIN, -1, 0, 1, INT_MAX};
    int cycle;
    unsigned i, before;
    fail_next = 1;
    assert(!skill_action_mgr_instance() && !live_blocks);
    for (cycle = 0; cycle < 3; ++cycle) {
        struct SkillActionMgr *manager = skill_action_mgr_instance();
        const struct StatusAction *retained;
        assert(manager && live_blocks == 2);
        before = allocations;
        assert(skill_action_mgr_instance() == manager && allocations == before);
        assert(!skill_action_mgr_find_efst(manager, 22));
        assert(!skill_action_mgr_has_efst_action(manager, 22));
        for (i = 0; i < sizeof ids / sizeof ids[0]; ++i) {
            skill_action_mgr_add_efst(manager, ids[i], &shared_status);
            before = allocations;
            skill_action_mgr_add_efst(manager, ids[i], NULL);
            assert(allocations == before);
        }
        before = allocations;
        for (i = 0; i < sizeof ids / sizeof ids[0]; ++i) {
            assert(skill_action_mgr_has_efst_action(manager, ids[i]));
            assert(skill_action_mgr_find_efst(manager, ids[i]) == &shared_status); record(3, ids[i]);
        }
        retained = skill_action_mgr_find_efst(manager, 0);
        assert(allocations == before);
        skill_action_mgr_destroy_instance();
        assert(!live_blocks && retained == &shared_status);
    }
    assert(!factory_calls);
}

#endif

int main(int argc, char **argv)
{
    if (argc > 1 && !strcmp(argv[1], "--shutdown-probe")) {
        skill_action_mgr_destroy_instance(); skill_action_mgr_destroy_instance();
        assert(!live_blocks);
        printf("Empty registry shutdown: %u allocation attempts, %u allocations.\n", try_calls, allocations);
        return try_calls || allocations ? 2 : 0;
    }
    if (argc > 1) { trace = fopen(argv[1], "wb"); assert(trace); }
    map_properties(); registry();
    if (trace) assert(!fclose(trace));
    printf("Actor helper owners: map flags, registry records, retained actions and cleanup passed; digest %08x.\n", trace_hash);
    return 0;
}
