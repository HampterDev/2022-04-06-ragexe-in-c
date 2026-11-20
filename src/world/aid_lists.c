#include "actor/game_actor.h"
#include "actor/render_ops.h"
#include "effects/skills/skill_state.h"
#include "std/list.h"
#include "world/world.h"
#include <string.h>

/* custom: the world's actors and skill units as they join and leave their lists, and the units
   looked up by their aid (world/aid_index.h). A node leaves its list without its object being
   read, which may be destroyed by then: the original destroyed a unit or an actor before it took it
   off its list, so it is still found while its destructor runs. */

#define MSG_LEAVE 0x35

struct Skill *world_get_skill_by_aid(struct World *self, int aid)
{
    struct IdLink *link;

    if (!self)
        return NULL;
    link = id_index_first(&self->skillIndex, (uint32_t)aid);
    return link ? ID_INDEX_ENTRY(link, struct SkillEntry, aidLink)->skill : NULL;
}

void world_set_skill_aid(struct World *self, struct Skill *skill, int aid)
{
    if (skill->unit_node) {
        world_index_room(&self->skillIndex);
        id_index_set_id(&self->skillIndex, &LIST_VALUE(skill->unit_node, struct SkillEntry)->aidLink,
                        (uint32_t)aid);
    }
    skill->unit_aid = aid;
}

void world_add_actor(struct World *self, struct GameActor *actor)
{
    struct ActorEntry made, *entry;

    world_add_object(self, actor);
    memset(&made, 0, sizeof made);
    made.actor = actor;
    entry = list_push_back(&self->actors, &made, sizeof made);
    actor->actor_node = LIST_NODE_OF(entry);
    world_index_room(&self->actorIndex);
    id_index_append(&self->actorIndex, &entry->aidLink, (uint32_t)actor->aid);
}

void world_erase_actor(struct World *self, struct ListNode *node)
{
    id_index_remove(&self->actorIndex, &LIST_VALUE(node, struct ActorEntry)->aidLink);
    list_erase(&self->actors, node, NULL);
}

void world_add_skill(struct World *self, struct Skill *skill)
{
    struct SkillEntry made, *entry;

    if (!skill)
        return;
    world_add_object(self, skill);
    memset(&made, 0, sizeof made);
    made.skill = skill;
    entry = list_push_back(&self->skills, &made, sizeof made);
    skill->unit_node = LIST_NODE_OF(entry);
    world_index_room(&self->skillIndex);
    id_index_append(&self->skillIndex, &entry->aidLink, (uint32_t)skill->unit_aid);
}

/* the entry leaves the units, the index and the units that left; its unit is not read */
static void erase_skill(struct World *self, struct SkillEntry *entry)
{
    if (entry->pendingNode)
        list_erase(&self->pendingSkills, entry->pendingNode, NULL);
    id_index_remove(&self->skillIndex, &entry->aidLink);
    list_erase(&self->skills, LIST_NODE_OF(entry), NULL);
}

BYTE world_remove_skill(struct World *self, int aid)
{
    struct IdLink *link = id_index_first(&self->skillIndex, (uint32_t)aid);
    struct SkillEntry *entry;

    if (!link)
        return 0;
    entry = ID_INDEX_ENTRY(link, struct SkillEntry, aidLink);
    render_object_send(entry->skill, NULL, MSG_LEAVE, 0, 0, 0, 0);
    if (game_actor_has_pending_hits((struct GameActor *)entry->skill) != 1) {
        /* alive: on the units, which a unit leaves only here, in world_process_leaving_skills and
           with the map (its processing never ends it: skill_on_process, graffi_skill_on_process) */
        world_remove_object(self, entry->skill);
        erase_skill(self, entry);
        return 1;
    }
    /* custom: the unit stays until its hits end, marked on its own entry, where the original put
       the aid on pendingSkills for good and so removed every later unit of that aid as well */
    if (!entry->pendingNode)
        entry->pendingNode = LIST_NODE_OF(list_push_back(&self->pendingSkills, &entry, sizeof entry));
    return 0;
}

void world_keep_skill(struct World *self, struct Skill *skill)
{
    struct SkillEntry *entry;

    if (!skill->unit_node)
        return;
    entry = LIST_VALUE(skill->unit_node, struct SkillEntry);
    if (entry->pendingNode) {
        list_erase(&self->pendingSkills, entry->pendingNode, NULL);
        entry->pendingNode = NULL;
    }
}

void world_process_leaving_skills(struct World *self)
{
    struct ListNode *p, *next;

    for (p = self->pendingSkills.head->next; p != self->pendingSkills.head; p = next) {
        struct SkillEntry *entry = *LIST_VALUE(p, struct SkillEntry *);

        render_object_send(entry->skill, NULL, MSG_LEAVE, 0, 0, 0, 0);
        if (game_actor_has_pending_hits((struct GameActor *)entry->skill) == 1) {
            next = p->next;
            continue;
        }
        world_remove_object(self, entry->skill); /* alive: on the units (world_remove_skill) */
        next = p->next;
        erase_skill(self, entry);
    }
}
