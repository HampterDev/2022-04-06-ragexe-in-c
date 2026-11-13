#include "world/pick_node.h"
#include "std/std.h"
#include "world_state_memory.h"
#include <stdio.h>

static struct PickInfo box(float left, float top, float right, float bottom, float depth, int aid, int kind)
{
    struct PickInfo p = {left, top, depth, right, bottom, depth, aid, 0x7d01, kind, 1};
    return p;
}

static void found(float x, float y, int aid, int kind)
{
    const struct PickInfo *p = actor_pick_node_find_pick_info(&g_actorPickNode, x, y);
    assert(p && p->aid == aid && p->kind == kind && p->job == 0x7d01 && p->attackable_as_pvp == 1);
}

int main(void)
{
    struct PickInfo item = box(10, 10, 30, 30, 10, 10, 1), actor = box(10, 10, 30, 30, 20, 20, 3);
    struct PickInfo skill = box(120, 20, 150, 40, 30, 30, 2), crossing = box(90, 140, 130, 180, 40, 40, 4);
    unsigned round, i, blocks = 0;
    actor_pick_node_construct(&g_actorPickNode);
    assert(live_blocks == 0 && g_actorPickNode.box_count == 0);
    for (round = 0; round < 8; ++round) {
        actor_pick_node_build(&g_actorPickNode, 2, 0, 0, 200, 200);
        actor_pick_node_insert(&g_actorPickNode, &item);
        actor_pick_node_insert(&g_actorPickNode, &actor);
        actor_pick_node_insert(&g_actorPickNode, &skill);
        actor_pick_node_insert(&g_actorPickNode, &crossing);
        found(20, 20, 20, 3);
        assert(actor_pick_node_find_pick_info(&g_actorPickNode, 10, 20) == NULL);
        found(130, 30, 30, 2);
        found(120, 160, 40, 4);
        assert(actor_pick_node_find_pick_info(&g_actorPickNode, 180, 180) == NULL);
        actor_pick_node_clear(&g_actorPickNode);
        assert(live_blocks == 0 && g_actorPickNode.box_count == 0 && !g_actorPickNode.child[0]);
        assert(actor_pick_node_find_pick_info(&g_actorPickNode, 20, 20) == NULL);
    }
    /* Frames without quarters, as after the first clear: the boxes' storage outlives the clear, so
       a frame no larger than the ones before allocates nothing. */
    for (round = 0; round < 4; ++round) {
        for (i = 0; i < 300; ++i) {
            struct PickInfo b = box((float)(i % 20) * 10.0f, (float)(i / 20) * 10.0f, (float)(i % 20) * 10.0f + 9.0f,
                                    (float)(i / 20) * 10.0f + 9.0f, (float)i, (int)i, 0);
            actor_pick_node_insert(&g_actorPickNode, &b);
        }
        assert(g_actorPickNode.box_count == 300 && g_actorPickNode.box_capacity >= 300);
        if (round == 0)
            blocks = live_blocks;
        assert(live_blocks == blocks && blocks == 1);
        found(55, 25, 45, 0);
        assert(actor_pick_node_find_pick_info(&g_actorPickNode, 1000, 1000) == NULL);
        actor_pick_node_clear(&g_actorPickNode);
        assert(g_actorPickNode.box_count == 0 && live_blocks == 1);
    }
    actor_pick_node_destruct(&g_actorPickNode);
    assert(live_blocks == 0);
    puts("pick state: copied records, depth, boundaries, quarter traversal, storage kept across frames and owned "
         "teardown passed");
    return 0;
}
