#include "ui/window_manager_state.h"
#include "ui/window_access.h"
#include "ui/wndmgr/wndmgr.h"
#include "game/context.h"
#include "actor/status_icons.h"
#include "ui/hideui.h"

#define WND_ID_FRAME_E 0xe
#define WND_ID_SHOP_TITLE 0xbd
#define BASIC_CMD_CLOSE_ALL 0xd6

static int in_list(struct List *list, struct UIWindow *wnd)
{
    struct ListNode *node;

    for (node = list->head->next; node != list->head; node = node->next)
        if (*LIST_VALUE(node, struct UIWindow *) == wnd)
            return 1;
    return 0;
}

static void close_all_command(struct UIWindowMgr *self)
{
    if (self->basic_info_wnd)
        ui_window_send_msg(self->basic_info_wnd, NULL, UI_MSG_COMMAND, BASIC_CMD_CLOSE_ALL, 0, 0, 0);
}

static int hide_close_tracked(struct UIWindowMgr *self)
{
    struct ListNode *node;
    int closed = 0;

    for (node = self->windows.head->next; node != self->windows.head; node = node->next) {
        struct UIWindow *wnd = *LIST_VALUE(node, struct UIWindow *);
        int id;

        if (!wnd->visible)
            continue;
        id = wnd->id;
        if (!ui_window_mgr_is_untracked_id(self, id)) {
            ui_window_mgr_post_quit1(self, id);
            closed = 1;
        } else if (!in_list(&self->hidden_wnds, wnd)) {
            list_push_back(&self->hidden_wnds, &wnd, sizeof wnd);
        }
    }
    return closed;
}

static void hide_all(struct UIWindowMgr *self)
{
    struct ListNode *node;
    struct GameNoticeWindows windows;

    for (node = self->windows.head->next; node != self->windows.head; node = node->next) {
        struct UIWindow *wnd = *LIST_VALUE(node, struct UIWindow *);

        game_get_notice_windows(&windows);
        if (wnd == windows.admin_whisper)
            continue;
        if (wnd == windows.notice)
            continue;
        ui_window_set_show(wnd, 0);
    }
    ui_window_mgr_post_quit1(self, WND_ID_FRAME_E);
    if (game_hide_player_status_icons())
        hideui_clear_count++;
    self->hide_ui_state = 2;
}

static int show_drop_new(struct UIWindowMgr *self)
{
    struct ListNode *node;
    int dropped = 0;
    struct GameNoticeWindows windows;

    for (node = self->windows.head->next; node != self->windows.head; node = node->next) {
        struct UIWindow *wnd = *LIST_VALUE(node, struct UIWindow *);

        game_get_notice_windows(&windows);
        if (wnd == windows.admin_whisper || wnd == windows.achievement || wnd == windows.notice)
            continue;
        if (!wnd->visible)
            continue;
        if (in_list(&self->hidden_wnds, wnd))
            ui_window_set_show(wnd, 0);
        else
            ui_window_mgr_post_quit1(self, wnd->id);
        dropped = 1;
    }
    return dropped;
}

static void show_all(struct UIWindowMgr *self)
{
    struct ListNode *node;

    for (node = self->hidden_wnds.head->next; node != self->hidden_wnds.head; node = node->next)
        ui_window_set_show(*LIST_VALUE(node, struct UIWindow *), 1);
    for (node = self->windows.head->next; node != self->windows.head; node = node->next) {
        struct UIWindow *wnd = *LIST_VALUE(node, struct UIWindow *);

        if (wnd->id == WND_ID_SHOP_TITLE)
            ui_window_set_show(wnd, g_windowMgr.show_shop_titles ? 1 : 0);
    }
    ui_window_mgr_make_window(self, WND_ID_FRAME_E);
    game_show_player_status_icons();
}

void ui_window_mgr_toggle_hide_ui(struct UIWindowMgr *self)
{
    struct ListNode *node, *next;

    for (node = self->hidden_wnds.head->next; node != self->hidden_wnds.head; node = next) {
        next = node->next;
        if (!in_list(&self->windows, *LIST_VALUE(node, struct UIWindow *)))
            list_erase(&self->hidden_wnds, node, NULL);
    }

    switch (self->hide_ui_state) {
    case 0:
        close_all_command(self);
        if (self->hidden_wnds.size == 0)
            for (node = self->windows.head->next; node != self->windows.head; node = node->next)
                if ((*LIST_VALUE(node, struct UIWindow *))->visible)
                    list_push_back(&self->hidden_wnds, LIST_VALUE(node, struct UIWindow *), sizeof(struct UIWindow *));
        self->hide_ui_state = 1;
        break;
    case 1:
        if (hide_close_tracked(self))
            close_all_command(self);
        else
            hide_all(self);
        break;
    case 2:
        if (show_drop_new(self)) {
            ui_window_mgr_show_shop_titles(self, 0);
            close_all_command(self);
            break;
        }
        show_all(self);
        self->hide_ui_state = 1;
        break;
    }
}
