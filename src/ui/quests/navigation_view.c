#include "ui/window_access.h"
#include "ui/navigation_view.h"
#include "ui/quests/navigation.h"
#include "ui/worldmap/route_view.h"
#include <mmsystem.h>

int ui_navigation_is_open(void) { return g_windowMgr.navigation_v4_wnd != NULL; }
void ui_navigation_open(void) { ui_window_open(NAVI_WINDOW_ID); }

void ui_navigation_relayout(void)
{
    if (g_windowMgr.navigation_v4_wnd)
        ui_navigation_v4_wnd_relayout((struct UINavigationV4Wnd *)g_windowMgr.navigation_v4_wnd);
}

void ui_navigation_invalidate(void)
{
    struct UIWindow *window = g_windowMgr.navigation_v4_wnd;
    if (window) ui_window_invalidate(window);
}

void ui_navigation_refresh_map(unsigned layers)
{
    struct UIRoMapWnd *window = (struct UIRoMapWnd *)g_windowMgr.kafra_move_map_service_info_wnd;
    if (!window) return;
    if (layers & NAVI_MAP_GUIDED) ui_ro_map_wnd_load_guided_map(window);
    if (layers & NAVI_MAP_RESULTS) ui_ro_map_wnd_load_result_maps(window);
    if (layers & NAVI_MAP_ROUTE) ui_ro_map_wnd_load_route_maps(window);
}

void ui_navigation_show_search(void)
{
    struct UIWindow *window = g_windowMgr.navigation_v4_wnd;
    if (!window) ui_navigation_open();
    else {
        ui_window_send(window, NULL, UI_MSG_COMMAND, NAVI_ID_MAXIMIZE, 0, 0, 0);
        ui_navigation_refresh_map(NAVI_MAP_RESULTS);
    }
    ui_window_mgr_raise_window(&g_windowMgr, g_windowMgr.navigation_v4_wnd);
}

void ui_navigation_show_map(void)
{
    struct UIWindow *window;
    ui_navigation_open();
    window = g_windowMgr.navigation_v4_wnd;
    if (!window) return;
    window->alpha_target = 0xff;
    window->alpha_time = timeGetTime();
    ui_navigation_v4_wnd_relayout((struct UINavigationV4Wnd *)window);
    ui_navigation_v4_wnd_layout_list_page((struct UINavigationV4Wnd *)window);
    ui_window_invalidate(window);
}

void ui_navigation_chat(const char *text) { ui_chat_line(text, 0x62b0ff); }
void ui_navigation_message_box(const char *text)
{
    ui_window_mgr_message_box(&g_windowMgr, text, 0, 1, 0, 0, "메시지", 0x118, 0x78, 0);
}
