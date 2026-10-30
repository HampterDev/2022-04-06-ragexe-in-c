#ifndef GAME_STORE_SEARCH_ITEMS_H
#define GAME_STORE_SEARCH_ITEMS_H

#include "iteminfo/item_types.h"

struct StoreSearchItem {
    int store_id;
    unsigned aid;
    char store_name[80];
    int price;
    unsigned short amount;
    struct ItemInfo item;
};

struct StoreSearchItem *store_search_item_copy(struct StoreSearchItem *item,
                                                        const struct StoreSearchItem *src);
void store_search_items_destroy(struct StoreSearchItem *first, struct StoreSearchItem *last);

#endif
