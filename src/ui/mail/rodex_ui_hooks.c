#include "ui/window_access.h"
#include "ui/window_manager_state.h"
#include "ui/mail/rodex_compose.h"

int ui_rodex_compose_allows_caret_move(void)
{
    return !g_windowMgr.rodex_compose_wnd
        || (struct UIWindow *)ui_rodex_compose_body(g_windowMgr.rodex_compose_wnd)
               == ui_window_mgr_get_focus_edit(&g_windowMgr);
}
