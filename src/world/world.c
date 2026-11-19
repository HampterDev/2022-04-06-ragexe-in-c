#include "actor/actor_ops.h"
#include "actor/object_ops.h"
#include "world/actor_lookup.h"
#include "effects/skills/skill_state.h"
#include "game/ground_item_state.h"
#include "world/ground_api.h"
#include "session/state.h"
#include "game/context.h"
#include "world/world.h"
#include "world/mode.h"

#include <math.h>
#include <string.h>
#include "actor/types.h"
#include "actor/game_actor.h"
#include "ui/wndmgr/wndmgr.h"
#include "std/list.h"
#include <stdint.h>

static void world_destruct(struct World *self)
{
    world_models_wait(self);
    scene_graph_node_destruct(&self->root);
    world_models_clear(self);
    if (self->ground)
        ground_free(self->ground);
    list_free(&self->idsNowhere, NULL);
    tree_free(&self->idsByCell, cell_ids_destroy);
    list_free(&self->items, NULL);
    world_free_aid_lists(self);
    list_free(&self->gameObjects, NULL);
}

void world_delete(struct World *self)
{
    world_destruct(self);
    mem_free(self);
}

void *world_get_actor_or_player(struct World *self, int aid)
{
    if (aid == g_session1.ownAid)
        return self->player;
    return world_get_game_actor_by_aid(self, (unsigned)aid);
}

struct Item *world_get_item_by_aid(struct World *self, int aid)
{
    struct ListNode *head, *node;
    if (!self || !(head = self->items.head)) return NULL;
    for (node = head->next; node != head; node = node->next) {
        struct Item *object = *LIST_VALUE(node, struct Item *);
        if (object && object->aid == aid) return object;
    }
    return NULL;
}

BYTE world_remove_item(struct World *self, int aid)
{
    struct ListNode *head = self->items.head, *n;

    for (n = head->next; n != head; n = n->next)
        if (*LIST_VALUE(n, struct Item *) && (*LIST_VALUE(n, struct Item *))->aid == aid)
            break;
    if (n == head)
        return 0;
    /* alive: on the items, which an item leaves only here and with the map (its processing never
       ends it: item_on_process) */
    world_remove_object(self, *LIST_VALUE(n, void *));
    list_erase(&self->items, n, NULL);
    return 1;
}

static void ids_nowhere_remove(struct World *w, int id)
{
    struct ListNode *head = w->idsNowhere.head, *n;

    for (n = head->next; n != head; n = n->next)
        if (*LIST_VALUE(n, int) == id)
            break;
    if (n == head)
        return;
    list_erase(&w->idsNowhere, n, NULL);
}

static struct CellIds *cellmap_at(struct World *w, float cell)
{
    struct CellIds *entry = tree_find(&w->idsByCell, &cell, cell_ids_less);

    if (!entry) {
        struct CellIds made;

        made.cell = cell;
        list_init(&made.ids);
        entry = tree_add(&w->idsByCell, &made, sizeof made, cell_ids_less, NULL);
    }
    return entry;
}

void world_place_id_at_cell(struct World *self, int id, const float *cell)
{
    world_forget_id_in_cell_map(self, id, 0);
    ids_nowhere_remove(self, id);
    list_push_back(&cellmap_at(self, (float)self->attr->width * cell[1] + cell[0])->ids, &id, sizeof id);
    world_update_near_flag_for_id(self, id, cell);
}

void world_place_id_at_client_cell(struct World *self, int id, const int *cell)
{
    float xz[2];

    xz[0] = 0;
    xz[1] = 0;
    game_mode_cell_to_world(game_current_mode(), cell[0], cell[1], &xz[0], &xz[1]);
    world_forget_id_in_cell_map(self, id, 0);
    ids_nowhere_remove(self, id);
    list_push_back(&cellmap_at(self, (float)self->attr->width * xz[1] + xz[0])->ids, &id, sizeof id);
    world_update_near_flag_for_id(self, id, xz);
}

void world_place_id_nowhere(struct World *self, int id, const float *pos)
{
    world_forget_id_in_cell_map(self, id, 0);
    ids_nowhere_remove(self, id);
    list_push_back(&self->idsNowhere, &id, sizeof id);
    world_update_near_flag_for_id(self, id, pos);
}
