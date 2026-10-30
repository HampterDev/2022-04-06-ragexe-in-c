#include "ui/window_manager_state.h"
#include "game/context.h"
#include "session/session_items_priv.h"
#include "session/session_lua.h"
#include "session/session_windows.h"
#include "ui/window_types.h"
#include "diag/perf.h"
#include "ui/character/gauge.h"
#include "ui/wndmgr/wndmgr.h"

void session_refresh_basic_info(struct Session *self)
{
    struct UIWindow *wnd;

    if (!game_current_mode())
        return;
    wnd = g_windowMgr.basic_info_wnd;
    if (wnd) {
        if (self->maxHp < self->hp)
            return;
        if (self->maxSp < self->sp)
            return;
        ui_window_refresh(wnd);
        ui_window_invalidate_children(g_windowMgr.basic_info_wnd);
    }
    game_mode_refresh_own_hp(game_current_mode());
}

static void invalidate_slot(struct UIWindow *const *slot)
{
    struct UIWindow *wnd;

    if (!game_current_mode())
        return;
    wnd = *slot;
    if (!wnd)
        return;
    ui_window_refresh(wnd);
    ui_window_invalidate_children(*slot);
}

void session_invalidate_homun_info_wnd(struct Session *self)
{
    invalidate_slot(&g_windowMgr.homun_info_wnd);
}

void session_invalidate_mer_info_wnd(struct Session *self)
{
    invalidate_slot(&g_windowMgr.mer_info_wnd);
}

void session_refresh_skill_tree(struct Session *self)
{
    struct UIWindow *wnd;

    session_init_skill_tree_view(self);
    wnd = g_windowMgr.new_skill_list_wnd;
    if (wnd)
        ui_window_send(wnd, NULL, 0x3c, 0, 0, 0, 0);
    wnd = g_windowMgr.basic_info_wnd;
    if (wnd)
        ui_basic_info_wnd_set_height((struct UIBasicInfoWnd *)wnd, wnd->height);
}

void session_invalidate_windows(struct Session *self)
{
    ui_window_mgr_invalidate_once_per_frame(&g_windowMgr);
}

void session_notify_messenger_group_wnd(struct Session *self)
{
    notify_item_window(&g_windowMgr.messenger_group_wnd);
}

void session_refresh_party_ui(struct Session *self)
{
    notify_item_window(&g_windowMgr.merchant_item_wnd);
}

void session_refresh_friend_ui(struct Session *self)
{
    notify_item_window(&g_windowMgr.item_store_wnd);
}

void session_notify_store_candidate_window(struct Session *self, int which)
{
    notify_item_window(store_candidate_window_slot(which));
}

void session_notify_store_setup_window(struct Session *self, BYTE alt, int value)
{
    struct UIWindow *wnd = *(alt ? &g_windowMgr.buying_store_make_wnd : &g_windowMgr.merchant_shop_make_wnd);
    if (wnd)
        ui_window_send(wnd, NULL, UIMSG_REFRESH, value, 0, 0, 0);
}

void session_notify_messenger_group_and_plates(struct Session *self)
{
    notify_item_window(&g_windowMgr.messenger_group_wnd);
    ui_window_mgr_restore_plates(&g_windowMgr);
}

void session_refresh_mouse_snap_state(struct Session *self, int skill)
{
    self->monsterSnapInEffect = 0;
    if (skill ? self->monsterSnapOnSkill != 0 : self->monsterSnapOnNoSkill != 0)
        self->monsterSnapInEffect = 1;
}

void session_refresh_equip_wnd(struct Session *self)
{
    if (g_windowMgr.equip_wnd)
        ui_window_send(g_windowMgr.equip_wnd, NULL, 0x6a, 0x237, 0, 0, 0);
}
