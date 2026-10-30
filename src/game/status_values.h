#ifndef GAMEMODE_STATUS_VALUES_H
#define GAMEMODE_STATUS_VALUES_H

#include "game/packet_handlers.h"
#include "game/gamemode.h"
#include "game/ui_bridge.h"
#include "game/session_bridge.h"

struct PacketLongLongParChange;

struct PacketLongParChange;

struct PacketNotifyExp;

void session_set_advanced_status(struct Session *session, int status, int value);
struct PacketNotifyExp2;

#endif
