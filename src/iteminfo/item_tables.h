#ifndef ITEMINFO_ITEM_TABLES_H
#define ITEMINFO_ITEM_TABLES_H
#include "iteminfo/item_table.h"

struct CardAffix {
    const char *illustName;
    const char *prefix;
    const char *postfix;
};

struct ItemMoveInfo {
    int item_id;
    unsigned char flags[8];
};

const struct CardAffix *card_affix_get(int id);
unsigned char card_affix_has(int id);
unsigned char book_item_has(int id);
const struct ItemTextLines *metal_process_get(int id);
const struct ItemMoveInfo *item_move_info_get(int id);
#endif
