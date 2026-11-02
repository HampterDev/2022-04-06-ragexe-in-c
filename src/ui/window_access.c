#include "ui/window_access.h"
#include "ui/window_mgr.h"

struct UIWindow *ui_window_find(int id)
{
    return ui_window_mgr_get_window(&g_windowMgr, id);
}

struct UIWindow *ui_window_open(int id)
{
    return ui_window_mgr_make_window(&g_windowMgr, id);
}

intptr_t ui_window_send(struct UIWindow *window, struct UIWindow *sender, int message,
                   intptr_t a, intptr_t b, intptr_t c, intptr_t d)
{
    return window ? window->ops->send_msg(window, sender, message, a, b, c, d) : 0;
}

void ui_window_show(struct UIWindow *window, int visible)
{
    if (window)
        window->ops->set_show(window, visible);
}

void ui_window_raise(struct UIWindow *window)
{
    if (window)
        ui_window_mgr_raise_window(&g_windowMgr, window);
}

void ui_window_move_safe(struct UIWindow *window, int x, int y)
{
    if (window)
        window->ops->move(window, x, y);
}

void ui_window_refresh(struct UIWindow *window)
{
    if (window)
        window->ops->invalidate(window);
}
