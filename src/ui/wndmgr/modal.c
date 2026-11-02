#include "ui/window_access.h"
#include "world/mode_manager.h"
#include "ui/wndmgr/wndmgr.h"
#include "core/app.h"
#include "ui/character/character.h"

int ui_window_mgr_modal_loop(struct UIWindowMgr *self, struct UIWindow *wnd)
{
    self->modal_result = 0;
    while (!g_quitRequested) {
        if (self->modal_result) {
            ui_window_mgr_post_quit2(self, wnd);
            return self->modal_result;
        }
        mode_update(client_current_mode());
    }
    return UI_ID_OK;
}

static int run_modal(struct UIWindowMgr *self, struct UIWindow *wnd, int drop_focus)
{
    int result;

    self->capture_root = self->capture ? ui_window_get_ancestor(self->capture) : NULL;
    self->capture = NULL;
    if (drop_focus) {
        if (self->focus_edit)
            ui_window_on_focus_lost(self->focus_edit);
        self->focus_edit = NULL;
    }
    self->modal_wnd = wnd;
    result = ui_window_mgr_modal_loop(self, wnd);
    self->modal_wnd = NULL;
    ui_window_mgr_focus_first_edit(self);
    return result;
}

int ui_window_mgr_do_modal(struct UIWindowMgr *self, struct UIWindow *wnd)
{
    if (wnd == NULL)
        return UI_ID_CANCEL;
    return run_modal(self, wnd, 1);
}

int ui_window_mgr_run_email_address_box(struct UIWindowMgr *self)
{
    struct UIWindow *wnd;

    if (self->modal_wnd)
        return UI_ID_CANCEL;
    wnd = (struct UIWindow *)ui_email_address_wnd_new();
    ui_window_create(wnd, 0x118, 0x30);
    ui_window_move(wnd, (g_renderer->width - 640) / 2 + 0xb9, (g_renderer->height - 480) / 2 + 0xb4);
    list_push_back(&self->windows, &wnd, sizeof wnd);
    return run_modal(self, wnd, 0);
}

int ui_window_mgr_message_box(struct UIWindowMgr *self, const char *text, int type, int arg3, int arg4,
                                      int arg5, const char *title, int width, int height, int arg9)
{
    struct UIWindow *box;

    if (self->modal_wnd) {
        if (ui_window_send_msg(self->modal_wnd, NULL, 0x9e, 0, 0, 0, 0) != 1 && arg4)
            ui_window_send_msg(self->modal_wnd, NULL, 0x39, (int)text, 0, 0, 0);
        return UI_ID_CANCEL;
    }
    box = (struct UIWindow *)ui_message_box_new();
    ui_message_box_create((struct UIMessageBox *)box, text, type, arg3, arg5, title, width, height, 1, NULL);
    if ((BYTE)arg9 == 1)
        ui_window_send_msg(box, NULL, 0x9d, 1, 0, 0, 0);
    list_push_back(&self->windows, &box, sizeof box);
    return run_modal(self, box, 1);
}
