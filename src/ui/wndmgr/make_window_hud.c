#include "navi/navi_manager.h"
#include "ui/misc/snap_mgr.h"
#include "session/state.h"
#include "contents/mouse.h"
#include "ui/window_access.h"
#include "ui/wndmgr/make_window_internal.h"
#include "ui/wndmgr/window_registry.h"
#include "world/mode_manager.h"
#include "core/object_type_tables.h"
#include "ui/character/character.h"
#include "ui/character/popup_menu.h"
#include "ui/chat/chat.h"
#include "ui/chat/chat_window.h"
#include "ui/chat/sub_chat.h"
#include "ui/hotkeys/hotkey_bars.h"
#include "ui/hotkeys/hotkey_guide.h"
#include "ui/items/items.h"
#include "ui/map/map_std.h"
#include "ui/quests/navigation.h"
#include "ui/quests/npc_input.h"
#include "ui/quests/npc_menu.h"
#include "ui/quests/quests.h"
#include "ui/quests/say_dialog.h"
#include "ui/render/character_render.h"
#include "ui/render/render_std.h"
#include "ui/social/social.h"
#include "ui/worldmap/romap.h"

/* 0x1 */
static struct UIWindow *make_new_chat_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->new_chat_wnd;
    if (wnd == NULL) {
        wnd = ui_new_chat_wnd_new();
        self->new_chat_wnd = wnd;
        ui_window_create(wnd, 0x258, self->chat_wnd_height);
        ui_window_mgr_add_window(self, wnd);
    }
    wm_set_info(self->new_chat_wnd, NULL);
    return wnd;
}

/* 0x1: post_quit1 */
static int close_new_chat_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    ui_window_mgr_post_quit1(self, 0x1a);
    if (wm_close_slot(self, &self->new_chat_wnd)) {
        self->chat_wnd_show = 0;
        closed = 1;
    }
    return closed;
}

/* 0xe */
static struct UIWindow *make_minimap_zoom_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->minimap_zoom_wnd;
    if (wnd == NULL) {
        wnd = ui_minimap_zoom_wnd_new();
        self->minimap_zoom_wnd = wnd;
        ui_window_create(wnd, 0x80, 0x8c);
        wm_move(wnd, g_renderer->width - 0x91, 0x11);
        ui_window_mgr_add_window(self, wnd);
    }
    wm_show(wnd);
    return wnd;
}

/* 0x11 */
static struct UIWindow *make_choose_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->choose_wnd;
    if (wnd == NULL) {
        wnd = ui_choose_wnd_new();
        self->choose_wnd = wnd;
        ui_window_create(wnd, 0x118, 0x78);
        wm_move_640(wnd, 0xc8, 0x12c);
        ui_window_mgr_add_window(self, wnd);
        wm_set_info(self->choose_wnd, &self->choose_info);
    }
    wm_show(wnd);
    return wnd;
}

/* 0x15, 0x8f */
static struct UIWindow *make_notify_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = window_id == 0x15 ? self->notify_level_up_wnd : self->notify_quest_wnd;
    if (wnd == NULL) {
        if (window_id == 0x15) {
            wnd = (struct UIWindow *)ui_notify_level_up_wnd_new();
            self->notify_level_up_wnd = wnd;
        } else {
            wnd = ui_notify_quest_wnd_new();
            self->notify_quest_wnd = wnd;
        }
        ui_window_create(wnd, 0x2b, 0x2b);
        wm_move(wnd, g_renderer->width - 0x2b, g_renderer->height - 0x2b);
        ui_window_mgr_add_window(self, wnd);
    }
    wm_show(wnd);
    return wnd;
}

/* 0x24 */
static struct UIWindow *make_short_cut_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->short_cut_wnd;
    if (wnd)
        return wnd;
    wnd = ui_short_cut_wnd_new();
    self->short_cut_wnd = wnd;
    ui_window_create(wnd, 0x122, 0x22);
    wm_move_640(wnd, 0xc8, 0xc8);
    ui_window_mgr_add_window(self, wnd);
    wm_set_info(self->short_cut_wnd, &self->shortcut_info);
    snap_mgr_register(&g_snapMgr, wnd);
    return wnd;
}

/* 0x31 */
static struct UIWindow *make_notify_job_level_up_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->notify_job_level_up_wnd;
    if (wnd == NULL) {
        wnd = ui_notify_job_level_up_wnd_new();
        self->notify_job_level_up_wnd = wnd;
        ui_window_create(wnd, 0x2b, 0x2b);
        wm_move(wnd, 0, g_renderer->height - 0x2b);
        ui_window_mgr_add_window(self, wnd);
    }
    wm_show(wnd);
    return wnd;
}

/* 0x38 */
static struct UIWindow *make_npc_edit_dialog_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->npc_edit_dialog_wnd;
    if (wnd == NULL) {
        wnd = ui_npc_edit_dialog_wnd_new();
        self->npc_edit_dialog_wnd = wnd;
        ui_window_create(wnd, 0xb0, 0x2e);
        wm_move_640(wnd, 0xc8, 0xc8);
        ui_window_mgr_add_window(self, wnd);
    }
    wm_show(wnd);
    return wnd;
}

/* 0x4e */
static struct UIWindow *make_illust_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->illust_wnd;
    if (wnd) {
        ui_window_send_msg(wnd, NULL, UI_MSG_SET_INFO, -1, 0, 0, 0);
        wm_show(wnd);
        return wnd;
    }
    wnd = ui_illust_wnd_new();
    self->illust_wnd = wnd;
    ui_window_create(wnd, 4, 4);
    wm_move_640(wnd, 0xc8, 0x78);
    list_push_front(&self->windows, &wnd, sizeof wnd);
    wm_window_list_sort(&self->windows);
    wm_show(wnd);
    return wnd;
}

/* 0x54 */
static struct UIWindow *make_talkbox_trap_input_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = ui_talkbox_trap_input_wnd_new();
    ui_window_create(wnd, 0x118, 0x78);
    wm_move_640_up(wnd, 0xb9, 0x12c, 0x3c);
    ui_window_mgr_add_window(self, wnd);
    return wnd;
}

/* 0x56 */
static struct UIWindow *make_emotion_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->emotion_wnd;
    if (wnd == NULL) {
        wnd = ui_emotion_wnd_new();
        self->emotion_wnd = wnd;
        ui_window_create(wnd, 0x177, 0x104);
        wm_move_640_up(wnd, 0xb9, 0x12c, 0x3c);
        wm_set_info(wnd, &self->emotion_info);
        ui_window_mgr_add_window(self, wnd);
    }
    wm_show(wnd);
    return wnd;
}

/* 0x57 */
static struct UIWindow *make_emotion_list_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->emotion_list_wnd;
    if (wnd == NULL) {
        wnd = ui_emotion_list_wnd_new();
        self->emotion_list_wnd = wnd;
        ui_window_create(wnd, 0xf6, 0x14a);
        wm_move_640_up(wnd, 0xb9, 0x190, 0x3c);
        wm_set_info(wnd, &self->emotion_list_info);
        ui_window_mgr_add_window(self, wnd);
    }
    wm_show(wnd);
    return wnd;
}

/* 0x64 */
static struct UIWindow *make_npc_text_edit_dialog_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->npc_text_edit_dialog_wnd;
    if (wnd == NULL) {
        wnd = ui_npc_text_edit_dialog_wnd_new();
        self->npc_text_edit_dialog_wnd = wnd;
        ui_window_create(wnd, 0x15a, 0x2e);
        wm_move_640(wnd, 0xc8, 0xc8);
        ui_window_mgr_add_window(self, wnd);
    }
    wm_show(wnd);
    return wnd;
}

/* 0x65 */
static struct UIWindow *make_graffi_strbox_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = ui_graffi_strbox_wnd_new();
    ui_window_create(wnd, 0x118, 0x78);
    wm_move_640_up(wnd, 0xb9, 0x12c, 0x3c);
    ui_window_mgr_add_window(self, wnd);
    return wnd;
}

/* 0x89 */
static struct UIWindow *make_memorial_dun_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    if (string_equal_cstr(&g_session1.mdName, ""))
        return NULL;
    wnd = self->memorial_dun_wnd;
    if (wnd == NULL) {
        wnd = ui_memorial_dun_wnd_new();
        self->memorial_dun_wnd = wnd;
        ui_window_create(wnd, 0xc8, 0x96);
        if (g_session1.mdWndX >= 0 && g_session1.mdWndY >= 0)
            wm_move(wnd, g_session1.mdWndX, g_session1.mdWndY);
        else
            wm_move(wnd, 0, 0x136);
        ui_window_mgr_add_window(self, wnd);
    }
    wm_show(wnd);
    g_session1.mdWndShown = 1;
    return wnd;
}

/* 0x89: post_quit1 */
static int close_memorial_dun_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    if (self->memorial_dun_wnd) {
        memorial_dun_info_reset();
        ui_window_mgr_post_quit2(self, self->memorial_dun_wnd);
        self->memorial_dun_wnd = NULL;
        g_session1.mdWndShown = 0;
        closed = 1;
    }
    return closed;
}

/* 0x8c */
static struct UIWindow *make_kafra_move_map_service_info_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->kafra_move_map_service_info_wnd;
    if (wnd == NULL) {
        wnd = ui_ro_map_wnd_new();
        self->kafra_move_map_service_info_wnd = wnd;
        ui_window_create(wnd, g_renderer->width, g_renderer->height);
        wm_move(wnd, 0, 0);
        ui_window_mgr_add_window(self, wnd);
    }
    wm_show(wnd);
    ui_window_mgr_raise_window(&g_windowMgr, self->kafra_move_map_service_info_wnd);
    ui_window_mgr_raise_window(&g_windowMgr, self->navigation_v4_wnd);
    return wnd;
}

/* 0x8c: post_quit1 */
static int close_kafra_move_map_service_info_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    if (wm_close_slot(self, &self->kafra_move_map_service_info_wnd)) {
        closed = 1;
        if (self->navigation_v4_wnd) {
            g_navi.windowHidden = 0;
            ui_navigation_v4_wnd_relayout((struct UINavigationV4Wnd *)self->navigation_v4_wnd);
        }
    }
    return closed;
}

/* 0xcb */
static struct UIWindow *make_navigation_v4_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    if (self->navigation_v4_wnd)
        return NULL;

    g_navi.windowHidden = 0;
    wnd = ui_navigation_v4_wnd_new();
    self->navigation_v4_wnd = wnd;
    ui_window_create(wnd, 0x110, 0x152);
    wm_set_info(self->navigation_v4_wnd, &self->navigation_info);
    ui_window_mgr_add_window(self, wnd);
    return wnd;
}

/* 0xcc */
static struct UIWindow *make_quest_display_wnd(struct UIWindowMgr *self, int window_id)
{
    if (self->quest_display_wnd)
        return NULL;
    self->quest_display_wnd = ui_quest_display_new();
    ui_quest_display_create((struct UIQuestDisplay *)self->quest_display_wnd, 0xfa, 0x190);
    wm_move(self->quest_display_wnd, self->screen_width - 0x140, 0x96);
    self->quest_display_wnd->list_rank = 5;
    ui_window_mgr_add_window(self, self->quest_display_wnd);
    wm_window_list_sort(&self->windows);
    wm_set_info(self->quest_display_wnd, &self->quest_display);
    return NULL;
}

/* 0xe5 */
static struct UIWindow *make_navigation_help_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    if (ui_window_mgr_get_window(self, 0xe5))
        return NULL;
    wnd = ui_navigation_help_wnd_new();
    ui_window_create(wnd, 0x136, 0xc8);
    wm_move(wnd, 0x64, 0x64);
    ui_window_mgr_add_window(self, wnd);
    return wnd;
}

/* 0x111 */
static struct UIWindow *make_mini_map_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    if (self->mini_map_wnd)
        return NULL;
    wnd = ui_mini_map_wnd_new();
    self->mini_map_wnd = wnd;
    ui_window_create(wnd, self->minimap_info.width, self->minimap_info.height);
    wm_set_info(wnd, &self->minimap_info);
    wm_show(wnd);
    ui_window_mgr_add_window(self, wnd);
    return wnd;
}

/* 0x114 */
static struct UIWindow *make_round_box_tooltip_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    if (ui_window_mgr_get_window(self, 0x114)) {
        wnd = self->round_box_tooltip_wnd;
        wm_show(wnd);
        return wnd;
    }
    wnd = ui_round_box_tooltip_wnd_new();
    self->round_box_tooltip_wnd = wnd;
    ui_window_create(wnd, 0x64, 0xc8);
    wm_move_640_up(wnd, 0xb9, 0x12c, 0x78);
    ui_window_mgr_add_window(self, wnd);
    wm_show(wnd);
    return wnd;
}

/* 0x114: post_quit1 */
static int close_round_box_tooltip_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    if (wm_close_slot(self, &self->round_box_tooltip_wnd)) {
        self->round_box_tooltip_show = 0;
        closed = 1;
    }
    return closed;
}

/* 0x121 */
static struct UIWindow *make_view_camera_info_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = ui_window_mgr_get_window(self, 0x121);
    if (wnd == NULL) {
        int x = g_renderer->width - 0xc8, y = 0xc8;

        wnd = ui_view_camera_info_wnd_new();
        ui_window_create(wnd, 0x96, 0x64);
        if (self->mini_map_wnd) {
            x = self->mini_map_wnd->x;
            y = self->mini_map_wnd->height + 0x14 + self->mini_map_wnd->y;
        }
        wm_move(wnd, x, y);
        wnd->list_rank = 1;
        ui_window_mgr_add_window(self, wnd);
        wm_window_list_sort(&self->windows);
    }
    ui_window_set_show(wnd, g_session1.cameraInfoShown);
    return wnd;
}

/* 0x12f */
static struct UIWindow *make_hotkey_guide_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = ui_window_mgr_get_window(self, 0x12f);
    if (wnd)
        return wnd;
    wnd = ui_hotkey_guide_wnd_new();
    ui_window_create(wnd, 0x3a6, 0x1f2);
    wm_move(wnd, wm_center_x(0x3a6), wm_center_y(0x1f2));
    ui_window_mgr_add_window(self, wnd);
    return wnd;
}

/* 0x131 */
static struct UIWindow *make_graphic_setting_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = ui_window_mgr_get_window(self, 0x131);
    if (wnd == NULL) {
        wnd = ui_graphic_setting_wnd_new();
        ui_window_create(wnd, 0x190, 0x15e);
        wm_move_640(wnd, 0xc8, 0xc8);
        ui_window_mgr_add_window(self, wnd);
        wm_set_info(wnd, &self->graphic_setting_info);
    }
    wm_show(wnd);
    return wnd;
}

/* 0x131: post_quit1 */
static int close_graphic_setting_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd;
    int closed = 0;

    wnd = ui_window_mgr_get_window(self, 0x131);
    if (wnd) {
        if (g_windowMgr.combo_box_wnd)
            ui_window_mgr_post_quit1(&g_windowMgr, 0x1a);
        ui_window_mgr_post_quit2(self, wnd);
        closed = 1;
    }
    return closed;
}

/* 0x132, 0x13a */
static struct UIWindow *make_navigation_side_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;
    int x = 0xc8, y = 0xc8;

    if (ui_window_mgr_get_window(self, window_id))
        return NULL;
    if (self->navigation_v4_wnd) {
        x = self->navigation_v4_wnd->x;
        y = self->navigation_v4_wnd->y;
    }
    if (window_id == 0x132) {
        wnd = ui_navigationroadicon_wnd_new();
        ui_window_create(wnd, 0x9e, 0x66);
        wm_move(wnd, x + 0x71, y + 0xd3);
    } else {
        wnd = ui_navigation_ruide_wnd_new();
        ui_window_create(wnd, 0x96, 0x152);
        wm_move(wnd, x + 0x110, y);
    }
    ui_window_mgr_add_window(self, wnd);
    return wnd;
}

/* 0x133 */
static struct UIWindow *make_menu_icon_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = ui_window_mgr_get_window(self, 0x133);
    if (wnd)
        return wnd;
    wnd = ui_menu_icon_wnd_new();
    ui_window_create(wnd, 0xdc, 0xc5);
    wm_move(wnd, 0, 0xe1);
    ui_window_mgr_add_window(self, wnd);
    return wnd;
}

/* 0x134 */
static struct UIWindow *make_show_warp_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = ui_window_mgr_get_window(self, 0x134);
    if (wnd == NULL) {
        wnd = ui_show_warp_wnd_new();
        ui_window_create(wnd, 0x118, 0x78);
        wm_move_640(wnd, 0xc8, 0x12c);
        ui_window_mgr_add_window(self, wnd);
    }
    wm_show(wnd);
    return wnd;
}

/* 0x141 */
static struct UIWindow *make_renew_quest_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = ui_window_mgr_get_window(self, 0x141);
    if (wnd == NULL) {
        wnd = ui_renew_quest_wnd_new();
        ui_window_create(wnd, 0x17d, 0x1d2);
        wm_move(wnd, wm_center_x(0x17d), wm_center_y(0x1d2));
        ui_window_mgr_add_window(self, wnd);
        wm_set_info(wnd, &self->quest_info);
        snap_mgr_register(&g_snapMgr, wnd);
    }
    wm_show(wnd);
    ui_window_mgr_post_quit1(self, 0x8f);
    self->quest_info.show = 1;
    return wnd;
}

/* 0x141: post_quit1 */
static int close_renew_quest_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd;
    int closed = 0;

    wnd = ui_window_mgr_get_window(self, 0x141);
    if (wnd) {
        ui_window_mgr_post_quit2(self, wnd);
        self->quest_info.show = 0;
        closed = 1;
    }
    return closed;
}

/* 0x142 */
static struct UIWindow *make_renew_detail_quest_info_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = ui_window_mgr_get_window(self, 0x142);
    if (wnd == NULL) {
        struct UIWindow *quest;

        wnd = ui_renew_detail_quest_info_wnd_new();
        ui_window_create(wnd, 0x156, 0x19c);
        quest = ui_window_mgr_get_window(self, 0x141);
        if (quest)
            wm_move(wnd, quest->x + quest->width, quest->y);
        else
            wm_move(wnd, wm_center_x(0x156), wm_center_y(0x19c));
        ui_window_mgr_add_window(self, wnd);
        snap_mgr_register(&g_snapMgr, wnd);
    }
    wm_show(wnd);
    return wnd;
}

/* 0x148 */
static struct UIWindow *make_check_attendance_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = ui_window_mgr_get_window(self, 0x148);
    if (wnd)
        return wnd;
    wnd = ui_check_attendance_wnd_new();
    ui_window_create(wnd, 0x1e8, 0x19d);
    {
        int y = wm_times_half(g_renderer->height - 0x19d);

        wm_move(wnd, wm_times_half(g_renderer->width - 0x1e8), y);
    }
    ui_window_mgr_add_window(self, wnd);
    return wnd;
}

/* The rows of the chat, NPC dialog, notification, navigation, map and quest windows.
   window_table.h describes the columns. In ID order; slots that no window ID opens
   come last. */
static const struct WmWindowRow k_hud_rows[] = {
    WM_ROW(0x1, WM_SLOT(new_chat_wnd), WM_UNTRACKED | WM_GET_SLOT, make_new_chat_wnd, close_new_chat_wnd),
    WM_ROW(0xe, WM_SLOT(minimap_zoom_wnd), WM_UNTRACKED, make_minimap_zoom_wnd, WM_CLOSE_SLOT),
    WM_ROW_OPEN_INFO(0x10, say_dialog_wnd, WM_UNTRACKED, WM_CLOSE_SLOT, ui_say_dialog_wnd_new, 0x118, 0xb4, WM_AT_640, 0xc8, 0x64, say_dialog_info),
    WM_ROW(0x11, WM_SLOT(choose_wnd), WM_UNTRACKED, make_choose_wnd, WM_CLOSE_SLOT),
    WM_ROW_OPEN(0x12, menu_wnd, WM_UNTRACKED, WM_CLOSE_SLOT, ui_menu_wnd_new, 4, 4, WM_AT_640, 0xc8, 0x12c),
    WM_ROW(0x15, WM_SLOT(notify_level_up_wnd), WM_UNTRACKED, make_notify_wnd, WM_CLOSE_SLOT),
    WM_ROW_OPEN(0x1a, combo_box_wnd, WM_UNTRACKED, WM_CLOSE_SLOT, ui_combo_box_wnd_new, 0x10, 0x10, WM_AT_640, 0xc8, 0x12c),
    WM_ROW(0x24, WM_SLOT(short_cut_wnd), WM_UNTRACKED, make_short_cut_wnd, WM_CLOSE_SLOT),
    WM_ROW_OPEN(0x27, choose_warp_wnd, WM_UNTRACKED, WM_CLOSE_SLOT, ui_choose_warp_wnd_new, 0x118, 0x78, WM_AT_640, 0xc8, 0x12c),
    WM_ROW(0x31, WM_SLOT(notify_job_level_up_wnd), WM_UNTRACKED, make_notify_job_level_up_wnd, WM_CLOSE_SLOT),
    WM_ROW(0x38, WM_SLOT(npc_edit_dialog_wnd), WM_UNTRACKED, make_npc_edit_dialog_wnd, WM_CLOSE_SLOT),
    /* The whisper windows; ui/chat/whisper.c creates them with this ID. */
    WM_ROW(0x43, WM_NO_SLOT, WM_UNTRACKED, NULL, WM_CLOSE_WINDOW),
    WM_ROW(0x4e, WM_SLOT(illust_wnd), WM_UNTRACKED, make_illust_wnd, WM_CLOSE_SLOT),
    WM_ROW(0x54, WM_NO_SLOT, 0, make_talkbox_trap_input_wnd, WM_CLOSE_WINDOW),
    WM_ROW(0x56, WM_SLOT(emotion_wnd), 0, make_emotion_wnd, WM_CLOSE_SLOT),
    WM_ROW(0x57, WM_SLOT(emotion_list_wnd), 0, make_emotion_list_wnd, WM_CLOSE_SLOT),
    WM_ROW(0x64, WM_SLOT(npc_text_edit_dialog_wnd), WM_UNTRACKED, make_npc_text_edit_dialog_wnd, WM_CLOSE_SLOT),
    WM_ROW(0x65, WM_NO_SLOT, WM_UNTRACKED, make_graffi_strbox_wnd, WM_CLOSE_WINDOW),
    WM_ROW_OPEN(0x67, prohibit_list_wnd, WM_GET_SLOT | WM_KEPT_BY_CLEAR, WM_CLOSE_SLOT, ui_prohibit_list_wnd_new, 0x1f4, 0x190, WM_AT_640, 0x64, 0x64),
    WM_ROW_OPEN(0x70, command_list_wnd, 0, WM_CLOSE_SLOT, ui_command_list_wnd_new, 0xfa, 0xc8, WM_AT_640, 0x190, 0xc8),
    WM_ROW_OPEN(0x74, auto_message_wnd, WM_UNTRACKED, WM_CLOSE_SLOT, ui_auto_message_wnd_new, 0x118, 0x30, WM_AT_CENTER, 0xb9, 0xb4),
    WM_ROW_OPEN(0x84, battle_msg_option_wnd, 0, WM_CLOSE_SLOT, ui_battle_msg_option_wnd_new, 0x118, 0xc8, WM_AT_640, 0x190, 0xc8),
    WM_ROW(0x89, WM_SLOT(memorial_dun_wnd), WM_UNTRACKED, make_memorial_dun_wnd, close_memorial_dun_wnd),
    WM_ROW(0x8c, WM_SLOT(kafra_move_map_service_info_wnd), 0, make_kafra_move_map_service_info_wnd, close_kafra_move_map_service_info_wnd),
    WM_ROW(0x8f, WM_SLOT(notify_quest_wnd), 0, make_notify_wnd, WM_CLOSE_SLOT),
    WM_ROW_OPEN(0x9b, esc_option_wnd, 0, WM_CLOSE_SLOT, ui_esc_option_wnd_new, 0x118, 0x8f, WM_AT_640, 0xb9, 0x12c),
    WM_ROW(0xcb, WM_SLOT(navigation_v4_wnd), WM_GET_SLOT | WM_ESC_LAST, make_navigation_v4_wnd, WM_CLOSE_SLOT),
    WM_ROW(0xcc, WM_SLOT(quest_display_wnd), WM_UNTRACKED | WM_GET_SLOT | WM_KEPT_BY_FORGET, make_quest_display_wnd, WM_CLOSE_SLOT),
    WM_ROW(0xe5, WM_NO_SLOT, 0, make_navigation_help_wnd, WM_CLOSE_WINDOW),
    WM_ROW_OPEN(0x104, combo_box_idwnd, 0, WM_CLOSE_SLOT, ui_combo_box_id_wnd_new, 0x10, 0x10, WM_AT_640, 0xc8, 0x12c),
    WM_ROW(0x111, WM_SLOT(mini_map_wnd), WM_GET_SLOT, make_mini_map_wnd, WM_CLOSE_SLOT),
    WM_ROW(0x114, WM_SLOT(round_box_tooltip_wnd), WM_KEPT_BY_CLEAR, make_round_box_tooltip_wnd, close_round_box_tooltip_wnd),
    WM_ROW(0x121, WM_NO_SLOT, WM_UNTRACKED, make_view_camera_info_wnd, WM_CLOSE_WINDOW),
    WM_ROW(0x12f, WM_NO_SLOT, 0, make_hotkey_guide_wnd, WM_CLOSE_WINDOW),
    WM_ROW(0x131, WM_NO_SLOT, 0, make_graphic_setting_wnd, close_graphic_setting_wnd),
    WM_ROW(0x132, WM_NO_SLOT, 0, make_navigation_side_wnd, WM_CLOSE_WINDOW),
    WM_ROW(0x133, WM_NO_SLOT, WM_UNTRACKED, make_menu_icon_wnd, WM_CLOSE_WINDOW),
    WM_ROW(0x134, WM_NO_SLOT, 0, make_show_warp_wnd, WM_CLOSE_WINDOW),
    WM_ROW(0x13a, WM_NO_SLOT, 0, make_navigation_side_wnd, WM_CLOSE_WINDOW),
    WM_ROW(0x141, WM_NO_SLOT, 0, make_renew_quest_wnd, close_renew_quest_wnd),
    WM_ROW(0x142, WM_NO_SLOT, 0, make_renew_detail_quest_info_wnd, WM_CLOSE_WINDOW),
    WM_ROW(0x148, WM_NO_SLOT, 0, make_check_attendance_wnd, WM_CLOSE_WINDOW),
    /* Set by navigation_cmd.c around the private airplane's modal loop. */
    WM_ROW_SLOT(private_airplane_wnd, 0),
};

const struct WmWindowRows wm_hud_windows = WM_WINDOW_ROWS(k_hud_rows);
