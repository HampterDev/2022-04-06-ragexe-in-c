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

static const struct SkillAction first_skill = {.cast_action = 8, .use_action = 7, .area = 42};
static const struct SkillAction second_skill = {.cast_action = 8, .use_action = 7, .area = 99};
static void second_status_on(int status, int value, SkillActor *actor)
{
    observed_id = 99;
}
static void second_object_appear(int level, SkillActor *unit, SkillActor *owner)
{
    observed_id = 99;
}
static const struct StatusAction first_status = {0};
static const struct StatusAction second_status = {.on = second_status_on};
static const struct SkillObjectAction first_object = {0};
static const struct SkillObjectAction second_object = {.appear = second_object_appear};

#define EFST_ACTION(id, cls)                                                                       \
    static void cls##_observe(int status, int value, SkillActor *actor)                            \
    {                                                                                              \
        observed_id = id;                                                                          \
    }                                                                                              \
    const struct StatusAction cls##_ops = {.on = cls##_observe};
#define SKILL_ACTION(id, cls)                                                                      \
    const struct SkillAction cls##_ops = {.cast_action = 8, .use_action = 7, .area = id};
#define SKILL_OBJECT_ACTION(id, cls)                                                               \
    static void cls##_observe(int level, SkillActor *unit, SkillActor *owner)                      \
    {                                                                                              \
        observed_id = id;                                                                          \
    }                                                                                              \
    const struct SkillObjectAction cls##_ops = {.appear = cls##_observe};
#include "skills/skill_actions.def"
#undef EFST_ACTION
#undef SKILL_ACTION
#undef SKILL_OBJECT_ACTION

static void registration_semantics(void)
{
    static const int ids[] = {INT_MIN, -1, 0, 1, INT_MAX};
    struct SkillActionMgr *mgr;
    const struct SkillAction *retained;
    unsigned i, pass, before;

    fail_next = 1;
    assert(!skill_action_mgr_instance() && !live_blocks);
    fail_next = 1;
    before = allocations;
    skill_action_mgr_destroy_instance();
    assert(!live_blocks && allocations == before && fail_next);
    fail_next = 0;
    mgr = skill_action_mgr_instance();
    assert(mgr && live_blocks == 4);
    before = allocations;
    assert(skill_action_mgr_instance() == mgr && allocations == before);
    assert(!skill_action_mgr_find_skill(mgr, 0));
    assert(!skill_action_mgr_find_efst(mgr, 0));
    assert(!skill_action_mgr_find_object(mgr, 0));
    assert(!skill_action_mgr_has_efst_action(mgr, 0));

    for (i = 0; i < sizeof ids / sizeof ids[0]; ++i) {
        before = allocations;
        skill_action_mgr_add_skill(mgr, ids[i], &first_skill);
        skill_action_mgr_add_efst(mgr, ids[i], &first_status);
        skill_action_mgr_add_object_action(mgr, ids[i], &first_object);
        assert(allocations == before + 3);
        skill_action_mgr_add_skill(mgr, ids[i], &second_skill);
        skill_action_mgr_add_efst(mgr, ids[i], &second_status);
        skill_action_mgr_add_object_action(mgr, ids[i], &second_object);
        assert(allocations == before + 3);
    }
    before = allocations;
    for (pass = 0; pass < 1000; ++pass) {
        for (i = 0; i < sizeof ids / sizeof ids[0]; ++i) {
            assert(skill_action_mgr_find_skill(mgr, ids[i]) == &first_skill);
            assert(skill_action_mgr_has_efst_action(mgr, ids[i]));
            assert(skill_action_mgr_find_efst(mgr, ids[i]) == &first_status);
            assert(skill_action_mgr_find_object(mgr, ids[i]) == &first_object);
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
    retained = skill_action_mgr_find_skill(mgr, 0);
    skill_action_mgr_destroy_instance();
    assert(!live_blocks && retained == &first_skill && retained->area == 42);
    before = allocations;
    skill_action_mgr_destroy_instance();
    assert(!live_blocks && allocations == before);
}

static void startup_bindings(void)
{
    struct SkillActionMgr *mgr = skill_action_mgr_instance();
    unsigned before = allocations, registrations = 0;
#define EFST_ACTION(id, cls) ++registrations;
#define SKILL_ACTION(id, cls) ++registrations;
#define SKILL_OBJECT_ACTION(id, cls) ++registrations;
#include "skills/skill_actions.def"
#undef EFST_ACTION
#undef SKILL_ACTION
#undef SKILL_OBJECT_ACTION
    skill_actions_register();
    assert(registrations == 126 && allocations == before + registrations);
    before = allocations;
    skill_actions_register();
#define EFST_ACTION(id, cls)                                                                       \
    assert(skill_action_mgr_find_efst(mgr, id) == &cls##_ops);                                     \
    observed_id = 0;                                                                               \
    skill_action_mgr_find_efst(mgr, id)->on(id, 9, NULL);                                          \
    assert(observed_id == id);
#define SKILL_ACTION(id, cls)                                                                      \
    assert(skill_action_mgr_find_skill(mgr, id) == &cls##_ops && cls##_ops.area == id);
#define SKILL_OBJECT_ACTION(id, cls)                                                               \
    assert(skill_action_mgr_find_object(mgr, id) == &cls##_ops);                                   \
    observed_id = 0;                                                                               \
    skill_action_mgr_find_object(mgr, id)->appear(9, NULL, NULL);                                  \
    assert(observed_id == id);
#include "skills/skill_actions.def"
#undef EFST_ACTION
#undef SKILL_ACTION
#undef SKILL_OBJECT_ACTION
    assert(allocations == before);
    skill_action_mgr_destroy_instance();
    assert(!live_blocks);
}

static int world_present = 1, attr_present = 1, world_token, attr_token;
static int cell_x_seen, cell_y_seen, position_calls, dispatch_tag, integer_seen;
static unsigned time_seen;
static Vec3 position_seen;
static SkillActor *actor_seen[3];

struct World *game_current_world(void)
{
    return world_present ? (struct World *)&world_token : NULL;
}
struct C3dAttr *game_current_map_attr(void)
{
    return attr_present ? (struct C3dAttr *)&attr_token : NULL;
}
void world_cell_to_world(struct World *world, int x, int y, float *out_x, float *out_z)
{
    assert(world == (struct World *)&world_token);
    cell_x_seen = x;
    cell_y_seen = y;
    ++position_calls;
    *out_x = -7.5f;
    *out_z = 22.5f;
}
float c3d_attr_get_height(struct C3dAttr *attr, float x, float z)
{
    assert(attr == (struct C3dAttr *)&attr_token && x == -7.5f && z == 22.5f);
    ++position_calls;
    return -1.25f;
}

static void observe_cast(unsigned time, SkillActor *caster)
{
    dispatch_tag = 1;
    time_seen = time;
    actor_seen[0] = caster;
}
static void observe_target(SkillActor *target)
{
    dispatch_tag = 2;
    actor_seen[0] = target;
}
static void observe_cell(unsigned time, Vec3 position)
{
    dispatch_tag = 3;
    time_seen = time;
    position_seen = position;
}
static void observe_use(int level, SkillActor *caster, SkillActor *target)
{
    dispatch_tag = 4;
    integer_seen = level;
    actor_seen[0] = caster;
    actor_seen[1] = target;
}
static void observe_ground(Vec3 position, int level, SkillActor *caster)
{
    dispatch_tag = 5;
    position_seen = position;
    integer_seen = level;
    actor_seen[0] = caster;
}
static void observe_hit(SkillActor *caster, SkillActor *target, SkillActor *unit)
{
    dispatch_tag = 6;
    actor_seen[0] = caster;
    actor_seen[1] = target;
    actor_seen[2] = unit;
}
static void observe_damage(SkillActor *source, SkillActor *target)
{
    dispatch_tag = 7;
    actor_seen[0] = source;
    actor_seen[1] = target;
}
static void observe_on(int status, int value, SkillActor *actor)
{
    assert(status == 7);
    dispatch_tag = 8;
    integer_seen = value;
    actor_seen[0] = actor;
}
static void observe_off(int status, int value, SkillActor *actor)
{
    assert(status == 7);
    dispatch_tag = 9;
    integer_seen = value;
    actor_seen[0] = actor;
}
static void observe_reapply(int status, int value, SkillActor *actor)
{
    assert(status == 7);
    dispatch_tag = 10;
    integer_seen = value;
    actor_seen[0] = actor;
}
static void observe_appear(int level, SkillActor *unit, SkillActor *owner)
{
    dispatch_tag = 11;
    integer_seen = level;
    actor_seen[0] = unit;
    actor_seen[1] = owner;
}
static void observe_leave(SkillActor *unit)
{
    dispatch_tag = 12;
    actor_seen[0] = unit;
}

static const struct SkillAction dispatch_skill = {.is_good_for_actor = 2,
                                                  .cast_action = 0,
                                                  .use_action = -1,
                                                  .skips_use_motion = 3,
                                                  .skips_attack_delay = 4,
                                                  .holds_ground_pose = 5,
                                                  .area = -2,
                                                  .cast = observe_cast,
                                                  .cast_on_target = observe_target,
                                                  .cast_at_cell = observe_cell,
                                                  .use = observe_use,
                                                  .use_on_ground = observe_ground,
                                                  .hit = observe_hit,
                                                  .damage_hit = observe_damage};
static const struct SkillAction no_op_skill = {.cast_action = 8, .use_action = 7};
static const struct StatusAction dispatch_status = {
    .on = observe_on, .off = observe_off, .reapply = observe_reapply};
static const struct SkillObjectAction dispatch_object = {.appear = observe_appear,
                                                         .leave = observe_leave};

static void dispatch_and_defaults(void)
{
    struct SkillActionMgr *mgr = skill_action_mgr_instance();
    struct SkillActionAnswer answer;
    int actor_tokens[3];
    SkillActor *a = (SkillActor *)&actor_tokens[0];
    SkillActor *b = (SkillActor *)&actor_tokens[1];
    SkillActor *c = (SkillActor *)&actor_tokens[2];
    unsigned before, repeat;
    int id;
    skill_action_mgr_add_skill(mgr, 7, &dispatch_skill);
    skill_action_mgr_add_skill(mgr, 8, &no_op_skill);
    skill_action_mgr_add_efst(mgr, 7, &dispatch_status);
    skill_action_mgr_add_efst(mgr, 8, &first_status);
    skill_action_mgr_add_object_action(mgr, 7, &dispatch_object);
    skill_action_mgr_add_object_action(mgr, 8, &first_object);
    before = allocations;
    for (repeat = 0; repeat < 1000; ++repeat) {
        assert(skill_action_mgr_is_good_for_actor(mgr, 7) == 2);
        assert(skill_action_mgr_cast_action(mgr, &answer, 7) == &answer && answer.found &&
               answer.value == 0);
        assert(skill_action_mgr_use_action(mgr, &answer, 7) == &answer && answer.found &&
               answer.value == -1);
        assert(skill_action_mgr_skips_use_motion(mgr, 7) == 3);
        assert(skill_action_mgr_skips_attack_delay(mgr, 7) == 4);
        assert(skill_action_mgr_holds_ground_pose(mgr, 7) == 5);
        assert(skill_action_mgr_get_skill_area(mgr, 7) == -2);
        assert(skill_action_mgr_cast(mgr, 7, UINT_MAX, a) && dispatch_tag == 1 &&
               time_seen == UINT_MAX && actor_seen[0] == a);
        assert(skill_action_mgr_cast_on_target(mgr, 7, b) && dispatch_tag == 2 && actor_seen[0] == b);
        position_calls = 0;
        assert(skill_action_mgr_cast_at_cell(mgr, 7, UINT_MAX, SHRT_MIN, SHRT_MAX));
        assert(dispatch_tag == 3 && time_seen == UINT_MAX && cell_x_seen == SHRT_MIN &&
               cell_y_seen == SHRT_MAX);
        assert(position_calls == 2 && position_seen.x == -5.0f && position_seen.y == -1.25f &&
               position_seen.z == 25.0f);
        assert(skill_action_mgr_use(mgr, 7, INT_MIN, a, b) && dispatch_tag == 4 &&
               integer_seen == INT_MIN && actor_seen[0] == a && actor_seen[1] == b);
        position_calls = 0;
        assert(skill_action_mgr_use_on_ground(mgr, 7, INT_MIN, INT_MAX, -9, c));
        assert(dispatch_tag == 5 && integer_seen == -9 && actor_seen[0] == c &&
               cell_x_seen == INT_MIN && cell_y_seen == INT_MAX);
        assert(position_calls == 2 && position_seen.x == -5.0f && position_seen.y == -1.25f &&
               position_seen.z == 25.0f);
        assert(skill_action_mgr_hit(mgr, 7, a, b, c) && dispatch_tag == 6 && actor_seen[0] == a &&
               actor_seen[1] == b && actor_seen[2] == c);
        assert(skill_action_mgr_damage_hit(mgr, 7, c, a) && dispatch_tag == 7 && actor_seen[0] == c &&
               actor_seen[1] == a);
        assert(skill_action_mgr_efst_on(mgr, 7, INT_MIN, a) && dispatch_tag == 8 &&
               integer_seen == INT_MIN && actor_seen[0] == a);
        assert(skill_action_mgr_efst_off(mgr, 7, INT_MAX, b) && dispatch_tag == 9 &&
               integer_seen == INT_MAX && actor_seen[0] == b);
        assert(skill_action_mgr_efst_reapply(mgr, 7, -1, c) && dispatch_tag == 10 &&
               integer_seen == -1 && actor_seen[0] == c);
        assert(skill_action_mgr_on_object_appear(mgr, 7, INT_MIN, c, a) && dispatch_tag == 11 &&
               integer_seen == INT_MIN && actor_seen[0] == c && actor_seen[1] == a);
        assert(skill_action_mgr_on_object_leave(mgr, 7, b) && dispatch_tag == 12 &&
               actor_seen[0] == b);
    }
    for (id = 8; id <= 9; ++id) {
        const int found = id == 8;
        dispatch_tag = 0;
        position_calls = 0;
        assert(!skill_action_mgr_is_good_for_actor(mgr, id));
        skill_action_mgr_cast_action(mgr, &answer, id);
        assert(answer.found == found && answer.value == 8);
        skill_action_mgr_use_action(mgr, &answer, id);
        assert(answer.found == found && answer.value == 7);
        assert(!skill_action_mgr_skips_use_motion(mgr, id) &&
               !skill_action_mgr_skips_attack_delay(mgr, id));
        assert(!skill_action_mgr_holds_ground_pose(mgr, id) &&
               !skill_action_mgr_get_skill_area(mgr, id));
        assert(skill_action_mgr_cast(mgr, id, 0, NULL) == found);
        assert(skill_action_mgr_cast_on_target(mgr, id, NULL) == found);
        assert(skill_action_mgr_cast_at_cell(mgr, id, 0, 0, 0) == found);
        assert(skill_action_mgr_use(mgr, id, 0, NULL, NULL) == found);
        assert(skill_action_mgr_use_on_ground(mgr, id, 0, 0, 0, NULL) == found);
        assert(skill_action_mgr_hit(mgr, id, NULL, NULL, NULL) == found);
        assert(skill_action_mgr_damage_hit(mgr, id, NULL, NULL) == found);
        assert(skill_action_mgr_efst_on(mgr, id, 0, NULL) == found);
        assert(skill_action_mgr_efst_off(mgr, id, 0, NULL) == found);
        assert(skill_action_mgr_efst_reapply(mgr, id, 0, NULL) == found);
        assert(skill_action_mgr_on_object_appear(mgr, id, 0, NULL, NULL) == found);
        assert(skill_action_mgr_on_object_leave(mgr, id, NULL) == found);
        assert(!dispatch_tag && position_calls == (found ? 4 : 0));
    }
    world_present = 0;
    position_calls = 0;
    assert(skill_action_mgr_cast_at_cell(mgr, 7, 0, 1, 2));
    assert(!position_calls && position_seen.x == 0 && position_seen.y == 0 && position_seen.z == 0);
    world_present = 1;
    attr_present = 0;
    assert(skill_action_mgr_use_on_ground(mgr, 7, 1, 2, 3, NULL));
    assert(!position_calls && position_seen.x == 0 && position_seen.y == 0 && position_seen.z == 0);
    attr_present = 1;
    assert(allocations == before);
    skill_action_mgr_destroy_instance();
    assert(!live_blocks);
}

int main(void)
{
    registration_semantics();
    startup_bindings();
    dispatch_and_defaults();
    puts("Skill action registry: immutable records, allocation-free dispatch, defaults, cell "
         "coordinates, 126 startup bindings and borrowed lifetime passed");
    return 0;
}
