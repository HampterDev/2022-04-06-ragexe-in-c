#ifndef ITEMINFO_ITEM_TYPES_H
#define ITEMINFO_ITEM_TYPES_H

#include "base.h"

#define ITEM_CARD_SLOTS 4
#define ITEM_OPTION_SLOTS 5

enum { ITEM_OPTION_TITLE = 9999 };

#pragma pack(push, 1)
struct ItemOption {
    unsigned short id;
    short value;
    unsigned char param;
};
#pragma pack(pop)
SIZE_ASSERT(ItemOption, 5);

struct ItemRecordA {
    int index;
    int price;
    int charged_price;
    int type;
    BYTE identified;
    char id_text[0x21];
    int equip_location;
    int view;
};

struct ItemCards {
    int card[ITEM_CARD_SLOTS];
};

struct ItemOptions {
    struct ItemOption option[ITEM_OPTION_SLOTS];
};
SIZE_ASSERT(ItemCards, 16);
SIZE_ASSERT(ItemOptions, 25);

struct ItemLogLine {
    int id;
    int amount;
    char action;
    char name[24];
    char time[24];
};
SIZE_ASSERT(ItemLogLine, 0x3c);

struct ItemInfo {
    int type;
    int index;
    int equip_location;
    int worn_location;
    int count;
    int price;
    int charged_price;
    int slot[ITEM_CARD_SLOTS];
    struct String id_text;
    struct String name;
    unsigned char identified;
    unsigned char damaged;
    int refine;
    unsigned short bind_on_equip_type;
    unsigned char expire_in_hours;
    int expire_time;
    unsigned short weight;
    int view;
    unsigned char favorite;
    unsigned char is_new;
    unsigned char move_pending;
    unsigned char by_index;
    int sale_kind;
    int sale_start_time;
    int sale_end_time;
    int sale_remaining;
    short grade;
    int option_count;
    struct ItemOption options[ITEM_OPTION_SLOTS];
    struct ItemLogLine guild_log;
};

#endif
