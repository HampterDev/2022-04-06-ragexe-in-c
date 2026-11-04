#include "actor/actor_ops.h"
#include "ui/window_keyboard.h"
#include "core/text_markup.h"
#include "session/chat_macros.h"
#include "replay/replay.h"
#include "contents/guild_info.h"
#include "session/state.h"
#include "session/session_talk.h"
#include "ui/window_manager_state.h"
#include "core/object_type.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "world/actor_lookup.h"
#include "game/context.h"
#include "contents/connection_api.h"
#include <string.h>
#include "ui/wndmgr/wndmgr.h"
#include "ui/draw.h"
#include "contents/clan_info.h"
#include "core/msg_str.h"
#include "session/session_items.h"
#include "session/session_names.h"
#include "session/session_options.h"
#include "session/session_state.h"
#include "session/session_windows.h"
#include "ui/chat/chat_window.h"
#include "ui/hotkeys/hotkeys.h"
#include "ui/media/connect_sns.h"

#define MODE_MSG_CHAT_LINE 6
#define MODE_MSG_PARTY_LINE 0x42
#define MODE_MSG_GUILD_LINE 0x81
#define MODE_MSG_CLAN_LINE 0x121
#define MODE_MSG_TALK_COMMAND 0x2a

static BYTE command(struct UIWindow *wnd, int id)
{
    if (wnd)
        ui_window_send_msg(wnd, NULL, UI_MSG_COMMAND, id, 0, 0, 0);
    return 1;
}

static BYTE toggle(struct UIWindowMgr *self, int window_id)
{
    if (!ui_window_mgr_post_quit1(self, window_id))
        ui_window_mgr_make_window(self, window_id);
    return 1;
}

static BYTE basic_command(struct UIWindowMgr *self, int id)
{
    return command(self->basic_info_wnd, id);
}
static BYTE basic_toggle(struct UIWindowMgr *self, int window_id)
{
    return self->basic_info_wnd ? toggle(self, window_id) : 1;
}

static BYTE messenger_tab(struct UIWindowMgr *self, int tab)
{
    int old;

    if (self->basic_info_wnd == NULL)
        return 1;
    old = self->messenger_group_info.radio;
    self->messenger_group_info.radio = tab;
    if (ui_window_mgr_post_quit1(&g_windowMgr, 0x45) && old == self->messenger_group_info.radio)
        return 1;
    ui_window_open(0x45);
    return 1;
}

static BYTE minimap(struct UIWindowMgr *self)
{
    if (g_windowMgr.kafra_move_map_service_info_wnd || !game_camera_is_free())
        return 1;
    if (self->minimap_argb == 0xffffffff) {
        self->minimap_argb = 0xaaffffff;
        return 1;
    }
    if (ui_window_mgr_post_quit1(self, 0xe))
        return 1;
    ui_window_mgr_make_window(self, 0xe);
    self->minimap_argb = 0xffffffff;
    return 1;
}

static BYTE chat_window(struct UIWindowMgr *self)
{
    struct UIWindow *chat = self->new_chat_wnd;
    BYTE allowed;

    if (chat == NULL)
        return 1;
    allowed = !g_replayPlaying || replay_recorder_instance()->info.options[6];
    if (chat->visible) {
        ui_window_send_msg(chat, NULL, 2, 0, 0, 0, 0);
        if (self->new_chat_wnd->state || !allowed)
            return 1;
        self->chat_wnd_show = 0;
        if (self->new_chat_wnd == NULL)
            ui_window_open(0x1);
        ui_window_set_show(self->new_chat_wnd, 0);
        ui_window_store_info(self->new_chat_wnd);
        return 1;
    }
    if (allowed) {
        self->chat_wnd_show = 1;
        if (self->new_chat_wnd == NULL)
            ui_window_open(0x1);
        ui_window_set_show(self->new_chat_wnd, 1);
        ui_window_mgr_raise_window(&g_windowMgr, self->new_chat_wnd);
    }
    ui_new_chat_wnd_place_bar((struct UINewChatWnd *)self->new_chat_wnd);
    return 1;
}

static BYTE companion_command(int aid, BYTE (*ready)(void *), int which)
{
    struct Mode *mode = game_current_base_mode();
    const int request[6] = {9, 0, 0, 0, 0, 0};
    void *actor;

    if (mode == NULL)
        return 1;
    actor = game_find_actor(aid);
    if (actor == NULL || !ready(actor))
        return 1;
    game_actor_set_request(actor, request, which);
    return 1;
}

static BYTE replay_command(struct UIWindowMgr *self, int id)
{
    if (g_replayPlaying && self->replay_control_wnd)
        command(self->replay_control_wnd, id);
    return 1;
}

#pragma pack(push, 1)
struct PacketBankOpen {
    WORD id;
    int aid;
};
struct PacketHotkeyRow {
    WORD id;
    WORD row;
    BYTE page;
};
#pragma pack(pop)

static void send_packet(WORD id, const void *packet)
{
    rag_connection_send_packet(rag_connection_instance(),
                              rag_connection_get_packet_length(rag_connection_instance(), id), packet);
}

static BYTE bank(void)
{
    struct PacketBankOpen packet;

    if (g_windowMgr.bank_new_wnd) {
        ui_window_mgr_post_quit1(&g_windowMgr, 0x113);
        return 1;
    }
    packet.id = 0x9ab;
    packet.aid = g_session1.ownAid;
    send_packet(0x9ab, &packet);
    return 1;
}

static BYTE hotkey_row(struct UIWindowMgr *self)
{
    struct PacketHotkeyRow packet;
    struct UIWindow *bar = self->short_cut_wnd;

    if (bar)
        ui_window_send_msg(bar, NULL, 0x8f, 1, session_get_instant_var(&g_session1, 0xa), 0, 0);
    packet.id = 0xb22;
    packet.row = (WORD)session_get_instant_var(&g_session1, 0xa);
    packet.page = packet.row == 0 ? ((BYTE)g_session1.hotkeyPage0) : packet.row == 1 ? ((BYTE)g_session1.hotkeyPage1) : 0;
    send_packet(0xb22, &packet);
    return 1;
}

static BYTE replay_allows(int action)
{
    const BYTE *option = &replay_recorder_instance()->info.options[0];

    switch (action) {
    case 0x67: case 0x6c: case 0x71: case 0x73: case 0x78:
        return 1;
    case 0x64: return option[0];
    case 0x65: case 0x7a: case 0x7b: case 0x7c: return option[1];
    case 0x6b: case 0x6d: return option[2];
    case 0x66: return option[3];
    case 0x68: case 0x69: return option[5];
    case 0x76: case 0x77: return option[6];
    case 0x75: return option[7];
    case 0x93: return option[8];
    }
    return 0;
}

BYTE ui_window_mgr_hot_key_action(struct UIWindowMgr *self, int kind)
{
    struct UIWindow *wnd;
    int action;

    if (kind != 4)
        return 0;
    action = ui_window_mgr_get_hot_key_behavior(self);
    if (action == -1)
        return 0;
    if (g_replayPlaying && action != 0x8b && action != 0x8c && action != 0x8d && !replay_allows(action))
        return 0;
    if (session_get_instant_var(&g_session1, 5) > 0 || self->private_airplane_wnd)
        return 0;
    if (action <= 0x2c) {
        if (self->short_cut_wnd)
            ui_window_send_msg(self->short_cut_wnd, NULL, 0x29, action % 9, action / 9, 0, 0);
        return 1;
    }
    if (self->modal_result == 0 && ui_window_mgr_get_window(self, 0x165))
        return 0;

    switch (action) {
    case 0x64: return basic_command(self, 0xc2);
    case 0x65: return basic_command(self, 0xbb);
    case 0x66: return basic_command(self, 0xbc);
    case 0x67: return basic_command(self, 0x161);
    case 0x68: return messenger_tab(self, 0);
    case 0x69: return messenger_tab(self, 1);
    case 0x6a: return basic_command(self, 0xce);
    case 0x6b: return basic_command(self, 0xef);
    case 0x6c: return basic_toggle(self, 0x8c);
    case 0x6d: return basic_command(self, 0xba);
    case 0x6e: return basic_toggle(self, g_guildInfo.guild_id ? 0x3b : 0xd4);
    case 0x6f: return basic_toggle(self, 0x57);
    case 0x70:
        if (client_current_mode())
            mode_send_message(client_current_mode(), 0x2a, 0x16, 0, 0, 0);
        return 1;
    case 0x71: return minimap(self);
    case 0x72: return basic_toggle(self, 0x56);
    case 0x73: return basic_toggle(self, 0x70);
    case 0x74:
        ui_window_mgr_toggle_hide_ui(self);
        return 1;
    case 0x75: return command(ui_window_mgr_get_window(self, 0x24), 0xd0);
    case 0x76:
        if (self->basic_info_wnd)
            ui_window_mgr_show_option_window(self, 0x1, 1 - self->chat_wnd_show);
        return 1;
    case 0x77: return chat_window(self);
    case 0x78:
        if (client_current_mode())
            mode_send_message(client_current_mode(), 0x29, 0, 0, 0, 0);
        return 1;
    case 0x79:
        mode_send_message(client_current_mode(), 0x44, 0, 0, 0, 0);
        return 1;
    case 0x7a:
        if (self->memorial_dun_wnd)
            ui_window_set_show(self->memorial_dun_wnd, self->memorial_dun_wnd->visible == 0);
        else
            ui_window_open(0x89);
        return 1;
    case 0x7b: return basic_toggle(self, 0x58);
    case 0x7c: return g_session1.homun_ai_flags[0] ? basic_toggle(self, 0x71) : 1;
    case 0x7d: return g_session1.mercPresent ? basic_toggle(self, 0x7d) : 1;
    case 0x7e:
        if (g_session1.homun_ai_flags[0] == 0)
            return 1;
        session_clear_homun_commands(&g_session1);
        return companion_command(g_session1.homunAid, actor_homun_ai_running, 0);
    case 0x7f:
        if (g_session1.mercPresent == 0)
            return 1;
        session_clear_merc_commands(&g_session1);
        return companion_command(g_session1.mercAid, actor_merc_ai_running, 1);
    case 0x80: return basic_toggle(self, 0x44);
    case 0x81:
        if (self->basic_info_wnd && !ui_window_mgr_post_quit1(&g_windowMgr, 0x35) &&
            session_get_party_member_count(&g_session1))
            ui_window_open(0x35);
        return 1;
    case 0x83:
        if (self->basic_info_wnd == NULL || !self->chat_bar_toggleable || self->new_chat_wnd == NULL)
            return 1;
        if (!ui_window_mgr_is_chat_edit_focused(self) && self->focus_edit)
            return 1;
        ui_new_chat_wnd_on_enter((struct UINewChatWnd *)self->new_chat_wnd);
        if (self->focus_edit)
            ui_window_send_msg(self->focus_edit, NULL, 0, 0, 0, 0, 0);
        return 1;
    case 0x84:
        wnd = ui_window_mgr_get_window(self, 0x133);
        if (wnd)
            ui_window_send_msg(wnd, NULL, 0x3c, 0, 0, 0, 0);
        return 1;
    case 0x87:
        g_session1.showCellInfo = 1 - g_session1.showCellInfo;
        return 1;
    case 0x89:
        if (self->basic_info_wnd == NULL)
            return 1;
        if (ui_window_find(0x13e) == NULL)
            game_send_message(0x143, 0, 0, 0, 0);
        else
            ui_window_mgr_post_quit1(&g_windowMgr, 0x13e);
        return 1;
    case 0x8a:
        command(ui_window_find(0x133), 0x187);
        return command(ui_window_find(0xb6), 0x187);
    case 0x8b:
        if (g_replayPlaying && self->replay_control_wnd) {
            command(self->replay_control_wnd,
                    replay_recorder_instance()->state == 6 ? 0x189 : 0x18a);
            ui_window_send_msg(self->replay_control_wnd, NULL, 0x23, 0, 0, 0, 0);
        }
        return 1;
    case 0x8c: return replay_command(self, 0x18d);
    case 0x8d: return replay_command(self, 0x18e);
    case 0x8e:
        if (g_windowMgr.navigation_v4_wnd)
            return command(g_windowMgr.navigation_v4_wnd, 0xc1);
        if (g_windowMgr.kafra_move_map_service_info_wnd)
            return command(g_windowMgr.kafra_move_map_service_info_wnd, 0xc0);
        ui_window_open(0xcb);
        return 1;
    case 0x90: return basic_toggle(self, 0xed);
    case 0x92: return bank();
    case 0x93: return basic_command(self, 0xb8);
    case 0x94: return hotkey_row(self);
    case 0x95:
        if (!game_camera_is_free())
            return 1;
        if (!session_is_not_minimap_exception(&g_session1)) {
            ui_message_box(msg_str(0xacf));
            return 1;
        }
        return toggle(&g_windowMgr, 0x111);
    case 0x96: return toggle(&g_windowMgr, 0x10e);
    case 0x97:
        command(ui_window_find(0x133), 0x1d4);
        return command((struct UIWindow *)object_as_type(ui_window_find(0xb6), OBJECT_BASIC_BUTTONS_WINDOW),
                       0x1d4);
    case 0x98:
        game_send_message(0x130, 0, 0, 0, 0);
        return 1;
    case 0x99:
        if (g_connectSnsMgr && connect_sns_mgr_is_window_closed() != 1)
            command(ui_window_find(0x11a), 0xc1);
        return 1;
    case 0x9a: return toggle(&g_windowMgr, 0x13b);
    case 0x9b: return command(ui_window_find(0x24), 0x21d);
    case 0x9d: return toggle(&g_windowMgr, 0x144);
    case 0x9f:
        if (g_session1.mapType != 0 && g_session1.mapType != 1)
            session_set_hide_other_players(&g_session1, 0);
        else
            session_set_hide_other_players(&g_session1, g_session1.hideOtherPlayers == 0);
        return 1;
    case 0xa0: return toggle(&g_windowMgr, 0x15a);
    case 0xc8: case 0xc9: case 0xca: case 0xcb: case 0xcc:
    case 0xcd: case 0xce: case 0xcf: case 0xd0: case 0xd1:
        if (client_current_mode())
            ui_window_mgr_send_chat_macro(action - 0xc8);
        return 1;
    case 0xd2: case 0xd3: case 0xd4: case 0xd5: case 0xd6:
    case 0xd7: case 0xd8: case 0xd9: case 0xda: case 0xdb:
        ui_window_mgr_flag_emote_hotkey(self, action);
        return 1;
    }
    return 0;
}

void ui_window_mgr_send_chat_macro(int index)
{
    const char *text;
    struct String parts[3];
    int type, at, target;
    size_t i;

    if (index < 0 || index >= CHAT_MACRO_COUNT) return;
    text = session_chat_macro(&g_session1, index).data;
    if (g_windowMgr.basic_info_wnd) {
        if (text_has_markup_tags(text, strlen(text))) {
            ui_message_box(msg_str(0xe53));
            return;
        }
    }
    if (text[0] != '/') {
        string_assign_cstr(&g_windowMgr.chat_line, text);
        target = g_session1.chatTarget;
        if (target == 2 && g_guildInfo.guild_id)
            mode_send_message(client_current_mode(), MODE_MSG_GUILD_LINE, 0, 0, 0, 0);
        else if (target == 1 && session_get_party_member_count(&g_session1))
            mode_send_message(client_current_mode(), MODE_MSG_PARTY_LINE, 0, 0, 0, 0);
        else if (target == 3 && g_clanInfo->in_clan)
            mode_send_message(client_current_mode(), MODE_MSG_CLAN_LINE, 0, 0, 0, 0);
        else
            mode_send_message(client_current_mode(), MODE_MSG_CHAT_LINE, 0, 0, 0, 0);
        return;
    }
    if (session_is_emotion_text(&g_session1, text))
        return;
    for (i = 0; i < 3; i++)
        string_init(&parts[i]);
    at = session_get_talk_type(&g_session1, text, &type, parts);
    if (at != -1)
        string_assign_cstr(&g_windowMgr.chat_line, text + at);
    mode_send_message(client_current_mode(), MODE_MSG_TALK_COMMAND, type, (int)(INT_PTR)parts, 0, 0);
    for (i = 3; i-- > 0;)
        string_free(&parts[i]);
}

static int edit_in_other_window(struct UIWindowMgr *self)
{
    struct UIWindow *parent;

    if (self->focus_edit == NULL)
        return 0;
    parent = self->focus_edit->parent;
    return parent && parent != self->new_chat_wnd && parent->visible;
}

static int edit_lets_through(int vk)
{
    switch (vk) {
    case VK_TAB: case VK_RETURN: case VK_SHIFT: case VK_CONTROL: case VK_MENU: case VK_CAPITAL:
    case VK_INSERT:
        return 1;
    }
    return vk >= VK_F1 && vk <= VK_F12;
}

static int altgr_down(void)
{
    return (GetKeyState(VK_CONTROL) & 0x8000) && (GetKeyState(VK_MENU) & 0x8000);
}

BYTE ui_window_mgr_on_key_down(struct UIWindowMgr *self, int vk, int lparam, int first_press)
{
    int kind, editing, pass;

    self->key_as_text = 1;
    if (game_current_base_mode() == NULL)
        return ui_window_mgr_handle_key(self, vk, first_press);
    if (ui_window_mgr_get_window(self, 0x9c) || (!self->chat_bar_shown && !edit_in_other_window(self)))
        self->key_as_text = 0;
    kind = ui_window_mgr_classify_key(self, vk, lparam);
    if (self->hot_key_wnd) {
        ui_hot_key_wnd_on_key((struct UIHotKeyWnd *)self->hot_key_wnd, kind);
        return 0;
    }
    editing = (self->chat_bar_shown && ui_window_mgr_is_chat_edit_focused(self)) || edit_in_other_window(self);
    if (editing && altgr_down())
        return ui_window_mgr_handle_key(self, vk, first_press);
    if (editing) {
        pass = self->key_modifier ? 1 : edit_lets_through(self->last_key);
        if (self->key_modifier == VK_SHIFT || !pass)
            return ui_window_mgr_handle_key(self, vk, first_press);
    }
    if (!first_press)
        return ui_window_mgr_handle_key(self, vk, first_press);
    if (g_session1.skillUseKey != vk)
        session_reset_option2(&g_session1);
    g_session1.lastKeyDown = vk;
    if ((vk == VK_RETURN || vk == VK_SPACE) && ui_window_mgr_on_enter(self))
        return 1;
    if (!ui_window_mgr_hot_key_action(self, kind))
        return ui_window_mgr_handle_key(self, vk, first_press);
    if (ui_window_mgr_is_edit_in_open_window(self) || ui_window_mgr_is_chat_edit_focused(self))
        self->key_as_text = 0;
    return 1;
}
