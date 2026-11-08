#include "ui/window_manager_state.h"
#include "session/session_items_priv.h"
#include "session/session_items.h"

#include <stdlib.h>
#include "ui/wndmgr/wndmgr.h"

static void take_count(struct List *list, struct ListNode *node, int count)
{
    struct ItemInfo *have = LIST_VALUE(node, struct ItemInfo);
    have->count -= count;
    if (have->count <= 0)
        list_erase(list, node, item_info_destroy);
}

static struct ListNode *find_node_by_item_id(struct List *list, const struct ItemInfo *item)
{
    struct ListNode *node;
    for (node = list->head->next; node != list->head; node = node->next)
        if (atoi(string_cstr(&LIST_VALUE(node, struct ItemInfo)->id_text))
            == atoi(string_cstr(&item->id_text)))
            return node;
    return NULL;
}

static void clear_move_pending(struct List *list, int index)
{
    struct ItemInfo *have = find_by_index(list, index);
    if (have)
        have->move_pending = 0;
}

static void notify(struct UIWindow *const *slot, int a)
{
    struct UIWindow *wnd = *slot;
    if (wnd)
        ui_window_send(wnd, NULL, UIMSG_REFRESH, a, 0, 0, 0);
}

void session_remove_own_store_item_by_id(struct Session *self, struct ItemInfo *item)
{
    struct ListNode *node = find_node_by_item_id(&self->ownStoreItems, item);
    struct ItemInfo *have;

    if (!node)
        return;
    have = LIST_VALUE(node, struct ItemInfo);
    string_assign(&item->id_text, string_cstr(&have->id_text), have->id_text.size);
    take_count(&self->ownStoreItems, node, item->count);
}

void session_remove_shop_goods_by_id(struct Session *self, const struct ItemInfo *item)
{
    struct ListNode *node = find_node_by_item_id(&self->shopItems, item);
    struct ItemInfo *have;

    if (!node)
        return;
    have = LIST_VALUE(node, struct ItemInfo);
    if (have->count == -1)
        return;
    have->count -= item->count;
    if (have->count <= 0)
        list_erase(&self->shopItems, node, item_info_destroy);
}

void session_remove_inventory_item(struct Session *self, int index, int count)
{
    struct ListNode *node = find_node_by_index(&self->itemList, index);

    if (node) {
        struct ItemInfo *have = LIST_VALUE(node, struct ItemInfo);
        if (have->count <= count)
            list_erase(&self->itemList, node, item_info_destroy);
        else
            have->count -= count;
        clear_move_pending(&self->itemList, index);
    }
    ui_window_mgr_refresh_item_windows(&g_windowMgr);
}

BYTE session_remove_store_setup_item(struct Session *self,
                                        const struct ItemInfo *item, BYTE whole)
{
    struct List *list = &self->storeSetupItems;
    struct ListNode *node;
    int seq = 0;

    for (node = list->head->next; node != list->head; node = node->next, seq++)
        if (LIST_VALUE(node, struct ItemInfo)->index == item->index)
            break;
    if (node == list->head) {
        notify(whole ? &g_windowMgr.buying_store_make_wnd : &g_windowMgr.merchant_shop_make_wnd, 0);
        return 0;
    }
    if (whole) {
        list_erase(list, node, item_info_destroy);
        notify(&g_windowMgr.buying_store_make_wnd, 0);
        return 1;
    }
    LIST_VALUE(node, struct ItemInfo)->count -= item->count;
    if (LIST_VALUE(node, struct ItemInfo)->count <= 0) {
        list_erase(list, node, item_info_destroy);
        notify(&g_windowMgr.merchant_shop_make_wnd, seq + 1);
        return 1;
    }
    notify(&g_windowMgr.merchant_shop_make_wnd, 0);
    return 1;
}

static BYTE remove_and_notify(struct List *list, const struct ItemInfo *item, struct UIWindow *const *slot)
{
    struct ListNode *node = find_node_by_index(list, item->index);

    if (!node) {
        notify_item_window(slot);
        return 0;
    }
    take_count(list, node, item->count);
    clear_move_pending(list, item->index);
    notify_item_window(slot);
    return 1;
}

BYTE session_remove_cart_item(struct Session *self, const struct ItemInfo *item)
{
    return remove_and_notify(&self->cartItems, item, &g_windowMgr.merchant_item_wnd);
}

BYTE session_remove_storage_item(struct Session *self, const struct ItemInfo *item)
{
    return remove_and_notify(&self->storageItems, item, &g_windowMgr.item_store_wnd);
}

BYTE session_mirror_remove(struct Session *self,
                                        const struct ItemInfo *item, int which, BYTE notifyWnd)
{
    struct ListNode *node = find_node_by_index(&self->storeCandidateItems, item->index);

    if (!node) {
        if (notifyWnd == 1)
            notify_item_window(store_candidate_window_slot(which));
        return 0;
    }
    if (which != 0)
        list_erase(&self->storeCandidateItems, node, item_info_destroy);
    else
        take_count(&self->storeCandidateItems, node, item->count);
    if (notifyWnd == 1)
        notify_item_window(store_candidate_window_slot(which));
    return 1;
}

void session_remove_own_store_item(struct Session *self, struct ItemInfo *item)
{
    struct ListNode *node = find_node_by_index(&self->ownStoreItems, item->index);
    struct ItemInfo *have;

    if (!node)
        return;
    have = LIST_VALUE(node, struct ItemInfo);
    string_assign(&item->id_text, string_cstr(&have->id_text), have->id_text.size);
    take_count(&self->ownStoreItems, node, item->count);
}

static void remove_by_index(struct List *list, const struct ItemInfo *item)
{
    struct ListNode *node = find_node_by_index(list, item->index);
    if (node)
        take_count(list, node, item->count);
}

void session_remove_store_purchase_item(struct Session *self, const struct ItemInfo *item)
{
    remove_by_index(&self->storePurchaseItems, item);
}

void session_sell_list_remove(struct Session *self, const struct ItemInfo *item)
{
    remove_by_index(&self->sellItems, item);
}

void session_remove_shop_goods(struct Session *self, const struct ItemInfo *item)
{
    remove_by_index(&self->shopItems, item);
}

void session_remove_counted_shop_goods(struct Session *self, const struct ItemInfo *item)
{
    struct ListNode *node = find_node_by_index(&self->shopItems, item->index);
    struct ItemInfo *have;

    if (!node)
        return;
    have = LIST_VALUE(node, struct ItemInfo);
    if (have->count == -1)
        return;
    have->count -= item->count;
    if (have->count <= 0)
        list_erase(&self->shopItems, node, item_info_destroy);
}

void session_purchase_list_remove(struct Session *self, const struct ItemInfo *item)
{
    struct ListNode *node = find_node_by_name(&self->purchaseItems, item);
    if (node)
        take_count(&self->purchaseItems, node, item->count);
}

void session_remove_shop_buy_item_keyed(struct Session *self, const struct ItemInfo *item)
{
    struct List *list = &self->purchaseItems;
    struct ListNode *node;

    for (node = list->head->next; node != list->head; node = node->next) {
        struct ItemInfo *have = LIST_VALUE(node, struct ItemInfo);
        int match = item->by_index ? have->index == item->index
                                       : string_equal(&have->id_text, &item->id_text);
        if (match)
            break;
    }
    if (node == list->head)
        return;
    LIST_VALUE(node, struct ItemInfo)->count -= item->count;
    if (LIST_VALUE(node, struct ItemInfo)->count <= 0)
        list_erase(list, node, item_info_destroy);
}

struct InventoryMatch *session_find_inventory_item(struct Session *self,
                                                   struct InventoryMatch *out, const struct ItemInfo *item)
{
    struct List *list = &self->storeCandidateItems;
    struct ListNode *node;

    for (node = list->head->next; node != list->head; node = node->next) {
        const struct ItemInfo *have = LIST_VALUE(node, struct ItemInfo);
        int count, i;

        if (!string_equal(&have->id_text, &item->id_text))
            continue;
        if (have->slot[0] != item->slot[0] || have->slot[1] != item->slot[1]
            || have->slot[2] != item->slot[2] || have->slot[3] != item->slot[3])
            continue;
        if (have->identified != item->identified || have->refine != item->refine)
            continue;
        count = have->option_count;
        if (count != item->option_count || count >= 5)
            continue;
        if (count == 0)
            break;
        for (i = 0; i < count; i++) {
            const struct ItemOption *a = &have->options[i], *b = &item->options[i];
            if (a->id == b->id && a->value == b->value && a->param == b->param)
                break;
        }
        if (i < count)
            break;
    }
    if (node != list->head) {
        out->found = 1;
        item_info_copy(&out->item, LIST_VALUE(node, struct ItemInfo));
    } else {
        struct ItemInfo empty;
        item_info_construct(&empty);
        out->found = 0;
        item_info_copy(&out->item, &empty);
        string_free(&empty.name);
        string_free(&empty.id_text);
    }
    return out;
}
