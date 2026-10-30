#ifndef CONTENTS_ACHIEVEMENT_H
#define CONTENTS_ACHIEVEMENT_H

#include "std/std.h"
#include "std/tree.h"

struct AchievementResource {
    struct String text;
    int target;
    int goto_id;
};

struct AchievementInfo {
    struct String text[3];
    int info_number;
    int reward_item;
    int reward_title;
    int reward_buff;
    struct Vec resources;
    int ui_type;
    struct String ui_text;
};

struct AchievementInfoEntry {
    int id;
    struct AchievementInfo info;
};

struct AchievementMgr;
extern struct AchievementMgr *g_achievementDb;

char achievement_create(void);
void achievement_destroy(void);
char achievement_get_info(struct AchievementMgr *self, struct AchievementInfo *out, int id);
struct String *achievement_text1(struct AchievementMgr *self, struct String *out, int id);
struct String *achievement_ui_text(struct AchievementMgr *self, struct String *out, int id);
int achievement_info_number(struct AchievementMgr *self, int id);
int achievement_reward_title(struct AchievementMgr *self, int id);
char achievement_has_reward(struct AchievementMgr *self, int id);
char achievement_collect_tab(struct AchievementMgr *self, struct Tree *out, int major, int minor);
int achievement_tab_count(struct AchievementMgr *self, int major, int minor);
int achievement_tab_count_flagged(struct AchievementMgr *self, int major, int minor);
char achievement_find_tab(struct AchievementMgr *self, int id, int *major, int *minor);

void achievement_info_init(struct AchievementInfo *info);
void achievement_info_copy(struct AchievementInfo *dst, const struct AchievementInfo *src);
void achievement_info_assign(struct AchievementInfo *dst, const struct AchievementInfo *src);
void achievement_info_destruct(struct AchievementInfo *info);

void achievement_info_entry_destroy(void *value);
struct AchievementInfo *achievement_at(struct Tree *map, int id);
char achievement_add(struct Tree *map, int id, const struct AchievementInfo *info);

#endif
