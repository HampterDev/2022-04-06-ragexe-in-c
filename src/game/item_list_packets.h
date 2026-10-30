#ifndef GAMEMODE_ITEM_LIST_PACKETS_H
#define GAMEMODE_ITEM_LIST_PACKETS_H
#include "base.h"
#include "iteminfo/item_types.h"
struct GameMode;
void game_mode_item_identify_list(struct GameMode *self, const unsigned char *packet);
void game_mode_item_composition_list(struct GameMode *self, const unsigned char *packet);

#pragma pack(push, 1)
struct PACKET_ZC_ITEMLIST {
    unsigned short packet_type;
    short packet_length;
    unsigned char records[];
};
struct PACKET_ZC_STORE_ITEMLIST {
    unsigned short packet_type;
    unsigned short packet_length;
    char name[24];
    unsigned char records[];
};
struct EquipRecordV2 {
    unsigned char base[30];
    int expire_time;
};
struct EquipRecordV3 {
    unsigned char base[30];
    int expire_time;
    unsigned short bind_on_equip_type;
    unsigned short view;
};
struct EquipRecord {
    short index;
    int item_id;
    unsigned char type;
    unsigned char identified;
    unsigned short equip_location;
    unsigned short worn_location;
    unsigned char damaged;
    unsigned char refine;
    int cards[ITEM_CARD_SLOTS];
};
struct EquipRecord4 {
    short index;
    int item_id;
    unsigned char type;
    unsigned short equip_location;
    unsigned short worn_location;
    unsigned char refine;
    int cards[ITEM_CARD_SLOTS];
    int expire_time;
    unsigned short bind_on_equip_type;
    unsigned short view;
    unsigned char flags;
};
struct ItemRecordV5 {
    short index;
    int item_id;
    unsigned char type;
    int equip_location;
    int worn_location;
    unsigned char refine;
    int cards[ITEM_CARD_SLOTS];
    int expire_time;
    unsigned short bind_on_equip_type;
    unsigned short view;
    unsigned char flags;
};
struct ItemRecordV6 {
    short index;
    int item_id;
    unsigned char type;
    int equip_location;
    int worn_location;
    unsigned char refine;
    int cards[ITEM_CARD_SLOTS];
    int expire_time;
    unsigned short bind_on_equip_type;
    unsigned short view;
    unsigned char option_count;
    struct ItemOption options[ITEM_OPTION_SLOTS];
    unsigned char flags;
};
struct ItemRecordV7 {
    short index;
    int item_id;
    unsigned char type;
    int equip_location;
    int worn_location;
    int cards[ITEM_CARD_SLOTS];
    int expire_time;
    unsigned short bind_on_equip_type;
    unsigned short view;
    unsigned char option_count;
    struct ItemOption options[ITEM_OPTION_SLOTS];
    unsigned char refine;
    unsigned char grade;
    unsigned char flags;
};
struct NormalTail {
    int cards[ITEM_CARD_SLOTS];
    int expire_time;
    unsigned char flags;
};
struct NormalRecord {
    short index;
    int item_id;
    unsigned char type;
    short count;
    unsigned short worn_location;
    struct NormalTail tail;
};
struct NormalRecordV5 {
    short index;
    int item_id;
    unsigned char type;
    short count;
    int worn_location;
    struct NormalTail tail;
};
struct PlainRecord {
    short index;
    int item_id;
    unsigned char type;
    unsigned char identified;
    short count;
    unsigned short worn_location;
    int cards[ITEM_CARD_SLOTS];
    int expire_time;
};
struct ItemAddedHead {
    unsigned short packet_type;
    short index;
    int amount;
    int item_id;
    unsigned char type;
    unsigned char identified;
    unsigned char damaged;
};
struct PACKET_ZC_ADD_ITEM_TO_STORE {
    unsigned short packet_type;
    short index;
    int amount;
    int item_id;
    unsigned char identified;
    unsigned char damaged;
    unsigned char refine;
    int cards[ITEM_CARD_SLOTS];
};
struct PACKET_ZC_ADD_ITEM_TO_CART {
    unsigned short packet_type;
    short index;
    int amount;
    int item_id;
    unsigned char identified;
    unsigned char damaged;
    unsigned char refine;
    int cards[ITEM_CARD_SLOTS];
};
struct PACKET_ZC_ADD_ITEM_TO_STORE2 {
    unsigned short packet_type;
    short index;
    int amount;
    int item_id;
    unsigned char type;
    unsigned char identified;
    unsigned char damaged;
    unsigned char refine;
    int cards[ITEM_CARD_SLOTS];
};
struct PACKET_ZC_ADD_ITEM_TO_CART2 {
    unsigned short packet_type;
    short index;
    int amount;
    int item_id;
    unsigned char type;
    unsigned char identified;
    unsigned char damaged;
    unsigned char refine;
    int cards[ITEM_CARD_SLOTS];
};
struct PACKET_ZC_ADD_ITEM_TO_STORE3 {
    unsigned short packet_type;
    short index;
    int amount;
    int item_id;
    unsigned char type;
    unsigned char identified;
    unsigned char damaged;
    unsigned char refine;
    int cards[ITEM_CARD_SLOTS];
    struct ItemOption options[ITEM_OPTION_SLOTS];
};
struct PACKET_ZC_ADD_ITEM_TO_CART3 {
    unsigned short packet_type;
    short index;
    int amount;
    int item_id;
    unsigned char type;
    unsigned char identified;
    unsigned char damaged;
    unsigned char refine;
    int cards[ITEM_CARD_SLOTS];
    struct ItemOption options[ITEM_OPTION_SLOTS];
};
struct PACKET_ZC_ADD_ITEM_TO_STORE4 {
    unsigned short packet_type;
    short index;
    int amount;
    int item_id;
    unsigned char type;
    unsigned char identified;
    unsigned char damaged;
    int cards[ITEM_CARD_SLOTS];
    struct ItemOption options[ITEM_OPTION_SLOTS];
    unsigned char refine;
    unsigned char grade;
};
struct PACKET_ZC_ADD_ITEM_TO_CART4 {
    unsigned short packet_type;
    short index;
    int amount;
    int item_id;
    unsigned char type;
    unsigned char identified;
    unsigned char damaged;
    int cards[ITEM_CARD_SLOTS];
    struct ItemOption options[ITEM_OPTION_SLOTS];
    unsigned char refine;
    unsigned char grade;
};
struct PACKET_ZC_DELETE_ITEM_FROM_STORE {
    unsigned short packet_type;
    short index;
    int amount;
};
struct PACKET_ZC_DELETE_ITEM_FROM_CART {
    unsigned short packet_type;
    short index;
    int amount;
};
struct PACKET_ZC_ITEM_THROW_ACK {
    unsigned short packet_type;
    unsigned short index;
    short count;
};
struct PACKET_ZC_NOTIFY_BIND_ON_EQUIP {
    unsigned short packet_type;
    unsigned short index;
};
struct PACKET_ZC_ITEMIDENTIFY_LIST {
    unsigned short packet_type;
    unsigned short packet_length;
    short indexes[];
};
#pragma pack(pop)
SIZE_ASSERT(EquipRecord, 30);
SIZE_ASSERT(EquipRecord4, 37);
SIZE_ASSERT(ItemRecordV5, 41);
SIZE_ASSERT(ItemRecordV6, 67);
SIZE_ASSERT(ItemRecordV7, 68);
SIZE_ASSERT(NormalTail, 21);
SIZE_ASSERT(NormalRecord, 32);
SIZE_ASSERT(NormalRecordV5, 34);
SIZE_ASSERT(PlainRecord, 32);
SIZE_ASSERT(ItemAddedHead, 15);
SIZE_ASSERT(EquipRecordV2, 34);
SIZE_ASSERT(EquipRecordV3, 38);
SIZE_ASSERT(PACKET_ZC_ADD_ITEM_TO_STORE, 31);
SIZE_ASSERT(PACKET_ZC_ADD_ITEM_TO_CART, 31);
SIZE_ASSERT(PACKET_ZC_ADD_ITEM_TO_STORE2, 32);
SIZE_ASSERT(PACKET_ZC_ADD_ITEM_TO_CART2, 32);
OFFSET_ASSERT(PACKET_ZC_ADD_ITEM_TO_STORE3, options, 0x20);
OFFSET_ASSERT(PACKET_ZC_ADD_ITEM_TO_CART3, options, 0x20);
OFFSET_ASSERT(PACKET_ZC_ADD_ITEM_TO_STORE4, refine, 0x38);
OFFSET_ASSERT(PACKET_ZC_ADD_ITEM_TO_CART4, refine, 0x38);
SIZE_ASSERT(PACKET_ZC_DELETE_ITEM_FROM_STORE, 8);
SIZE_ASSERT(PACKET_ZC_DELETE_ITEM_FROM_CART, 8);
SIZE_ASSERT(PACKET_ZC_ITEM_THROW_ACK, 6);
SIZE_ASSERT(PACKET_ZC_NOTIFY_BIND_ON_EQUIP, 4);
#endif
