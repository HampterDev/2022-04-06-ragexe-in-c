#ifndef SESSION_SHOP_INVENTORY_H
#define SESSION_SHOP_INVENTORY_H

#include "iteminfo/item_types.h"

struct Session;
struct InventoryMatch {
    unsigned char found;
    struct ItemInfo item;
};

struct InventoryMatch *session_find_inventory_item(struct Session *session,
    struct InventoryMatch *out, const struct ItemInfo *item);
int session_shop_item_count(struct Session *session);
struct ItemInfo *session_shop_item_at(struct Session *session, struct ItemInfo *out, int index);
unsigned char session_sell_list_add(struct Session *session, const struct ItemInfo *item,
    unsigned char no_merge, unsigned char notify);
unsigned char session_mirror_remove(struct Session *session, const struct ItemInfo *item,
    int variant, unsigned char notify);

#endif
