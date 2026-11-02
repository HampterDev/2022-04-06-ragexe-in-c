#include "base.h"
#include "std/std.h"
#include "skills/skill_action.h"
#include "game/context.h"
#include "resources/attr_queries.h"
#include "core/fatal.h"
#include <assert.h>
#include <limits.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct {
    unsigned char *memory;
    size_t size;
} blocks[1024];
static unsigned live_blocks, allocations, observed_id;
static int fail_next, expect_bad_call;
static jmp_buf bad_call;

void string_free(struct String *value)
{
    (void)value;
    abort();
}
int string_compare(const struct String *a, const struct String *b)
{
    (void)a;
    (void)b;
    abort();
}

void *mem_alloc(size_t size)
{
    unsigned i;
    unsigned char *p = malloc(size + 16);
    assert(p);
    memset(p, 0xa5, size + 16);
    for (i = 0; i < 1024 && blocks[i].memory; ++i) {
    }
    assert(i < 1024);
    blocks[i].memory = p;
    blocks[i].size = size;
    ++live_blocks;
    ++allocations;
    return p;
}

void *mem_try_alloc(size_t size)
{
    if (fail_next) {
        fail_next = 0;
        return NULL;
    }
    return mem_alloc(size);
}

void mem_free(void *p)
{
    size_t size;
    unsigned i, j;
    if (!p)
        return;
    for (i = 0; i < 1024 && blocks[i].memory != p; ++i) {
    }
    assert(i < 1024);
    size = blocks[i].size;
    for (j = 0; j < 16; ++j)
        assert(blocks[i].memory[size + j] == 0xa5);
    free(p);
    blocks[i].memory = NULL;
    --live_blocks;
}

__declspec(noreturn) void fatal_error(const char *message)
{
    if (!strcmp(message, "invalid function call")) {
        assert(expect_bad_call);
        longjmp(bad_call, 1);
    }
    abort();
}

static void second_status_on(int status, int value, SkillActor *actor)
{
    (void)status;
    (void)value;
    (void)actor;
    observed_id = 99;
}
static const struct StatusAction first_status = {0};
static const struct StatusAction second_status = {.on = second_status_on};

#define EFST_ACTION(id, cls)                                                                       \
    static void cls##_observe(int status, int value, SkillActor *actor)                            \
    {                                                                                              \
        (void)status;                                                                              \
        (void)value;                                                                               \
        (void)actor;                                                                               \
        observed_id = id;                                                                          \
    }                                                                                              \
    const struct StatusAction cls##_ops = {.on = cls##_observe};
#include "skills/skill_actions.def"
#undef EFST_ACTION

static void registration_semantics(void)
{
    static const int ids[] = {INT_MIN, -1, 0, 1, INT_MAX};
    struct SkillActionMgr *mgr;
    const struct StatusAction *retained;
    unsigned i, pass, before;

    fail_next = 1;
    assert(!skill_action_mgr_instance() && !live_blocks);
    fail_next = 1;
    before = allocations;
    skill_action_mgr_destroy_instance();
    assert(!live_blocks && allocations == before && fail_next);
    fail_next = 0;
    mgr = skill_action_mgr_instance();
    assert(mgr && live_blocks == 2);
    before = allocations;
    assert(skill_action_mgr_instance() == mgr && allocations == before);
    assert(!skill_action_mgr_find_efst(mgr, 0));
    assert(!skill_action_mgr_has_efst_action(mgr, 0));

    for (i = 0; i < sizeof ids / sizeof ids[0]; ++i) {
        before = allocations;
        skill_action_mgr_add_efst(mgr, ids[i], &first_status);
        assert(allocations == before + 1);
        skill_action_mgr_add_efst(mgr, ids[i], &second_status);
        assert(allocations == before + 1);
    }
    before = allocations;
    for (pass = 0; pass < 1000; ++pass) {
        for (i = 0; i < sizeof ids / sizeof ids[0]; ++i) {
            assert(skill_action_mgr_has_efst_action(mgr, ids[i]));
            assert(skill_action_mgr_find_efst(mgr, ids[i]) == &first_status);
        }
    }
    assert(allocations == before);
    skill_action_mgr_add_efst(mgr, 2, NULL);
    assert(skill_action_mgr_has_efst_action(mgr, 2));
    expect_bad_call = 1;
    if (!setjmp(bad_call)) {
        skill_action_mgr_find_efst(mgr, 2);
        abort();
    }
    expect_bad_call = 0;
    retained = skill_action_mgr_find_efst(mgr, 0);
    skill_action_mgr_destroy_instance();
    assert(!live_blocks && retained == &first_status && !retained->on);
    before = allocations;
    skill_action_mgr_destroy_instance();
    assert(!live_blocks && allocations == before);
}

static void startup_bindings(void)
{
    struct SkillActionMgr *mgr = skill_action_mgr_instance();
    unsigned before = allocations, registrations = 0;
#define EFST_ACTION(id, cls) ++registrations;
#include "skills/skill_actions.def"
#undef EFST_ACTION
    skill_actions_register();
    assert(registrations == 5 && allocations == before + registrations);
    before = allocations;
    skill_actions_register();
#define EFST_ACTION(id, cls)                                                                       \
    assert(skill_action_mgr_find_efst(mgr, id) == &cls##_ops);                                     \
    observed_id = 0;                                                                               \
    skill_action_mgr_find_efst(mgr, id)->on(id, 9, NULL);                                          \
    assert(observed_id == id);
#include "skills/skill_actions.def"
#undef EFST_ACTION
    assert(allocations == before);
    skill_action_mgr_destroy_instance();
    assert(!live_blocks);
}

static int dispatch_tag, integer_seen;
static SkillActor *actor_seen;

static void observe_on(int status, int value, SkillActor *actor)
{
    assert(status == 7);
    dispatch_tag = 8;
    integer_seen = value;
    actor_seen = actor;
}
static void observe_off(int status, int value, SkillActor *actor)
{
    assert(status == 7);
    dispatch_tag = 9;
    integer_seen = value;
    actor_seen = actor;
}
static void observe_reapply(int status, int value, SkillActor *actor)
{
    assert(status == 7);
    dispatch_tag = 10;
    integer_seen = value;
    actor_seen = actor;
}

static const struct StatusAction dispatch_status = {
    .on = observe_on, .off = observe_off, .reapply = observe_reapply};

static void dispatch_and_defaults(void)
{
    struct SkillActionMgr *mgr = skill_action_mgr_instance();
    SkillActor *a = (SkillActor *)&dispatch_tag, *b = (SkillActor *)&integer_seen,
               *c = (SkillActor *)&actor_seen;
    unsigned before, pass;
    int id;

    skill_action_mgr_add_efst(mgr, 7, &dispatch_status);
    skill_action_mgr_add_efst(mgr, 8, &first_status);
    before = allocations;
    for (pass = 0; pass < 100; ++pass) {
        assert(skill_action_mgr_efst_on(mgr, 7, INT_MIN, a) && dispatch_tag == 8 &&
               integer_seen == INT_MIN && actor_seen == a);
        assert(skill_action_mgr_efst_off(mgr, 7, INT_MAX, b) && dispatch_tag == 9 &&
               integer_seen == INT_MAX && actor_seen == b);
        assert(skill_action_mgr_efst_reapply(mgr, 7, -1, c) && dispatch_tag == 10 &&
               integer_seen == -1 && actor_seen == c);
    }
    for (id = 8; id <= 9; ++id) {
        const int found = id == 8;
        dispatch_tag = 0;
        assert(skill_action_mgr_efst_on(mgr, id, 0, NULL) == found);
        assert(skill_action_mgr_efst_off(mgr, id, 0, NULL) == found);
        assert(skill_action_mgr_efst_reapply(mgr, id, 0, NULL) == found);
        assert(!dispatch_tag);
    }
    assert(allocations == before);
    skill_action_mgr_destroy_instance();
    assert(!live_blocks);
}

int main(void)
{
    registration_semantics();
    startup_bindings();
    dispatch_and_defaults();
    puts("Skill action registry: immutable status records, allocation-free dispatch, defaults, "
         "5 startup bindings and borrowed lifetime passed");
    return 0;
}
