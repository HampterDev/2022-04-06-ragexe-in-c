#include "game/mode_util.h"
#include "actor/actor_ops.h"
#include "platform/language.h"
#include "session/number_codec.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type.h"
#include "ui/window_access.h"
#include "platform/win_types.h"
#include <string.h>
#include "game/mode_msg.h"
#include "game/popup_entries.h"
#include "game/world_pick.h"
#include "core/app.h"
#include "game/talk_type.h"
#include "platform/clientinfo.h"
#include "session/session_party.h"
#include "ui/controls/popups.h"
#include "ui/window_mgr.h"
#include "ui/wndmgr/wndmgr.h"

#define CZ_EQUIPWIN_MICROSCOPE  0x2d6
#define CZ_GM_FULLSTRIP         0x7f5
#define CZ_KICK_STORE_ASSISTANT 0xaf9

enum {
    WID_POPUP_MENU = 0x12,
    WID_PET_INFO = 0x58,
    WID_PROHIBIT_LIST = 0x67,
    WID_HOMUN_INFO = 0x71,
    WID_MER_INFO = 0x7d,
};

#define COLOR_WHITE 0xffffff
#define COLOR_C_CODE 0x64ffff

static void actor_request_equip_view(struct GameActor *actor, int flag)
{
    int request[6] = { 9, 0, 0, 0, 0, 0 };

    game_actor_set_request(actor, request, flag);
}

int game_mode_msg_show_equip_of(struct GameMode *self)
{
    struct GameActor *actor = mode_actor(self, g_session1.homunAid);

    if (actor && actor_homun_ai_running(actor))
        actor_request_equip_view(actor, 0);
    return 0;
}

int game_mode_msg_set_popup_target_name(struct GameMode *self, const char *name)
{
    int entry;

    string_assign_cstr(&self->popup_target_name, name);
    self->popup_entries.last = self->popup_entries.first;
    entry = POPUP_WHISPER_BY_NAME;
    vec_push(&self->popup_entries, &entry, sizeof entry);
    entry = POPUP_ADD_FRIEND_BY_NAME;
    vec_push(&self->popup_entries, &entry, sizeof entry);
    return 0;
}

static void show_actor_info(struct GameMode *self)
{
    struct String name, char_code, tmp;
    const char *relation;
    void *target;

    game_mode_get_actor_name(self, &name, self->popup_target_aid);
    if (ui_window_mgr_is_friend(&g_windowMgr, string_cstr(&name)))
        relation = msg_str(0x66);
    else
        relation = session_is_guild_member_aid(&g_session1, self->popup_target_aid) ? "GuildMember" : 0;
    string_init(&char_code);
    target = object_as_type(mode_actor(self, self->popup_target_aid), OBJECT_PC);
    if (target) {
        int cid = actor_get_char_id(target);

        string_free(&char_code);
        string_move(&char_code, session_scramble_number(&g_session1, &tmp, cid));
        string_free(&tmp);
    }
    ui_window_mgr_send_msg_(&g_windowMgr, 0xe, (intptr_t)string_cstr(&name), (intptr_t)relation,
                        (intptr_t)string_cstr(&char_code), 0);
    string_free(&char_code);
    string_free(&name);
}

static void copy_char_code(struct GameMode *self)
{
    struct String char_code;
    void *target = object_as_type(mode_actor(self, self->popup_target_aid), OBJECT_PC);
    HGLOBAL mem;
    int cid;

    if (!target)
        return;
    cid = actor_get_char_id(target);
    session_scramble_number(&g_session1, &char_code, cid);
    mem = GlobalAlloc(GHND, char_code.size + 1);
    memcpy(GlobalLock(mem), string_cstr(&char_code), char_code.size);
    GlobalUnlock(mem);
    if (OpenClipboard(g_hMainWnd)) {
        EmptyClipboard();
        SetClipboardData(CF_TEXT, mem);
        CloseClipboard();
        chat_refresh();
        chat_print("C-Code copied to clipboard", COLOR_C_CODE);
    }
    string_free(&char_code);
}

static void open_gm_window(BYTE (*allowed)(int), int kind, int text_id)
{
    struct UIWindow *w;
    int y;

    if (!allowed(g_session1.ownAid))
        return;
    w = ui_cnt_wnd_new();
    ui_window_create(w, 0xb6, 0x2e);
    y = scale_y_from480(200);
    ui_window_move_safe(w, ui_center_x640(200), y);
    ui_window_mgr_add_window(&g_windowMgr, w);
    ui_window_send(w, NULL, 0x50, kind, 0, 0, 0);
    ui_window_send(w, NULL, 0x1d, (intptr_t)msg_str(text_id), 0, 0, 0);
    ui_window_send(w, NULL, 0x51, 1, 0, 0, 0);
}

static void open_confirm_box(void (*callback)(void))
{
    struct UIWindow *w = ui_normal_mb_new();

    ui_normal_mb_create(w, 0, msg_str(0x259), 2, 1, 0, 0x118, 0x78, 0);
    ui_window_mgr_add_window(&g_windowMgr, w);
    ui_normal_mb_set_answer(w, 0xb3, callback);
}

static void self_msg_with_name(struct GameMode *self, int msg)
{
    struct String name;

    game_mode_get_actor_name(self, &name, self->popup_target_aid);
    self_msg(self, msg, (intptr_t)string_cstr(&name), 0, 0, 0);
    string_free(&name);
}

static void toggle_window(int id)
{
    if (!close_window(id))
        ui_window_open(id);
}

int game_mode_msg_popup_choice(struct GameMode *self, unsigned index)
{
    struct String name;
    char line[256];
    int target, entry;

    if (g_windowMgr.menu_wnd)
        close_window(WID_POPUP_MENU);
    if (g_windowMgr.item_store_wnd)
        return 0;
    if (VEC_COUNT(&self->popup_entries, int) <= index)
        return 0;

    entry = ((int *)self->popup_entries.first)[index];
    target = self->popup_target_aid;
    switch (entry) {
    case POPUP_TRADE:
        if (self->npc_dialog_open == 1 || g_windowMgr.say_dialog_wnd || g_windowMgr.choose_wnd) {
            chat_print_msg(0x783, COLOR_WHITE);
            return 0;
        }
        if (!self->world || !self->world->player || actor_get_effect_state(self->world->player) == 2)
            return 0;
        self_msg(self, MODE_MSG_REQ_EXCHANGE, target, 0, 0, 0);
        return 0;
    case POPUP_PARTY_INVITE:
        self_msg_with_name(self, MODE_MSG_PARTY_INVITE);
        return 0;
    case POPUP_ADD_FRIEND:
        game_mode_get_actor_name(self, &name, target);
        ui_window_mgr_request_add_friend(&g_windowMgr, string_cstr(&name));
        string_free(&name);
        return 0;
    case POPUP_WHISPER_BY_NAME:
        ui_window_mgr_notify_whisper_wnd(&g_windowMgr, string_cstr(&self->popup_target_name));
        return 0;
    case POPUP_WHISPER_BLOCK:
    case POPUP_WHISPER_UNBLOCK:
        game_mode_get_actor_name(self, &name, target);
        game_mode_process_talk_type(self, entry == POPUP_WHISPER_BLOCK ? 0x12 : 0x13, &name);
        string_free(&name);
        return 0;
    case POPUP_ACTOR_INFO:
        show_actor_info(self);
        return 0;
    case POPUP_ADD_FRIEND_BY_NAME:
        ui_window_mgr_request_add_friend(&g_windowMgr, string_cstr(&self->popup_target_name));
        return 0;
    case POPUP_GUILD_INVITE:
        self_msg(self, MODE_MSG_GUILD_INVITE, target, 0, 0, 0);
        return 0;
    case POPUP_GM_PROHIBIT_REMOVE:
        open_gm_window(clientinfo_is_yellow_aid, 0x97, 0x185);
        return 0;
    case POPUP_GM_PROHIBIT_ADD:
        open_gm_window(clientinfo_is_admin_aid, 0x98, 0x186);
        return 0;
    case POPUP_GUILD_ALLY:
        self_msg(self, MODE_MSG_GUILD_ALLY, target, 0, 0, 0);
        return 0;
    case POPUP_GUILD_HOSTILE:
        self_msg(self, MODE_MSG_GUILD_HOSTILE, target, 0, 0, 0);
        return 0;
    case POPUP_DISCONNECT:
        self_msg(self, MODE_MSG_DISCONNECT_CHARACTER, target, 0, 0, 0);
        return 0;
    case POPUP_PET_FEED:
        if (g_windowMgr.rodex_compose_wnd) {
            chat_print_msg(0xba9, CHAT_COLOR_RED);
            return 0;
        }
        open_confirm_box(game_mode_confirm_feed_pet);
        return 0;
    case POPUP_PET_PERFORMANCE:
        self_msg(self, MODE_MSG_COMMAND_PET, 2, 0, 0, 0);
        return 0;
    case POPUP_PET_RETURN_TO_EGG:
        self_msg(self, MODE_MSG_COMMAND_PET, 3, 0, 0, 0);
        return 0;
    case POPUP_PET_UNEQUIP:
        self_msg(self, MODE_MSG_COMMAND_PET, 4, 0, 0, 0);
        return 0;
    case POPUP_PET_INFO:
        self_msg(self, MODE_MSG_COMMAND_PET, 0, 0, 0, 0);
        toggle_window(WID_PET_INFO);
        return 0;
    case POPUP_COPY_GID: {
        struct Language *lang;

        sprintf(line, "%s : %d (GID)", string_cstr(&g_session1.serverName), target);
        lang = g_language;
        language_cancel_input(lang);
        language_replace_selection(lang, line);
        return 0;
    }
    case POPUP_PROHIBIT_LIST:
        ui_window_send(ui_window_open(WID_PROHIBIT_LIST), NULL, 0x22, target, 0, 0, 0);
        return 0;
    case POPUP_REQ_JOIN_BABY:
        self_msg(self, MODE_MSG_REQ_JOIN_BABY, target, 0, 0, 0);
        return 0;
    case POPUP_HOMUN_INFO:
        self_msg(self, MODE_MSG_COMMAND_MERCENARY, 0, 0, 0, 0);
        toggle_window(WID_HOMUN_INFO);
        return 0;
    case POPUP_HOMUN_FEED:
        open_confirm_box(game_mode_confirm_feed_homun);
        return 0;
    case POPUP_HOMUN_SHOW_EQUIP:
        return game_mode_msg_show_equip_of(self);
    case POPUP_MERCENARY_COMMAND:
        self_msg(self, MODE_MSG_MERCENARY_COMMAND, 1, 0, 0, 0);
        toggle_window(WID_MER_INFO);
        return 0;
    case POPUP_MERCENARY_SHOW_EQUIP: {
        struct GameActor *actor = mode_actor(self, g_session1.mercAid);

        if (!actor || !actor_merc_ai_running(actor))
            return 0;
        actor_request_equip_view(actor, 1);
        return 0;
    }
    case POPUP_VIEW_EQUIPMENT: {
#pragma pack(push, 1)
        struct { unsigned short id; int target; } packet;
#pragma pack(pop)
        packet.id = CZ_EQUIPWIN_MICROSCOPE;
        packet.target = target;
        send_packet(CZ_EQUIPWIN_MICROSCOPE, &packet);
        return 0;
    }
    case POPUP_GM_FULLSTRIP: {
#pragma pack(push, 1)
        struct { unsigned short id; int target; } packet;
#pragma pack(pop)
        if (!clientinfo_is_admin_aid(g_session1.ownAid))
            return 0;
        packet.id = CZ_GM_FULLSTRIP;
        packet.target = target;
        send_packet(CZ_GM_FULLSTRIP, &packet);
        return 0;
    }
    case POPUP_COPY_CHAR_CODE:
        copy_char_code(self);
        return 0;
    case POPUP_WRITE_RODEX:
        self_msg_with_name(self, MODE_MSG_OPEN_WRITE_RODEX);
        return 0;
    case POPUP_KICK_STORE_ASSISTANT: {
#pragma pack(push, 1)
        struct { unsigned short id; int target; } packet;
#pragma pack(pop)
        packet.id = CZ_KICK_STORE_ASSISTANT;
        packet.target = target;
        send_packet(CZ_KICK_STORE_ASSISTANT, &packet);
        return 0;
    }
    }
    return 0;
}
