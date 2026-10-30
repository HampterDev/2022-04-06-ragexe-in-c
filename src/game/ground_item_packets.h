#ifndef GAMEMODE_GROUND_ITEM_PACKETS_H
#define GAMEMODE_GROUND_ITEM_PACKETS_H
#include <stdint.h>
#include "base.h"
struct GameMode;

#pragma pack(push, 1)
struct PacketGroundItemEntry {
    uint16_t id;
    int32_t aid, item_id;
    uint8_t identified;
    int16_t x, y, count;
    uint8_t sub_x, sub_y;
};
struct PacketGroundItemFall {
    uint16_t id;
    int32_t aid, item_id;
    uint8_t identified;
    int16_t x, y;
    uint8_t sub_x, sub_y;
    int16_t count;
};
struct PacketGroundItemFall4 {
    uint16_t id;
    int32_t aid, item_id;
    uint16_t type;
    uint8_t identified;
    int16_t x, y;
    uint8_t sub_x, sub_y;
    int16_t count;
};
#pragma pack(pop)
SIZE_ASSERT(PacketGroundItemEntry, 19);
SIZE_ASSERT(PacketGroundItemFall, 19);
SIZE_ASSERT(PacketGroundItemFall4, 21);

void game_mode_item_entry(struct GameMode *self, const struct PacketGroundItemEntry *packet);
void game_mode_item_fall_entry(struct GameMode *self, const struct PacketGroundItemFall *packet);
void game_mode_item_fall_entry4(struct GameMode *self, const struct PacketGroundItemFall4 *packet);
#endif
