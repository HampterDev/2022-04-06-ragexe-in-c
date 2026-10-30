#include "game/replay_actor_events.h"
#include "ui/window_manager_state.h"
#include <string.h>
#include <windows.h>
#include <mmsystem.h>
#include "game/mode_msg.h"
#include "game/talk_type.h"
#include "ui/ui_std.h"

#define CZ_CREATE_CHATROOM          0xd5
#define CZ_REQ_ENTER_ROOM           0xd9
#define CZ_CHANGE_CHATROOM          0xde
#define CZ_REQ_ROLE_CHANGE          0xe0
#define CZ_REQ_EXPEL_MEMBER         0xe2
#define CZ_EXIT_ROOM                0xe3

#define CHAT_ROOM_PACKET_MAX 0x400

int game_mode_msg_chat_room_msg(struct GameMode *self, int kind, int arg)
{
    struct UIWindow *log;

    game_mode_process_talk_type(self, kind, (void *)arg);
    log = g_windowMgr.command_list_wnd;
    if (log)
        ui_window_refresh(log);
    return 0;
}

#pragma pack(push, 1)
struct ChatRoomHeader {
    unsigned short id;
    unsigned short len;
    unsigned short max_users;
    unsigned char type;
    char password[8];
};
#pragma pack(pop)

static void send_chat_room(struct GameMode *self, const struct ChatRoomInfo *room, unsigned short id)
{
    struct ChatRoomHeader header;
    unsigned char packet[CHAT_ROOM_PACKET_MAX];

    string_assign(&self->own_chat_room.title, string_cstr(&room->title), room->title.size);
    self->own_chat_room.type = room->type;
    self->own_chat_room.users = room->users;
    self->own_chat_room.max_users = room->max_users;
    self->own_chat_room.room_id = room->room_id;
    string_assign(&self->own_chat_room.password, string_cstr(&room->password), room->password.size);
    header.id = id;
    memset(header.password, 0, sizeof(header.password));
    header.len = (unsigned short)room->title.size + 0xf;
    memcpy(header.password, string_cstr(&room->password), room->password.size);
    header.max_users = (unsigned short)room->max_users;
    header.type = (unsigned char)room->type;
    memcpy(packet, &header, sizeof(header));
    memcpy(packet + sizeof(header), string_cstr(&room->title), room->title.size);
    game_mode_record_own_chat_room(self);
    send_raw((short)header.len, packet);
}

int game_mode_msg_create_chat_room(struct GameMode *self, const struct ChatRoomInfo *room)
{
    send_chat_room(self, room, CZ_CREATE_CHATROOM);
    return 0;
}

int game_mode_msg_change_chat_room(struct GameMode *self, const struct ChatRoomInfo *room)
{
    if (!g_windowMgr.chat_room_wnd)
        return 0;
    send_chat_room(self, room, CZ_CHANGE_CHATROOM);
    return 0;
}

int game_mode_msg_enter_chat_room(struct GameMode *self, int room_id, const char *password)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        int room_id;
        char password[8];
    } packet;
#pragma pack(pop)
    char copy[12];
    struct UIWindow *in_room = g_windowMgr.chat_room_wnd;

    if (timeGetTime() > self->chat_room_enter_time + 1000)
        self->chat_room_pending = 0;
    if (in_room || self->chat_room_pending)
        return 0;
    self->chat_room_pending = 1;
    self->chat_room_enter_time = timeGetTime();
    packet.id = CZ_REQ_ENTER_ROOM;
    packet.room_id = room_id;
    if (password) {
        strcpy(copy, password);
        memcpy(packet.password, copy, 8);
    } else {
        memset(packet.password, 0, 8);
    }
    send_packet(CZ_REQ_ENTER_ROOM, &packet);
    return 0;
}

int game_mode_msg_chat_room_role(const char *name, int role)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        int role;
        char name[24];
    } packet;
#pragma pack(pop)
    char copy[64];

    packet.id = CZ_REQ_ROLE_CHANGE;
    strcpy(copy, name);
    packet.role = role;
    memcpy(packet.name, copy, 24);
    send_packet(CZ_REQ_ROLE_CHANGE, &packet);
    return 0;
}

int game_mode_msg_chat_room_expel(const char *name)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        char name[24];
    } packet;
#pragma pack(pop)
    char copy[64];

    packet.id = CZ_REQ_EXPEL_MEMBER;
    strcpy(copy, name);
    memcpy(packet.name, copy, 24);
    send_packet(CZ_REQ_EXPEL_MEMBER, &packet);
    return 0;
}

int game_mode_msg_exit_chat_room(void)
{
    if (!g_windowMgr.chat_room_wnd)
        return 0;
    send_id(CZ_EXIT_ROOM);
    return 0;
}
