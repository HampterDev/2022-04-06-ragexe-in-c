#include "ui/misc/snap_mgr.h"
#include "ui/wndmgr/wndmgr.h"

void ui_window_mgr_update_snap_rect(struct UIWindowMgr *self, struct UIWindow *wnd)
{
    struct ListNode *node;
    struct WmSnapRect *entry;

    for (node = self->plates.head->next; node != self->plates.head; node = node->next)
        if (*LIST_VALUE(node, struct UIWindow *) == wnd)
            return;
    if (!g_session1.windowSnap || wnd == NULL)
        return;
    entry = tree_get(&self->snap_rects, &wnd, sizeof wnd, sizeof *entry, tree_less_uint);
    snap_info_set(&entry->info, wnd->x, wnd->y, wnd->width, wnd->height);
}

void ui_window_mgr_get_snap_delta(struct UIWindowMgr *self, struct UIWindow *wnd, int *dx, int *dy, int flag)
{
    struct SnapInfo mine, screen;
    struct TreeNode *node;
    BYTE free_x = 1, free_y = 1, wnd_is_root;
    int sx = 0, sy = 0, ox, oy;

    *dx = 0;
    *dy = 0;
    snap_info_set(&mine, wnd->x, wnd->y, wnd->width, wnd->height);
    snap_info_set(&screen, 0, 0, self->screen_width, self->screen_height);
    snap_info_flip(&screen);
    snap_info_get_delta_snap(&screen, &mine, &sx, &sy);
    if (sx) {
        free_x = 0;
        *dx = sx;
    }
    if (sy) {
        free_y = 0;
        *dy = sy;
    }
    if (!free_x && !free_y)
        return;

    ox = 0;
    oy = 0;
    wnd_is_root = snap_mgr_is_root_window(&g_snapMgr, wnd);
    for (node = tree_first(&self->snap_rects); node != self->snap_rects.head; node = tree_next(node)) {
        struct WmSnapRect *entry = TREE_VALUE(node, struct WmSnapRect);
        struct UIWindow *other = entry->wnd;
        BYTE connected = snap_mgr_are_connected(&g_snapMgr, wnd, other);
        BYTE other_is_root = snap_mgr_is_root_window(&g_snapMgr, other);
        BYTE in_root_group = snap_mgr_in_root_group(&g_snapMgr, other);

        if (wnd == other)
            continue;
        if (flag) {
            if (other_is_root || connected || in_root_group)
                continue;
        } else if (wnd_is_root && (in_root_group || connected)) {
            continue;
        }
        snap_info_get_delta_snap(&entry->info, &mine, &ox, &oy);
        if (ox && free_x) {
            free_x = 0;
            *dx = ox;
        }
        if (oy && free_y) {
            free_y = 0;
            *dy = oy;
        }
        if (!free_x && !free_y)
            return;
    }
}
