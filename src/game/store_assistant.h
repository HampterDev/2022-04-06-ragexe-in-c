#ifndef GAMEMODE_STORE_ASSISTANT_H
#define GAMEMODE_STORE_ASSISTANT_H

#include "game/packet_handlers.h"
#include "game/gamemode.h"
#include "game/ui_bridge.h"
#include "game/session_bridge.h"

struct BuyingStoreEntry;
struct VendingEntry;
struct PacketVendingList4;

struct PacketPurchaseItemListFromMc4;
struct PacketPurchaseMyItemList3;

void game_mode_on_store_assistant_entry(struct GameMode *self, int aid, int job, short x, short y,
                                               unsigned char sex, short head, int weapon, int shield,
                                               short accessory, short accessory2, short accessory3,
                                               short head_palette, short body_palette, short robe,
                                               struct String title);

#endif
