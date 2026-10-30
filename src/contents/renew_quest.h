#ifndef CONTENTS_RENEW_QUEST_H
#define CONTENTS_RENEW_QUEST_H

#include "std/std.h"
#include "std/tree.h"
#include "contents/quest.h"

OFFSET_ASSERT(RenewQuestBase, text, 0x04);

struct RecommendedQuestEntry {
    unsigned id;
    struct RenewQuestA quest;
};

struct OngoingQuestEntry {
    unsigned id;
    struct RenewQuestB quest;
};

struct RenewQuestMgr {
    struct Tree quests_b;
    struct Tree quests_a;
    int hunt_name_length;
};

struct RenewQuestA *renew_quest_a_construct(struct RenewQuestA *self);
void renew_quest_a_destruct(struct RenewQuestA *self);

struct RenewQuestB *renew_quest_b_at(struct Tree *map, const unsigned *key);
void recommended_quest_entry_destroy(void *entry);
void ongoing_quest_entry_destroy(void *entry);

enum {
    RENEW_QUEST_TITLE = 0,
    RENEW_QUEST_ICON = 1,
    RENEW_QUEST_ICON_TIP = 2,
    RENEW_QUEST_SUMMARY = 3,
    RENEW_QUEST_NPC_SPRITE = 5,
    RENEW_QUEST_NPC_MAP = 6
};

void renew_quest_put_recommended(struct RenewQuestMgr *self, unsigned quest_id,
                                            const struct RenewQuestA *info);

struct lua_State;
int renew_quest_lua_add_ongoing_description(struct lua_State *L);
int renew_quest_lua_add_ongoing_reward_info(struct lua_State *L);
int renew_quest_lua_add_recommended_quest_info(struct lua_State *L);
int renew_quest_lua_insert_recommended_quest_info(struct lua_State *L);
int renew_quest_lua_set_recommended_quest_active(struct lua_State *L);

struct RenewQuestA *renew_quest_a_at(struct Tree *map, unsigned key);

#endif
