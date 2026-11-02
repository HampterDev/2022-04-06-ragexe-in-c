#ifndef GAMEMODE_ITEM_UPGRADE_H
#define GAMEMODE_ITEM_UPGRADE_H

#include <stdint.h>
#include "game/gamemode.h"
#include "game/ui_bridge.h"

struct PacketRepairItemList2;
void game_mode_on_refining_material_list(struct GameMode *self, short index, unsigned char count,
                                                const void *materials, unsigned char blessing_count);
void game_mode_on_repair_item_list2(struct GameMode *self, const struct PacketRepairItemList2 *packet);

#endif
