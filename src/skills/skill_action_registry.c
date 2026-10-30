#include "skills/skill_action_registry.h"
#include "core/memory.h"
#include "core/fatal.h"
#include "std/tree.h"

struct ActionEntry {
    int id;
    const void *action;
};

struct SkillActionMgr {
    struct Tree skill_actions;
    struct Tree efst_actions;
    struct Tree object_actions;
};

static struct SkillActionMgr *skill_action_mgr;

struct SkillActionMgr *skill_action_mgr_instance(void)
{
    struct SkillActionMgr *self;
    if (skill_action_mgr)
        return skill_action_mgr;
    self = mem_try_alloc(sizeof *self);
    if (!self)
        return NULL;
    tree_init(&self->skill_actions);
    tree_init(&self->efst_actions);
    tree_init(&self->object_actions);
    skill_action_mgr = self;
    return self;
}

void skill_action_mgr_destroy_instance(void)
{
    struct SkillActionMgr *self = skill_action_mgr;

    if (!self)
        return;
    tree_free(&self->object_actions, NULL);
    tree_free(&self->efst_actions, NULL);
    tree_free(&self->skill_actions, NULL);
    mem_free(self);
    skill_action_mgr = NULL;
}

static void register_action_entry(struct Tree *map, int id, const void *action)
{
    struct ActionEntry entry;

    entry.id = id;
    entry.action = action;
    tree_add(map, &entry, sizeof entry, tree_less_int, NULL);
}

static const void *find_action_entry(const struct Tree *map, int id)
{
    const struct ActionEntry *entry = tree_find(map, &id, tree_less_int);

    if (!entry)
        return NULL;
    if (!entry->action)
        fatal_error("invalid function call");
    return entry->action;
}

void skill_action_mgr_add_skill(struct SkillActionMgr *self, int id,
                              const struct SkillAction *action)
{
    register_action_entry(&self->skill_actions, id, action);
}

void skill_action_mgr_add_efst(struct SkillActionMgr *self, int id,
                             const struct StatusAction *action)
{
    register_action_entry(&self->efst_actions, id, action);
}

void skill_action_mgr_add_object_action(struct SkillActionMgr *self, int id,
                                        const struct SkillObjectAction *action)
{
    register_action_entry(&self->object_actions, id, action);
}

const struct SkillAction *skill_action_mgr_find_skill(struct SkillActionMgr *self, int id)
{
    return find_action_entry(&self->skill_actions, id);
}

const struct StatusAction *skill_action_mgr_find_efst(struct SkillActionMgr *self, int id)
{
    return find_action_entry(&self->efst_actions, id);
}

const struct SkillObjectAction *skill_action_mgr_find_object(struct SkillActionMgr *self, int id)
{
    return find_action_entry(&self->object_actions, id);
}

unsigned char skill_action_mgr_has_efst_action(struct SkillActionMgr *self, int id)
{
    return tree_find(&self->efst_actions, &id, tree_less_int) != NULL;
}
