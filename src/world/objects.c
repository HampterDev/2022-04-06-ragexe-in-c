#include "actor/object_ops.h"
#include "world/world.h"
#include "world/world_imports.h"
#include "actor/game_actor.h"
#include "effects/skills/skill_state.h"
#include "ui/wndmgr/wndmgr.h"
#include "std/list.h"

static struct ListNode *find_value(struct List *l, const void *value)
{
    struct ListNode *head = l->head, *n;

    for (n = head->next; n != head; n = n->next)
        if (*LIST_VALUE(n, void *) == value)
            break;
    return n;
}

static void erase_actor(struct World *self, struct ListNode *n)
{
    actor_index_drop_node(n);
    list_erase(&self->actors, n, NULL);
}

void world_process_actors(struct World *self)
{
    struct ListNode *n, *next, *a, *p, *s, *o;
    void *obj, *skill;
    int id;

    world_models_process(self);

    for (n = self->gameObjects.head->next; n != self->gameObjects.head; n = next) {
        obj = *LIST_VALUE(n, void *);
        if (game_object_process((struct GameObject *)obj)) {
            next = n->next;
            continue;
        }
        a = find_value(&self->actors, obj);
        if (a != self->actors.head) {
            struct GameActor *actor = *LIST_VALUE(a, struct GameActor *);

            world_forget_id_in_cell_map(self, actor->aid, 1);
            world_forget_id_in_list_ec(self, actor->aid, 1);
            erase_actor(self, a);
        }
        obj = *LIST_VALUE(n, void *);
        if (obj)
            game_object_destroy((struct GameObject *)obj);
        next = list_erase(&self->gameObjects, n, NULL);
    }

    for (p = self->pendingSkills.head->next; p != self->pendingSkills.head; p = p->next) {
        id = *LIST_VALUE(p, int);
        for (s = self->skills.head->next; s != self->skills.head; s = s->next)
            if ((*LIST_VALUE(s, struct Skill *))->unit_aid == id)
                break;
        if (s == self->skills.head)
            continue;
        skill = *LIST_VALUE(s, void *);
        if (!skill)
            continue;
        render_object_send(skill, NULL, 0x35, 0, 0, 0, 0);
        if (game_actor_has_pending_hits(skill) == 1) {
            struct ListNode *head = self->pendingSkills.head, *q;

            for (q = head->next; q != head; q = q->next)
                if (*LIST_VALUE(q, int) == id)
                    break;
            if (q != head)
                continue;
            list_push_back(&self->pendingSkills, &id, sizeof id);
            continue;
        }
        o = find_value(&self->gameObjects, skill);
        if (o != self->gameObjects.head) {
            game_object_destroy((struct GameObject *)skill);
            list_erase(&self->gameObjects, o, NULL);
        }
        list_erase(&self->skills, s, NULL);
    }
}

char world_remove_actor_by_aid(struct World *self, unsigned aid)
{
    struct ListNode *head = self->actors.head, *n;

    for (n = head->next; n != head; n = n->next)
        if ((unsigned)(*LIST_VALUE(n, struct GameActor *))->aid == aid)
            break;
    if (n == head)
        return 0;
    world_forget_id_in_cell_map(self, (int)aid, 1);
    world_forget_id_in_list_ec(self, (*LIST_VALUE(n, struct GameActor *))->aid, 1);
    world_remove_object(self, *LIST_VALUE(n, void *));
    erase_actor(self, n);
    return 1;
}

void world_clear_objects(struct World *self)
{
    struct ListNode *n;
    void *obj;

    actor_index_clear();

    for (n = self->gameObjects.head->next; n != self->gameObjects.head;) {
        obj = *LIST_VALUE(n, void *);
        if (obj)
            game_object_destroy((struct GameObject *)obj);
        n = list_erase(&self->gameObjects, n, NULL);
    }
    self->player = NULL;
    list_clear(&self->actors, NULL);
    list_clear(&self->items, NULL);
    list_clear(&self->skills, NULL);
    effect_mgr_reset(g_effectMgr);
    list_clear(&self->pendingSkills, NULL);
    tree_clear(&self->idsByCell, cell_ids_destroy);
    list_clear(&self->idsNowhere, NULL);
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
