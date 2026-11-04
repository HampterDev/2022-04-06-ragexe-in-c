#include "ui/window_access.h"
#include "session/csession.h"
#include "contents/game_session.h"
#include "session/session_achievements.h"
#include "session/session_lua.h"

#include <string.h>
#include <time.h>
#include "platform/clock.h"

#define WND_ACHIEVEMENT     0xa

static void local_date(int t, WORD *year, WORD *month, WORD *day)
{
    struct tm *tm = platform_time_local(t < 0 ? 0 : t);

    *year = (WORD)(tm->tm_year + 1900);
    *month = (WORD)(tm->tm_mon + 1);
    *day = (WORD)tm->tm_mday;
}

void session_put_achievement(struct Session *self,
                                       const struct AchievementEntry *entry)
{
    int id = entry->id;
    struct AchievementPair *found = tree_find(&self->achievements, &id, tree_less_int);
    struct AchievementPair pair;
    struct SessionAchievementState rec;

    if (found) {
        struct SessionAchievementState *have = &found->value;
        have->completed = entry->completed;
        memcpy(have->objectives, entry->objectives, sizeof have->objectives);
        have->completed_at = entry->completed_at;
        have->rewarded = entry->reward;
        local_date(entry->completed_at, &have->year, &have->month, &have->day);
        return;
    }
    rec.known = 1;
    rec.completed = entry->completed;
    memcpy(rec.objectives, entry->objectives, sizeof rec.objectives);
    rec.completed_at = entry->completed_at;
    rec.rewarded = entry->reward;
    local_date(entry->completed_at, &rec.year, &rec.month, &rec.day);
    pair.id = entry->id;
    pair.value = rec;
    tree_add(&self->achievements, &pair, sizeof pair, tree_less_int, NULL);
}

struct SessionAchievementState *session_get_achievement(struct Session *self,
                                                             struct SessionAchievementState *out, int id)
{
    struct AchievementPair *found = tree_find(&self->achievements, &id, tree_less_int);

    if (found) {
        *out = found->value;
        return out;
    }
    out->known = 0;
    out->completed = 0;
    memset(out->objectives, 0, sizeof out->objectives);
    out->completed_at = 0;
    out->rewarded = 0;
    return out;
}

static unsigned char title_name_less(const void *a, const void *b, void *pred)
{
    struct String nameA, nameB;
    unsigned char less;

    session_get_title_string(&g_session1, &nameA, *(const int *)a);
    session_get_title_string(&g_session1, &nameB, *(const int *)b);
    less = string_compare(&nameA, &nameB) < 0;
    string_free(&nameB);
    string_free(&nameA);
    return less;
}

int session_add_title(struct Session *self, int id)
{
    int title = achievement_reward_title(g_achievementDb, id);
    struct Vec *titles = &self->titles;
    struct UIWindow *wnd;
    int *it;

    if (title == -1)
        return -1;
    for (it = titles->first; it != (int *)titles->last; it++)
        if (*it == title)
            return -1;
    vec_push(titles, &title, sizeof(int));
    std_sort(titles->first, titles->last, sizeof(int), title_name_less, NULL);
    wnd = ui_window_find(WND_ACHIEVEMENT);
    if (wnd)
        ui_window_send(wnd, NULL, 0x3c, 0, 0, 0, 0);
    return title;
}

void session_add_new_title(struct Session *self, int id)
{
    struct Vec *ids = &self->newTitles;
    int *it;

    for (it = ids->first; it != (int *)ids->last; it++)
        if (*it == id)
            return;
    vec_push(ids, &id, sizeof(int));
}

void session_get_latest_achievements(struct Session *self, int *second, int *first)
{
    struct TreeNode *node;
    /* next is read only once best is set, which sets it too, or for a time of -1 or less, which a
       32-bit stack address (positive) and -1 both pass over; it is not an address. */
    int best = -1, next = -1, at;

    for (node = tree_first(&self->achievements); node != self->achievements.head; node = tree_next(node)) {
        const struct AchievementPair *achievement = TREE_VALUE(node, const struct AchievementPair);

        if (!achievement->value.completed)
            continue;
        at = achievement->value.completed_at;
        if (best < at) {
            next = best;
            best = at;
            *second = *first;
            *first = achievement->id;
        } else if (next < at) {
            next = at;
            *second = achievement->id;
        }
    }
}
