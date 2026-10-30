#include "core/memory.h"
#include <string.h>
#include "contents/renew_quest.h"

struct RenewQuestMgr *g_questTable;

struct RenewQuestA *renew_quest_a_construct(struct RenewQuestA *self)
{
    int i;

    renew_quest_base_construct(&self->base);
    string_init(&self->image);
    for (i = 0; i < 3; i++)
        vec_init(&self->lists[i]);
    for (i = 0; i < 3; i++) {
        struct String *s;

        for (s = self->lists[i].first; s != self->lists[i].last; s++)
            string_free(s);
        self->lists[i].last = self->lists[i].first;
    }
    return self;
}

void renew_quest_a_destruct(struct RenewQuestA *self)
{
    int i;

    for (i = 2; i >= 0; i--)
        string_vec_free(&self->lists[i]);
    string_free(&self->image);
    renew_quest_base_destruct(&self->base);
}

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
    tree_init(&self->quests_a);
    self->hunt_name_length = 0x18;
    return self;
}

static void renew_quest_destruct(struct RenewQuestMgr *self)
{
    tree_free(&self->quests_a, recommended_quest_entry_destroy);
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
