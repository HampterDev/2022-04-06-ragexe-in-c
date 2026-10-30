#ifndef GAMEMODE_INVENTORY_LISTS_H
#define GAMEMODE_INVENTORY_LISTS_H

#include "game/packet_handlers.h"
#include "ui/window_manager_state.h"
#include "game/objects.h"
#include "game/item_info.h"

#pragma pack(push, 1)
struct EquipItemV7 {
    short index;
    int item_id;
    unsigned char type;
    int location;
    int wear_state;
    unsigned char refining_level;
    int slot[ITEM_CARD_SLOTS];
    int hire_expire_date;
    unsigned short bind_on_equip_type;
    unsigned short sprite_number;
    unsigned char option_count;
    struct ItemOption options[ITEM_OPTION_SLOTS];
    unsigned char flags;
};
struct NormalItem {
    short index;
    int item_id;
    unsigned char type;
    short amount;
    int wear_state;
    int slot[ITEM_CARD_SLOTS];
    int hire_expire_date;
    unsigned char flags;
};
#pragma pack(pop)
SIZE_ASSERT(EquipItemV7, 0x43);
SIZE_ASSERT(NormalItem, 0x22);

#define WINDOW_STORAGE 0x21
enum { UIM_STORAGE_REFRESH = 0x17 };
#include "ui/shops/storage.h"

void item_list_decode_equip_v7(struct ItemInfo *item, const struct EquipItemV7 *e);
void item_list_decode_normal(struct ItemInfo *item, const struct NormalItem *e);

struct PACKET_ZC_SPLIT_SEND_ITEMLIST_EQUIPV8;
struct PACKET_ZC_SPLIT_SEND_ITEMLIST_SET;

#endif
