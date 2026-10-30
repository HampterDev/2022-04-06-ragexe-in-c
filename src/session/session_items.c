#include "replay/replay.h"
#include "ui/window_manager_state.h"
#include "session/session_items_priv.h"
#include "session/session_items.h"
#include "session/session_windows.h"

#include <stdlib.h>
#include "iteminfo/item_types.h"
#include "ui/wndmgr/wndmgr.h"

unsigned equip_location_to_base(unsigned location)
{
    switch (location) {
    case 0x400:    return 0x100;
    case 0x800:    return 0x200;
    case 0xc00:    return 0x300;
    case 0x1000:   return 0x1;
    case 0x1400:   return 0x101;
    case 0x1800:   return 0x201;
    case 0x1c00:   return 0x301;
    case 0x2000:   return 0x4;
    case 0x4000:   return 0x2;
    case 0x10000:  return 0x10;
    case 0x20000:  return 0x2;
    case 0x40000:  return 0x20;
    case 0x80000:  return 0x40;
    case 0x100000: return 0x8;
    case 0x200000: return 0x80;
    default:       return location;
    }
}

int *equip_location_slots(int *out, unsigned location)
{
    int bits[EQUIP_SLOT_COUNT];
    int count = 0, slot;
    int mask = (int)location;

    for (slot = 0; slot < EQUIP_SLOT_COUNT; slot++) {
        if (mask & 1)
            bits[count++] = slot;
        mask >>= 1;
    }
    if (count == 0) {
        out[0] = out[1] = out[2] = -1;
    } else if (count == 1) {
        out[0] = -1;
        out[1] = -1;
        out[2] = bits[0];
    } else if (count == 2) {
        out[0] = -1;
        out[1] = bits[1];
        out[2] = bits[0];
    } else {
        out[0] = bits[2];
        out[1] = bits[1];
        out[2] = bits[0];
    }
    return out;
}

static void slots_of(int slots[3], unsigned location)
{
    if (location == ITEM_LOCATION_AMMO) {
        slots[0] = 0xf;
        slots[1] = -1;
        slots[2] = -1;
    } else {
        int packed[3];
        equip_location_slots(packed,
                             equip_location_to_base(location));
        slots[0] = packed[2];
        slots[1] = packed[1];
        slots[2] = packed[0];
    }
}

int *session_equip_slots_of(int *out, unsigned location)
{
    int packed[3];

    if (location == ITEM_LOCATION_AMMO) {
        out[0] = -1;
        out[1] = -1;
        out[2] = 0xf;
        return out;
    }
    equip_location_slots(packed, equip_location_to_base(location));
    out[0] = packed[0];
    out[1] = packed[1];
    out[2] = packed[2];
    return out;
}

BYTE session_set_worn_item(struct Session *self, const struct ItemInfo *item)
{
    int slots[3], i;

    slots_of(slots, item->worn_location);
    for (i = 0; i < 3; i++) {
        int slot = slots[i];
        if (slot == -1 || slot >= EQUIP_SLOT_COUNT)
            continue;
        item_info_assign(&self->equipSlots[slot], item);
        self->equipSlots[slot].count = 1;
    }
    session_refresh_equip_wnd(self);
    ui_window_mgr_invalidate_once_per_frame(&g_windowMgr);
    return 1;
}

BYTE session_set_viewed_item(struct Session *self, const struct ItemInfo *item)
{
    int slots[3], i;

    slots_of(slots, item->worn_location);
    for (i = 0; i < 3; i++) {
        struct ItemInfo *dest;
        if (slots[i] == -1)
            continue;
        dest = &self->viewedEquips[slots[i]];
        item_info_assign(dest, item);
        dest->count = 1;
        dest->expire_time = item->expire_time;
        dest->bind_on_equip_type = item->bind_on_equip_type;
        dest->view = item->view;
    }
    return 1;
}

struct ItemInfo *session_get_inventory_item(struct Session *self,
                                                    struct ItemInfo *out, int index)
{
    struct ItemInfo *have;

    if (self->itemList.size == 0) {
        item_info_construct_with_arg(out);
        return out;
    }
    have = find_by_index(&self->itemList, index);
    if (have)
        item_info_copy(out, have);
    else
        item_info_construct_with_arg(out);
    return out;
}

BYTE session_add_inventory_item(struct Session *self,
                                          const struct ItemInfo *item, BYTE refresh)
{
    struct ItemInfo *have = find_by_index(&self->itemList, item->index);

    if (have) {
        have->count += item->count;
        have->is_new = item->is_new;
        have->favorite = item->favorite;
        if (refresh)
            ui_window_mgr_refresh_item_windows(&g_windowMgr);
        return 0;
    }
    item_info_copy(list_push_back(&self->itemList, NULL, sizeof(struct ItemInfo)), item);
    if (refresh)
        ui_window_mgr_refresh_item_windows(&g_windowMgr);
    return 1;
}

BYTE session_sell_list_add(struct Session *self,
                                     const struct ItemInfo *item, BYTE noMerge, BYTE notify)
{
    struct ItemInfo *have = find_by_index(&self->storeSetupItems, item->index);

    if (have) {
        if (!noMerge) {
            have->count += item->count;
            if (notify == 1)
                notify_item_window(&g_windowMgr.merchant_shop_make_wnd);
        }
        return 0;
    }
    item_info_copy(list_push_back(&self->storeSetupItems, NULL, sizeof(struct ItemInfo)), item);
    if (notify == 1)
        notify_item_window(noMerge ? &g_windowMgr.buying_store_make_wnd : &g_windowMgr.merchant_shop_make_wnd);
    return 1;
}

BYTE session_add_cart_item(struct Session *self, const struct ItemInfo *item)
{
    struct ItemInfo *have = find_by_index(&self->cartItems, item->index);

    if (have) {
        have->count += item->count;
        notify_item_window(&g_windowMgr.merchant_item_wnd);
        return 0;
    }
    item_info_copy(list_push_back(&self->cartItems, NULL, sizeof(struct ItemInfo)), item);
    notify_item_window(&g_windowMgr.merchant_item_wnd);
    return 1;
}

BYTE session_mirror_add(struct Session *self,
                                     const struct ItemInfo *item, int which, BYTE notify)
{
    struct ItemInfo *have = find_by_index(&self->storeCandidateItems, item->index);

    if (have) {
        have->count += item->count;
        if (notify == 1)
            notify_item_window(store_candidate_window_slot(which));
        return 0;
    }
    item_info_copy(list_push_back(&self->storeCandidateItems, NULL, sizeof(struct ItemInfo)), item);
    if (notify == 1)
        notify_item_window(store_candidate_window_slot(which));
    return 1;
}

void session_add_own_store_item(struct Session *self, const struct ItemInfo *item)
{
    struct ItemInfo *have = find_by_index(&self->ownStoreItems, item->index);
    if (have)
        have->count += item->count;
    else
        item_info_copy(list_push_back(&self->ownStoreItems, NULL, sizeof(struct ItemInfo)), item);
}

void session_purchase_add(struct Session *self, const struct ItemInfo *item)
{
    struct ItemInfo *have = find_by_index(&self->storePurchaseItems, item->index);
    if (have)
        have->count += item->count;
    else
        item_info_copy(list_push_back(&self->storePurchaseItems, NULL, sizeof(struct ItemInfo)), item);
}

void session_add_other_store_item(struct Session *self, const struct ItemInfo *item)
{
    struct ItemInfo *have = find_by_index(&self->otherStoreItems, item->index);
    if (have)
        have->count += item->count;
    else
        item_info_copy(list_push_back(&self->otherStoreItems, NULL, sizeof(struct ItemInfo)), item);
}

void session_sellable_add(struct Session *self, const struct ItemInfo *item)
{
    struct ItemInfo *have = find_by_index(&self->shopItems, item->index);
    if (!have) {
        item_info_copy(list_push_back(&self->shopItems, NULL, sizeof(struct ItemInfo)), item);
        return;
    }
    if (have->count != -1)
        have->count += item->count;
}

BYTE session_add_storage_item(struct Session *self,
                                     const struct ItemInfo *item, BYTE notify)
{
    struct ItemInfo *have;

    if (g_replayPlaying && !replay_recorder_instance()->info.options[2])
        return 1;
    have = find_by_index(&self->storageItems, item->index);
    if (have) {
        have->count += item->count;
        if (notify)
            notify_item_window(&g_windowMgr.item_store_wnd);
        return 0;
    }
    item_info_copy(list_push_back(&self->storageItems, NULL, sizeof(struct ItemInfo)), item);
    if (notify)
        notify_item_window(&g_windowMgr.item_store_wnd);
    return 1;
}

void session_add_shop_goods_by_name(struct Session *self, const struct ItemInfo *item)
{
    struct ItemInfo *have = find_by_name(&self->shopItems, item);

    if (!have) {
        item_info_copy(list_push_back(&self->shopItems, NULL, sizeof(struct ItemInfo)), item);
        return;
    }
    if (have->count == -1)
        have->count = -1;
    else
        have->count += item->count;
}

void session_add_shop_buy_item(struct Session *self, const struct ItemInfo *item)
{
    struct ItemInfo *have = find_by_name(&self->purchaseItems, item);
    if (have)
        have->count += item->count;
    else
        item_info_copy(list_push_back(&self->purchaseItems, NULL, sizeof(struct ItemInfo)), item);
}

void session_add_shop_buy_item_keyed(struct Session *self, const struct ItemInfo *item)
{
    struct ListNode *node;
    struct List *list = &self->purchaseItems;

    for (node = list->head->next; node != list->head; node = node->next) {
        struct ItemInfo *have = LIST_VALUE(node, struct ItemInfo);
        BYTE match = have->index == item->index;
        if (!item->by_index)
            match = string_equal(&have->id_text, &item->id_text);
        if (match == 1) {
            have->count += item->count;
            return;
        }
    }
    item_info_copy(list_push_back(list, NULL, sizeof(struct ItemInfo)), item);
}

void session_append_market_goods(struct Session *self,
                                        const struct ItemRecordA *packetItem, int count, WORD weight)
{
    struct ItemInfo item;

    item_info_construct(&item);
    item_info_assign_from_record_a(&item, packetItem);
    item.count = count;
    item.weight = weight;
    item.refine = 0;
    item.damaged = 0;
    item_info_set_item_id(&item, atoi(string_cstr(&item.id_text)));
    item_info_copy(list_push_back(&self->shopItems, NULL, sizeof(struct ItemInfo)), &item);
    string_free(&item.name);
    string_free(&item.id_text);
}

void session_append_shop_goods_record(struct Session *self,
                                              const struct ItemRecordA *packetItem, int fromInventory)
{
    struct ItemInfo item;

    if (fromInventory) {
        struct ItemInfo have;
        session_get_inventory_item(self, &have, packetItem->index);
        if ((unsigned)have.index > 0) {
            item_info_copy(&item, &have);
            item_info_assign_from_record_a(&item, packetItem);
            item_info_set_item_id(&item, atoi(string_cstr(&item.id_text)));
            item_info_copy(list_push_back(&self->shopItems, NULL, sizeof(struct ItemInfo)), &item);
            string_free(&item.name);
            string_free(&item.id_text);
        }
        string_free(&have.name);
        string_free(&have.id_text);
        return;
    }
    item_info_construct(&item);
    item_info_assign_from_record_a(&item, packetItem);
    item.refine = 0;
    item.damaged = 0;
    item_info_set_item_id(&item, atoi(string_cstr(&item.id_text)));
    item.equip_location = packetItem->equip_location;
    item.view = packetItem->view;
    item_info_copy(list_push_back(&self->shopItems, NULL, sizeof(struct ItemInfo)), &item);
    string_free(&item.name);
    string_free(&item.id_text);
}

void session_push_shop_goods(struct Session *self, const struct ItemInfo *item)
{
    item_info_copy(list_push_back(&self->shopItems, NULL, sizeof(struct ItemInfo)), item);
}

void session_add_shop_sell_item(struct Session *self, const struct ItemInfo *item)
{
    struct ItemInfo have;

    session_get_inventory_item(self, &have, item->index);
    if ((unsigned)have.index > 0) {
        struct ItemInfo *entry = find_by_index(&self->sellItems, item->index);
        if (entry) {
            int total = entry->count + item->count;
            if (have.count >= total)
                entry->count = total;
        } else if (have.count >= item->count) {
            item_info_copy(list_push_back(&self->sellItems, NULL, sizeof(struct ItemInfo)), item);
        }
    }
    string_free(&have.name);
    string_free(&have.id_text);
}

static int filter_type(int type)
{
    switch (type) {
    case 4: case 5: case 8: case 9:
    case 11: case 12: case 13: case 14: case 15:
        return 1;
    default:
        return 0;
    }
}

void session_remove_inventory_other_types(struct Session *self)
{
    struct ListNode *node = self->itemList.head->next;

    while (node != self->itemList.head) {
        if (filter_type(LIST_VALUE(node, struct ItemInfo)->type))
            node = node->next;
        else
            node = list_erase(&self->itemList, node, item_info_destroy);
    }
    ui_window_mgr_refresh_item_windows(&g_windowMgr);
}

void session_remove_inventory_filter_types(struct Session *self)
{
    struct ListNode *node = self->itemList.head->next;

    while (node != self->itemList.head) {
        if (filter_type(LIST_VALUE(node, struct ItemInfo)->type))
            node = list_erase(&self->itemList, node, item_info_destroy);
        else
            node = node->next;
    }
    ui_window_mgr_refresh_item_windows(&g_windowMgr);
}

void session_clear_inventory(struct Session *self)
{
    list_clear(&self->itemList, item_info_destroy);
    ui_window_mgr_refresh_item_windows(&g_windowMgr);
}

void session_clear_lists(struct Session *self)
{
    list_clear(&self->purchaseItems, item_info_destroy);
    list_clear(&self->sellItems, item_info_destroy);
    list_clear(&self->shopItems, item_info_destroy);
}

void session_clear_purchase_lists(struct Session *self)
{
    list_clear(&self->storePurchaseItems, item_info_destroy);
    list_clear(&self->otherStoreItems, item_info_destroy);
}

void session_clear_shop_buy_list(struct Session *self) { list_clear(&self->purchaseItems, item_info_destroy); }
void session_clear_storage(struct Session *self) { list_clear(&self->storageItems, item_info_destroy); }
void session_clear_cart(struct Session *self) { list_clear(&self->cartItems, item_info_destroy); }
void session_clear_other_store_items(struct Session *self) { list_clear(&self->otherStoreItems, item_info_destroy); }
void session_clear_store_candidates(struct Session *self) { list_clear(&self->storeCandidateItems, item_info_destroy); }
void session_clear_own_store_items(struct Session *self) { list_clear(&self->ownStoreItems, item_info_destroy); }
void session_clear_purchase_list(struct Session *self) { list_clear(&self->storePurchaseItems, item_info_destroy); }
void session_clear_store_setup(struct Session *self) { list_clear(&self->storeSetupItems, item_info_destroy); }

void session_clear_item_list(struct Session *self, int which)
{
    if (which < 0 || (unsigned)which >= 11)
        return;
    list_clear(&self->cashShopGoods[which], item_info_destroy);
}

void session_clear_cash_shop_goods(struct Session *self)
{
    int i;
    for (i = 0; i < 11; i++)
        list_clear(&self->cashShopGoods[i], item_info_destroy);
}

static void copy_list(struct List *dest, struct List *src)
{
    struct ListNode *node;

    list_clear(dest, item_info_destroy);
    for (node = src->head->next; node != src->head; node = node->next)
        item_info_copy(list_push_back(dest, NULL, sizeof(struct ItemInfo)), LIST_VALUE(node, struct ItemInfo));
}

void session_mirror_from_cart(struct Session *self)
{
    copy_list(&self->storeCandidateItems, &self->cartItems);
}

void session_copy_store_purchase_to_sent(struct Session *self)
{
    copy_list(&self->storePurchaseSent, &self->storePurchaseItems);
}

void session_clear_worn_items(struct Session *self)
{
    int i;
    for (i = 0; i < EQUIP_SLOT_COUNT; i++)
        item_info_clear(&self->equipSlots[i]);
    session_refresh_equip_wnd(self);
    ui_window_mgr_invalidate_once_per_frame(&g_windowMgr);
}

void session_clear_viewed_items(struct Session *self)
{
    int i;
    for (i = 0; i < EQUIP_SLOT_COUNT; i++)
        item_info_clear(&self->viewedEquips[i]);
}

void session_inventory_update(struct Session *self, const struct ItemInfo *item,
                                          BYTE notify)
{
    struct ListNode *node = find_node_by_index(&self->itemList, item->index);

    if (!node) {
        item_info_copy(list_push_back(&self->itemList, NULL, sizeof(struct ItemInfo)), item);
    } else if (item->count == 0) {
        list_erase(&self->itemList, node, item_info_destroy);
    } else {
        item_info_assign(LIST_VALUE(node, struct ItemInfo), item);
    }
    if (notify)
        ui_window_mgr_refresh_item_windows(&g_windowMgr);
}

void session_clear_inventory_new_flags(struct Session *self)
{
    struct ListNode *node;

    for (node = self->itemList.head->next; node != self->itemList.head; node = node->next)
        if (LIST_VALUE(node, struct ItemInfo)->is_new == 1)
            LIST_VALUE(node, struct ItemInfo)->is_new = 0;
}

void session_fill_store_candidates(struct Session *self, int count, const short *indexes)
{
    struct ItemInfo item;
    int i;

    session_clear_store_candidates(self);
    for (i = 0; i < count; i++) {
        session_get_inventory_item(&g_session1, &item, indexes[i]);
        if ((unsigned)item.index > 0)
            item_info_copy(list_push_back(&self->storeCandidateItems, NULL, sizeof(struct ItemInfo)), &item);
        string_free(&item.name);
        string_free(&item.id_text);
    }
}
