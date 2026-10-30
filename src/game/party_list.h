#ifndef GAMEMODE_PARTY_LIST_H
#define GAMEMODE_PARTY_LIST_H

#include "game/packet_handlers.h"
#include "game/gamemode.h"
#include "game/ui_bridge.h"
#include "game/session_bridge.h"

#pragma pack(push, 2)
struct PartyListMember {
    int aid;
    int char_id;
    char name[24];
    char map_name[16];
    unsigned char role;
    unsigned char state;
    unsigned short job;
    unsigned short level;
};
#pragma pack(pop)
SIZE_ASSERT(PartyListMember, 0x36);

struct PacketGroupList2;

struct PacketGroupList3;

void game_mode_set_party_list(struct GameMode *self, const char *party_name, const struct Vec *members);
struct PacketNotifyMemberInfoToGroupm;

#endif
