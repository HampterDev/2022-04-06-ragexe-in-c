#ifndef SESSION_SESSION_ACHIEVEMENTS_H
#define SESSION_SESSION_ACHIEVEMENTS_H

#include "session/csession.h"
#include "contents/game_session.h"

#define ACH_OBJECTIVES 10

#pragma pack(push, 1)
struct AchievementEntry {
    int id;
    unsigned char completed;
    int objectives[ACH_OBJECTIVES];
    int completed_at;
    unsigned char reward;
};
#pragma pack(pop)
SIZE_ASSERT(AchievementEntry, 0x32);

struct AchievementPair {
    int id;
    struct SessionAchievementState value;
};
SIZE_ASSERT(AchievementPair, 0x3c);

struct TrackedLine {
    struct String text;
    BYTE done;
};

struct TrackedAchievement {
    BYTE expanded;
    BYTE completed;
    int id;
    struct String name;
    struct Vec lines;
};

void session_put_achievement(struct Session *self, const struct AchievementEntry *entry);
struct SessionAchievementState *session_get_achievement(struct Session *self,
                                                             struct SessionAchievementState *out, int id);
int session_add_title(struct Session *self, int id);
void session_add_new_title(struct Session *self, int id);
void session_get_latest_achievements(struct Session *self, int *second, int *first);

void tracked_achievements_clear(struct Vec *v);
void tracked_achievements_free(struct Vec *v);
BYTE session_toggle_tracked_achievement(struct Session *self, int id);
void session_refresh_tracked_achievement(struct Session *self, int id);

#include "session/ranking.h"

#endif
