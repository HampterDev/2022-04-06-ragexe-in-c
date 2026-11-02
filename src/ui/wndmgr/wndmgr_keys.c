#include "ui/window_keyboard.h"
#include "navi/navi_manager.h"
#include "session/state.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include "ui/wndmgr/wndmgr.h"
#include "ui/worldmap/navi_wnd.h"
#include "ui/chat/chat_window_parts.h"
#include "ui/chat/sub_chat_map.h"
#include "core/app.h"
#include "core/msg_str.h"
#include "ui/chat/chat_window.h"
#include "platform/window.h"

#define WND_ID_NONE (-1)
#define WND_ID_SAY_DIALOG 0x10
#define WND_ID_SKILL_LIST 0x25
#define WND_ID_WORLD_MAP 0x8c
#define WND_ID_ESC_MENU 0x9b
#define WND_ID_NAVI_GUIDE 0x13a
#define WND_ID_NEW_SELECT_CHAR 0x115
#define MSG_ESC_QUIT_ASK 0x11
#define MSG_NAVI_STOP_GUIDE 0x8bf
#define MSG_BACK_TO_LOGIN_ASK 0xd68
#define MODE_MSG_QUIT 2
#define MODE_MSG_SCREENSHOT 0x44
#define LOGIN_REQ_BACK_TO_LOGIN 0x271b
#define MODAL_ESCAPED 0xb4
#define EDIT_OFF_SCREEN (-100)
#define CMD_WORLD_MAP_ALT 0xd3
#define CMD_WORLD_MAP_DUNGEONS 0xb8
#define CMD_SKILL_LIST_TAB 0xd0

static int ask(struct UIWindowMgr *mgr, const char *text)
{
    return ui_window_mgr_message_box(mgr, text, UI_MSGBOX_QUESTION, 1, 0, 0, "메시지", 0x118, 0x78, 0);
}

static void click(struct UIWindow *wnd, int id)
{
    ui_window_send_msg(wnd, NULL, UI_MSG_COMMAND, id, 0, 0, 0);
}

static int escape_chat_edit(struct UIWindowMgr *self)
{
    struct UIWindow *chat = self->new_chat_wnd, *edit = self->focus_edit, *parent;
    struct TreeNode *it;

    if (chat == NULL || edit == NULL || (parent = edit->parent) == NULL || edit->x == EDIT_OFF_SCREEN)
        return 0;
    if (parent != ui_chat_window_tabs(chat)) {
        for (it = tree_first(&self->sub_chat_wnds); it != self->sub_chat_wnds.head; it = tree_next(it))
            if (TREE_VALUE(it, struct SubChatWnd)->wnd == parent)
                break;
        if (it == self->sub_chat_wnds.head)
            return 0;
    }
    ui_window_send_msg(parent, self->focus_edit, UI_MSG_DEFAULT_BUTTON, 1, 0, 0, 0);
    return 1;
}

static void escape_menu(struct UIWindowMgr *self)
{
    if (ui_window_mgr_post_quit1(self, WND_ID_ESC_MENU))
        return;
    ui_window_mgr_make_window(self, WND_ID_ESC_MENU);
}

static void escape_outside_game(struct UIWindowMgr *self)
{
    if (ui_window_mgr_get_window(self, WND_ID_NEW_SELECT_CHAR) && g_session1.canBackToLogin == 1) {
        if (ask(&g_windowMgr, msg_str(MSG_BACK_TO_LOGIN_ASK)) == UI_MSGBOX_ANSWER_YES)
            mode_send_message(client_current_mode(), LOGIN_REQ_BACK_TO_LOGIN, 0, 0, 0, 0);
        return;
    }
    if (ask(self, msg_str(MSG_ESC_QUIT_ASK)) == UI_MSGBOX_ANSWER_YES) {
        connection_disconnect(rag_connection_transport(rag_connection_instance()));
        mode_send_message(client_current_mode(), MODE_MSG_QUIT, 0, 0, 0, 0);
    }
}

static void escape_navigation(void)
{
    if (ask(&g_windowMgr, msg_str(MSG_NAVI_STOP_GUIDE)) != UI_MSGBOX_ANSWER_YES)
        return;
    g_navi.windowPage = 0;
    g_navi.windowCollapsed = 0;
    navi_manager_reset(&g_navi, 1);
    world_view_clear_marks(&g_worldView);
    world_view_clear_level_marks(&g_worldView);
    world_view_clear_current(&g_worldView);
    ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_NAVI_GUIDE);
}

static void escape(struct UIWindowMgr *self)
{
    struct UIWindow *wnd;
    int id;

    if (self->auto_message_wnd)
        g_session1.autoMessageOn = 0;
    if ((ui_window_mgr_is_chat_edit_focused(self) ||
         (self->id_list.head->next != self->id_list.head && *LIST_VALUE(self->id_list.head->next, int) == 1)) &&
        self->chat_bar_shown && self->chat_bar_toggleable) {
        self->chat_bar_shown = 0;
        ui_new_chat_wnd_place_bar((struct UINewChatWnd *)self->new_chat_wnd);
        return;
    }
    if (escape_chat_edit(self))
        return;
    if (self->modal_wnd) {
        self->modal_result = MODAL_ESCAPED;
        return;
    }

    id = self->id_list.head->next != self->id_list.head ? *LIST_VALUE(self->id_list.head->next, int) : WND_ID_NONE;

    wnd = ui_window_find(id);
    if (wnd) {
        switch (id) {
        case 0x84: case 0xb5: case 0xb8: case 0xcb: case 0x119: case 0x146:
            click(wnd, UI_ID_CLOSE);
            return;
        case 0x16: case 0x17: case 0x18: case 0x19: case 0x29: case 0x2c: case 0xa1: case 0xae: case 0xb2:
            click(wnd, UI_ID_CANCEL);
            return;
        }
    }
    if (id != WND_ID_NONE) {
        if (ui_window_mgr_post_quit1(self, id) == 1)
            return;
    } else if (g_navi.guideState == 2) {
        escape_navigation();
        return;
    }

    if (g_quitRequested || !g_modeMgr.loopCond)
        return;
    if (ui_window_mgr_get_window(self, WND_ID_SAY_DIALOG))
        return;
    if (game_current_base_mode())
        escape_menu(self);
    else
        escape_outside_game(self);
}

static void tab(struct UIWindowMgr *self)
{
    struct UIWindow *wnd;

    if (HIBYTE(platform_async_key_state(VK_CONTROL)))
        return;
    if (self->key_modifier == 0) {
        if ((wnd = ui_window_mgr_get_window(self, WND_ID_WORLD_MAP)) != NULL) {
            click(wnd, CMD_WORLD_MAP_DUNGEONS);
            return;
        }
        if ((wnd = ui_window_mgr_get_window(self, WND_ID_SKILL_LIST)) != NULL) {
            click(wnd, CMD_SKILL_LIST_TAB);
            return;
        }
    }
    if (self->focus_edit == NULL) {
        ui_window_mgr_focus_first_edit(self);
        return;
    }
    if (platform_async_key_state(VK_SHIFT)) {
        if (ui_window_mgr_is_first_edit(self, self->focus_edit))
            ui_window_mgr_set_focus_edit_last(self);
        else
            ui_window_mgr_focus_prev_edit(self);
    } else {
        if (ui_window_mgr_is_last_edit(self, self->focus_edit))
            ui_window_mgr_set_focus_edit_first(self);
        else
            ui_window_mgr_focus_next_edit(self);
    }
}

BYTE ui_window_mgr_handle_key(struct UIWindowMgr *self, int vk, int first_press)
{
    struct UIWindow *wnd;

    switch (vk) {
    case VK_TAB:
        tab(self);
        break;
    case VK_RETURN:
        if (first_press)
            ui_window_mgr_on_enter(self);
        break;
    case VK_MENU:
        if (self->key_modifier == 0 && (wnd = ui_window_mgr_get_window(self, WND_ID_WORLD_MAP)) != NULL)
            click(wnd, CMD_WORLD_MAP_ALT);
        break;
    case VK_ESCAPE:
        if (first_press)
            escape(self);
        break;
    case VK_PRIOR:
        if (self->active_top_wnd)
            ui_window_send_msg3_to_tree(self->active_top_wnd);
        break;
    case VK_NEXT:
        if (self->active_top_wnd)
            ui_window_send_msg4_to_tree(self->active_top_wnd);
        break;
    case VK_LEFT:
        ui_window_mgr_send_key_left(self);
        break;
    case VK_UP:
        ui_window_mgr_send_key_up(self);
        break;
    case VK_RIGHT:
        ui_window_mgr_send_key_right(self);
        break;
    case VK_DOWN:
        ui_window_mgr_send_key_down(self);
        break;
    case VK_SNAPSHOT:
    case VK_SCROLL:
        if (first_press)
            mode_send_message(client_current_mode(), MODE_MSG_SCREENSHOT, 0, 0, 0, 0);
        break;
    }
    return 0;
}
