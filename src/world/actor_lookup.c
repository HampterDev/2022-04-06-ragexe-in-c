#include "actor/actor_ops.h"
#include "actor/game_actor.h"
#include "world/actor_lookup.h"
#include "world/world.h"
#include "std/list.h"

/* The player character with a character ID in the world. Apart from world/world.c, whose
   other functions reach the actors, the effects and the windows, so that the guild code
   asking for a member's actor does not. */

void *world_find_pc_by_char_id(struct World *self, int value)
{
    struct ListNode *head = self->actors.head, *n;

    for (n = head->next; n != head; n = n->next) {
        void *actor = *LIST_VALUE(n, void *);
        if (((struct GameActor *)actor)->object_type == 0
            && actor_get_char_id(actor) == value)
            break;
    }
    return n != self->actors.head ? *LIST_VALUE(n, void *) : NULL;
}
