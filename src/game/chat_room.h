#ifndef GAMEMODE_CHAT_ROOM_H
#define GAMEMODE_CHAT_ROOM_H

#include "base.h"

struct ChatRoomInfo {
    struct String title;
    struct String password;
    int type;
    int users;
    int max_users;
    int room_id;
};

struct ChatRoomOwner {
    unsigned room_id;
    unsigned aid;
};

struct GameMode;
struct Tree;

#pragma pack(push, 1)
struct ChatRoomMemberEntry {
    int role;
    char name[24];
};
struct PACKET_ZC_ENTER_ROOM {
    unsigned short id;
    short length;
    int room_id;
    struct ChatRoomMemberEntry members[];
};
#pragma pack(pop)
SIZE_ASSERT(ChatRoomMemberEntry, 28);
OFFSET_ASSERT(PACKET_ZC_ENTER_ROOM, members, 8);

struct ChatRoomInfo *chat_room_info_copy(struct ChatRoomInfo *room, const struct ChatRoomInfo *from);
struct ChatRoomInfo *chat_room_info_assign(struct ChatRoomInfo *room, const struct ChatRoomInfo *from);
void game_mode_zc_enter_room(struct GameMode *mode, const struct PACKET_ZC_ENTER_ROOM *packet);

#endif
