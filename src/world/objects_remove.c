#include "actor/object_ops.h"
#include "world/world.h"
#include "std/list.h"

/* Apart from world/objects.c, whose processing of the objects reaches the actors, the
   effects and the windows: the view context and the world remove objects through this. */

static struct ListNode *find_value(struct List *l, const void *value)
{
    struct ListNode *head = l->head, *n;

    for (n = head->next; n != head; n = n->next)
        if (*LIST_VALUE(n, void *) == value)
            break;
    return n;
}

void world_remove_object(struct World *self, void *object)
{
    struct ListNode *n = find_value(&self->gameObjects, object);

    if (n == self->gameObjects.head)
        return;
    if (object)
        game_object_destroy((struct GameObject *)object);
    list_erase(&self->gameObjects, n, NULL);
}
