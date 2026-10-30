#include "contents/connection_api.h"
#ifndef GAMEMODE_NET_H
#define GAMEMODE_NET_H

struct RagConnection;

void net_send_header_only(unsigned short packet_id);
void net_send_header_only2(unsigned short packet_id);

#endif
