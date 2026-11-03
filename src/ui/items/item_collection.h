#ifndef UI_ITEMS_ITEM_COLLECTION_H
#define UI_ITEMS_ITEM_COLLECTION_H

struct ItemInfo;

/* Shows `item` in the item collection window and, when item comparison is on and
   `no_compare` is 0, the worn item it would replace in the comparison window. Installed
   as g_uiHooks.show_item_link for item-link clicks in the generic widgets. */
void item_collection_show_link(struct ItemInfo *item, int no_compare);

#endif
