#include "world/pick_node.h"
#include "std/std.h"
#include "world_state_memory.h"
#include <stdio.h>

static struct PickInfo *box(float left, float top, float right, float bottom, float depth, int aid, int kind)
{
    struct PickInfo *p = mem_alloc(sizeof *p);
    *p = (struct PickInfo){left, top, depth, right, bottom, depth, aid, 0x7d01, kind, 1};
    return p;
}

int main(void)
{
    struct PickInfo *item, *actor, *skill, *crossing;
    unsigned round;
    actor_pick_node_construct(&g_actorPickNode);
    assert(live_blocks == 1 && g_actorPickNode.boxes.size == 0);
    for (round = 0; round < 8; ++round) {
        actor_pick_node_build(&g_actorPickNode, 2, 0, 0, 200, 200);
        item = box(10, 10, 30, 30, 10, 10, 1);
        actor = box(10, 10, 30, 30, 20, 20, 3);
        skill = box(120, 20, 150, 40, 30, 30, 2);
        crossing = box(90, 140, 130, 180, 40, 40, 4);
        actor_pick_node_insert(&g_actorPickNode, item);
        actor_pick_node_insert(&g_actorPickNode, actor);
        actor_pick_node_insert(&g_actorPickNode, skill);
        actor_pick_node_insert(&g_actorPickNode, crossing);
        assert(actor_pick_node_find_pick_info(&g_actorPickNode, 20, 20) == actor);
        assert(actor->aid == 20 && actor->job == 0x7d01 && actor->attackable_as_pvp == 1);
        assert(actor_pick_node_find_pick_info(&g_actorPickNode, 10, 20) == NULL);
        assert(actor_pick_node_find_pick_info(&g_actorPickNode, 130, 30) == skill);
        assert(actor_pick_node_find_pick_info(&g_actorPickNode, 120, 160) == crossing);
        assert(actor_pick_node_find_pick_info(&g_actorPickNode, 180, 180) == NULL);
        actor_pick_node_clear(&g_actorPickNode);
        assert(live_blocks == 1 && g_actorPickNode.boxes.size == 0);
        assert(actor_pick_node_find_pick_info(&g_actorPickNode, 20, 20) == NULL);
    }
    actor_pick_node_destruct(&g_actorPickNode);
    assert(live_blocks == 0);
    puts("pick state: shared records, depth, boundaries, quarter traversal and owned teardown passed");
    return 0;
}
