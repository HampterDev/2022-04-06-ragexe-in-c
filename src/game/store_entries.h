#ifndef GAMEMODE_STORE_ENTRIES_H
#define GAMEMODE_STORE_ENTRIES_H

#include "game/item_info.h"

#pragma pack(push, 1)
struct BuyingStoreEntry {
    int price;
    short count;
    unsigned char type;
    int item_id;
};

struct VendingEntry {
    int price;
    short count;
    short index;
    unsigned char type;
    int item_id;
    unsigned char identified;
    unsigned char damaged;
    unsigned char refine;
    int slot[ITEM_CARD_SLOTS];
    struct ItemOption options[ITEM_OPTION_SLOTS];
    int equip_location;
    unsigned short view;
};

struct VendingEntry4 {
    int price;
    short count;
    short index;
    unsigned char type;
    int item_id;
    unsigned char identified;
    unsigned char damaged;
    int slot[ITEM_CARD_SLOTS];
    struct ItemOption options[ITEM_OPTION_SLOTS];
    int equip_location;
    unsigned short view;
    unsigned char refine;
    unsigned char grade;
};

struct MyVendingEntry {
    int price;
    short index;
    short count;
    unsigned char type;
    int item_id;
    unsigned char identified;
    unsigned char damaged;
    unsigned char refine;
    int slot[ITEM_CARD_SLOTS];
    struct ItemOption options[ITEM_OPTION_SLOTS];
};

struct MyVendingEntry3 {
    int price;
    short index;
    short count;
    unsigned char type;
    int item_id;
    unsigned char identified;
    unsigned char damaged;
    int slot[ITEM_CARD_SLOTS];
    struct ItemOption options[ITEM_OPTION_SLOTS];
    unsigned char refine;
    unsigned char grade;
};
#pragma pack(pop)
SIZE_ASSERT(BuyingStoreEntry, 11);
SIZE_ASSERT(VendingEntry, 0x3f);
SIZE_ASSERT(VendingEntry4, 0x40);
SIZE_ASSERT(MyVendingEntry, 0x39);
SIZE_ASSERT(MyVendingEntry3, 0x3a);

#pragma pack(push, 1)
struct PACKET_ZC_PC_PURCHASE_RESULT {
    unsigned short packet_type;
    unsigned char result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_PC_PURCHASE_RESULT, 3);

void buying_store_entry_to_item(struct ItemInfo *item, const struct BuyingStoreEntry *e, int position);

#endif
