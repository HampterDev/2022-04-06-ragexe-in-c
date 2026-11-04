#include "render/renderer.h"
#include "ui/window_access.h"
#include "ui/wndmgr/make_window_internal.h"
#include "ui/wndmgr/window_table.h"
#include "ui/wndmgr/wndmgr_handlers.h"

void wm_move(struct UIWindow *wnd, int x, int y)
{
    ui_window_move(wnd, x, y);
}

void wm_set_info(struct UIWindow *wnd, void *info)
{
    ui_window_send_msg(wnd, NULL, UI_MSG_SET_INFO, (intptr_t)info, 0, 0, 0);
}

void wm_show(struct UIWindow *wnd)
{
    ui_window_set_show(wnd, 1);
}

int wm_center_x(int width)
{
    return (g_renderer->width - width) / 2;
}
int wm_center_y(int height)
{
    return (g_renderer->height - height) / 2;
}

void wm_move_640(struct UIWindow *wnd, int x, int y480)
{
    int y = scale_y_from480(y480);

    wm_move(wnd, ui_center_x640(x), y);
}
void wm_move_640_up(struct UIWindow *wnd, int x, int y480, int up)
{
    int y = scale_y_from480(y480) - up;

    wm_move(wnd, ui_center_x640(x), y);
}
void wm_move_center(struct UIWindow *wnd, int x, int y480)
{
    int y = ui_center_y480(y480);

    wm_move(wnd, ui_center_x640(x), y);
}

struct UIWindow *wm_register(struct UIWindowMgr *self, int window_id, struct UIWindow *wnd)
{
    struct WmWindowEntry *entry;
    struct UIWindow *top;

    if (wnd == NULL)
        return NULL;
    wnd->id = window_id;
    entry = tree_get(&self->windows_by_id, &window_id, sizeof window_id, sizeof *entry, tree_less_int);
    entry->wnd = wnd;
    for (top = wnd; top->parent; top = top->parent)
        ;

    self->active_top_wnd = top;
    return wnd;
}

static void wm_place(struct UIWindow *wnd, int at, int x, int y)
{
    switch (at) {
    case WM_AT_XY:
        wm_move(wnd, x, y);
        break;
    case WM_AT_640:
        wm_move_640(wnd, x, y);
        break;
    case WM_AT_CENTER:
        wm_move_center(wnd, x, y);
        break;
    case WM_AT_640_UP:
        wm_move_640_up(wnd, x, y, 0x3c);
        break;
    }
}

struct UIWindow *wm_open(struct UIWindowMgr *self, struct UIWindow **slot, WmNew new_window, WmNew1 new_window1,
                         int arg, int width, int height, int at, int x, int y, void *info)
{
    struct UIWindow *wnd = *slot;

    if (wnd == NULL) {
        wnd = new_window ? new_window() : new_window1(arg);
        *slot = wnd;
        ui_window_create(wnd, width, height);
        wm_place(wnd, at, x, y);
        ui_window_mgr_add_window(self, wnd);
        if (info)
            wm_set_info(wnd, info);
    }
    wm_show(wnd);
    return wnd;
}

struct UIWindow *wm_open_row(struct UIWindowMgr *self, int window_id)
{
    const struct WmWindowRow *row = wm_window_row(window_id);
    const struct WmOpen *open = &row->open;

    return wm_open(self, wm_row_slot(self, row), open->new_window, open->new_window1, open->arg, open->width,
                   open->height, open->at, open->x, open->y, open->info ? (unsigned char *)self + open->info : NULL);
}

int wm_times_half(int v)
{
    return (int)((double)v * 0.5);
}

void wm_move_on_centre(struct UIWindow *wnd)
{
    wm_move(wnd, g_renderer->centreX - wnd->width / 2, g_renderer->centreY - wnd->height / 2);
}

static struct UIWindow *make_window_by_id(struct UIWindowMgr *self, int window_id);

struct UIWindow *ui_window_mgr_make_window(struct UIWindowMgr *self, int window_id)
{
    const struct WmWindowRow *row = wm_window_row(window_id);
    struct UIWindow *wnd;
    int tracked;

    if (g_windowMgrHandlers.drop_dragged_plate)
        g_windowMgrHandlers.drop_dragged_plate(self);
    tracked = !ui_window_mgr_is_untracked_id(self, window_id);
    if (tracked) {
        ui_window_mgr_forget_window_id(self, window_id);
        list_insert(&self->id_list, row && (row->flags & WM_ESC_LAST) ? self->id_list.head : self->id_list.head->next,
                    &window_id, sizeof window_id);
    }
    wnd = make_window_by_id(self, window_id);
    /* ESC closes the listed IDs; a request that left no window must not stay listed. */
    if (wnd == NULL && tracked && ui_window_mgr_get_window(self, window_id) == NULL)
        ui_window_mgr_forget_window_id(self, window_id);
    return wnd;
}

static struct UIWindow *make_window_by_id(struct UIWindowMgr *self, int window_id)
{
    const struct WmWindowRow *row = wm_window_row(window_id);

    if (row == NULL || row->make == NULL)
        return NULL;
    return wm_register(self, window_id, row->make(self, window_id));
}

int scale_y_from480(int n)
{
    return n * g_renderer->height / 480;
}
