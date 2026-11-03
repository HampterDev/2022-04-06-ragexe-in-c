#include "ui/window_manager_state.h"
#include "ui/window_mgr.h"
#include "ui/wndmgr/window_table.h"
#include "ui/wndmgr/wndmgr_handlers.h"

static void forget(struct UIWindow **ref, const struct UIWindow *wnd)
{
    if (*ref == wnd)
        *ref = NULL;
}

void ui_window_mgr_forget_window(struct UIWindowMgr *self, const struct UIWindow *wnd)
{
    size_t i;

    for (i = 0; i < wm_window_row_count(); i++) {
        const struct WmWindowRow *row = wm_window_row_at(i);

        if (row->slot != WM_NO_SLOT && !(row->flags & WM_KEPT_BY_FORGET))
            forget(wm_row_slot(self, row), wnd);
    }
    /* The window manager's own references; focus_drawn is only compared, never used. */
    forget(&self->capture_root, wnd);
    forget(&self->capture, wnd);
    forget(&self->focus_edit, wnd);
    forget(&self->modal_wnd, wnd);
    forget(&self->mouse_wnd, wnd);
    forget(&self->active_top_wnd, wnd);
    if (g_windowMgrHandlers.forget_window)
        g_windowMgrHandlers.forget_window(wnd);
}
