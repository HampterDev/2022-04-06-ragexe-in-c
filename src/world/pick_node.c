#include "world/pick_node.h"
#include "world/pathfinder_state.h"
#include "world/path_info.h"
#include "world/scene_graph.h"
#include "core/numeric.h"
#include "std/std.h"

struct ActorPickNode g_actorPickNode;

static void pick_node_init(struct ActorPickNode *self)
{
    list_init(&self->boxes);
    self->child[0] = self->child[1] = self->child[2] = self->child[3] = NULL;
}

struct ActorPickNode *actor_pick_node_construct(struct ActorPickNode *self)
{
    pick_node_init(self);
    return self;
}

void actor_pick_node_destruct(struct ActorPickNode *self)
{
    actor_pick_node_clear(self);
    list_free(&self->boxes, NULL);
}

void actor_pick_node_clear(struct ActorPickNode *self)
{
    struct ListNode *n;
    void *box;
    int i;

    for (n = self->boxes.head->next; n != self->boxes.head; n = n->next) {
        box = *LIST_VALUE(n, void *);
        if (box)
            mem_free(box);
    }
    list_clear(&self->boxes, NULL);
    for (i = 0; i < 4; i++)
        if (self->child[i]) {
            actor_pick_node_destruct(self->child[i]);
            mem_free(self->child[i]);
            self->child[i] = NULL;
        }
}

static int actor_pick_node_quadrant(struct ActorPickNode *self, int x, int y)
{
    int cx = self->rect[2] / 2, cy = self->rect[3] / 2;

    if (x > cx)
        return (y > cy) + 1;
    if (y > cy)
        return 3;
    return 0;
}

static void child_rect(int *r, int q)
{
    switch (q) {
    case 0:
        r[2] = r[2] / 2;
        r[3] = r[3] / 2;
        break;
    case 1:
        r[0] = r[2] / 2 + 1;
        r[3] = r[3] / 2;
        break;
    case 2:
        r[0] = r[2] / 2 + 1;
        r[1] = r[3] / 2 + 1;
        break;
    case 3:
        r[2] = r[2] / 2;
        r[1] = r[3] / 2 + 1;
        break;
    }
}

void actor_pick_node_build(struct ActorPickNode *self, int depth, int left, int top,
                                     int right, int bottom)
{
    struct ActorPickNode *child;
    int r[4], i;

    actor_pick_node_clear(self);
    self->rect[0] = left;
    self->rect[1] = top;
    self->rect[2] = right;
    self->rect[3] = bottom;
    if (depth == 0)
        return;
    for (i = 0; i < 4; i++) {
        child = (struct ActorPickNode *)mem_alloc(sizeof *child);
        if (child)
            pick_node_init(child);
        self->child[i] = child;
        memcpy(r, self->rect, sizeof r);
        child_rect(r, i);
        actor_pick_node_build(child, depth - 1, r[0], r[1], r[2], r[3]);
    }
}

void actor_pick_node_insert(struct ActorPickNode *self, struct PickInfo *box)
{
    int x, y;

    if (self->child[0]) {
        y = (int)box->top;
        x = (int)box->left;
        do
            self = self->child[actor_pick_node_quadrant(self, x, y)];
        while (self->child[0]);
    }
    list_push_back(&self->boxes, &box, sizeof box);
}

struct PickInfo *actor_pick_node_find_pick_info(struct ActorPickNode *self, float x,
                                                        float y)
{
    struct ListNode *n;
    struct PickInfo *box, *best;
    float depth, bestDepth;
    int q, i;

    if (!self->child[0]) {
        best = NULL;
        bestDepth = 0.0f;
        for (n = self->boxes.head->next; n != self->boxes.head; n = n->next) {
            box = *LIST_VALUE(n, struct PickInfo *);
            if (x > box->left && box->right > x && y > box->top && box->bottom > y) {
                depth = box->far_z + box->near_z;
                if (depth > bestDepth) {
                    bestDepth = depth;
                    best = box;
                }
            }
        }
        return best;
    }
    q = actor_pick_node_quadrant(self, (int)x, (int)y);
    best = actor_pick_node_find_pick_info(self->child[q], x, y);
    if (best)
        return best;
    for (i = 0; i < 4; i++)
        if (i != q) {
            best = actor_pick_node_find_pick_info(self->child[i], x, y);
            if (best)
                return best;
        }
    return NULL;
}
