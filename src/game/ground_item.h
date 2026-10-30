#ifndef GAMEMODE_GROUND_ITEM_H
#define GAMEMODE_GROUND_ITEM_H

#include "game/packet_handlers.h"
#include "game/ground_item_packets.h"
#include "game/ground_item_state.h"
#include "game/gamemode.h"
#include "game/ui_bridge.h"
#include "game/session_bridge.h"

struct ItemDropInfo {
    char item_id_text[0x20];
    int aid;
    int x, y;
    int sub_x, sub_y;
    int falls;
    int count;
    unsigned char is_identified;
    unsigned short type;
    unsigned char show_drop_effect;
    unsigned short drop_effect_mode;
};
void item_place(struct Item *item, struct GameMode *mode, const struct ItemDropInfo *info);

enum { ITEM_MSG_DROP = 4 };

struct PacketItemFallEntry5;
struct PacketItemPickupParty2;

#endif
