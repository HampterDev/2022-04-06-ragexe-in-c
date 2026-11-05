#include "game/gamemode.h"
#include "ui/window_types.h"
#include <limits.h>
#include "ui/window_manager_state.h"
#include "ui/window_access.h"
#include <string.h>
#include "game/quest_packets.h"
#include "game/quest_info.h"
#include "game/tips.h"
#include "game/client.h"
#include "core/msg_str.h"
#include "core/text_util.h"

#pragma pack(push, 1)
struct HuntUpdateEx {
    unsigned quest_id;
    unsigned hunt_id;
    short needed;
    short hunted;
};
struct HuntUpdateEx2 {
    unsigned quest_id;
    __int64 hunt_id;
    short needed;
    short hunted;
};
struct HuntingQuestInfo {
    unsigned quest_id;
    int monster;
    short needed;
    short hunted;
};
struct PACKET_ZC_UPDATE_MISSION_HUNT {
    unsigned short id;
    short length;
    short count;
    unsigned char entries[];
};
struct PACKET_ZC_HUNTING_QUEST_INFO {
    unsigned short id;
    unsigned short length;
    unsigned char entries[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_UPDATE_MISSION_HUNT, 6);
SIZE_ASSERT(PACKET_ZC_HUNTING_QUEST_INFO, 4);
SIZE_ASSERT(HuntUpdateEx, 12);
SIZE_ASSERT(HuntUpdateEx2, 16);
SIZE_ASSERT(HuntingQuestInfo, 12);

#define HUNT_CHAT_COLOR 0xffff
#define CHAT_CHANNEL_QUEST 0x12
#define WINDOW_QUEST_DETAIL 0x142
enum { UIM_QUEST_REFRESH = 0x17 };

static void quest_destroy(struct RenewQuestB *quest)
{
    quest_hunt_destroy(quest);
}

static struct RenewQuestB *quest_slot(unsigned quest_id)
{
    return renew_quest_get_or_add(g_questTable, quest_id);
}

static void update_target(struct Vec *lines, struct RenewQuestB *quest, unsigned quest_id, int slot,
                          short needed, short hunted)
{
    struct String line, text, part;

    quest->plain.hunt_done[slot] = hunted;
    quest->plain.hunt_goal[slot] = needed;
    quest_hunt_get_line(&line, quest, slot);
    string_vec_push_move(lines, &line);
    string_free(&line);

    string_init(&text);
    if (quest->plain.hunt_mob[slot] == QUEST_NAME_FROM_TABLE) {
        renew_quest_condition_text(g_questTable, &part, quest->plain.hunt_flags[slot], quest->plain.hunt_min_level[slot],
                         quest->plain.hunt_max_level[slot]);
        string_assign(&text, string_cstr(&part), part.size);
        string_free(&part);
    } else {
        string_assign_cstr(&text, quest->plain.hunt_name[slot]);
    }
    string_init(&part);
    string_format(&part, msg_str(0x64a), string_cstr(&quest->base.text[RENEW_QUEST_TEXT_TITLE]), string_cstr(&text),
               (int)quest->plain.hunt_done[slot], (int)needed);
    string_assign(&text, string_cstr(&part), part.size);
    string_free(&part);
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (int)string_cstr(&text), HUNT_CHAT_COLOR,
                        CHAT_CHANNEL_QUEST, 0);
    quest_hunt_assign(quest_slot(quest_id), quest);
    string_free(&text);
}

static void redraw_quest_windows(void)
{
    struct UIWindow *wnd;

    if (g_windowMgr.quest_display_wnd != 0)
        ui_window_invalidate(g_windowMgr.quest_display_wnd);
    wnd = ui_window_find(WINDOW_QUEST_DETAIL);
    if (wnd != 0)
        UIWINDOW_SENDMSG(wnd, UIM_QUEST_REFRESH, 0, 0, 0, 0);
}

static void finish(struct GameMode *self, struct Vec *lines)
{
    if (lines->first != lines->last)
        game_mode_show_quest_hunt_list(self, lines);
    redraw_quest_windows();
    string_vec_free(lines);
}

static int hunt_packet_valid(const void *packet, size_t size, size_t header_size, size_t entry_size)
{
    const struct PACKET_ZC_UPDATE_MISSION_HUNT *hunt = packet;
    unsigned short length;
    short count;
    if (!packet || size < header_size || size > USHRT_MAX)
        return 0;
    length = hunt->length;
    if (length != size || (size - header_size) % entry_size != 0)
        return 0;
    if (header_size == 6) {
        count = hunt->count;
        if (count < 0 || (size_t)count != (size - header_size) / entry_size)
            return 0;
    }
    return 1;
}

void game_mode_zc_update_mission_hunt_ex2(struct GameMode *self, const void *packet, size_t size)
{
    struct Vec lines = { 0, 0, 0 };
    const struct HuntUpdateEx2 *e;
    int count;
    struct RenewQuestB quest;
    const struct RenewQuestB *found;
    int n, slot;

    if (!hunt_packet_valid(packet, size, 6, sizeof *e))
        return;
    e = (const void *)((const struct PACKET_ZC_UPDATE_MISSION_HUNT *)packet)->entries;
    count = (int)((size - 6) / sizeof *e);

    for (n = 0; n < (int)count; n++, e++) {
        found = renew_quest_ongoing_info(g_questTable, e->quest_id);
        if (found->base.text[RENEW_QUEST_TEXT_TITLE].size == 0)
            continue;
        quest_hunt_copy(&quest, found);
        for (slot = 0; slot < QUEST_HUNT_SLOTS && slot < quest.plain.hunt_count; slot++) {
            if (quest.plain.hunt_id[slot] == e->hunt_id)
                break;
        }
        if (slot < QUEST_HUNT_SLOTS && slot < quest.plain.hunt_count)
            update_target(&lines, &quest, e->quest_id, slot, e->needed, e->hunted);
        quest_destroy(&quest);
    }
    finish(self, &lines);
}

void game_mode_zc_update_mission_hunt_ex(struct GameMode *self, const void *packet, size_t size)
{
    struct Vec lines = { 0, 0, 0 };
    const struct HuntUpdateEx *e;
    int count;
    struct RenewQuestB quest;
    const struct RenewQuestB *found;
    int n, slot;

    if (!hunt_packet_valid(packet, size, 6, sizeof *e))
        return;
    e = (const void *)((const struct PACKET_ZC_UPDATE_MISSION_HUNT *)packet)->entries;
    count = (int)((size - 6) / sizeof *e);

    for (n = 0; n < count; n++, e++) {
        found = renew_quest_ongoing_info(g_questTable, e->quest_id);
        if (found->base.text[RENEW_QUEST_TEXT_TITLE].size == 0)
            continue;
        quest_hunt_copy(&quest, found);
        for (slot = 0; slot < QUEST_HUNT_SLOTS && slot < quest.plain.hunt_count; slot++) {
            if (quest.plain.hunt_id[slot] == (__int64)(int)e->hunt_id)
                break;
        }
        if (slot < QUEST_HUNT_SLOTS && slot < quest.plain.hunt_count)
            update_target(&lines, &quest, e->quest_id, slot, e->needed, e->hunted);
        quest_destroy(&quest);
    }
    finish(self, &lines);
}

void game_mode_zc_hunting_quest_info(struct GameMode *self, const void *packet, size_t size)
{
    const struct HuntingQuestInfo *e;
    int count;
    struct RenewQuestB quest;
    const struct RenewQuestB *found;
    int n, slot;

    if (!hunt_packet_valid(packet, size, 4, sizeof *e))
        return;
    e = (const void *)((const struct PACKET_ZC_HUNTING_QUEST_INFO *)packet)->entries;
    count = (int)((size - 4) / sizeof *e);

    (void)self;
    for (n = 0; n < count; n++, e++) {
        found = renew_quest_ongoing_info(g_questTable, e->quest_id);
        if (found->base.text[RENEW_QUEST_TEXT_TITLE].size == 0)
            continue;
        quest_hunt_copy(&quest, found);
        /* entry n is looked for from slot n on: two objectives on the same monster id (the server sends one
         * id for every monster-type or map objective) each keep their own slot */
        for (slot = n; slot < QUEST_HUNT_SLOTS; slot++) {
            if (e->monster == quest.plain.hunt_mob[slot]) {
                quest.plain.hunt_done[slot] = e->hunted;
                quest.plain.hunt_goal[slot] = e->needed;
                quest_hunt_assign(quest_slot(e->quest_id), &quest);
                break;
            }
        }
        quest_destroy(&quest);
    }
    redraw_quest_windows();
}
