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

static void world_destruct(struct World *self)
{
    world_models_wait(self);
    scene_graph_node_destruct(&self->root);
    world_models_clear(self);
    if (self->ground)
        ground_free(self->ground);
    list_free(&self->idsNowhere, NULL);
    tree_free(&self->idsByCell, cell_ids_destroy);
    list_free(&self->pendingSkills, NULL);
    list_free(&self->skills, NULL);
    list_free(&self->items, NULL);
    actor_index_clear();
    list_free(&self->actors, NULL);
    list_free(&self->gameObjects, NULL);
}

void world_delete(struct World *self)
{
    world_destruct(self);
    mem_free(self);
}

void *world_get_actor_or_player(struct World *self, int aid)
{
    struct ListNode *head, *n;

    if (aid == g_session1.ownAid)
        return self->player;
    head = self->actors.head;
    for (n = head->next; n != head; n = n->next)
        if ((*LIST_VALUE(n, struct GameActor *))->aid == aid)
            return *LIST_VALUE(n, void *);
    return NULL;
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

struct Skill *world_get_skill_by_aid(struct World *self, int aid)
{
    struct ListNode *head, *node;
    if (!self || !(head = self->skills.head)) return NULL;
    for (node = head->next; node != head; node = node->next) {
        struct Skill *object = *LIST_VALUE(node, struct Skill *);
        if (object && object->unit_aid == aid) return object;
    }
    return NULL;
}

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

BYTE world_remove_item(struct World *self, int aid)
{
    struct ListNode *head = self->items.head, *n, *o;
    void *item;

    for (n = head->next; n != head; n = n->next)
        if (*LIST_VALUE(n, struct Item *) && (*LIST_VALUE(n, struct Item *))->aid == aid)
            break;
    if (n == head)
        return 0;
    item = *LIST_VALUE(n, void *);
    head = self->gameObjects.head;
    for (o = head->next; o != head; o = o->next)
        if (*LIST_VALUE(o, void *) == item)
            break;
    if (o != head) {
        if (item)
            game_object_destroy((struct GameObject *)item);
        list_erase(&self->gameObjects, o, NULL);
    }
    list_erase(&self->items, n, NULL);
    return 1;
}

BYTE world_remove_skill(struct World *self, int aid)
{
    struct ListNode *head = self->skills.head, *n, *p;
    void *skill;

    for (n = head->next; n != head; n = n->next)
        if (*LIST_VALUE(n, struct Skill *) && (*LIST_VALUE(n, struct Skill *))->unit_aid == aid)
            break;
    if (n == head)
        return 0;
    skill = *LIST_VALUE(n, void *);
    if (!skill)
        return 0;
    render_object_send(skill, NULL, 0x35, 0, 0, 0, 0);
    if (game_actor_has_pending_hits(skill) == 1) {
        world_remove_object(self, skill);
        list_erase(&self->skills, n, NULL);
        return 1;
    }
    head = self->pendingSkills.head;
    for (p = head->next; p != head; p = p->next)
        if (*LIST_VALUE(p, int) == aid)
            break;
    if (p == head)
        list_push_back(&self->pendingSkills, &aid, sizeof aid);
    return 0;
}

void world_send_msg_to_object(struct World *self, void *sender, int key1, int key2,
                                       int a, int b, int c, int d, int e)
{
    struct ListNode *head, *n;
    unsigned __int64 serial;
    void *obj;

    if (!self)
        return;
    if (key1 == 0 && key2 == 0)
        return;
    serial = (unsigned __int64)(unsigned)key2 << 32 | (unsigned)key1;
    head = self->gameObjects.head;
    for (n = head->next; n != head; n = n->next)
        if ((*LIST_VALUE(n, struct GameObject *))->serial == serial)
            break;
    if (n == head)
        return;
    obj = *LIST_VALUE(n, void *);
    render_object_send(obj, sender, a, b, c, d, e);
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
