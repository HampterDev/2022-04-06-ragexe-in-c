#include "game/store_entries.h"

void buying_store_entry_to_item(struct ItemInfo *item, const struct BuyingStoreEntry *e, int position)
{
    item_info_construct(item);
    item->price = e->price;
    item->charged_price = e->price;
    item->count = e->count;
    item->type = e->type;
    item->index = position;
    item->identified = 1;
    item_info_set_item_id(item, e->item_id);
}
