#include "ui/misc/snap_mgr.h"
#include "session/state.h"
#include "core/object_type.h"
#include "ui/window_access.h"
#include "game/context.h"
#include "ui/wndmgr/make_window_internal.h"
#include "ui/wndmgr/window_registry.h"
#include "ui/items/items.h"
#include "world/mode_manager.h"
#include "ui/character/equip_wnd.h"
#include "ui/character/skill_list_wnd.h"
#include "session/session_jobs.h"
#include "ui/character/character.h"
#include "ui/character/dress_room.h"
#include "ui/character/gauge.h"
#include "ui/character/skill_dialogs.h"
#include "ui/character/skill_tree.h"
#include "ui/items/item_collection_internal.h"
#include "ui/items/item_list.h"
#include "ui/quests/achievement.h"
#include "ui/render/graphic_setting.h"

static void wm_clear_menu_mark(int val1)
{
    struct UIWindow *w = ui_window_find(0x133);

    if (w)
        ui_window_send_msg(w, NULL, 6, 0x116, val1, 0, 0);
}

static struct UIWindow *make_basic_info(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = self->basic_info_wnd, *w;

    if (wnd == NULL) {
        wnd = ui_basic_info_wnd_new();
        self->basic_info_wnd = wnd;
        ui_window_create(wnd, 0xdc, 0x37);
        wm_move(wnd, 0, 0);
        ui_window_mgr_add_window(self, wnd);
        ui_window_send_msg(self->basic_info_wnd, self->basic_info_wnd, UI_MSG_SET_INFO,
                 (int)(INT_PTR)&self->basic_info_info, 0, 0, 0);
        snap_mgr_register_root(&g_snapMgr, wnd);
        w = ui_window_open(0x133);
        if (w) {
            int x = 0, y = 0;

            ui_window_to_screen(wnd, &x, &y);
            wm_move(w, x, wnd->height + y);
        }
    }
    {
        wm_show(wnd);
    }
    if (self->item_info.show)
        ui_window_mgr_make_window(self, 0x8);
    if (self->equip_info.show)
        ui_window_mgr_make_window(self, 0xa);
    if (self->status_info.show)
        ui_window_mgr_make_window(self, 0xb);
    if (self->skill_list_info.show)
        ui_window_mgr_make_window(self, 0x25);
    if (self->quest_info.show)
        ui_window_mgr_make_window(self, 0x141);
    if (game_camera_is_free())
        ui_window_mgr_make_window(self, 0xe);
    ui_window_mgr_show_option_window(self, 0x1, self->chat_wnd_show);
    ui_window_mgr_show_option_window(self, 0x24, self->shortcut_info.show);
    ui_window_mgr_show_option_window(self, 0xf, 0);
    if (self->messenger_group_info.show)
        ui_window_mgr_make_window(self, 0x45);
    if (g_session1.lowMemory && g_session1.autoOpenDetailWindow && g_spriteMode == 2 && g_textureMode == 2)
        ui_window_mgr_make_window(self, 0x131);
    if (session_has_tracked_achievements(&g_session1) == 1)
        ui_window_mgr_make_window(self, 0x10f);
    ui_window_mgr_restore_plates(self);
    return wnd;
}

static struct UIWindow *wm_frame_cast(struct UIWindow *wnd)
{
    return object_as_type(wnd, OBJECT_FRAME_WINDOW);
}

/* 0x8 */
static struct UIWindow *make_item_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->item_wnd;
    if (wnd == NULL) {
        wnd = ui_item_wnd_new();
        self->item_wnd = wnd;
        ui_window_create(wnd, 0x118, 0x78);
        wm_move(wnd, 0, 0x78);
        ui_window_mgr_add_window(self, wnd);
        wm_set_info(self->item_wnd, &self->item_info);
        snap_mgr_register(&g_snapMgr, wnd);
    }
    wm_show(wnd);
    self->item_info.show = 1;
    wm_clear_menu_mark(0xba);
    return wnd;
}

/* 0x8: post_quit1 */
static int close_item_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    if (self->item_wnd) {
        ui_item_wnd_clear_new_marks((struct UIItemWnd *)self->item_wnd, 0);
        ui_window_mgr_post_quit2(self, self->item_wnd);
        self->item_wnd = NULL;
        self->item_info.show = 0;
        closed = 1;
    }
    return closed;
}

/* 0xa */
static struct UIWindow *make_equip_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->equip_wnd;
    if (wnd == NULL) {
        wnd = ui_equip_wnd_new();
        self->equip_wnd = wnd;
        ui_equip_wnd_set_viewing_other(wnd, 0);
        ui_window_create(wnd, 0x118, 0xa7);
        wm_move_640(wnd, 0xb9, 0x12c);
        ui_window_mgr_add_window(self, wnd);
        wm_set_info(self->equip_wnd, &self->equip_info);
        snap_mgr_register(&g_snapMgr, wnd);
    }
    wm_show(wnd);
    self->equip_info.show = 1;
    return wnd;
}

/* 0xa: post_quit1 */
static int close_equip_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    if (wm_close_slot(self, &self->equip_wnd)) {
        self->equip_info.show = 0;
        g_session1.newTitles.last = g_session1.newTitles.first;
        closed = 1;
    }
    return closed;
}

/* 0xb */
static struct UIWindow *make_status_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->status_wnd;
    if (wnd == NULL) {
        wnd = ui_status_wnd_new();
        self->status_wnd = wnd;
        ui_window_create(wnd, 0x118, 0x8d);
        wm_move_640(wnd, 0xb9, 0x12c);
        ui_window_mgr_add_window(self, wnd);
        wm_set_info(self->status_wnd, &self->status_info);
        snap_mgr_register(&g_snapMgr, wnd);
    }
    wm_show(wnd);
    self->status_info.show = 1;
    ui_window_mgr_post_quit1(self, 0x15);
    wm_clear_menu_mark(0xb8);
    return wnd;
}

/* 0xb: post_quit1 */
static int close_status_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    if (wm_close_slot(self, &self->status_wnd)) {
        self->status_info.show = 0;
        closed = 1;
    }
    return closed;
}

/* 0xc: post_quit1 */
static int close_item_collection_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    if (wm_close_slot(self, &self->item_collection_wnd)) {
        closed = 1;
        if (self->item_collection_comparison_wnd)
            ui_window_mgr_post_quit1(self, 0xea);
    }
    return closed;
}

/* 0xf */
static struct UIWindow *make_item_drop_cnt_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->item_drop_cnt_wnd;
    if (wnd)
        return wnd;
    wnd = ui_item_drop_cnt_wnd_new();
    self->item_drop_cnt_wnd = wnd;
    ui_window_create(wnd, 0xb6, 0x2e);
    wm_move_640(wnd, 0xc8, 0xc8);
    ui_window_mgr_add_window(self, wnd);
    return wnd;
}

/* 0x25 */
static struct UIWindow *make_new_skill_list_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->new_skill_list_wnd;
    if (wnd == NULL) {
        wnd = ui_new_skill_list_wnd_new();
        self->new_skill_list_wnd = wnd;
        ui_window_create(wnd, 0x230, 0x190);
        wm_move_640(wnd, 0x32, 0x32);
        ui_window_mgr_add_window(self, wnd);
        wm_set_info(self->new_skill_list_wnd, &self->skill_list_info);
    }
    wm_show(wnd);
    self->skill_list_info.show = 1;
    ui_window_mgr_post_quit1(self, 0x31);
    wm_clear_menu_mark(0xbc);
    return wnd;
}

/* 0x25: post_quit1 */
static int close_new_skill_list_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    ui_window_mgr_post_quit1(self, 0x2e);
    if (wm_close_slot(self, &self->new_skill_list_wnd)) {
        self->skill_list_info.show = 0;
        closed = 1;
    }
    return closed;
}

/* 0x2e */
static struct UIWindow *make_skill_describe_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->skill_describe_wnd;
    if (wnd == NULL) {
        wnd = ui_skill_describe_wnd_new();
        self->skill_describe_wnd = wnd;
        ui_window_create(wnd, 0x118, 0x78);
        wm_move_640_up(wnd, 0xb9, 0x12c, 0x78);
        ui_window_mgr_add_window(self, wnd);
    }
    wm_show(wnd);
    return wnd;
}

/* 0x2e: post_quit1 */
static int close_skill_describe_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    if (wm_close_slot(self, &self->skill_describe_wnd)) {
        self->skill_describe_show = 0;
        closed = 1;
    }
    return closed;
}

/* 0x37 */
static struct UIWindow *make_skill_name_change_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->skill_name_change_wnd;
    if (wnd == NULL) {
        wnd = ui_skill_name_change_wnd_new();
        self->skill_name_change_wnd = wnd;
        ui_window_create(wnd, 0x118, 0x30);
        wm_move_center(wnd, 0xb9, 0xb4);
        ui_window_mgr_add_window(self, wnd);
    }
    wm_show(wnd);
    return wnd;
}

/* 0x3a */
static struct UIWindow *make_notify_item_obtain_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->notify_item_obtain_wnd;
    if (wnd == NULL) {
        wnd = ui_notify_item_obtain_wnd_new();
        self->notify_item_obtain_wnd = wnd;
        ui_window_create(wnd, 0xb4, 0x20);
        wm_move(wnd, ui_center_x640(0xdc), 0x4b);
        ui_window_mgr_add_window(self, wnd);
    }
    wm_show(wnd);
    return wnd;
}

/* 0x4d */
static struct UIWindow *make_monster_info_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = ui_monster_info_wnd_new();
    ui_window_create(wnd, 0x190, 0xc8);
    wm_move(wnd, 0, 0);
    ui_window_mgr_add_window(self, wnd);
    return wnd;
}

/* 0x58 */
static struct UIWindow *make_pet_info_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    if (g_session1.petJob == -1)
        return NULL;
    wnd = self->pet_info_wnd;
    if (wnd == NULL) {
        wnd = ui_pet_info_wnd_new();
        self->pet_info_wnd = wnd;
        ui_window_create(wnd, 0x118, 0xa0);
        wm_move_640_up(wnd, 0xb9, 0x190, 0x3c);
        wm_set_info(wnd, &self->pet_info_info);
        ui_window_mgr_add_window(self, wnd);
    }
    wm_show(wnd);
    return wnd;
}

/* 0x5a: post_quit1 */
static int close_pet_egg_list_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    if (wm_close_slot(self, &self->pet_egg_list_wnd)) {
        closed = 1;
        mode_send_message(client_current_mode(), 0x92, -1, 0, 0, 0);
    }
    return closed;
}

/* 0x5b */
static struct UIWindow *make_pet_taming_deceive_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->pet_taming_deceive_wnd;
    if (wnd == NULL) {
        wnd = ui_pet_taming_deceive_wnd_new();
        self->pet_taming_deceive_wnd = wnd;
        ui_window_create(wnd, 0xc4, 0x100);
        wm_move(wnd, g_renderer->width / 2 - 0x62, g_renderer->height / 2 - 0x80);
        ui_window_mgr_add_window(self, wnd);
    }
    wm_show(wnd);
    return wnd;
}

/* 0x63 */
static struct UIWindow *make_spell_list_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    if (self->spell_list_wnd) {
        ui_window_mgr_post_quit2(self, self->spell_list_wnd);
        self->spell_list_wnd = NULL;
    }
    wnd = WM_OPEN(spell_list_wnd, ui_spell_list_wnd_new, 0xc8, 0xc8, WM_AT_640, 0xc8, 0xc8);
    return wnd;
}

/* 0x71 */
static struct UIWindow *make_homun_info_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    if (g_session1.homunJob == -1)
        return NULL;
    wnd = self->homun_info_wnd;
    if (wnd == NULL) {
        wnd = ui_homun_info_wnd_new();
        self->homun_info_wnd = wnd;
        ui_window_create(wnd, 0x118, 0xd0);
        wm_move_640_up(wnd, 0xb9, 0x190, 0x3c);
        wm_set_info(wnd, &self->homun_info_info);
        ui_window_mgr_add_window(self, wnd);
    }
    wm_show(wnd);
    return wnd;
}

/* 0x72 */
static struct UIWindow *make_homun_skill_list_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->homun_skill_list_wnd;
    if (wnd == NULL) {
        wnd = ui_skill_list_wnd_new();
        self->homun_skill_list_wnd = wnd;
        ui_skill_list_set_kind(wnd, WM_SKILL_LIST_HOMUN);
        ui_window_create(wnd, 0x118, 0x78);
        wm_move_640(wnd, 0xc8, 0xc8);
        ui_window_mgr_add_window(self, wnd);
        wm_set_info(self->homun_skill_list_wnd, &self->homun_skill_list_info);
    }
    ui_skill_list_set_kind(wnd, WM_SKILL_LIST_HOMUN);
    wm_show(wnd);
    self->homun_skill_list_info.show = 1;
    return wnd;
}

/* 0x72: post_quit1 */
static int close_homun_skill_list_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    if (wm_close_slot(self, &self->homun_skill_list_wnd)) {
        self->homun_skill_list_info.show = 0;
        closed = 1;
    }
    return closed;
}

/* 0x7d */
static struct UIWindow *make_mer_info_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    if (g_session1.mercJob == -1)
        return NULL;
    wnd = self->mer_info_wnd;
    if (wnd == NULL) {
        wnd = ui_mer_info_wnd_new();
        self->mer_info_wnd = wnd;
        ui_window_create(wnd, 0x118, 0xb4);
        wm_move_640_up(wnd, 0xb9, 0x190, 0x3c);
        wm_set_info(wnd, &self->mer_info_info);
        ui_window_mgr_add_window(self, wnd);
    }
    wm_show(wnd);
    return wnd;
}

/* 0x7e */
static struct UIWindow *make_mer_skill_list_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->mer_skill_list_wnd;
    if (wnd == NULL) {
        wnd = ui_skill_list_wnd_new();
        self->mer_skill_list_wnd = wnd;
        ui_skill_list_set_kind(wnd, WM_SKILL_LIST_MERCENARY);
        ui_window_create(wnd, 0x118, 0x78);
        wm_move_640(wnd, 0xc8, 0xc8);
        ui_window_mgr_add_window(self, wnd);
        wm_set_info(self->mer_skill_list_wnd, &self->mer_skill_list_info);
    }
    ui_skill_list_set_kind(wnd, WM_SKILL_LIST_MERCENARY);
    wm_show(wnd);
    self->mer_skill_list_info.show = 1;
    return wnd;
}

/* 0x7e: post_quit1 */
static int close_mer_skill_list_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    if (wm_close_slot(self, &self->mer_skill_list_wnd)) {
        self->mer_skill_list_info.show = 0;
        closed = 1;
    }
    return closed;
}

/* 0x8b */
static struct UIWindow *make_other_equip_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->other_equip_wnd;
    if (wnd == NULL) {
        wnd = ui_equip_wnd_new();
        self->other_equip_wnd = wnd;
        ui_equip_wnd_set_viewing_other(wnd, 1);
        ui_window_create(wnd, 0x118, 0xa7);
        wm_move_640(wnd, 0xb9, 0x12c);
        ui_window_mgr_add_window(self, wnd);
        snap_mgr_register(&g_snapMgr, wnd);
    }
    wm_show(wnd);
    return wnd;
}

/* 0xc4 */
static struct UIWindow *make_collection_system_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->collection_system_wnd;
    if (wnd == NULL) {
        wnd = ui_collection_system_wnd_new();
        self->collection_system_wnd = wnd;
        ui_window_create(wnd, 0xc4, 0x100);
        wm_move(wnd, g_renderer->width / 2 - 0x62, g_renderer->height / 2 - 0x80);
        ui_window_mgr_add_window(self, wnd);
    }
    wm_show(wnd);
    return wnd;
}

/* 0xea */
static struct UIWindow *make_item_collection_comparison_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->item_collection_comparison_wnd;
    if (wnd == NULL) {
        wnd = ui_item_collection_comparison_wnd_new();
        self->item_collection_comparison_wnd = wnd;
        ui_window_create(wnd, 0x118, 0x78);
        ui_window_mgr_add_window(self, wnd);
    }
    wm_set_info(wnd, NULL);
    wm_show(wnd);
    return wnd;
}

/* 0xea: post_quit1 */
static int close_item_collection_comparison_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    if (wm_close_slot(self, &self->item_collection_comparison_wnd)) {
        closed = 1;
        if (self->item_collection_wnd)
            ui_window_mgr_post_quit1(self, 0xc);
    }
    return closed;
}

/* 0x103 */
static struct UIWindow *make_second_costume_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = wm_frame_cast(ui_window_mgr_get_window(self, 0x103));
    if (wnd)
        return wnd;
    wnd = ui_second_costume_wnd_new();
    if (!wnd)
        return NULL;
    self->second_costume_wnd = wnd;
    ui_window_create(wnd, 0x18b, 0xcd);
    {
        int y = scale_y_from480(0xc8) + 0x2e;

        wm_move(wnd, ui_center_x640(0xc8), y);
    }
    wm_show(wnd);
    ui_window_mgr_add_window(self, wnd);
    return wnd;
}

/* 0x10e */
static struct UIWindow *make_achievement_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = wm_frame_cast(ui_window_mgr_get_window(self, 0x10e));
    if (wnd == NULL) {
        wnd = ui_achievement_wnd_new();
        if (!wnd)
            return NULL;
        ui_window_create(wnd, 0x2ee, 0x1d4);
        wm_move(wnd, wm_center_x(0x2ee), wm_center_y(0x1d4));
        wm_show(wnd);
        ui_window_mgr_add_window(self, wnd);
    }
    wm_clear_menu_mark(0x1d1);
    return wnd;
}

/* 0x10f */
static struct UIWindow *make_ach_tracing_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = wm_frame_cast(ui_window_mgr_get_window(self, 0x10f));
    if (wnd)
        return wnd;
    wnd = ui_ach_tracing_wnd_new();
    if (!wnd)
        return NULL;
    ui_window_create(wnd, 0x12c, 0x96);
    wm_move(wnd, g_renderer->width - 0x15e, wm_center_y(0x96));
    wm_show(wnd);
    wnd->list_rank = 5;
    ui_window_mgr_add_window(self, wnd);
    wm_window_list_sort(&self->windows);
    wm_set_info(wnd, &self->ach_tracing_pos);
    return wnd;
}

/* 0x126 */
static struct UIWindow *make_preview_equip_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = ui_window_mgr_get_window(self, 0x126);
    if (wnd)
        return wnd;
    wnd = ui_preview_equip_wnd_new();
    ui_window_create(wnd, 0x118, 0x104);
    {
        int y = (int)((float)(g_renderer->height - 0x104) * 0.5f);

        wm_move(wnd, (int)((float)(g_renderer->width - 0x118) * 0.5f), y);
    }
    wm_set_info(wnd, &self->preview_equip_pos);
    ui_window_mgr_add_window(self, wnd);
    return wnd;
}

/* The rows of the character, item, pet, homunculus and mercenary windows.
   window_table.h describes the columns. In ID order. */
static const struct WmWindowRow k_character_rows[] = {
    WM_ROW(0x0, WM_SLOT(basic_info_wnd), WM_UNTRACKED, make_basic_info, WM_CLOSE_SLOT),
    WM_ROW(0x8, WM_SLOT(item_wnd), 0, make_item_wnd, close_item_wnd),
    WM_ROW(0xa, WM_SLOT(equip_wnd), 0, make_equip_wnd, close_equip_wnd),
    WM_ROW(0xb, WM_SLOT(status_wnd), 0, make_status_wnd, close_status_wnd),
    WM_ROW_OPEN_INFO(0xc, item_collection_wnd, 0, close_item_collection_wnd, ui_item_collection_wnd_new, 0x118, 0x78, WM_AT_640, 0xb9, 0x12c, item_collection_info),
    WM_ROW(0xf, WM_SLOT(item_drop_cnt_wnd), WM_UNTRACKED, make_item_drop_cnt_wnd, WM_CLOSE_SLOT),
    WM_ROW(0x25, WM_SLOT(new_skill_list_wnd), WM_GET_SLOT, make_new_skill_list_wnd, close_new_skill_list_wnd),
    WM_ROW(0x2e, WM_SLOT(skill_describe_wnd), 0, make_skill_describe_wnd, close_skill_describe_wnd),
    WM_ROW_OPEN(0x2f, card_item_illust_wnd, 0, WM_CLOSE_SLOT, ui_card_item_illust_wnd_new, 0x12c, 0x1a1, WM_AT_640, 0, 0),
    WM_ROW(0x37, WM_SLOT(skill_name_change_wnd), 0, make_skill_name_change_wnd, WM_CLOSE_SLOT),
    WM_ROW(0x3a, WM_SLOT(notify_item_obtain_wnd), WM_UNTRACKED, make_notify_item_obtain_wnd, WM_CLOSE_SLOT),
    WM_ROW(0x4d, WM_NO_SLOT, 0, make_monster_info_wnd, WM_CLOSE_WINDOW),
    WM_ROW_OPEN(0x4f, make_target_list_wnd, WM_UNTRACKED, WM_CLOSE_UNCOUNTED, ui_make_target_list_wnd_new, 0x118, 0x96, WM_AT_640_UP, 0xb9, 0x12c),
    WM_ROW_OPEN(0x50, make_target_process_wnd, WM_UNTRACKED, WM_CLOSE_UNCOUNTED, ui_make_target_process_wnd_new, 0x118, 0x96, WM_AT_640_UP, 0xb9, 0x12c),
    WM_ROW_OPEN_INFO(0x52, combined_card_item_collection_wnd, 0, WM_CLOSE_SLOT, ui_combined_card_item_collection_wnd_new, 0x118, 0x78, WM_AT_640, 0xb9, 0x12c, combined_card_item_collection_info),
    WM_ROW(0x58, WM_SLOT(pet_info_wnd), WM_GET_SLOT, make_pet_info_wnd, WM_CLOSE_SLOT),
    WM_ROW_OPEN(0x5a, pet_egg_list_wnd, WM_UNTRACKED, close_pet_egg_list_wnd, ui_pet_egg_list_wnd_new, 0xc8, 0xc8, WM_AT_640, 0xc8, 0xc8),
    WM_ROW(0x5b, WM_SLOT(pet_taming_deceive_wnd), WM_UNTRACKED, make_pet_taming_deceive_wnd, WM_CLOSE_SLOT),
    WM_ROW_OPEN(0x5e, making_arrow_list_wnd, WM_UNTRACKED, WM_CLOSE_SLOT, ui_making_arrow_list_wnd_new, 0xc8, 0xc8, WM_AT_640, 0xc8, 0xc8),
    WM_ROW_OPEN(0x5f, select_cart_wnd, WM_UNTRACKED, WM_CLOSE_SLOT, ui_select_cart_wnd_new, 0x181, 0x62, WM_AT_640, 0xc8, 0xc8),
    WM_ROW(0x63, WM_SLOT(spell_list_wnd), WM_UNTRACKED | WM_KEPT_BY_CLEAR, make_spell_list_wnd, WM_CLOSE_SLOT),
    WM_ROW(0x71, WM_SLOT(homun_info_wnd), WM_GET_SLOT, make_homun_info_wnd, WM_CLOSE_SLOT),
    WM_ROW(0x72, WM_SLOT(homun_skill_list_wnd), WM_GET_SLOT, make_homun_skill_list_wnd, close_homun_skill_list_wnd),
    WM_ROW(0x7d, WM_SLOT(mer_info_wnd), WM_GET_SLOT, make_mer_info_wnd, WM_CLOSE_SLOT),
    WM_ROW(0x7e, WM_SLOT(mer_skill_list_wnd), WM_GET_SLOT, make_mer_skill_list_wnd, close_mer_skill_list_wnd),
    WM_ROW(0x8b, WM_SLOT(other_equip_wnd), 0, make_other_equip_wnd, WM_CLOSE_SLOT),
    WM_ROW(0xc4, WM_SLOT(collection_system_wnd), WM_UNTRACKED | WM_GET_SLOT, make_collection_system_wnd, WM_CLOSE_SLOT),
    WM_ROW(0xea, WM_SLOT(item_collection_comparison_wnd), 0, make_item_collection_comparison_wnd, close_item_collection_comparison_wnd),
    WM_ROW_OPEN_INFO(0xfa, combined_card_item_collection_comparison_wnd, WM_KEPT_BY_CLEAR, WM_CLOSE_SLOT, ui_combined_card_item_collection_comparison_wnd_new, 0x118, 0x78, WM_AT_640, 0xb9, 0x12c, combined_card_item_collection_info),
    WM_ROW(0x103, WM_SLOT(second_costume_wnd), 0, make_second_costume_wnd, WM_CLOSE_SLOT),
    WM_ROW(0x10e, WM_NO_SLOT, 0, make_achievement_wnd, WM_CLOSE_WINDOW),
    WM_ROW(0x10f, WM_NO_SLOT, WM_UNTRACKED, make_ach_tracing_wnd, WM_CLOSE_WINDOW),
    WM_ROW(0x126, WM_NO_SLOT, 0, make_preview_equip_wnd, WM_CLOSE_WINDOW),
};

const struct WmWindowRows wm_character_windows = WM_WINDOW_ROWS(k_character_rows);
