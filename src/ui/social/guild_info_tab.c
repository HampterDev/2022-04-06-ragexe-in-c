#include "ui/social/guild_windows.h"
#include "contents/guild_info.h"
#include "ui/window_manager_state.h"
#include "game/context.h"
#include "core/markup_tags.h"
#include <stdio.h>
#include <string.h>
#include "ui/social/guild_manage.h"
#include "core/file_util.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "core/app.h"
#include "contents/guild_emblem_transport.h"

#define INFO_TAB_WND_ID (GUILD_TAB_WND_ID_FIRST + GUILD_TAB_INFO)

#define MSG_GUILD_NAME 0x148
#define MSG_GUILD_LEVEL 0x149
#define MSG_GUILD_MASTER 0x14a
#define MSG_GUILD_MEMBERS 0x14b
#define MSG_GUILD_AVG_LEVEL 0x14c
#define MSG_GUILD_CASTLES 0x14d
#define MSG_GUILD_EXP 0x14f
#define MSG_GUILD_EMBLEM 0x150
#define MSG_GUILD_ALLIES 0x152
#define MSG_GUILD_ENEMIES 0x153
#define GUILD_EXP_RED_FROM_LEVEL 0x32

#define EMBLEM_X 0x12c
#define EMBLEM_Y 0x41
#define EMBLEM_SIZE 0x18
#define MSG_EMBLEM_NO_FILES 0xbdd
#define MSG_EMBLEM_BAD 0xe03
#define MSG_EMBLEM_REFUSED 0xe02
#define MSG_DISBAND_STORAGE_WARNING 0xa04
#define WND_ID_DISBAND_GUILD 0xd7

#define RELATION_X 0xcc
#define RELATION_ROW 0x10
#define RELATION_ROWS_MAX 3
#define ALLY_TOP 0x9d
#define ALLY_BOTTOM 0xcd
#define ENEMY_TOP 0xe7
#define ENEMY_BOTTOM 0x117
#define MENU_END_RELATION 0x1b

#define AGIT_MSG_ROW_MENU 0xa2
#define AGIT_MSG_PRESSED 0xa3

struct String *guild_name_cut_nameless(struct String *out, const struct String *name)
{
    unsigned pos;

    string_copy(out, name);
    pos = text_view_find(string_view(name), string_view(&text_tag_NAMELESS), 0);
    if (pos != (unsigned)-1) {
        struct String head;

        string_clear(out);
        string_init(&head);
        string_assign(&head, string_cstr(name), pos < name->size ? pos : name->size);
        string_free(out);
        *out = head;
    }
    return out;
}

static struct EmblemRes *emblem_of(int guild_id, struct String *path)
{
    struct String name;

    guild_mgr_emblem_file_name(g_guildMgr, path, guild_id);
    string_copy(&name, path);
    return res_mgr_get_string(g_resMgr(), name);
}

static void hide_agit_menu(struct UIGuildInfoManageWnd *self)
{
    if (self->agit_menu && self->agit_menu->list.base.visible) {
        ui_window_set_show_base(&self->agit_menu->list.base, 0);
        self->agit_menu->agit_index = -1;
        ui_window_invalidate_base(&self->base.frame.base);
    }
}

static void show_agit_menu(struct UIGuildInfoManageWnd *self, int x, int y)
{
    if (!self->agit_menu)
        return;
    ui_window_move_base(&self->agit_menu->list.base, x, y);
    ui_window_set_show_base(&self->agit_menu->list.base, 1);
    self->agit_menu->agit_index = self->agit_list->selected;
    ui_window_invalidate_base(&self->base.frame.base);
}

static void text_at(struct UIWindow *wnd, int x, int y, const char *text)
{
    ui_window_text_out(wnd, x, y, text, 0, 0, 12, 0, 0, 0);
}

static void draw_relations(struct UIWindow *wnd, int kind, int panel_y, int top, int bottom, int selected)
{
    struct ListNode *node;
    int y = top, i = 0;

    ui_window_fill_rect(wnd, 0xc8, panel_y, 0xa8, 0x30, ui_skin_colour(0xe, 6));
    for (node = g_guildInfo.relations.head->next; node != g_guildInfo.relations.head && y < bottom;
         node = node->next) {
        if (LIST_VALUE(node, struct GuildRelation)->kind != kind)
            continue;
        if (i == selected)
            ui_window_fill_rect(wnd, RELATION_X, y, 0x50, RELATION_ROW, ui_skin_colour(6, 2));
        text_at(wnd, RELATION_X, y, string_cstr(&LIST_VALUE(node, struct GuildRelation)->name));
        i++;
        y += RELATION_ROW;
    }
}

void ui_guild_info_manage_wnd_on_draw(struct UIWindow *window)
{
    struct UIGuildInfoManageWnd *self = (struct UIGuildInfoManageWnd *)window;
    struct UIWindow *wnd = GUILD_WND(self);
    struct GuildInfo *g = &g_guildInfo;
    struct String text;
    struct BitmapRes *online;
    struct EmblemRes *emblem;
    char line[0x400];
    int width;

    ui_guild_wnd_on_draw((struct UIWindow *)(&self->base));
    sprintf(line, "%s : %s", msg_str(MSG_GUILD_NAME), string_cstr(&g->name));
    text_at(wnd, 8, 0x31, line);
    sprintf(line, "%s : %d", msg_str(MSG_GUILD_LEVEL), g->level);
    text_at(wnd, 8, 0x41, line);
    guild_name_cut_nameless(&text, &g_guildInfo.master_name);
    sprintf(line, "%s : %s", msg_str(MSG_GUILD_MASTER), string_cstr(&text));
    text_at(wnd, 8, 0x51, line);
    string_free(&text);
    sprintf(line, "%s : %d / %d", msg_str(MSG_GUILD_MEMBERS), g->member_count, g->max_members);
    text_at(wnd, 8, 0x61, line);
    width = ui_window_text_width(wnd, line, 0, 0, 12, 0, 0);
    sprintf(line, "%s", UI_SKIN "basic_interface\\grp_online.bmp");
    online = ui_skin_bitmap(line);
    if (g_serviceType == 0) {
        ui_window_draw_bitmap(wnd, 0x80, 0x5f, online, 1);
        sprintf(line, "   %d", g->online_count);
        text_at(wnd, 0x8a, 0x61, line);
    } else {
        ui_window_draw_bitmap(wnd, width + 0x10, 0x5f, online, 1);
        sprintf(line, " %d", g->online_count);
        (void)ui_window_text_width(wnd, msg_str(MSG_GUILD_MASTER), 0, 0, 12, 0, 0);
        text_at(wnd, online->width + 0x10 + width, 0x61, line);
    }
    sprintf(line, "%s : %d", msg_str(MSG_GUILD_AVG_LEVEL), g->average_level);
    text_at(wnd, 8, 0x71, line);
    sprintf(line, "%s : %s", msg_str(MSG_GUILD_CASTLES), string_cstr(&g->castle_name));
    text_at(wnd, 8, 0x81, line);
    sprintf(line, "%s : %d", msg_str(MSG_GUILD_EXP), g->exp);
    ui_window_text_out(wnd, 0xc8, 0x31, line, 0, 0, 12, g->level >= GUILD_EXP_RED_FROM_LEVEL ? 0xff : 0, 0, 0);
    sprintf(line, "%s", msg_str(MSG_GUILD_EMBLEM));
    text_at(wnd, 0xc8, 0x49, line);
    sprintf(line, "%s", msg_str(MSG_GUILD_ALLIES));
    text_at(wnd, 0xc8, 0x8b, line);
    sprintf(line, "%s", msg_str(MSG_GUILD_ENEMIES));
    text_at(wnd, 0xc8, 0xd5, line);
    ui_window_fill_rect(wnd, EMBLEM_X - 1, EMBLEM_Y - 1, 0x1a, 0x1a, ui_skin_colour(6, 2));
    emblem = emblem_of(self->guild_id, &text);
    if (emblem)
        ui_window_draw_bitmap(wnd, EMBLEM_X, EMBLEM_Y, &emblem->picture.bmp, 1);
    string_free(&text);
    draw_relations(wnd, GUILD_RELATION_ALLY, 0x9b, ALLY_TOP, ALLY_BOTTOM, self->ally_selected);
    draw_relations(wnd, GUILD_RELATION_ENEMY, 0xe5, ENEMY_TOP, ENEMY_BOTTOM, self->enemy_selected);
}

void ui_guild_info_manage_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIGuildInfoManageWnd *self = (struct UIGuildInfoManageWnd *)window;
    ui_guild_wnd_on_l_btn_down((struct UIWindow *)(&self->base), x, y);
    hide_agit_menu(self);
}

void ui_guild_info_manage_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIGuildInfoManageWnd *self = (struct UIGuildInfoManageWnd *)window;
    hide_agit_menu(self);
}

void ui_guild_info_manage_wnd_on_r_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIGuildInfoManageWnd *self = (struct UIGuildInfoManageWnd *)window;
    int count;

    if ((unsigned)(x - 0xc9) > 0x56)
        return;
    if (y > ALLY_TOP && y < ALLY_BOTTOM) {
        count = guild_info_count_relations(&g_guildInfo, GUILD_RELATION_ALLY);
        self->ally_selected = -1;
        self->enemy_selected = -1;
        if (count) {
            self->ally_selected = (y - ALLY_TOP) / RELATION_ROW;
            if (count > self->ally_selected)
                ui_guild_info_manage_wnd_send_msg(&self->base.frame.base, NULL, GUILD_MSG_OPEN_MENU, 0, 0, 0, 0);
        }
    } else if (y > ENEMY_TOP && y < ENEMY_BOTTOM) {
        count = guild_info_count_relations(&g_guildInfo, GUILD_RELATION_ENEMY);
        self->ally_selected = -1;
        self->enemy_selected = -1;
        if (count) {
            self->enemy_selected = (y - ENEMY_TOP) / RELATION_ROW;
            if (count > self->enemy_selected)
                ui_guild_info_manage_wnd_send_msg(&self->base.frame.base, NULL, GUILD_MSG_OPEN_MENU, 0, 0, 0, 0);
        }
    } else {
        return;
    }
    ui_window_invalidate_base(&self->base.frame.base);
}

void ui_guild_info_manage_wnd_on_process(struct UIWindow *window)
{
    struct UIGuildInfoManageWnd *self = (struct UIGuildInfoManageWnd *)window;
    struct String path;
    struct EmblemRes *emblem = emblem_of(self->guild_id, &path);

    if (emblem && emblem->picture.changed == 1)
        ui_window_invalidate_base(&self->base.frame.base);
    string_free(&path);
}

static void destroy_string(void *value) { string_free(value); }

static void list_emblem_files(struct UIGuildInfoManageWnd *self)
{
    char folder[0x100];

    list_clear(&self->emblem_files, destroy_string);
    sprintf(folder, "%s\\emblem\\", g_clientDir);
    dir_list_names_list(&self->emblem_files, folder, "*.bmp");
    dir_list_names_list(&self->emblem_files, folder, "*.gif");
}

static void browse_emblems(struct UIGuildInfoManageWnd *self)
{
    struct ListNode *node;
    struct UIWindow *combo;
    unsigned count;
    int height, x, y;

    if (g_windowMgr.combo_box_wnd)
        ui_close_window(WND_ID_COMBO);
    list_emblem_files(self);
    count = self->emblem_files.size;
    if (!count) {
        ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_EMBLEM_NO_FILES), UI_MSGBOX_OK, 1, 0, 0, "Caution",
                               0x118, 0x78, 0);
        return;
    }
    combo = ui_make_window(WND_ID_COMBO);
    for (node = self->emblem_files.head->next; node != self->emblem_files.head; node = node->next)
        ui_window_send_msg(combo, NULL, COMBO_MSG_ADD_LINE, (intptr_t)string_cstr(LIST_VALUE(node, struct String)),
                           0, 0, 0);
    height = (int)(count << 4);
    if (height > 0xa0)
        height = 0xa0;
    ui_window_send_msg(combo, NULL, COMBO_MSG_SELECT, 0, 0, 0, 0);
    ui_window_resize(combo, 0xa0, height);
    ui_window_send_msg(combo, NULL, COMBO_MSG_OPENER_ID, INFO_TAB_WND_ID, 0, 0, 0);
    x = GUILD_WND(self)->width - 0xa3;
    y = 0x56;
    ui_window_to_screen(GUILD_WND(self), &x, &y);
    ui_window_move(combo, x + 1, y + 1);
}

static void emblem_chosen(struct UIGuildInfoManageWnd *self, int index)
{
    struct String name;
    struct ListNode *node;
    int i = 0;

    string_init(&name);
    for (node = self->emblem_files.head->next; node != self->emblem_files.head; node = node->next, i++) {
        if (index == i) {
            string_assign_cstr(&name, string_cstr(LIST_VALUE(node, struct String)));
            break;
        }
    }
    if (!name.size) {
        string_free(&name);
        return;
    }
    {
        struct File file;
        struct String file_name;
        BYTE opened;

        file_construct(&file);
        string_init(&file_name);
        string_format(&file_name, "..\\emblem\\%s", string_cstr(&name));
        opened = file_open(&file, string_cstr(&file_name), 0, 1, 0);
        string_free(&file_name);
        if (opened == 1) {
            if (g_guildMgr->emblem_max_size < (unsigned)file.size) {
                ui_message_box(msg_str(MSG_EMBLEM_BAD));
            } else {
                if (!guild_emblem_upload(file.buffer, (size_t)file.size))
                    ui_message_box(msg_str(MSG_EMBLEM_REFUSED));
            }
        }
        file_destruct(&file);
    }
    string_free(&name);
}

static void menu_choice(struct UIGuildInfoManageWnd *self, int choice)
{
    struct Mode *mode = game_current_base_mode();
    struct ListNode *node;
    int kind, selected, i = 0;

    if (VEC_AT(&self->menu, struct UIMenuEntry, choice).id != MENU_END_RELATION)
        return;
    if (self->ally_selected != -1) {
        kind = GUILD_RELATION_ALLY;
        selected = self->ally_selected;
    } else if (self->enemy_selected != -1) {
        kind = GUILD_RELATION_ENEMY;
        selected = self->enemy_selected;
    } else {
        return;
    }
    for (node = g_guildInfo.relations.head->next; node != g_guildInfo.relations.head; node = node->next) {
        if (i >= RELATION_ROWS_MAX)
            return;
        if (LIST_VALUE(node, struct GuildRelation)->kind != kind)
            continue;
        if (i == selected) {
            if (LIST_VALUE(node, struct GuildRelation)->id)
                mode_send_message(mode, MODE_REQ_GUILD_RELATION_END, LIST_VALUE(node, struct GuildRelation)->id, kind, 0, 0);
            return;
        }
        i++;
    }
}

static void open_menu(struct UIGuildInfoManageWnd *self)
{
    ui_menu_entry_vec_clear(&self->menu);
    if (g_windowMgr.menu_wnd)
        ui_close_window(WND_ID_MENU);
    if (g_guildInfo.is_master) {
        struct UIMenuEntry entry;

        ui_menu_entry_construct(&entry, msg_str(0x15f), MENU_END_RELATION);
        ui_menu_entry_vec_push_move(&self->menu, &entry);
        string_free(&entry.text);
    }
    guild_open_menu(&self->menu, INFO_TAB_WND_ID);
}

static void refresh(struct UIGuildInfoManageWnd *self)
{
    struct UIWindow *wnd = GUILD_WND(self);

    self->guild_id = g_guildInfo.guild_id;
    if (g_guildInfo.is_master) {
        ui_window_move_base(&self->emblem_button->button.base, wnd->width - self->emblem_button->button.base.width - 8, 0x45);
        ui_window_move_base(&self->disband_button->button.base, wnd->width - self->disband_button->button.base.width - 0xf,
                 wnd->height - self->disband_button->button.base.height - 5);
    } else {
        ui_window_move_base(&self->emblem_button->button.base, GUILD_OFF_SCREEN, GUILD_OFF_SCREEN);
        ui_window_move_base(&self->disband_button->button.base, GUILD_OFF_SCREEN, GUILD_OFF_SCREEN);
    }
    ui_guild_info_manage_wnd_fill_castles(self);
    ui_window_invalidate_children(wnd);
}

intptr_t ui_guild_info_manage_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                            intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIGuildInfoManageWnd *self = (struct UIGuildInfoManageWnd *)window;
    switch (msg) {
    case UI_MSG_COMMAND:
        switch (val1) {
        case UI_ID_CLOSE:
            ui_close_window(GUILD_WND_ID);
            break;
        case GUILD_ID_EMBLEM_BROWSE:
            browse_emblems(self);
            break;
        case GUILD_INFO_ID_DISBAND:
            ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_DISBAND_STORAGE_WARNING), UI_MSGBOX_OK, 0, 0, 0, "메시지", 0x118,
                                   0x78, 0);
            ui_make_window(WND_ID_DISBAND_GUILD);
            break;
        }
        return 0;
    case 0xd:
        return 0;
    case COMBO_MSG_CHOSEN:
        if (self->emblem_files.size)
            emblem_chosen(self, val1);
        return 0;
    case GUILD_MSG_MENU_CHOICE:
        menu_choice(self, val1);
        return 0;
    case GUILD_MSG_OPEN_MENU:
        open_menu(self);
        return 0;
    case GUILD_MSG_REFRESH:
        refresh(self);
        return 0;
    case AGIT_MSG_ROW_MENU:
        show_agit_menu(self, val1, val2);
        return 0;
    case AGIT_MSG_PRESSED:
        hide_agit_menu(self);
        return 0;
    }
    return ui_guild_wnd_send_msg((struct UIWindow *)(&self->base), sender, msg, val1, val2, val3, val4);
}
