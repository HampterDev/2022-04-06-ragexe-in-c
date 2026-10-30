#include "game/context.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "actor/liveness.h"
#include "std/list.h"
#include "actor/clone_owner.h"

DWORD clone_owner_stale;

int actor_is_live(const void *actor)
{
    const struct List *actors;
    const struct ListNode *node;

    if (!actor)
        return 0;
    if (actor == game_current_player())
        return 1;
    actors = game_current_actor_list();
    if (!actors || !actors->head)
        return 0;
    for (node = actors->head->next; node != actors->head; node = node->next)
        if (*LIST_VALUE(node, void *) == actor)
            return 1;
    return 0;
}
