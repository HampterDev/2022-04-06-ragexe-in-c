#include "contents/connection_api.h"
#ifndef GAMEMODE_CONNECTION_H
#define GAMEMODE_CONNECTION_H

#include <windows.h>

#include "base.h"

struct GameMode;
struct String;

#pragma pack(push, 1)
struct PACKET_SC_NOTIFY_BAN {
    unsigned short id;
    unsigned char code;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_SC_NOTIFY_BAN, 3);

void game_mode_send_keep_alive(struct GameMode *self, char force);
void game_mode_move_to_server(struct GameMode *self, struct String map_name, int x, int y,
                           unsigned long ip, int port, struct String server_map_name);
void game_mode_notify_ban(struct GameMode *self, const struct PACKET_SC_NOTIFY_BAN *packet);
void game_mode_isvr_disconnect(struct GameMode *self);
struct PACKET_ZC_ACK_REQ_DISCONNECT;
void game_mode_ack_req_disconnect(struct GameMode *self,
                               const struct PACKET_ZC_ACK_REQ_DISCONNECT *packet);

void game_mode_initialize(struct GameMode *self);

struct List;
extern DWORD g_firstKeepAliveTime;
extern struct List g_keepAliveTimes;

#endif
