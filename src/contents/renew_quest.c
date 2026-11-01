#include "core/memory.h"
#include "contents/renew_quest.h"

struct RenewQuestMgr *g_questTable;

struct RenewQuestB *renew_quest_b_at(struct Tree *map, const unsigned *key)
{
    struct OngoingQuestEntry *entry = tree_find(map, key, tree_less_uint);
    struct OngoingQuestEntry made;

    if (!entry) {
        made.id = *key;
        quest_hunt_construct(&made.quest);
        entry = tree_add(map, &made, sizeof made, tree_less_uint, NULL);
    }
    return &entry->quest;
}

static struct RenewQuestMgr *renew_quest_construct(struct RenewQuestMgr *self)
{
    tree_init(&self->quests_b);
    self->hunt_name_length = 0x18;
    return self;
}

static void renew_quest_destruct(struct RenewQuestMgr *self)
{
    tree_free(&self->quests_b, ongoing_quest_entry_destroy);
}

char renew_quest_create(void)
{
    struct RenewQuestMgr *mgr;

    if (g_questTable)
        return 0;
    mgr = (struct RenewQuestMgr *)mem_try_alloc(sizeof *mgr);
    if (mgr)
        renew_quest_construct(mgr);
    g_questTable = mgr;
    if (!mgr)
        return 0;
    return 1;
}

void renew_quest_destroy(void)
{
    if (!g_questTable)
        return;
    renew_quest_destruct(g_questTable);
    mem_free(g_questTable);
    g_questTable = 0;
}
