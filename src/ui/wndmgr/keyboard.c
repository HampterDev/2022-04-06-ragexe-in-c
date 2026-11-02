#include "ui/controls/edit.h"
#include "ui/window_keyboard.h"
#include "contents/client_lua_calls.h"
#include "actor/render_object.h"
#include "session/state.h"
#include "world/mode_manager.h"
#include "ui/wndmgr/wndmgr.h"
#include "ui/character/character.h"
#include "ui/chat/chat_window_parts.h"
#include "ui/character/select_char_new.h"

#define WM_KEY_NONE 0
#define WM_KEY_MODAL 1
#define WM_KEY_MODIFIER 2
#define WM_KEY_COMBO 3
#define WM_KEY_PLAIN 4

static int key_from_scan_code(int lparam)
{
    switch (lparam) {
    case 0x100001: return 'Q';
    case 0x110001: return 'W';
    case 0x120001: return 'E';
    case 0x130001: return 'R';
    case 0x140001: return 'T';
    case 0x150001: return 'Y';
    case 0x160001: return 'U';
    case 0x170001: return 'I';
    case 0x180001: return 'O';
    case 0x190001: return 'P';
    case 0x1e0001: return 'A';
    case 0x1f0001: return 'S';
    case 0x200001: return 'D';
    case 0x210001: return 'F';
    case 0x220001: return 'G';
    case 0x230001: return 'H';
    case 0x240001: return 'J';
    case 0x250001: return 'K';
    case 0x260001: return 'L';
    case 0x2c0001: return 'Z';
    case 0x2d0001: return 'X';
    case 0x2e0001: return 'C';
    case 0x2f0001: return 'V';
    case 0x300001: return 'B';
    case 0x310001: return 'N';
    case 0x320001: return 'M';
    case 0x330001: return VK_OEM_COMMA;
    case 0x340001: return VK_OEM_PERIOD;
    }
    return VK_PROCESSKEY;
}

int ui_window_mgr_classify_key(struct UIWindowMgr *self, int vk, int lparam)
{
    int key, mod;

    if (self->modal_wnd && self->hot_key_wnd)
        return WM_KEY_MODAL;
    key = vk == VK_PROCESSKEY ? key_from_scan_code(lparam) : vk;
    self->last_key = key;
    self->key_modifier = 0;
    if (key == VK_ESCAPE)
        return WM_KEY_NONE;
    if ((GetAsyncKeyState(VK_MENU) >> 8 & 0xff) && self->last_key != VK_MENU)
        self->key_modifier = VK_MENU;
    else if ((GetAsyncKeyState(VK_SHIFT) >> 8 & 0xff) && self->last_key != VK_SHIFT)
        self->key_modifier = VK_SHIFT;
    else if ((GetAsyncKeyState(VK_CONTROL) >> 8 & 0xff) && self->last_key != VK_CONTROL)
        self->key_modifier = VK_CONTROL;
    else if ((GetAsyncKeyState(VK_TAB) >> 8 & 0xff) && self->last_key != VK_TAB)
        self->key_modifier = VK_TAB;
    mod = self->key_modifier;
    key = self->last_key;
    if (mod == 0) {
        if (key == VK_TAB || key == VK_MENU || key == VK_CONTROL || key == VK_SHIFT)
            return WM_KEY_MODIFIER;
        return WM_KEY_PLAIN;
    }
    if ((key == VK_TAB && mod != VK_CONTROL) || (key == VK_MENU && mod == VK_TAB) ||
        (key == VK_SHIFT && mod == VK_TAB))
        return WM_KEY_COMBO;
    if ((key == VK_MENU && (mod == VK_CONTROL || mod == VK_SHIFT)) ||
        (key == VK_CONTROL && (mod == VK_TAB || mod == VK_SHIFT))) {
        self->last_key = mod;
        self->key_modifier = key;
    }
    return WM_KEY_PLAIN;
}

BYTE ui_window_mgr_on_enter(struct UIWindowMgr *self)
{
    struct UIWindow *wnd, *chat;
    struct ListNode *node;

    wnd = self->modal_wnd;
    if (wnd == NULL && self->npc_edit_dialog_wnd) {
        wnd = self->npc_edit_dialog_wnd;
    }
    if (wnd == NULL && self->npc_text_edit_dialog_wnd) {
        wnd = self->npc_text_edit_dialog_wnd;
    }
    if (wnd == NULL)
        wnd = self->choose_wnd;
    if (wnd == NULL)
        wnd = self->say_dialog_wnd;
    if (wnd == NULL)
        wnd = self->combo_box_wnd;
    if (wnd == NULL && self->combo_box_idwnd) {
        ui_window_send_msg(self->combo_box_idwnd, NULL, 0x13, 0, 0, 0, 0);
        return 1;
    }
    if (wnd == NULL)
        wnd = self->choose_warp_wnd;
    if (wnd == NULL)
        wnd = self->make_target_list_wnd;
    if (wnd == NULL)
        wnd = self->make_target_process_wnd;
    if (wnd == NULL)
        wnd = self->party_setting_wnd;
    if (wnd) {
        ui_window_send_msg(wnd, NULL, UI_MSG_DEFAULT_BUTTON, 0, 0, 0, 0);
        return 1;
    }

    if (self->focus_edit) {
        struct UIWindow *parent = self->focus_edit->parent;

        chat = self->new_chat_wnd;

        if (g_windowMgr.chat_bar_toggleable && parent == chat)
            return 0;
        ui_window_send_msg(self->focus_edit, NULL, UI_MSG_DEFAULT_BUTTON, 0, 0, 0, 0);
        return 1;
    }
    if (self->new_select_char_wnd) {
        ui_new_select_char_wnd_send_msg(self->new_select_char_wnd, NULL, UI_MSG_DEFAULT_BUTTON, 0, 0, 0, 0);
        return 1;
    }
    if (self->new_chat_wnd && g_windowMgr.chat_bar_shown == 1) {
        ui_window_send_msg(self->new_chat_wnd, NULL, UI_MSG_DEFAULT_BUTTON, 0, 0, 0, 0);
        return 0;
    }
    for (node = self->windows.head->next; node != self->windows.head; node = node->next) {
        wnd = *LIST_VALUE(node, struct UIWindow *);
        if (!wnd->ops->is_frame)
            continue;

        ui_window_send_msg(wnd, NULL, UI_MSG_DEFAULT_BUTTON, 0, 0, 0, 0);
    }
    return 0;
}

#define WM_SHORTCUT_USE 0x29

static void use_shortcut(struct UIWindow *bar, int row, int slot)
{
    ui_window_send_msg(bar, NULL, WM_SHORTCUT_USE, slot, row, 0, 0);
}

BYTE ui_window_mgr_excute_msg_in_battle_mode(struct UIWindowMgr *self, int lparam)
{
    struct UIWindow *bar;

    if (!g_session1.battle_mode || g_session1.battle_mode_suspended)
        return 0;
    bar = self->short_cut_wnd;
    if (bar == NULL)
        return 0;
    switch (lparam) {
    case 0x100001: case 0x110001: case 0x120001: case 0x130001: case 0x140001: case 0x150001:
    case 0x160001: case 0x170001: case 0x180001:
        use_shortcut(bar, 1, (lparam >> 16) - 0x10);
        break;
    case 0x1e0001: case 0x1f0001: case 0x200001: case 0x210001: case 0x220001: case 0x230001:
    case 0x240001: case 0x250001: case 0x260001:
        use_shortcut(bar, 2, (lparam >> 16) - 0x1e);
        break;
    case 0x2c0001: case 0x2d0001: case 0x2e0001: case 0x2f0001: case 0x300001: case 0x310001:
    case 0x320001: case 0x330001: case 0x340001:
        use_shortcut(bar, 0, (lparam >> 16) - 0x2c);
        break;
    }
    return 0;
}

int ui_window_mgr_get_hot_key_behavior(struct UIWindowMgr *self)
{
    struct HotKey key;
    struct UIWindow *parent;
    int behavior = -1, count, i;

    client_lua_call(g_session1.luaState, "GetBehaviorOfHotKey2", "ddd>d", session_get_instant_var(&g_session1, 10),
            self->last_key, self->key_modifier, &behavior);
    if (behavior != -1)
        return behavior;
    if (self->key_modifier != VK_SHIFT || g_windowMgr.chat_bar_shown)
        return -1;
    if (self->focus_edit) {
        parent = self->focus_edit->parent;
        if (parent && parent != self->new_chat_wnd && parent->visible)
            return -1;
    }
    count = session_get_hot_key_list_size(&g_session1, 0);
    for (i = 0; i < count && i < HOTKEY_SLOTS; i++) {
        session_get_hot_key(&g_session1, &key, 0, i);
        if (key.code == self->last_key && g_session1.hotkeys0[i].is_skill) {
            string_free(&key.action);
            string_free(&key.label);
            return i;
        }
        string_free(&key.action);
        string_free(&key.label);
    }
    return -1;
}

BYTE ui_window_mgr_is_hot_key_of_window(struct UIWindowMgr *self)
{
    BYTE is_window = 0;

    client_lua_call(g_session1.luaState, "IsBeHaviorOfWindow", "d>b", ui_window_mgr_get_hot_key_behavior(self),
            &is_window);
    return is_window;
}

BYTE ui_window_mgr_is_reserved_key(struct UIWindowMgr *self, int key)
{
    (void)self;
    switch (key) {
    case VK_TAB: case VK_RETURN: case VK_SHIFT: case VK_CONTROL: case VK_MENU: case VK_CAPITAL:
    case VK_INSERT:
        return 1;
    }
    return key >= VK_F1 && key <= VK_F12;
}

BYTE ui_window_mgr_is_first_edit(struct UIWindowMgr *self, struct UIWindow *edit)
{
    (void)self;
    return edit && ui_window_prev_focusable(edit->parent, edit) == NULL;
}

BYTE ui_window_mgr_is_last_edit(struct UIWindowMgr *self, struct UIWindow *edit)
{
    (void)self;
    return edit && ui_window_next_focusable(edit->parent, edit) == NULL;
}

BYTE ui_window_mgr_is_edit_in_open_window(struct UIWindowMgr *self)
{
    struct UIWindow *holder;

    if (self->focus_edit == NULL)
        return 0;
    holder = self->focus_edit->parent;
    return holder && holder != self->new_chat_wnd && holder->visible;
}

void ui_window_mgr_set_text_at_battle_mode(struct UIWindowMgr *self)
{
    struct UIWindow *chat = self->new_chat_wnd, *edit;

    if (chat == NULL)
        return;
    edit = ui_chat_window_edit(chat);
    if (self->focus_edit == edit)
        ui_edit_ctrl_set_text((struct UIEditCtrl *)edit, "");
    edit = ui_chat_window_whisper_edit(self->new_chat_wnd);
    if (self->focus_edit == edit)
        ui_edit_ctrl_set_text((struct UIEditCtrl *)edit, ui_edit_ctrl_get_text(edit));
}

#define MODE_MSG_TOGGLE_WINDOW 0x1f
#define WM_KEY_WINDOW_FIRST 0xd2

static const int key_windows[9] = { 0xd, 0x23, 0x30, 0x31, 0x32, 0x33, 0x40, 0x42, 0x43 };

void ui_window_mgr_window_hotkey(struct UIWindowMgr *self, int key)
{
    int i = key - WM_KEY_WINDOW_FIRST, svc = g_serviceType, window_id = 0;

    (void)self;
    if (client_current_mode() == NULL || i < 0 || i > 8)
        return;
    if (svc == 0xe || svc == 0x14)
        window_id = key_windows[(i + 7) % 9];
    else if (svc == 0 || (svc == 6 && i <= 1) || (svc == 7 && i == 2) || (svc == 8 && (i == 3 || i == 4))
             || (svc == 4 && i == 6))
        window_id = key_windows[i];
    else if (svc == 0xc && i == 0)
        window_id = 0x33;
    else if (svc == 0xb && i == 0)
        window_id = 0x40;
    if (window_id)
        mode_send_message(client_current_mode(), MODE_MSG_TOGGLE_WINDOW, window_id, 0, 0, 0);
}

BYTE ui_window_mgr_is_chat_edit_focused(struct UIWindowMgr *self)
{
    struct UIWindow *chat = self->new_chat_wnd;

    return chat && (self->focus_edit == ui_chat_window_edit(chat) || self->focus_edit == ui_chat_window_whisper_edit(chat));
}
