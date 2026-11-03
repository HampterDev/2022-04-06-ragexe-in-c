#include "ui/misc/snap_mgr.h"
#include "session/state.h"
#include "game/context.h"
#include "ui/window_access.h"
#include "ui/wndmgr/wndmgr.h"
#include "ui/wndmgr/window_table.h"

struct UIWindow *ui_window_mgr_get_window(struct UIWindowMgr *self, int window_id)
{
    const struct WmWindowRow *row = wm_window_row(window_id);
    struct WmWindowEntry *entry;

    if (row && row->get)
        return row->get(self, window_id);
    if (row && (row->flags & WM_GET_SLOT))
        return *wm_row_slot(self, row);
    entry = tree_find(&self->windows_by_id, &window_id, tree_less_int);
    return entry ? entry->wnd : NULL;
}

BYTE ui_window_mgr_is_untracked_id(struct UIWindowMgr *self, int window_id)
{
    const struct WmWindowRow *row = wm_window_row(window_id);
    struct UIWindow *wnd;

    (void)self;
    if (row == NULL)
        return 0;
    if (row->flags & WM_UNTRACKED)
        return 1;
    if (row->flags & WM_UNTRACKED_UNLESS_SHOWN) {
        wnd = ui_window_find(window_id);
        return wnd == NULL || !wnd->visible;
    }
    return 0;
}

void ui_window_mgr_forget_window_id(struct UIWindowMgr *self, int window_id)
{
    struct ListNode *node;

    if (ui_window_mgr_is_untracked_id(self, window_id))
        return;
    for (node = self->id_list.head->next; node != self->id_list.head; node = node->next) {
        if (*LIST_VALUE(node, int) == window_id) {
            list_erase(&self->id_list, node, NULL);
            return;
        }
    }
}

void ui_window_mgr_touch_window_id(struct UIWindowMgr *self, int window_id)
{
    const struct WmWindowRow *row = wm_window_row(window_id);

    if (ui_window_mgr_is_untracked_id(self, window_id))
        return;
    ui_window_mgr_forget_window_id(self, window_id);
    if (row && (row->flags & WM_ESC_LAST))
        list_push_back(&self->id_list, &window_id, sizeof window_id);
    else
        list_push_front(&self->id_list, &window_id, sizeof window_id);
}

void ui_window_mgr_post_quit2(struct UIWindowMgr *self, struct UIWindow *wnd)
{
    size_t i;

    for (i = 0; i < wm_window_row_count(); i++) {
        const struct WmWindowRow *row = wm_window_row_at(i);

        if ((row->flags & WM_FORGOTTEN_BY_QUIT) && wnd == *wm_row_slot(self, row))
            *wm_row_slot(self, row) = NULL;
    }
    if (wnd == self->mouse_wnd)
        self->mouse_wnd = NULL;
    list_push_back(&self->quit_list, &wnd, sizeof wnd);
}

int wm_close_slot(struct UIWindowMgr *self, struct UIWindow **slot)
{
    if (*slot == NULL)
        return 0;
    ui_window_mgr_post_quit2(self, *slot);
    *slot = NULL;
    return 1;
}

int wm_close_slot_stored(struct UIWindowMgr *self, struct UIWindow **slot)
{
    if (*slot == NULL)
        return 0;
    ui_window_store_info(*slot);
    ui_window_mgr_post_quit2(self, *slot);
    *slot = NULL;
    return 1;
}

int wm_close_row_slot(struct UIWindowMgr *self, int window_id)
{
    return wm_close_slot(self, wm_row_slot(self, wm_window_row(window_id)));
}

int wm_close_row_stored(struct UIWindowMgr *self, int window_id)
{
    return wm_close_slot_stored(self, wm_row_slot(self, wm_window_row(window_id)));
}

int wm_close_row_uncounted(struct UIWindowMgr *self, int window_id)
{
    wm_close_slot(self, wm_row_slot(self, wm_window_row(window_id)));
    return 0;
}

BYTE ui_window_mgr_post_quit1(struct UIWindowMgr *self, int window_id)
{
    const struct WmWindowRow *row;
    struct UIWindow *wnd;
    struct WmClosedRect *saved;
    struct WindowInfo *rect;
    int closed = 0;

    wnd = ui_window_mgr_get_window(self, window_id);
    if (wnd) {
        saved = tree_find(&self->closed_rects, &window_id, tree_less_int);
        if (saved) {
            rect = saved->rect;
            rect->x = wnd->x;
            rect->y = wnd->y;
            rect->width = wnd->width;
            rect->height = wnd->height;
        } else {
            rect = (struct WindowInfo *)mem_alloc(sizeof *rect);
            if (rect) {
                rect->x = wnd->x;
                rect->y = wnd->y;
                rect->width = wnd->width;
                rect->height = wnd->height;
            }
            saved = tree_get(&self->closed_rects, &window_id, sizeof window_id, sizeof *saved, tree_less_int);
            saved->rect = rect;
        }
    }
    game_current_base_mode();
    if (self->modal_wnd)
        return 0;
    if (ui_window_mgr_get_window(self, window_id) == self->active_top_wnd)
        self->active_top_wnd = NULL;

    row = wm_window_row(window_id);
    if (row && row->close) {
        closed = row->close(self, window_id);
    } else {
        wnd = ui_window_mgr_get_window(self, window_id);
        if (wnd) {
            ui_window_mgr_post_quit2(self, wnd);
            closed = 1;
        }
    }

    tree_erase_key(&self->windows_by_id, &window_id, tree_less_int, NULL);
    if (closed)
        ui_window_mgr_forget_window_id(self, window_id);
    return (BYTE)closed;
}

void ui_window_mgr_destroy_window(struct UIWindowMgr *self, struct UIWindow *wnd)
{
    struct ListNode *node;

    if (wnd == NULL)
        return;
    for (node = self->windows.head->next; node != self->windows.head; node = node->next)
        if (*LIST_VALUE(node, struct UIWindow *) == wnd)
            break;
    if (node == self->windows.head)
        return;
    if (g_session1.windowSnap)
        tree_erase_key(&self->snap_rects, &wnd, tree_less_uint, NULL);
    snap_mgr_unregister(&g_snapMgr, wnd);
    ui_window_store_info(wnd);
    if (wnd)
        ui_window_destroy(wnd);
    list_erase(&self->windows, node, NULL);
}
