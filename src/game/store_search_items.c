#include "iteminfo/item_api.h"
#include <string.h>
#include "std/std.h"
#include "game/store_search_items.h"

struct StoreSearchItem *store_search_item_copy(struct StoreSearchItem *item,
                                                        const struct StoreSearchItem *src)
{
    item->store_id = src->store_id;
    item->aid = src->aid;
    memcpy(item->store_name, src->store_name, sizeof item->store_name);
    item->price = src->price;
    item->amount = src->amount;
    item_info_copy(&item->item, &src->item);
    return item;
}

void store_search_items_destroy(struct StoreSearchItem *first, struct StoreSearchItem *last)
{
    for (; first != last; first++) {
        item_info_destruct(&first->item);
    }
}
