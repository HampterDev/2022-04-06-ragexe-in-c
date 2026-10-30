#ifndef CONTENTS_QUEST_H
#define CONTENTS_QUEST_H
#include "base.h"
#include "std/container_types.h"

#define RENEW_QUEST_TEXT_TITLE      0
#define RENEW_QUEST_TEXT_ICON       1
#define RENEW_QUEST_TEXT_ICON_TIP   2
#define RENEW_QUEST_TEXT_SUMMARY    3
#define RENEW_QUEST_TEXT_NPC_SPRITE 5
#define RENEW_QUEST_TEXT_NPC_MAP    6
#define RENEW_QUEST_HUNT_MAX 3
#define RENEW_QUEST_HUNT_NAME 0x33
#define RENEW_QUEST_ANY_MOB 0x7cff

struct RenewQuestBase {
    char active;
    struct String text[7];
    int npc_x;
    int npc_y;
};

struct RenewQuestA {
    struct RenewQuestBase base;
    struct String image;
    struct Vec lists[3];
};

struct QuestDeadline {
    short year, month, day, hour, minute;
};

struct RenewQuestPlain {
    struct QuestDeadline deadline;
    char hunt_count;
    int hunt_mob[RENEW_QUEST_HUNT_MAX];
    short hunt_done[RENEW_QUEST_HUNT_MAX];
    char hunt_name[RENEW_QUEST_HUNT_MAX][RENEW_QUEST_HUNT_NAME];
    short hunt_goal[RENEW_QUEST_HUNT_MAX];
    __int64 hunt_id[RENEW_QUEST_HUNT_MAX];
    int hunt_flags[RENEW_QUEST_HUNT_MAX];
    short hunt_min_level[RENEW_QUEST_HUNT_MAX];
    short hunt_max_level[RENEW_QUEST_HUNT_MAX];
};

struct RenewQuestB {
    struct RenewQuestBase base;
    char shown;
    struct Vec description;
    struct RenewQuestPlain plain;
    struct String reward_exp;
    struct String reward_job_exp;
    struct Vec rewards;
    int cool_time_quest;
    int cool_time_quest_high;
};

struct RenewQuestReward {
    unsigned item_id;
    unsigned count;
};

struct RenewQuestMgr;
extern struct RenewQuestMgr *g_questTable;

struct RenewQuestB *quest_hunt_construct(struct RenewQuestB *quest);
struct RenewQuestB *quest_hunt_copy(struct RenewQuestB *quest, const struct RenewQuestB *from);
struct RenewQuestB *quest_hunt_assign(struct RenewQuestB *quest, const struct RenewQuestB *from);
void quest_hunt_destroy(struct RenewQuestB *quest);
struct RenewQuestBase *renew_quest_base_construct(struct RenewQuestBase *self);
void renew_quest_base_destruct(struct RenewQuestBase *self);
struct RenewQuestBase *renew_quest_base_copy(struct RenewQuestBase *self,
                                                        const struct RenewQuestBase *src);
char renew_quest_create(void);
void renew_quest_destroy(void);
void renew_quest_load(void);
struct String *renew_quest_condition_text(struct RenewQuestMgr *self, struct String *out,
                                                        unsigned name_mask, int min_level,
                                                        int max_level);
struct RenewQuestB *renew_quest_ongoing_info(struct RenewQuestMgr *self, unsigned quest_id);
struct RenewQuestA *renew_quest_recommended_info(struct RenewQuestMgr *self, unsigned quest_id);
struct RenewQuestB *renew_quest_find_ongoing(struct RenewQuestMgr *self, unsigned quest_id);
struct RenewQuestA *renew_quest_find_recommended(struct RenewQuestMgr *self, unsigned quest_id);
struct RenewQuestBase *renew_quest_find(struct RenewQuestMgr *self, char ongoing, unsigned quest_id);
void renew_quest_read_ongoing(struct RenewQuestMgr *self, unsigned quest_id,
                                         struct RenewQuestB *info);
char renew_quest_set_active(struct RenewQuestMgr *self, unsigned quest_id, char active);
char renew_quest_set_recommended_active(struct RenewQuestMgr *self, unsigned quest_id, char active);
char renew_quest_set_shown(struct RenewQuestMgr *self, unsigned quest_id, char shown);
void renew_quest_refresh_windows(void);
void renew_quest_save_simple_view_list(struct RenewQuestMgr *self);
void renew_quest_save_recommended_active_list(struct RenewQuestMgr *self);

void renew_quest_clear_ongoing(struct RenewQuestMgr *quests);
void renew_quest_remove(struct RenewQuestMgr *quests, unsigned quest_id);
struct RenewQuestB *renew_quest_get_or_add(struct RenewQuestMgr *quests, unsigned quest_id);
#endif
