#include "actor/object_ops.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "../src/actor/tetra_ball.c"

struct EffectMgr *g_effectMgr;
static int effect, present, ball_count, find_calls, spawn_calls;
static int added_slot, added_type, moved_slot, removed_slot, removed_effect;

void *object_as_type(void *object, enum ObjectType type)
{
    assert(type == OBJECT_TETRA_BALL_EFFECT);
    return object;
}

struct Effect *effect_mgr_find_effect(struct EffectMgr *mgr, int aid, int type)
{
    assert(mgr == g_effectMgr && aid == 123);
    assert(type >= 0x4ee && type <= 0x4f1);
    ++find_calls;
    return present ? (struct Effect *)&effect : NULL;
}

struct Effect *effect_mgr_spawn(struct EffectMgr *mgr, int aid, int type, Vec3 at,
                                  int target, int unused, unsigned char reuse, int key)
{
    assert(mgr == g_effectMgr && aid == 123);
    assert(at.x == 0 && at.y == 0 && at.z == 0);
    assert(target == 0 && unused == -1 && reuse == 0 && key == 0);
    ++spawn_calls;
    added_type = type;
    return (struct Effect *)&effect;
}

void tetra_ball_effect_add_ball(struct TetraBallEffect *object, int type, int slot)
{
    assert((void *)object == (void *)&effect);
    added_type = type;
    added_slot = slot;
}

int tetra_ball_effect_count(struct TetraBallEffect *object)
{
    assert((void *)object == (void *)&effect);
    return ball_count;
}

void tetra_ball_effect_move_first_to(struct TetraBallEffect *object, int slot)
{
    assert((void *)object == (void *)&effect);
    moved_slot = slot;
}

void tetra_ball_effect_remove_ball(struct TetraBallEffect *object, int slot)
{
    assert((void *)object == (void *)&effect);
    removed_slot = slot;
}

void effect_mgr_remove(struct EffectMgr *mgr, struct Effect *object, unsigned char unlink)
{
    assert(mgr == g_effectMgr && (void *)object == &effect && unlink == 1);
    ++removed_effect;
}

int main(void)
{
    struct GameActor actor, expected;
    const int kinds[] = { 0x4ee, 0x4f0, 0x4ef, 0x4f1 };
    const int invalid[] = { -1, 5, INT_MIN, INT_MAX };
    int i, calls;

    memset(&actor, 0xa5, sizeof actor);
    actor.aid = 123;
    actor.cell_x = 321;
    actor.cell_y = 654;
    memset(actor.effect_slots, 0, sizeof actor.effect_slots);
    expected = actor;
    present = 1;
    for (i = 0; i < 5; ++i) {
        game_actor_set_tetra_ball(&actor, i, 1, i + 1);
        expected.effect_slots[i] = i + 1;
        assert(added_slot == i && added_type == kinds[(i + 1) & 3]);
        assert(memcmp(&actor, &expected, sizeof actor) == 0);
        calls = find_calls;
        game_actor_set_tetra_ball(&actor, i, 1, i + 1);
        game_actor_set_tetra_ball(&actor, i, 1, 0);
        assert(find_calls == calls);
    }
    calls = find_calls;
    for (i = 0; i < 4; ++i) {
        game_actor_set_tetra_ball(&actor, invalid[i], 1, 7);
        game_actor_set_tetra_ball(&actor, invalid[i], 0, 7);
    }
    assert(find_calls == calls && spawn_calls == 0);
    assert(memcmp(&actor, &expected, sizeof actor) == 0);
    ball_count = 2;
    game_actor_set_tetra_ball(&actor, 4, 0, 0);
    expected.effect_slots[4] = 0;
    assert(removed_slot == 4 && removed_effect == 0);
    assert(memcmp(&actor, &expected, sizeof actor) == 0);
    ball_count = 1;
    game_actor_set_tetra_ball(&actor, 3, 0, 0);
    expected.effect_slots[3] = 0;
    assert(removed_effect == 1);
    assert(memcmp(&actor, &expected, sizeof actor) == 0);
    present = 0;
    game_actor_set_tetra_ball(&actor, 4, 1, 3);
    expected.effect_slots[4] = 3;
    assert(spawn_calls == 1 && moved_slot == 4 && added_type == 0x4f1);
    assert(memcmp(&actor, &expected, sizeof actor) == 0);
    puts("actor effects: five independent slots, invalid indices, creation/removal and preserved cell coordinates passed");
    return 0;
}
