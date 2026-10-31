#include "game/packet_handlers.h"
#include "session/character_name.h"
#include "game/actor_packets.h"
#include "contents/manner_filter.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "ui/window_access.h"
#include "core/fatal.h"
#include <windows.h>
#include <string.h>
#include "game/packets.h"
#include "platform/language.h"
#include "game/chat.h"
#include "platform/clientinfo.h"
#include "ui/ui_std.h"
#include "ui/chat/chat_room_wnd.h"

#define CHAT_COLOR_RED          0xff
#define CHAT_COLOR_YELLOW       0xffff
#define CHAT_MSG_CLEAR          3
#define CHAT_MSG_ADD            1

#define ACTOR_MSG_CHAT_ROOM     0x20
#define CHAT_ROOM_TITLE_MAX     0x3c

void game_mode_zc_room_newentry(struct GameMode *self, const struct PacketRoomEntry *pkt)
{
    struct GameActor *owner;
    struct ChatRoomOwner *room_owner;
    struct ChatRoomInfo room;
    unsigned room_id;

    if (pkt->length < (int)sizeof *pkt)
        return;
    room_id = (unsigned)pkt->room_id;
    room_owner = tree_get(&self->chat_room_owners, &room_id, sizeof room_id, sizeof *room_owner, tree_less_uint);
    room_owner->aid = pkt->aid;
    owner = world_get_game_actor_by_aid(self->world, pkt->aid);
    if (!owner)
        return;
    string_init(&room.title);
    string_init(&room.password);
    string_assign(&room.title, (const char *)(pkt + 1), pkt->length - sizeof *pkt);
    room.type = pkt->type;
    room.users = pkt->users;
    room.max_users = pkt->max_users;
    room.room_id = *&pkt->room_id;
    if (room.title.size > CHAT_ROOM_TITLE_MAX) {
        string_truncate(&room.title, CHAT_ROOM_TITLE_MAX);
    }
    actor_send(owner, ACTOR_MSG_CHAT_ROOM, (int)&room, 0);
    string_free(&room.password);
    string_free(&room.title);
}

#pragma pack(push, 1)
struct PACKET_ZC_REFUSE_ENTER_ROOM {
    unsigned short id;
    BYTE reason;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_REFUSE_ENTER_ROOM, 3);

void game_mode_zc_refuse_enter_room(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_REFUSE_ENTER_ROOM *packet = (const struct PACKET_ZC_REFUSE_ENTER_ROOM *)pkt;
    static const short reason_text[8] = { 0x43, 7, 0x44, 0, 0x37, 0x1b0, 0x1b1, 0x1b2 };
    unsigned reason = packet->reason;

    self->chat_room_pending = 0;
    if (reason > 7 || reason == 3)
        return;
    ui_window_mgr_send_msg_(&g_windowMgr, CHAT_MSG_CLEAR, 0, 0, 0, 0);
    ui_window_mgr_send_msg_(&g_windowMgr, CHAT_MSG_ADD, (int)msg_str(reason_text[reason]), CHAT_COLOR_RED, 0, 0);
}

#define WND_CHAT_ROOM           0x1c
#define WND_CHAT_ROOM_SETUP     0x1e
#define ACTOR_MSG_CHAT_ROOM_GONE 0x21
#define CHAT_COLOR_ROOM_JOIN    0x808000
#define CHAT_COLOR_ROOM_LEAVE   0x80
#define UIM_ROOM_LINE           5
enum {
    ROOM_WND_SET_INFO = 0x22,
    ROOM_WND_OWN = 0x2b,
    ROOM_WND_MEMBER_ADD = 0x2c,
    ROOM_WND_MEMBER_REMOVE = 0x2d,
    ROOM_WND_ROLE = 0x2e,
};

#pragma pack(push, 1)
struct PACKET_ZC_ACK_CREATE_CHATROOM {
    unsigned short id;
    BYTE result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_CREATE_CHATROOM, 3);

void game_mode_zc_ack_create_chatroom(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_ACK_CREATE_CHATROOM *packet = (const struct PACKET_ZC_ACK_CREATE_CHATROOM *)pkt;
    struct UIWindow *wnd;

    self->chat_room_pending = 0;
    switch (packet->result) {
    case 0:
        ui_window_mgr_send_msg_(&g_windowMgr, CHAT_MSG_CLEAR, 0, 0, 0, 0);
        ui_window_mgr_send_msg_(&g_windowMgr, CHAT_MSG_ADD, (int)msg_str(0x40), 0xffff00, 0, 0);
        wnd = ui_window_open(WND_CHAT_ROOM);
        self->own_chat_room.users = 1;
        wnd_send(wnd, ROOM_WND_SET_INFO, (int)&self->own_chat_room, 0, 0, 0);
        wnd_send(wnd, ROOM_WND_OWN, 0, 0, 0, 0);
        wnd_send(wnd, ROOM_WND_MEMBER_ADD, (int)session_character_name(&g_session1), 0, 0, 0);
        break;
    case 1:
        ui_window_mgr_send_msg_(&g_windowMgr, CHAT_MSG_CLEAR, 0, 0, 0, 0);
        ui_window_mgr_send_msg_(&g_windowMgr, CHAT_MSG_ADD, (int)msg_str(0x41), CHAT_COLOR_RED, 0, 0);
        break;
    case 2:
        ui_window_mgr_send_msg_(&g_windowMgr, CHAT_MSG_CLEAR, 0, 0, 0, 0);
        ui_window_mgr_send_msg_(&g_windowMgr, CHAT_MSG_ADD, (int)msg_str(0x42), CHAT_COLOR_RED, 0, 0);
        break;
    }
}

#pragma pack(push, 1)
struct PACKET_ZC_DESTROY_ROOM {
    unsigned short id;
    unsigned room_id;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_DESTROY_ROOM, 6);

void game_mode_zc_destroy_room(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_DESTROY_ROOM *packet = (const struct PACKET_ZC_DESTROY_ROOM *)pkt;
    unsigned room_id;
    unsigned owner_aid;
    struct ChatRoomOwner *room_owner;
    struct GameActor *owner;

    room_id = packet->room_id;
    room_owner = tree_get(&self->chat_room_owners, &room_id, sizeof room_id, sizeof *room_owner, tree_less_uint);
    owner_aid = room_owner->aid;
    owner = world_get_game_actor_by_aid(self->world, owner_aid);

    if (owner)
        actor_send(owner, ACTOR_MSG_CHAT_ROOM_GONE, 0, 0);
    if (g_windowMgr.chat_room_wnd && ((struct UIChatRoomWnd *)g_windowMgr.chat_room_wnd)->room.room_id == (int)packet->room_id) {
        ui_window_mgr_post_quit1(&g_windowMgr, WND_CHAT_ROOM);
        ui_window_mgr_post_quit1(&g_windowMgr, WND_CHAT_ROOM_SETUP);
    }
}

#pragma pack(push, 1)
struct PACKET_ZC_MEMBER_NEWENTRY {
    unsigned short id;
    short users;
    char name[24];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_MEMBER_NEWENTRY, 28);

void game_mode_zc_member_newentry(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_MEMBER_NEWENTRY *packet = (const struct PACKET_ZC_MEMBER_NEWENTRY *)pkt;
    char name[25], line[0x400];
    struct ChatRoomInfo room;
    struct UIWindow *wnd;

    (void)self;
    memcpy(name, packet->name, 24);
    name[24] = 0;
    wnd = ui_window_open(WND_CHAT_ROOM);
    chat_room_info_copy(&room, &((struct UIChatRoomWnd *)wnd)->room);
    room.users = packet->users;
    wnd_send(wnd, ROOM_WND_SET_INFO, (int)&room, 0, 0, 0);
    wnd_send(wnd, ROOM_WND_MEMBER_ADD, (int)name, 1, 0, 0);
    sprintf(line, msg_str(0xb3), name);
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_ROOM_LINE, (int)line, CHAT_COLOR_ROOM_JOIN, 0, 0);
    string_free(&room.password);
    string_free(&room.title);
}

#pragma pack(push, 1)
struct PACKET_ZC_MEMBER_EXIT {
    unsigned short id;
    short users;
    char name[24];
    BYTE type;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_MEMBER_EXIT, 29);

void game_mode_zc_member_exit(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_MEMBER_EXIT *packet = (const struct PACKET_ZC_MEMBER_EXIT *)pkt;
    char name[25], line[0x400];
    struct ChatRoomInfo room;
    struct UIWindow *wnd;

    (void)self;
    memcpy(name, packet->name, 24);
    name[24] = 0;
    if (strcmp(session_character_name(&g_session1), name) == 0) {
        ui_window_mgr_post_quit1(&g_windowMgr, WND_CHAT_ROOM);
        ui_window_mgr_post_quit1(&g_windowMgr, WND_CHAT_ROOM_SETUP);
        return;
    }
    wnd = ui_window_open(WND_CHAT_ROOM);
    chat_room_info_copy(&room, &((struct UIChatRoomWnd *)wnd)->room);
    room.users = packet->users;
    wnd_send(wnd, ROOM_WND_SET_INFO, (int)&room, 0, 0, 0);
    wnd_send(wnd, ROOM_WND_MEMBER_REMOVE, (int)name, 0, 0, 0);
    if (packet->type <= 1) {
        sprintf(line, msg_str(packet->type == 0 ? 0xb4 : 0xb5), name);
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_ROOM_LINE, (int)line, CHAT_COLOR_ROOM_LEAVE, 0, 0);
    }
    string_free(&room.password);
    string_free(&room.title);
}

#pragma pack(push, 1)
struct PACKET_ZC_ROLE_CHANGE {
    unsigned short id;
    int role;
    char name[24];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ROLE_CHANGE, 30);

void game_mode_zc_role_change(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_ROLE_CHANGE *packet = (const struct PACKET_ZC_ROLE_CHANGE *)pkt;
    char name[25];
    struct UIWindow *wnd = ui_window_open(WND_CHAT_ROOM);

    (void)self;
    memcpy(name, packet->name, 24);
    name[24] = 0;
    wnd_send(wnd, ROOM_WND_ROLE, (int)name, packet->role, 0, 0);
}

void game_mode_zc_change_chatroom(struct GameMode *self, const unsigned char *pkt)
{
    const struct PacketRoomEntry *packet = (const struct PacketRoomEntry *)pkt;
    struct ChatRoomInfo room;
    struct ChatRoomOwner *room_owner;
    void *wnd;

    unsigned room_id;

    room_id = packet->room_id;
    room_owner = tree_get(&self->chat_room_owners, &room_id, sizeof room_id, sizeof *room_owner, tree_less_uint);
    room_owner->aid = packet->aid;
    string_init(&room.title);
    string_init(&room.password);
    string_assign(&room.title, (const char *)(packet + 1), packet->length - sizeof *packet);
    room.type = packet->type;
    room.users = packet->users;
    room.max_users = packet->max_users;
    room.room_id = packet->room_id;
    if (room.title.size > CHAT_ROOM_TITLE_MAX) {
        string_truncate(&room.title, CHAT_ROOM_TITLE_MAX);
    }
    wnd = ui_window_open(WND_CHAT_ROOM);
    wnd_send(wnd, ROOM_WND_SET_INFO, (int)&room, 0, 0, 0);
    string_free(&room.password);
    string_free(&room.title);
}

static void whisper_setting_line(const struct String *name, int msg)
{
    struct String line;
    const char *text = msg_str(msg);

    string_copy(&line, name);
    string_append_cstr(&line, text);
    ui_window_mgr_send_msg_(&g_windowMgr, CHAT_MSG_ADD, (int)string_cstr(&line), CHAT_COLOR_YELLOW, 0, 0);
    string_free(&line);
}

#pragma pack(push, 1)
struct PACKET_ZC_SETTING_WHISPER_PC2 {
    unsigned short id;
    char name[24];
    BYTE type;
    BYTE result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_SETTING_WHISPER_PC2, 28);

void game_mode_zc_setting_whisper_pc2(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_SETTING_WHISPER_PC2 *packet = (const struct PACKET_ZC_SETTING_WHISPER_PC2 *)pkt;
    struct String name;
    unsigned type = packet->type, result = packet->result;

    (void)self;
    ui_window_mgr_send_msg_(&g_windowMgr, CHAT_MSG_CLEAR, 0, 0, 0, 0);
    string_init(&name);
    string_assign_cstr(&name, packet->name);
    if (type <= 1 && result <= 2)
        whisper_setting_line(&name, (type == 0 ? 0xd3d : 0xd40) + result);
    string_free(&name);
}

#pragma pack(push, 1)
struct PACKET_ZC_SETTING_WHISPER_PC {
    unsigned short id;
    BYTE type;
    BYTE result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_SETTING_WHISPER_PC, 4);

void game_mode_zc_setting_whisper_pc(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_SETTING_WHISPER_PC *packet = (const struct PACKET_ZC_SETTING_WHISPER_PC *)pkt;
    unsigned type = packet->type, result = packet->result;

    ui_window_mgr_send_msg_(&g_windowMgr, CHAT_MSG_CLEAR, 0, 0, 0, 0);
    if (type == 0 && (result == 1 || result == 2))
        whisper_setting_line(&self->whisper_setting_name, 0xc2 + result - 1);
    else if (type == 1 && result <= 2)
        whisper_setting_line(&self->whisper_setting_name, 0xc4 + result);
}

#pragma pack(push, 1)
struct PACKET_ZC_SETTING_WHISPER_STATE {
    unsigned short id;
    BYTE type;
    BYTE result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_SETTING_WHISPER_STATE, 4);

void game_mode_zc_setting_whisper_state(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_SETTING_WHISPER_STATE *packet = (const struct PACKET_ZC_SETTING_WHISPER_STATE *)pkt;
    unsigned type = packet->type, result = packet->result;

    (void)self;
    ui_window_mgr_send_msg_(&g_windowMgr, CHAT_MSG_CLEAR, 0, 0, 0, 0);
    if (type <= 1 && result <= 1)
        ui_window_mgr_send_msg_(&g_windowMgr, CHAT_MSG_ADD, (int)msg_str((type == 0 ? 0xd63 : 0xd65) + result),
                              CHAT_COLOR_YELLOW, 0, 0);
}

#define CHAT_MSG_ROOM_LINE      5
#define ACTOR_MSG_TALK          7
#define CHAT_COLOR_ROOM         0x222222
#define CHAT_COLOR_PLAYER       0xffffff
#define CHAT_COLOR_GM           0xffff
#define CHAT_COLOR_GM_SVC5      0xffff00
#define CHAT_COLOR_GM_SVC10     0x1d8dff
#define CHAT_TEXT_MAX           0x169
#define SERVICE_TYPE_THAI          5
#define SERVICE_TYPE_GERMANY         10

#pragma pack(push, 1)
struct PACKET_ZC_NOTIFY_CHAT {
    unsigned short id;
    unsigned short length;
    unsigned aid;
    char text[];
};
#pragma pack(pop)

void game_mode_zc_notify_chat(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_NOTIFY_CHAT *packet = (const struct PACKET_ZC_NOTIFY_CHAT *)pkt;
    size_t len = packet->length - 8;
    unsigned aid = packet->aid;
    struct Vec text;
    struct GameActor *speaker;
    char *line;

    if (len - 1 > CHAT_TEXT_MAX - 1)
        return;
    vec_init(&text);
    vec_resize(&text, len + 1, 1);
    line = text.first;
    memcpy_s(text.first, (char *)text.last - (char *)text.first, packet->text, len);
    speaker = world_get_game_actor_by_aid(self->world, aid);
    if (speaker && speaker->object_type == 0 && word_filter_contains(&g_nameFilter, line) == 1) {
        word_filter_censor(&g_nameFilter, line);
    }
    if (g_windowMgr.chat_room_wnd != 0) {
        ui_window_mgr_send_msg_(&g_windowMgr, CHAT_MSG_ROOM_LINE, (int)line, CHAT_COLOR_ROOM, 0, 0);
    } else {
        if (clientinfo_is_yellow_aid(aid)) {
            int color = g_serviceType == SERVICE_TYPE_THAI ? CHAT_COLOR_GM_SVC5
                      : g_serviceType == SERVICE_TYPE_GERMANY ? CHAT_COLOR_GM_SVC10 : CHAT_COLOR_GM;

            ui_window_mgr_send_msg_(&g_windowMgr, CHAT_MSG_ADD, (int)line, color, 1, 0);
        } else if (g_session1.hideChat != 1 && g_session1.hideChatAndGuild != 1) {
            ui_window_mgr_send_msg_(&g_windowMgr, CHAT_MSG_ADD, (int)line, CHAT_COLOR_PLAYER, 1, 0);
        }
        speaker = world_get_game_actor_by_aid(self->world, aid);
        if (speaker)
            actor_send(speaker, ACTOR_MSG_TALK, (int)line, 0);
    }
    if (g_session1.autoSaveChat != 0)
        game_mode_log_whisper(self, line);
    vec_free(&text);
}

#pragma pack(push, 1)
struct PACKET_ZC_WHISPER_LIST {
    unsigned short id;
    short length;
    char names[][24];
};
#pragma pack(pop)

void game_mode_zc_whisper_list(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_WHISPER_LIST *packet = (const struct PACKET_ZC_WHISPER_LIST *)pkt;
    char name[25];
    int count = (packet->length - 4) / 24;
    int i;

    (void)self;
    ui_window_mgr_send_msg_(&g_windowMgr, CHAT_MSG_CLEAR, 0, 0, 0, 0);
    name[24] = 0;
    ui_window_mgr_send_msg_(&g_windowMgr, CHAT_MSG_ADD, (int)msg_str(count != 0 ? 0xd44 : 0xd43), 0xffff00, 0, 0);
    for (i = 0; i < count; i++) {
        if (24 * i + 4 > packet->length)
            return;
        memcpy(name, packet->names[i], 24);
        ui_window_mgr_send_msg_(&g_windowMgr, CHAT_MSG_ADD, (int)name, 0xffff00, 0, 0);
    }
}
