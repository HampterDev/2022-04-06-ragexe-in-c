#ifndef WORLD_PICK_NODE_H
#define WORLD_PICK_NODE_H

#include "base.h"
#include "std/container_types.h"

struct PickInfo {
    float left, top, near_z;
    float right, bottom, far_z;
    int aid;
    int job;
    int kind;
    int attackable_as_pvp;
};
SIZE_ASSERT(PickInfo, 0x28);

struct ActorPickNode {
    int rect[4];
    struct ActorPickNode *child[4];
    struct List boxes;
};
SIZE_ASSERT(ActorPickNode, 0x28);

extern struct ActorPickNode g_actorPickNode;
struct ActorPickNode *actor_pick_node_construct(struct ActorPickNode *self);
void actor_pick_node_destruct(struct ActorPickNode *self);
void actor_pick_node_clear(struct ActorPickNode *self);
void actor_pick_node_build(struct ActorPickNode *self, int depth, int left, int top, int right, int bottom);
void actor_pick_node_insert(struct ActorPickNode *self, struct PickInfo *box);
struct PickInfo *actor_pick_node_find_pick_info(struct ActorPickNode *self, float x, float y);

#endif
