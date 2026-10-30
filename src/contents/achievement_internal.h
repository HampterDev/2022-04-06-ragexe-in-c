#ifndef CONTENTS_ACHIEVEMENT_INTERNAL_H
#define CONTENTS_ACHIEVEMENT_INTERNAL_H

#include "contents/achievement.h"

struct AchievementMgr {
    int tab_stride;
    struct Tree achievements;
    struct Tree tabs;
};

struct AchievementTab {
    int key;
    struct Vec ids;
};

char achievement_load(void);
struct Vec *achievement_tab_at(struct Tree *tabs, int key);

#endif
