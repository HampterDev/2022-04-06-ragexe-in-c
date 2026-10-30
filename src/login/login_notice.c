#include "render/renderer.h"
#include "ui/window_manager_state.h"
#include "contents/mouse.h"
#include "ui/window_access.h"
#include "login/loginmode.h"

#include <string.h>
#include "ui/window_types.h"
#include "core/msg_str.h"

static void notice_show(void *wnd, int id)
{
    UIWND_SENDMSG(wnd, 0, 0x1f, (int)msg_str(id), 0, 0, 0);
    UIWND_SENDMSG(wnd, 0, 0x1e, 0, 0, 0, 0);
}

BYTE login_mode_add_notice(struct LoginMode *self, int msg, BYTE append)
{
    struct Vec *ids = &self->messageIds;
    struct UIWindow *wnd;
    int value;

    if (append) {
        wnd = g_windowMgr.menu_wnd;
    } else {
        if (ids->first != ids->last) {
            memmove(ids->first, ids->last, 0);
            ids->last = ids->first;
        }
        if (g_windowMgr.menu_wnd)
            ui_window_mgr_post_quit1(&g_windowMgr, 0x12);
        wnd = ui_window_open(0x12);
    }
    if (!wnd)
        return 0;

    switch (msg) {
    case 0x2d: notice_show(wnd, 0x873); break;
    case 0x2e: notice_show(wnd, 0x874); break;
    case 0x2f: notice_show(wnd, 0x875); break;
    default:   goto placed;
    }
    value = msg;
    vec_push(ids, &value, sizeof value);

placed:
    if (ids->first == ids->last)
        return 0;
    wnd = g_windowMgr.menu_wnd;
    if (wnd) {
        int x = g_mouse.x + 1, y = g_mouse.y + 1;
        int maxX = g_renderer->width - wnd->width;
        int maxY = g_renderer->height - wnd->height;

        if (x < 1)
            x = 1;
        if (x > maxX)
            x = maxX;
        if (y < 1)
            y = 1;
        if (y > maxY)
            y = maxY;
        ui_window_move(wnd, x, y);
    }
    return 1;
}
