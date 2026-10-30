#ifndef GAMEMODE_PACKETS_ITEM_UPDATE_H
#define GAMEMODE_PACKETS_ITEM_UPDATE_H

#include "game/packet_handlers.h"
#include "game/objects.h"
#include "game/item_info.h"

struct PACKET_ZC_UPDATE_CARDSLOT;
struct PACKET_ZC_GRADE_ENCHANT_ACK;
struct PACKET_ZC_CHANGE_ITEM_OPTION3;

void game_mode_zc_change_item_option(struct GameMode *self, unsigned index, unsigned refine,
                                               struct ItemCards cards, struct ItemOptions options);
void game_mode_zc_change_item_option2(struct GameMode *self, unsigned index, unsigned is_damaged,
                                                unsigned refine, struct ItemCards cards, struct ItemOptions options);

#endif
