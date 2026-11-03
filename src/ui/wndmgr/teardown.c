#include "session/state.h"
#include "ui/wndmgr/wndmgr.h"
#include "ui/wndmgr/window_table.h"

#include "ui/chat/sub_chat_map.h"

#define SUB_CHAT_WNDS(self) (&(self)->sub_chat_wnds)

#define WM_MODAL_END_TEARDOWN 0xb4
#define WHISPER_WND_ID 0x43

static void destroy_window(struct UIWindow *wnd)
{
    ui_window_store_info(wnd);
    if (wnd)
        ui_window_destroy(wnd);
}

static void destroy_plates(struct UIWindowMgr *self)
{
    struct ListNode *node;

    for (node = self->plates.head->next; node != self->plates.head; node = node->next) {
        struct UIWindow **plate = LIST_VALUE(node, struct UIWindow *);

        ui_window_mgr_destroy_window(self, *plate);
        *plate = NULL;
    }
    list_clear(&self->plates, NULL);
}

void ui_window_mgr_close_all_windows(struct UIWindowMgr *self)
{
    struct ListNode *node;

    if (self->modal_wnd)
        self->modal_result = WM_MODAL_END_TEARDOWN;
    for (node = self->windows.head->next; node != self->windows.head; node = node->next) {
        struct UIWindow **wnd = LIST_VALUE(node, struct UIWindow *);

        if (self->modal_wnd && self->capture_root == *wnd) {
            self->close_capture_root = 1;
            continue;
        }
        destroy_window(*wnd);
        *wnd = NULL;
    }
    list_clear(&self->windows, NULL);
    list_clear(&self->quit_list, NULL);
    tree_clear(&self->snap_rects, NULL);
    self->message_boxes.last = self->message_boxes.first;
    ui_window_mgr_clear_slots(self);
    destroy_plates(self);
}

static int not_a_tab(struct UIWindowMgr *self, struct UIWindow *wnd)
{
    int i, other = 1;

    for (i = 0; other && (unsigned)i < self->sub_chat_wnds.size; i++) {
        struct SubChatWnd *sub = tree_get(SUB_CHAT_WNDS(self), &i, sizeof i, sizeof *sub, tree_less_int);

        other = wnd != sub->wnd;
    }
    return other;
}

void ui_window_mgr_close_all_but_chat(struct UIWindowMgr *self)
{
    struct ListNode *node, *next;
    struct TreeNode *it;
    struct Tree tabs;
    struct UIWindow *chat;

    if (self->modal_wnd)
        self->modal_result = WM_MODAL_END_TEARDOWN;
    if (self->new_chat_wnd == NULL) {
        ui_window_mgr_close_all_windows(self);
        return;
    }
    for (node = self->windows.head->next; node != self->windows.head; node = next) {
        struct UIWindow *wnd = *LIST_VALUE(node, struct UIWindow *);

        next = node->next;
        if (!not_a_tab(self, wnd) || wnd == self->new_chat_wnd || wnd->id == WHISPER_WND_ID)
            continue;
        if (wnd == self->memorial_dun_wnd && g_session1.mdWndShown) {
            g_session1.mdWndX = wnd->x;
            g_session1.mdWndY = wnd->y;
        }
        if (self->modal_wnd && self->capture_root == wnd) {
            self->close_capture_root = 1;
            continue;
        }
        destroy_window(wnd);
        list_erase(&self->windows, node, NULL);
    }
    for (node = self->quit_list.head->next; node != self->quit_list.head; node = next) {
        struct UIWindow *wnd = *LIST_VALUE(node, struct UIWindow *);

        next = node->next;
        if (not_a_tab(self, wnd) && self->new_chat_wnd != wnd)
            list_erase(&self->quit_list, node, NULL);
    }
    for (it = tree_first(&self->snap_rects); it != self->snap_rects.head;) {
        if (TREE_VALUE(it, struct WmSnapRect)->wnd != self->new_chat_wnd)
            it = tree_erase(&self->snap_rects, it, NULL);
        else
            it = tree_next(it);
    }
    chat = self->new_chat_wnd;
    tree_copy(&tabs, SUB_CHAT_WNDS(self), sizeof(struct SubChatWnd), NULL);
    ui_window_mgr_clear_slots(self);
    self->new_chat_wnd = chat;
    tree_free(SUB_CHAT_WNDS(self), NULL);
    tree_copy(SUB_CHAT_WNDS(self), &tabs, sizeof(struct SubChatWnd), NULL);
    destroy_plates(self);
    tree_free(&tabs, NULL);
}

void ui_window_mgr_clear_slots(struct UIWindowMgr *self)
{
    size_t i;

    tree_clear(&self->windows_by_id, NULL);
    tree_clear(SUB_CHAT_WNDS(self), NULL);
    self->message_boxes.last = self->message_boxes.first;
    list_clear(&self->hidden_wnds, NULL);
    self->hide_ui_state = 0;

    /* The window manager's own references; capture_root and focus_drawn stay set (a known,
       harmless gap: the windows' destructors forget capture_root). */
    self->capture = NULL;
    self->focus_edit = NULL;
    self->modal_wnd = NULL;
    self->mouse_wnd = NULL;
    self->active_top_wnd = NULL;
    for (i = 0; i < wm_window_row_count(); i++) {
        const struct WmWindowRow *row = wm_window_row_at(i);

        if (row->slot != WM_NO_SLOT && !(row->flags & WM_KEPT_BY_CLEAR))
            *wm_row_slot(self, row) = NULL;
    }
}
