#ifndef UI_QUESTS_QUESTS_STD_H
#define UI_QUESTS_QUESTS_STD_H

#include "ui/ui_std.h"

struct String;
struct Vec;
struct StdBanner38 {
    struct String first, second;
    int third, fourth;
};
void attendance_banners_free(struct Vec *v);

#endif
