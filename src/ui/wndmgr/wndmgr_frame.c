#include "ui/window_access.h"
#include <stdint.h>
#include "contents/mouse.h"
#include "world/mode_manager.h"
#include "ui/wndmgr/wndmgr.h"
#include "ui/social/party_windows.h"

#define MOUSE_PRESSED 1
#define MOUSE_RELEASED 3
#define MOUSE_DOUBLE 4

static void drop_ref(struct UIWindow **ref, struct UIWindow *wnd)
{
    if (*ref && (*ref == wnd || ui_window_has_ancestor(*ref, wnd)))
        *ref = NULL;
}

static void destroy_closed_windows(struct UIWindowMgr *self)
{
    struct ListNode *node;

    for (node = self->quit_list.head->next; node != self->quit_list.head; node = node->next) {
        struct UIWindow *wnd = *LIST_VALUE(node, struct UIWindow *);

        if (self->replay_wnd == wnd)
            self->replay_wnd = NULL;
        drop_ref(&self->capture, wnd);
        drop_ref(&self->focus_edit, wnd);
        drop_ref(&self->modal_wnd, wnd);
        drop_ref(&self->mouse_wnd, wnd);
        ui_window_mgr_destroy_window(self, wnd);
    }
    list_clear(&self->quit_list, NULL);
}

static void delete_queued_browsers(struct UIWindowMgr *self)
{
    struct ListNode *node;

    for (node = self->browsers_to_delete.head->next; node != self->browsers_to_delete.head; node = node->next) {
        int hwnd = *LIST_VALUE(node, int);
        struct WmBrowser *entry = tree_find(&self->browsers, &hwnd, tree_less_uint);
        struct WebBrowser *browser;

        if (entry) {
            browser = entry->browser;
            if (browser) {
                if (browser == self->browser)
                    self->browser = NULL;
                wm_destroy_browser(self, browser);
            }
            tree_erase(&self->browsers, TREE_NODE_OF(entry), NULL);
        } else {
            browser = self->browser;
            if (browser && (HWND)(intptr_t)hwnd == browser->hwnd) {
                if (browser)
                    wm_destroy_browser(self, browser);
                self->browser = NULL;
            }
        }
    }
    list_clear(&self->browsers_to_delete, NULL);
}

static void close_removed_plates(struct UIWindowMgr *self)
{
    struct ListNode *id, *node, *next;

    for (id = self->plate_ids.head->next; id != self->plate_ids.head; id = id->next) {
        for (node = self->plates.head->next; node != self->plates.head; node = next) {
            struct UIWindow *wnd = *LIST_VALUE(node, struct UIWindow *);
            struct UIMiniPartyWnd *plate = (struct UIMiniPartyWnd *)wnd;

            next = node->next;
            if (*LIST_VALUE(id, int) == plate->member.aid) {
                ui_window_mgr_destroy_window(self, wnd);
                list_erase(&self->plates, node, NULL);
            }
        }
    }
    list_clear(&self->plate_ids, NULL);
}

static void raise_on_press(struct UIWindowMgr *self, struct UIWindow *wnd)
{
    struct UIWindow *root = ui_window_get_ancestor(wnd);

    if (root && root->ops->is_frame)
        ui_window_mgr_raise_window(self, root);
}

static int double_click_allowed(void)
{
    return !g_replayPlaying || g_modeMgr.curModeType == 0;
}

static void mouse_to_capture(struct UIWindowMgr *self, int mx, int my)
{
    int sx = 0, sy = 0, x, y;

    ui_window_to_screen(self->capture, &sx, &sy);
    x = mx - sx;
    y = my - sy;
    if (g_mouse.button[MOUSE_LEFT] == MOUSE_PRESSED) {
        raise_on_press(self, self->capture);
        ui_window_on_l_btn_down(self->capture, x, y);
    }
    if (g_mouse.button[MOUSE_LEFT] == MOUSE_DOUBLE && double_click_allowed()) {
        ui_window_on_l_btn_down(self->capture, x, y);
        ui_window_on_l_btn_dbl_clk(self->capture, x, y);
    }
    ui_window_on_mouse_shape(self->capture, x, y);
    if (mx == self->mouse_x_last && my == self->mouse_y_last)
        ui_window_on_mouse_hover(self->capture, x, y);
    else
        ui_window_on_mouse_move(self->capture, x, y);
    if (g_mouse.wheel)
        ui_window_on_wheel(self->capture, g_mouse.wheel);
    if (g_mouse.button[MOUSE_LEFT] == MOUSE_RELEASED)
        ui_window_on_l_btn_up(self->capture, x, y);
}

static int mouse_to_window(struct UIWindowMgr *self, struct UIWindow *wnd, int mx, int my)
{
    int answer = ui_window_is_click_through(wnd) ? 2 : 1;
    int sx = 0, sy = 0, x, y;

    ui_window_to_screen(wnd, &sx, &sy);
    x = mx - sx;
    y = my - sy;
    if (g_mouse.button[MOUSE_LEFT] == MOUSE_DOUBLE && double_click_allowed())
        ui_window_on_l_btn_dbl_clk(wnd, x, y);
    if (g_mouse.button[MOUSE_LEFT] == MOUSE_PRESSED) {
        raise_on_press(self, wnd);
        ui_window_on_l_btn_down(wnd, x, y);
    }
    if (g_mouse.button[MOUSE_RIGHT] == MOUSE_PRESSED) {
        raise_on_press(self, wnd);
        ui_window_on_r_btn_down(wnd, x, y);
    }
    ui_window_on_mouse_shape(wnd, x, y);
    if (mx == self->mouse_x_last && my == self->mouse_y_last)
        ui_window_on_mouse_hover(wnd, x, y);
    else
        ui_window_on_mouse_move(wnd, x, y);
    if (g_mouse.wheel)
        ui_window_on_wheel(wnd, g_mouse.wheel);
    if (g_mouse.button[MOUSE_RIGHT] == MOUSE_RELEASED)
        ui_window_on_r_btn_up(wnd, x, y);
    if (g_mouse.button[MOUSE_LEFT] == MOUSE_RELEASED)
        ui_window_on_l_btn_up(wnd, x, y);
    return answer;
}

static void close_combo_box(struct UIWindowMgr *self, struct UIWindow *combo, int window_id, struct UIWindow *wnd)
{
    if (combo && (wnd == NULL || (wnd != combo && !ui_window_has_ancestor(wnd, combo))))
        ui_window_mgr_post_quit1(self, window_id);
}

int ui_window_mgr_process_input(struct UIWindowMgr *self)
{
    int mx = g_mouse.x, my = g_mouse.y;
    int answer = 0;
    struct UIWindow *wnd;
    struct ListNode *node;
    struct TreeNode *it;

    if (self->modal_wnd == NULL)
        destroy_closed_windows(self);
    if (self->browsers_to_delete.size)
        delete_queued_browsers(self);
    close_removed_plates(self);

    if (self->capture) {
        answer = 1;
        mouse_to_capture(self, mx, my);
    } else {
        wnd = ui_window_mgr_window_at(self, mx, my);
        if (self->modal_wnd && wnd && wnd != self->modal_wnd && !ui_window_has_ancestor(wnd, self->modal_wnd))
            wnd = NULL;
        if (g_mouse.button[MOUSE_LEFT] == MOUSE_PRESSED)
            self->active_top_wnd = wnd ? ui_window_get_ancestor(wnd) : NULL;
        if (wnd)
            answer = mouse_to_window(self, wnd, mx, my);
        if (g_mouse.button[MOUSE_LEFT] == MOUSE_PRESSED) {
            close_combo_box(self, self->combo_box_wnd, 0x1a, wnd);
            close_combo_box(self, self->combo_box_idwnd, 0x104, wnd);
        }

        if (wnd) {
            if (self->mouse_wnd && wnd != self->mouse_wnd) {
                int sx = 0, sy = 0;

                ui_window_to_screen(self->mouse_wnd, &sx, &sy);
                ui_window_on_mouse_move(self->mouse_wnd, mx - sx, my - sy);
                ui_window_on_mouse_shape(self->mouse_wnd, mx - sx, my - sy);
            }
            self->mouse_wnd = wnd;
        }
    }

    self->mouse_y_last = my;
    self->mouse_x_last = mx;
    if (self->modal_wnd)
        answer = 1;
    for (it = tree_first(&self->browsers); it != self->browsers.head; it = tree_next(it)) {
        struct WebBrowser *browser = TREE_VALUE(it, struct WmBrowser)->browser;

        if (browser && browser->cursor_shown) {
            answer = 1;
            break;
        }
    }
    if (self->windows.head->next != self->windows.head) {
        node = self->windows.head->prev;
        for (;;) {
            ui_window_track_mouse(*LIST_VALUE(node, struct UIWindow *), mx, my);
            if (node == self->windows.head->next)
                break;
            node = node->prev;
        }
    }
    return answer;
}

void ui_window_mgr_on_process(struct UIWindowMgr *self)
{
    struct ListNode *node;
    struct TreeNode *it;

    for (node = self->windows.head->next; node != self->windows.head; node = node->next)
        ui_window_on_process(*LIST_VALUE(node, struct UIWindow *));
    for (it = tree_first(&self->browsers); it != self->browsers.head; it = tree_next(it)) {
        struct WebBrowser *browser = TREE_VALUE(it, struct WmBrowser)->browser;

        if (browser)
            web_browser_update(browser);
        if (self->browsers.size == 0)
            break;
    }
}
