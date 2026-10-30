#include "std/std.h"
#include "game/gamemode.h"

void *container_peer_value(struct ListNode *node)
{
    return *LIST_VALUE(node, void *);
}

struct TreeNode *container_peer_next(struct TreeNode *node)
{
    return tree_next(node);
}
