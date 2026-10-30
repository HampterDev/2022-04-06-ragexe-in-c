#ifndef GAMEMODE_NAVIGATION_H
#define GAMEMODE_NAVIGATION_H

#include "base.h"
#include <stdint.h>
struct GameMode;
#pragma pack(push, 1)
struct PacketNavigationActive {
    uint16_t packet_type;
    uint8_t type, flag, hide_window;
    char map_name[16];
    int16_t x, y, mob_id;
};
#pragma pack(pop)
SIZE_ASSERT(PacketNavigationActive, 27);
void game_mode_on_navigation_active(struct GameMode *self, const struct PacketNavigationActive *packet);

#endif
