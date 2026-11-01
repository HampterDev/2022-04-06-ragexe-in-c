#include "game/packet_handlers.h"
#include "game/gamemode.h"
#include "ui/window_access.h"
#include "ui/chat/balloon.h"
#include "contents/client_lua_calls.h"
#include "game/own_states.h"
#include "session/state.h"
#include "core/memory.h"
#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include "game/tips.h"
#include "game/quest_info.h"
#include "game/client.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "session/session_items.h"
#include "ui/ui_std.h"
#include "ui/window.h"

#define STATE_ICON_NO_END 999999
#define EFST_MOVHASTE_INFINITY 0x2a

struct String *state_icon_get_time_left_text(struct GameMode *self, struct String *out, int icon,
                                                       int *kind)
{
    const struct OwnStateEntry *entry = NULL;
    size_t i;
    char part[100];
    int seconds, hours, minutes;

    (void)self;
    string_init(out);
    *kind = icon;
    for (i = 0; i < own_states_count(); ++i) {
        const struct OwnStateEntry *candidate = own_states_at(i);
        int listed_as = candidate->index == EFST_MOVHASTE_INFINITY ? 0xc : candidate->index;
        if (icon == listed_as) {
            entry = candidate;
            break;
        }
    }
    if (!entry)
        return out;
    *kind = entry->index;
    if (entry->end_tick == STATE_ICON_NO_END || entry->end_tick == 0)
        return out;

    {
        seconds = (int)(entry->end_tick / 1000 - timeGetTime() / 1000);
    }
    hours = seconds / 3600;
    if (hours) {
        sprintf(part, "%d %s", hours, msg_str(0x6a1));
        string_assign_cstr(out, part);
    }
    seconds -= hours * 3600;
    minutes = seconds / 60;
    seconds = seconds % 60;
    if (minutes) {
        sprintf(part, " %d %s", minutes, msg_str(0x70f));
        string_append_cstr(out, part);
    }
    if (seconds) {
        sprintf(part, " %d %s", seconds, msg_str(0x710));
        string_append_cstr(out, part);
    }
    return out;
}

#define ICON_TOP          0xa8
#define ICON_ROW_HEIGHT   0x23
#define ICON_COLUMN_WIDTH 0x2d
#define ICON_HELD_ITEM    0x2b7
#define ICON_OVERHEAT        0x175

#define TIP_SPACE " "

static char tip_is_empty(struct UITransBalloonText *tip)
{
    struct TextView first = ui_trans_balloon_text_get_line(tip, 0);
    return first.size == 1 && first.data[0] == ' ';
}

static void put_time_left(struct String *text, const struct String *time_left)
{
    struct String filled;

    string_init(&filled);
    string_format(&filled, string_cstr(text), string_cstr(time_left));
    string_assign(text, string_cstr(&filled), filled.size);
    string_free(&filled);
}

void game_mode_update_state_icon_tip(struct GameMode *self, int x, int y)
{
    struct UITransBalloonText *tip;
    int count, per_column, columns, right, from_right, column, column_edge, from_top, row, row_edge;
    int slot, icon, kind, has_limit, limit_line;
    struct String time_left, text;

    if (self->state_icon_tip)
        ui_window_set_show_base(&self->state_icon_tip->balloon.base, 0);
    if (own_states_count() == 0)
        return;

    count = (int)own_states_count();
    per_column = (g_renderer->height - 0xa9) / ICON_ROW_HEIGHT;
    columns = count / per_column + (count % per_column != 0);
    right = g_renderer->width - 0x12;
    if (right - columns * ICON_COLUMN_WIDTH + 0xf > x || x > right || y < ICON_TOP
        || y > per_column * ICON_ROW_HEIGHT + ICON_TOP) {
        self->tip_icon = -1;
        return;
    }
    from_right = right - x;
    column = from_right / ICON_COLUMN_WIDTH;
    column_edge = (column + (from_right != column * ICON_COLUMN_WIDTH)) * ICON_COLUMN_WIDTH;
    if (column_edge - 0xf < from_right && from_right < column_edge)
        return;
    from_top = y - ICON_TOP;
    row = from_top / ICON_ROW_HEIGHT;
    row_edge = (row + (from_top != row * ICON_ROW_HEIGHT)) * ICON_ROW_HEIGHT;
    if (row_edge - 5 < from_top && from_top < row_edge)
        return;
    slot = column * per_column + row;
    if (slot >= count)
        return;

    icon = g_session1.stateIconSlots[slot];
    if (icon == -1) {
        tip = self->state_icon_tip;
        self->tip_icon = -1;
        if (tip)
            ui_balloon_text_set_text(&tip->balloon, TIP_SPACE, 0x23);
        return;
    }

    has_limit = 0;
    limit_line = -1;
    client_lua_call(g_session1.luaState, "GetTimeLimitInfo", "d>dd", icon, &has_limit, &limit_line);
    string_init(&time_left);
    kind = icon;
    if (has_limit) {
        struct String left;

        string_free(&time_left);
        string_move(&time_left, state_icon_get_time_left_text(self, &left, icon, &kind));
        string_free(&left);
    }

    if (self->tip_icon == icon && self->state_icon_tip && icon != ICON_OVERHEAT && icon != ICON_HELD_ITEM) {
        if (ui_trans_balloon_text_get_line(self->state_icon_tip, 0).size)
            ui_window_set_show_base(&self->state_icon_tip->balloon.base, 1);
    } else {
        struct String description;

        if (!self->state_icon_tip) {
            tip = mem_alloc(sizeof(struct UITransBalloonText));
            tip = tip ? (struct UITransBalloonText *)ui_trans_balloon_text_construct(&tip->balloon.base) : 0;
            self->state_icon_tip = tip;
            ui_window_create((struct UIWindow *)tip, 0x32, 0x32);
            ui_window_mgr_add_window(&g_windowMgr, &self->state_icon_tip->balloon.base);
            ui_balloon_text_set_font_height(&self->state_icon_tip->balloon, 0xb);
            self->state_icon_tip->frame_colour = 0xc0c0c0;
        } else {
            ui_balloon_text_set_text(&self->state_icon_tip->balloon, TIP_SPACE, 0x23);
        }
        string_init(&description);

        if (icon == ICON_HELD_ITEM) {
            struct ItemInfo item;
            unsigned color = 0;

            session_get_inventory_item(&g_session1, &item, g_session1.wornArrowIndex);
            if ((unsigned)item.index > 0) {
                struct String line;
                struct String name = {0};

                item_info_get_display_name(&item, &name);
                string_init(&line);
                string_format(&line, "%s : %d", string_cstr(&name), item.count);
                string_assign(&description, string_cstr(&line), line.size);
                string_free(&line);
                string_free(&name);
                color = 0xfabebe;
            }
            item_info_destruct(&item);
            if (description.size)
                ui_trans_balloon_text_set_first_line(self->state_icon_tip, string_cstr(&description), color);
        } else {
            int line;

            for (line = 1;; line++) {
                const char *line_text = 0;
                int red = 0, green = 0, blue = 0;
                unsigned color;
                char first;

                while (!client_lua_call(g_session1.luaState, "GetStateIconDescript", "dd>sddd", kind, line,
                                   &line_text, &red, &green, &blue)) {
                    line_text = 0;
                    red = green = blue = 0;
                }
                string_init(&text);
                string_assign_cstr(&text, line_text);
                if (!text.size) {
                    string_free(&text);
                    break;
                }
                if (line == limit_line)
                    put_time_left(&text, &time_left);
                color = (unsigned char)blue << 16 | (unsigned char)green << 8 | (unsigned char)red;
                first = line == 1 || tip_is_empty(self->state_icon_tip);
                if (first)
                    ui_trans_balloon_text_set_first_line(self->state_icon_tip, string_cstr(&text), color);
                else
                    ui_trans_balloon_text_add_colored_line(self->state_icon_tip, string_cstr(&text), color);
                string_free(&text);
            }
            if (icon == ICON_OVERHEAT) {
                if (timeGetTime() - g_session1.overheatStart > 10000) {
                    g_session1.overheatValue = 0;
                } else if (g_session1.overheatValue) {
                    struct String note;

                    string_init(&note);
                    string_format(&note, msg_str(0xbd2), g_session1.overheatValue);
                    string_assign(&description, string_cstr(&note), note.size);
                    string_free(&note);
                }
                if (description.size)
                    ui_trans_balloon_text_add_colored_line(self->state_icon_tip, string_cstr(&description), 0);
            }
        }

        if (!tip_is_empty(self->state_icon_tip)) {
            int tip_x, tip_top, tip_y;

            ui_window_set_show_base(&self->state_icon_tip->balloon.base, 1);
            tip = self->state_icon_tip;
            tip_x = right - slot / per_column * ICON_COLUMN_WIDTH - tip->balloon.base.width - 0x1e;
            tip_top = slot % per_column * ICON_ROW_HEIGHT + 0xab;
            tip_y = tip->balloon.base.height + tip_top > g_renderer->height ? g_renderer->height - tip->balloon.base.height : tip_top;
            ui_trans_balloon_text_move(&tip->balloon.base, tip_x, tip_y);
        }
        self->tip_icon = icon;
        string_free(&description);
    }

    if (has_limit && time_left.size && limit_line != -1 && self->state_icon_tip) {
        const char *line_text = 0;

        if (client_lua_call(g_session1.luaState, "GetStateIconDescript", "dd>s", kind, limit_line, &line_text)) {
            string_init(&text);
            string_assign_cstr(&text, line_text);
            put_time_left(&text, &time_left);
            ui_trans_balloon_text_set_line(self->state_icon_tip, string_cstr(&text), limit_line - 1);
            string_free(&text);
        }
    }
    string_free(&time_left);
}

struct String *quest_hunt_get_line(struct String *out, const struct RenewQuestB *quest, int i)
{
    struct String part;
    const char *name = quest->plain.hunt_name[i];

    string_init(out);
    string_init(&part);
    if (strlen(name) == 0)
        renew_quest_condition_text(g_questTable, &part, quest->plain.hunt_flags[i], quest->plain.hunt_min_level[i], quest->plain.hunt_max_level[i]);
    else
        string_format(&part, "%s", name);
    string_assign(out, string_cstr(&part), part.size);
    string_free(&part);

    if (quest->plain.hunt_done[i] == quest->plain.hunt_goal[i])
        string_format(&part, "%s [ %s ]", string_cstr(out), msg_str(0x7ee));
    else
        string_format(&part, "%s [ %d / %d ]", string_cstr(out), quest->plain.hunt_done[i], quest->plain.hunt_goal[i]);
    string_assign(out, string_cstr(&part), part.size);
    string_free(&part);
    return out;
}

void game_mode_show_quest_hunt_list(struct GameMode *self, const struct Vec *lines)
{
    struct UITransBalloonText *list;
    const struct String *line;

    self->notice_shown_at = timeGetTime() - 15000;
    list = (struct UITransBalloonText *)self->notice_wnd;
    if (!list) {
        list = mem_alloc(sizeof(struct UITransBalloonText));
        list = list ? (struct UITransBalloonText *)ui_trans_balloon_text_construct(&list->balloon.base) : 0;
        self->notice_wnd = (struct UIWindow *)list;
        ui_window_create((struct UIWindow *)list, 8, 8);
        ui_window_mgr_add_to_window_list(&g_windowMgr, (struct UIWindow *)list);
    } else {
        text_array_resize(&list->balloon.lines, 0);
    }
    for (line = (const struct String *)lines->first; line != (const struct String *)lines->last; line++)
        ui_trans_balloon_text_add_line(list, string_cstr(line));

    ui_trans_balloon_text_move(&list->balloon.base, (g_renderer->width - 0x280) / 2 - list->balloon.base.width / 2 + 0x140, (g_renderer->height - 0x1e0) / 2 + 0x64);
    list->balloon.colour = 0xffff;
    list->balloon.outline = 0;
}
