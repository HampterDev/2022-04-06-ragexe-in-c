#ifndef CONTENTS_RENEW_QUEST_H
#define CONTENTS_RENEW_QUEST_H

#include "std/std.h"
#include "std/tree.h"
#include "contents/quest.h"

OFFSET_ASSERT(RenewQuestBase, text, 0x04);

struct OngoingQuestEntry {
    unsigned id;
    struct RenewQuestB quest;
};

struct RenewQuestMgr {
    struct Tree quests_b;
    int hunt_name_length;
};

struct RenewQuestB *renew_quest_b_at(struct Tree *map, const unsigned *key);
void ongoing_quest_entry_destroy(void *entry);

struct lua_State;
int renew_quest_lua_add_ongoing_description(struct lua_State *L);
int renew_quest_lua_add_ongoing_reward_info(struct lua_State *L);

#endif
