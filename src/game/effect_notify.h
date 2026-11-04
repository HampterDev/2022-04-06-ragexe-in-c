#ifndef GAMEMODE_EFFECT_NOTIFY_H
#define GAMEMODE_EFFECT_NOTIFY_H

#include "game/packet_handlers.h"
#include "game/gamemode.h"
#include "game/session_bridge.h"
#include <stdint.h>

struct PacketNotifyEffect;
struct PacketNotifyEffect2;
struct PacketNotifyEffect3;
struct PacketNpcShowEfstUpdate;

struct PacketNotifyEffect4;
struct PacketClearEffect;
struct PacketCongratulation;
void game_mode_notify_effect_with_num(struct GameMode *self, int aid, int effect_id, int64_t num);

#endif
