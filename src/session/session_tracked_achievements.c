#include "ui/window_manager_state.h"
#include "session/csession.h"
#include "session/session_achievements.h"

#include <string.h>

#define LINE_WIDTH  0x28

static void line_copy(struct TrackedLine *line, const struct TrackedLine *src)
{
    string_copy(&line->text, &src->text);
    line->done = src->done;
}

static void lines_free(struct Vec *v)
{
    struct TrackedLine *line;

    for (line = (struct TrackedLine *)v->first; line != (struct TrackedLine *)v->last; line++)
        string_free(&line->text);
    vec_free(v);
}

static void lines_copy(struct Vec *dst, const struct Vec *src)
{
    const struct TrackedLine *line;

    vec_init(dst);
    vec_reserve(dst, VEC_COUNT(src, struct TrackedLine), sizeof(struct TrackedLine));
    for (line = (const struct TrackedLine *)src->first; line != (const struct TrackedLine *)src->last; line++) {
        struct TrackedLine copy;

        line_copy(&copy, line);
        vec_push(dst, &copy, sizeof copy);
    }
}

static void entry_copy(struct TrackedAchievement *entry, const struct TrackedAchievement *src)
{
    entry->expanded = src->expanded;
    entry->completed = src->completed;
    entry->id = src->id;
    string_copy(&entry->name, &src->name);
    lines_copy(&entry->lines, &src->lines);
}

static void entry_destroy(struct TrackedAchievement *entry)
{
    lines_free(&entry->lines);
    string_free(&entry->name);
}

void tracked_achievements_clear(struct Vec *v)
{
    struct TrackedAchievement *entry;

    for (entry = (struct TrackedAchievement *)v->first; entry != (struct TrackedAchievement *)v->last; entry++)
        entry_destroy(entry);
    vec_clear(v);
}

void tracked_achievements_free(struct Vec *v)
{
    tracked_achievements_clear(v);
    vec_free(v);
}

static void set_line(struct TrackedLine *line, const struct AchievementInfo *info, size_t i, int progress)
{
    const struct AchievementResource *cond = (const struct AchievementResource *)info->resources.first + i;
    struct String part, text, count;

    if (info->ui_type == 0) {
        if (cond->text.size > LINE_WIDTH) {
            string_init(&part);
            string_substring(&part, &cond->text, 0, LINE_WIDTH);
            string_init(&text);
            string_format(&text, "- %s...", string_cstr(&part));
            string_assign(&line->text, string_cstr(&text), text.size);
            string_free(&text);
            string_free(&part);
        } else {
            string_init(&text);
            string_format(&text, "- %s", string_cstr(&cond->text));
            string_assign(&line->text, string_cstr(&text), text.size);
            string_free(&text);
        }
        line->done = progress > 0;
    } else if (info->ui_type == 1) {
        string_init(&count);
        string_format(&count, "(%d/%d)", progress, cond->target);
        if (cond->text.size + count.size > LINE_WIDTH) {
            string_init(&part);
            string_substring(&part, &cond->text, 0, LINE_WIDTH - count.size);
            string_init(&text);
            string_format(&text, "- %s...%s", string_cstr(&part), string_cstr(&count));
            string_assign(&line->text, string_cstr(&text), text.size);
            string_free(&text);
            string_free(&part);
        } else {
            string_init(&text);
            string_format(&text, "- %s %s", string_cstr(&cond->text), string_cstr(&count));
            string_assign(&line->text, string_cstr(&text), text.size);
            string_free(&text);
        }
        line->done = progress == cond->target;
        string_free(&count);
    }
}

static void make_line(struct TrackedLine *line, const struct AchievementInfo *info, size_t i, int progress)
{
    string_init(&line->text);
    line->done = 0;
    set_line(line, info, i, progress);
}

BYTE session_toggle_tracked_achievement(struct Session *self, int id)
{
    struct Vec *tracked = &self->trackedAchievements;
    struct TrackedAchievement *it = (struct TrackedAchievement *)tracked->first;
    struct TrackedAchievement *end = (struct TrackedAchievement *)tracked->last;
    struct SessionAchievementState *record;
    struct AchievementPair *found;

    for (; it != end; it++)
        if (it->id == id)
            break;

    found = tree_find(&self->achievements, &id, tree_less_int);
    if (!found) {
        struct AchievementPair pair;

        pair.id = id;
        pair.value.known = 1;
        pair.value.completed = 0;
        memset(pair.value.objectives, 0, sizeof pair.value.objectives);
        pair.value.completed_at = 0;
        pair.value.rewarded = 0;
        found = tree_add(&self->achievements, &pair, sizeof pair, tree_less_int, NULL);
    }
    record = &found->value;
    if (record == NULL)
        return 0;

    if (it != (struct TrackedAchievement *)tracked->last) {
        entry_destroy(it);
        vec_erase(tracked, it, it + 1);
    } else {
        struct AchievementInfo info;
        struct TrackedAchievement entry, added;
        size_t i, count;

        if (VEC_COUNT(tracked, struct TrackedAchievement) >= 3) {
            ui_window_mgr_send_msg_(&g_windowMgr, 1, (intptr_t)msg_str(0xa78), 0x64ffff, 0, 0);
            return 0;
        }
        achievement_info_init(&info);
        achievement_get_info(g_achievementDb, &info, id);
        entry.expanded = 0;
        entry.completed = record->completed;
        entry.id = id;
        string_init(&entry.name);
        vec_init(&entry.lines);
        string_assign(&entry.name, string_cstr(&info.text[0]), info.text[0].size);
        count = VEC_COUNT(&info.resources, struct AchievementResource);
        for (i = 0; i < count; i++) {
            struct TrackedLine line, copy;

            make_line(&line, &info, i, record->objectives[i]);
            line_copy(&copy, &line);
            vec_push(&entry.lines, &copy, sizeof copy);
            string_free(&line.text);
        }
        entry_copy(&added, &entry);
        vec_push(tracked, &added, sizeof added);
        entry_destroy(&entry);
        achievement_info_destruct(&info);
    }

    if (VEC_COUNT(tracked, struct TrackedAchievement) == 0) {
        ui_window_mgr_post_quit1(&g_windowMgr, 0x10f);
        return 0;
    }
    return 1;
}

void session_refresh_tracked_achievement(struct Session *self, int id)
{
    struct Vec *tracked = &self->trackedAchievements;
    struct TrackedAchievement *it = (struct TrackedAchievement *)tracked->first;
    struct TrackedAchievement *end = (struct TrackedAchievement *)tracked->last;
    struct SessionAchievementState record;
    struct AchievementPair *found;
    struct AchievementInfo info;
    size_t i;

    for (; it != end; it++)
        if (it->id == id)
            break;
    if (it == (struct TrackedAchievement *)tracked->last)
        return;
    found = tree_find(&g_session1.achievements, &id, tree_less_int);
    if (found) {
        record = found->value;
    } else {
        record.known = 0;
        record.completed = 0;
        memset(record.objectives, 0, sizeof record.objectives);
        record.completed_at = 0;
        record.rewarded = 0;
    }
    achievement_info_init(&info);
    achievement_get_info(g_achievementDb, &info, id);
    it->completed = record.completed;
    for (i = 0; i < VEC_COUNT(&info.resources, struct AchievementResource); i++)
        set_line((struct TrackedLine *)it->lines.first + i, &info, i, record.objectives[i]);
    achievement_info_destruct(&info);
}
