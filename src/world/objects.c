#include "actors/object_ops.h"
#include "world/world.h"
#include "world/world_imports.h"
#include "actors/game_actor.h"
#include "ui/wndmgr/wndmgr.h"
#include "std/list.h"

/* custom: an actor that ends is found on the world's actors by its node there (actor_node), where
   the original searched the list for it; the objects and the units that left are gone through in
   the original's order */
void world_process_actors(struct World *self)
{
    struct ListNode *n, *next, *a;
    struct GameObject *obj;

    world_models_process(self);

    for (n = self->gameObjects.head->next; n != self->gameObjects.head; n = next) {
        obj = *LIST_VALUE(n, struct GameObject *);
        if (game_object_process(obj)) {
            next = n->next;
            continue;
        }
        a = obj->actor_node;
        if (a) {
            struct GameActor *actor = (struct GameActor *)obj;

            world_forget_id_in_cell_map(self, actor->aid, 1);
            world_forget_id_in_list_ec(self, actor->aid, 1);
            obj->actor_node = NULL;
            world_erase_actor(self, a);
        }
        obj->world_node = NULL;
        game_object_destroy(obj);
        next = list_erase(&self->gameObjects, n, NULL);
    }

    world_process_leaving_skills(self);
}

char world_remove_actor_by_aid(struct World *self, unsigned aid)
{
    struct GameActor *actor = world_get_game_actor_by_aid(self, aid);
    struct ListNode *node;

    if (!actor)
        return 0;
    node = actor->actor_node;
    world_forget_id_in_cell_map(self, (int)aid, 1);
    world_forget_id_in_list_ec(self, actor->aid, 1);
    /* alive: on the actors, which an actor leaves when it is destroyed (above, here and with the
       map); still on them while its destructor runs, as in the original */
    world_remove_object(self, actor);
    world_erase_actor(self, node);
    return 1;
}

void world_clear_objects(struct World *self)
{
    struct ListNode *n;
    void *obj;

    for (n = self->gameObjects.head->next; n != self->gameObjects.head;) {
        obj = *LIST_VALUE(n, void *);
        if (obj) {
            ((struct GameObject *)obj)->world_node = NULL;
            game_object_destroy((struct GameObject *)obj);
        }
        n = list_erase(&self->gameObjects, n, NULL);
    }
    self->player = NULL;
    list_clear(&self->items, NULL);
    world_clear_aid_lists(self);
    effect_mgr_reset(g_effectMgr);
    tree_clear(&self->idsByCell, cell_ids_destroy);
    list_clear(&self->idsNowhere, NULL);
}
