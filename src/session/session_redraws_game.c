#include "ui/window_manager_state.h"
#include "game/context.h"
#include "session/session_lua.h"
#include "session/session_windows.h"
#include "ui/wndmgr/wndmgr.h"

/* The session's redraws that reach above the window manager: the game mode's own HP bar,
   the basic info window's height and the party plates. The others, in session_redraws.c,
   only use the window manager. */

void (*g_sessionBasicInfoHeight)(struct UIBasicInfoWnd *wnd, int height);

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

void session_refresh_skill_tree(struct Session *self)
{
    struct UIWindow *wnd;

    session_init_skill_tree_view(self);
    wnd = g_windowMgr.new_skill_list_wnd;
    if (wnd)
        ui_window_send(wnd, NULL, 0x3c, 0, 0, 0, 0);
    wnd = g_windowMgr.basic_info_wnd;
    if (wnd)
        if (g_sessionBasicInfoHeight)
            g_sessionBasicInfoHeight((struct UIBasicInfoWnd *)wnd, wnd->height);
}

void session_notify_messenger_group_and_plates(struct Session *self)
{
    notify_item_window(&g_windowMgr.messenger_group_wnd);
    ui_window_mgr_restore_plates(&g_windowMgr);
}
