#include "actor/object_ops.h"
#include "world/world.h"
#include "std/list.h"

/* The world's list of objects as the rest of the client changes it, apart from world/objects.c,
   whose processing of the objects reaches the actors, the effects and the windows. custom: each
   object keeps its node on the list (struct GameObject, world_node), so that it leaves the list
   without the original's search of it (0x65e620). */

int world_add_object(struct World *self, void *object)
{
    struct GameObject *obj = object;
    void *value;

    if (!self->gameObjects.head)
        return 0;
    value = list_push_back(&self->gameObjects, &obj, sizeof obj);
    if (obj)
        obj->world_node = LIST_NODE_OF(value);
    return 1;
}

/* The node is cleared before the object is destroyed, so that a destructor that removes the object
   again finds it off the list. */
void world_remove_object(struct World *self, void *object)
{
    struct GameObject *obj = object;
    struct ListNode *node;

    if (!obj || !(node = obj->world_node))
        return;
    obj->world_node = NULL;
    game_object_destroy(obj);
    list_erase(&self->gameObjects, node, NULL);
}
