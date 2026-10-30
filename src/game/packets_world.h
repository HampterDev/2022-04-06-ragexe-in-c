#ifndef GAMEMODE_PACKETS_WORLD_H
#define GAMEMODE_PACKETS_WORLD_H

#include "game/packet_handlers.h"
#include "game/actor_packets.h"
#include "game/objects.h"
#include "game/party_member.h"

struct PACKET_ZC_UPDATE_MAPINFO;
struct PACKET_ZC_TALKBOX_CHATCONTENTS;
struct PACKET_ZC_RESURRECTION;
struct PACKET_ZC_SHOW_IMAGE;
struct PACKET_ZC_SHOW_IMAGE2;
struct PACKET_ZC_MONSTER_INFO;

void game_mode_update_cut_in(struct GameMode *self);

#endif
