#include "ui/window_manager_state.h"
#include "ui/window_access.h"
#include <string.h>
#include "contents/renew_quest.h"
#include "contents/ui_window.h"
#include "contents/ui_window_mgr.h"
#include "ui/ui_std.h"
#include "ui/wndmgr/wndmgr.h"

#define QUEST_WND_ID 0x141
#define QUEST_WND_MSG_REFRESH 0x17

void renew_quest_put_recommended(struct RenewQuestMgr *self, unsigned quest_id,
                                            const struct RenewQuestA *info)
{
    struct RecommendedQuestEntry entry;
    int added, i;

    tree_erase_key(&self->quests_a, &quest_id, tree_less_uint, recommended_quest_entry_destroy);
    entry.id = quest_id;
    renew_quest_base_copy(&entry.quest.base, &info->base);
    string_copy(&entry.quest.image, &info->image);
    for (i = 0; i < 3; i++)
        string_vec_copy(&entry.quest.lists[i], &info->lists[i]);
    tree_add(&self->quests_a, &entry, sizeof entry, tree_less_uint, &added);
    if (!added)
        renew_quest_a_destruct(&entry.quest);
}

char renew_quest_set_active(struct RenewQuestMgr *self, unsigned quest_id, char active)
{
    struct OngoingQuestEntry *entry = tree_find(&self->quests_b, &quest_id, tree_less_uint);

    if (!entry)
        return 0;
    entry->quest.base.active = active;
    return 1;
}

char renew_quest_set_recommended_active(struct RenewQuestMgr *self, unsigned quest_id, char active)
{
    struct RecommendedQuestEntry *entry = tree_find(&self->quests_a, &quest_id, tree_less_uint);

    if (!entry)
        return 0;
    entry->quest.base.active = active;
    return 1;
}

char renew_quest_set_shown(struct RenewQuestMgr *self, unsigned quest_id, char shown)
{
    struct OngoingQuestEntry *entry = tree_find(&self->quests_b, &quest_id, tree_less_uint);

    if (!entry)
        return 0;
    entry->quest.shown = shown;
    return 1;
}

void renew_quest_refresh_windows(void)
{
    struct UIWindow *wnd;

    if (g_windowMgr.quest_display_wnd)
        ui_window_invalidate(g_windowMgr.quest_display_wnd);
    wnd = (struct UIWindow *)ui_window_find(QUEST_WND_ID);
    if (wnd)
        ui_window_send(wnd, NULL, QUEST_WND_MSG_REFRESH, 1, 0, 0, 0);
}

static void write_string(struct File *file, const struct String *s)
{
    file_write(file, string_cstr(s), (int)s->size);
}

static void save_list(const struct Tree *map, size_t flag_offset, const char *path,
                      const char *reset, const char *open)
{
    struct String name, line;
    struct File file;
    struct TreeNode *node;

    string_init(&name);
    string_assign_cstr(&name, path);
    file_construct(&file);
    if (file_open(&file, string_cstr(&name), 0x1001, 1, 1)) {
        string_init(&line);
        string_assign_cstr(&line, reset);
        write_string(&file, &line);
        string_assign_cstr(&line, open);
        write_string(&file, &line);
        for (node = tree_first(map); node != map->head; node = tree_next(node)) {
            if (TREE_VALUE(node, const char)[flag_offset] != 0)
                continue;
            string_format(&line, "\t%d,\r\n", (int)*TREE_VALUE(node, unsigned));
            write_string(&file, &line);
        }
        string_assign_cstr(&line, "}");
        write_string(&file, &line);
        file_close(&file);
        string_free(&line);
    }
    file_destruct(&file);
    string_free(&name);
}

void renew_quest_save_simple_view_list(struct RenewQuestMgr *self)
{
    save_list(&self->quests_b, offsetof(struct OngoingQuestEntry, quest.shown),
              "SaveData\\OngoingSimpleViewList.lua",
              "OngoingSimpleViewList = {}\r\n\r\n", "OngoingSimpleViewList = \r\n{\r\n");
}

void renew_quest_save_recommended_active_list(struct RenewQuestMgr *self)
{
    save_list(&self->quests_a, offsetof(struct RecommendedQuestEntry, quest.base.active),
              "SaveData\\RecommendedActiveList.lua",
              "RecommendedActiveList = {}\r\n\r\n", "RecommendedActiveList = \r\n{\r\n");
}

void renew_quest_clear_ongoing(struct RenewQuestMgr *quests)
{
    tree_clear(&quests->quests_b, ongoing_quest_entry_destroy);
}

void renew_quest_remove(struct RenewQuestMgr *quests, unsigned quest_id)
{
    tree_erase_key(&quests->quests_b, &quest_id, tree_less_uint, ongoing_quest_entry_destroy);
}

struct RenewQuestB *renew_quest_get_or_add(struct RenewQuestMgr *quests, unsigned quest_id)
{
    return renew_quest_b_at(&quests->quests_b, &quest_id);
}
