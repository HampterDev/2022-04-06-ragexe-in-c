#include "ui/window_manager_state.h"
#include "session/session_items_priv.h"
#include "session/session_items.h"

#include <stdlib.h>
#include <string.h>
#include "ui/ui_std.h"
#include "ui/wndmgr/wndmgr.h"
#include <stdint.h>

static struct ItemInfo *copy_by_index(struct ItemInfo *out, struct List *list, int index)
{
    struct ItemInfo *have = find_by_index(list, index);
    if (have)
        return item_info_copy(out, have);
    return item_info_construct_with_arg(out);
}

static struct ItemInfo *copy_nth_unchecked(struct ItemInfo *out, struct List *list, int n)
{
    struct ListNode *node;
    int i = 0;

    for (node = list->head->next; node != list->head; node = node->next, i++)
        if (i == n)
            return item_info_copy(out, LIST_VALUE(node, struct ItemInfo));
    return item_info_construct_with_arg(out);
}

static int count_by_index(struct List *list, int index)
{
    struct ItemInfo *have = find_by_index(list, index);
    return have ? have->count : 0;
}

static struct ItemInfo *find_by_name_text(struct List *list, const char *name)
{
    struct ListNode *node;

    for (node = list->head->next; node != list->head; node = node->next) {
        struct ItemInfo *item = LIST_VALUE(node, struct ItemInfo);
        if (string_equal_cstr(&item->id_text, name))
            return item;
    }
    return NULL;
}

struct ItemInfo *session_get_store_setup_item_by_index(struct Session *self,
                                                        struct ItemInfo *out, int index)
{
    copy_by_index(out, &self->storeSetupItems, index);
    return out;
}

struct ItemInfo *session_get_store_setup_item_at(struct Session *self,
                                                   struct ItemInfo *out, int n)
{
    copy_nth(out, &self->storeSetupItems, n);
    return out;
}

int session_get_store_setup_count(struct Session *self, int index)
{
    return count_by_index(&self->storeSetupItems, index);
}

struct ItemInfo *session_cart_item_by_index(struct Session *self,
                                                        struct ItemInfo *out, int index)
{
    copy_by_index(out, &self->cartItems, index);
    return out;
}

struct ItemInfo *session_get_cart_item_at(struct Session *self,
                                                   struct ItemInfo *out, int n)
{
    copy_nth(out, &self->cartItems, n);
    return out;
}

int session_get_cart_item_count(struct Session *self, int index)
{
    return count_by_index(&self->cartItems, index);
}

struct ItemInfo *session_mirror_item_by_index(struct Session *self,
                                                        struct ItemInfo *out, int index)
{
    copy_by_index(out, &self->storeCandidateItems, index);
    return out;
}

struct ItemInfo *session_mirror_item_at(struct Session *self,
                                                   struct ItemInfo *out, int n)
{
    copy_nth(out, &self->storeCandidateItems, n);
    return out;
}

int session_get_store_candidate_count(struct Session *self, int index)
{
    return count_by_index(&self->storeCandidateItems, index);
}

struct ItemInfo *session_get_shop_item_by_index(struct Session *self,
                                                        struct ItemInfo *out, int index)
{
    struct ItemInfo *have = find_by_index(&self->ownStoreItems, index);
    if (have)
        item_info_copy(out, have);
    else
        item_info_construct(out);
    return out;
}

struct ItemInfo *session_get_shop_item_by_id(struct Session *self,
                                                     struct ItemInfo *out, int itemId)
{
    struct ListNode *node;

    for (node = self->ownStoreItems.head->next; node != self->ownStoreItems.head; node = node->next)
        if (item_id(LIST_VALUE(node, struct ItemInfo)) == itemId)
            return item_info_copy(out, LIST_VALUE(node, struct ItemInfo));
    return item_info_construct(out);
}

struct ItemInfo *session_shop_item_at(struct Session *self,
                                                   struct ItemInfo *out, int n)
{
    copy_nth(out, &self->ownStoreItems, n);
    return out;
}

struct ItemInfo *session_get_store_purchase_item_by_index(struct Session *self,
                                                        struct ItemInfo *out, int index)
{
    copy_by_index(out, &self->storePurchaseItems, index);
    return out;
}

struct ItemInfo *session_purchase_item_by_id(struct Session *self,
                                                       struct ItemInfo *out, const char *name)
{
    struct ItemInfo *have = find_by_name_text(&self->storePurchaseItems, name);
    if (have)
        return item_info_copy(out, have);
    return item_info_construct_with_arg(out);
}

struct ItemInfo *session_purchase_item(struct Session *self,
                                                   struct ItemInfo *out, int n)
{
    copy_nth_unchecked(out, &self->storePurchaseItems, n);
    return out;
}

int session_get_store_purchase_count_by_name(struct Session *self, const char *name)
{
    struct ItemInfo *have = find_by_name_text(&self->storePurchaseItems, name);
    return have ? have->count : 0;
}

int session_get_store_purchase_count(struct Session *self, int index)
{
    return count_by_index(&self->storePurchaseItems, index);
}

int64_t session_purchase_total(struct Session *self)
{
    struct ListNode *node;
    int64_t total = 0;

    for (node = self->storePurchaseItems.head->next; node != self->storePurchaseItems.head; node = node->next) {
        const struct ItemInfo *item = LIST_VALUE(node, struct ItemInfo);
        total += (int64_t)item->charged_price * item->count;
    }
    return total;
}

struct ItemInfo *session_get_sent_store_purchase_item(struct Session *self,
                                                        struct ItemInfo *out, int index)
{
    copy_by_index(out, &self->storePurchaseSent, index);
    return out;
}

struct ItemInfo *session_shop_list_item(struct Session *self,
                                                   struct ItemInfo *out, int n)
{
    copy_nth(out, &self->otherStoreItems, n);
    return out;
}

struct ItemInfo *session_find_purchase_item(struct Session *self,
                                                       struct ItemInfo *out, const char *name)
{
    struct ItemInfo *have = find_by_name_text(&self->purchaseItems, name);
    if (have)
        return item_info_copy(out, have);
    return item_info_construct_with_arg(out);
}

struct ItemInfo *session_get_shop_buy_item_at(struct Session *self,
                                                   struct ItemInfo *out, int n)
{
    copy_nth_unchecked(out, &self->purchaseItems, n);
    return out;
}

int session_get_shop_buy_count_by_name(struct Session *self, const char *name)
{
    struct ItemInfo *have = find_by_name_text(&self->purchaseItems, name);
    return have ? have->count : 0;
}

int session_get_shop_buy_count(struct Session *self, int index)
{
    return count_by_index(&self->purchaseItems, index);
}

int session_get_shop_buy_total(struct Session *self)
{
    struct ListNode *node;
    int total = 0;

    for (node = self->purchaseItems.head->next; node != self->purchaseItems.head; node = node->next) {
        const struct ItemInfo *item = LIST_VALUE(node, struct ItemInfo);
        total += item->charged_price * item->count;
    }
    return total;
}

struct ItemInfo *session_get_shop_sell_item_by_index(struct Session *self,
                                                        struct ItemInfo *out, int index)
{
    copy_by_index(out, &self->sellItems, index);
    return out;
}

struct ItemInfo *session_get_shop_sell_item_at(struct Session *self,
                                                   struct ItemInfo *out, int n)
{
    copy_nth(out, &self->sellItems, n);
    return out;
}

int session_get_shop_sell_count(struct Session *self, int index)
{
    return count_by_index(&self->sellItems, index);
}

static struct ItemInfo *find_shop_goods_by_id(struct Session *self, const struct ItemInfo *item)
{
    struct ListNode *node;

    for (node = self->shopItems.head->next; node != self->shopItems.head; node = node->next)
        if (item_id(LIST_VALUE(node, struct ItemInfo)) == item_id(item))
            return LIST_VALUE(node, struct ItemInfo);
    return NULL;
}

int session_get_shop_goods_count_by_id(struct Session *self,
                                             const struct ItemInfo *item)
{
    struct ItemInfo *have = find_shop_goods_by_id(self, item);
    return have ? have->count : 0;
}

int session_get_shop_goods_count_by_index(struct Session *self,
                                                const struct ItemInfo *item)
{
    return count_by_index(&self->shopItems, item->index);
}

WORD session_get_shop_goods_weight_by_id(struct Session *self,
                                              const struct ItemInfo *item)
{
    struct ItemInfo *have = find_shop_goods_by_id(self, item);
    return have ? have->weight : 0;
}

WORD session_get_shop_goods_weight_by_index(struct Session *self,
                                                 const struct ItemInfo *item)
{
    struct ItemInfo *have = find_by_index(&self->shopItems, item->index);
    return have ? have->weight : 0;
}

WORD session_shop_buy_weight_by_id(struct Session *self)
{
    struct ListNode *node;
    WORD sum = 0;

    for (node = self->purchaseItems.head->next; node != self->purchaseItems.head; node = node->next) {
        const struct ItemInfo *item = LIST_VALUE(node, struct ItemInfo);
        struct ItemInfo *have = find_shop_goods_by_id(self, item);
        WORD factor = have ? have->weight : 0;
        sum = (WORD)(sum + (WORD)item->count * factor);
    }
    return sum;
}

WORD session_shop_buy_weight_by_index(struct Session *self)
{
    struct ListNode *node;
    WORD sum = 0;

    for (node = self->purchaseItems.head->next; node != self->purchaseItems.head; node = node->next) {
        const struct ItemInfo *item = LIST_VALUE(node, struct ItemInfo);
        struct ItemInfo *have = find_by_index(&self->shopItems, item->index);
        WORD factor = have ? have->weight : 0;
        sum = (WORD)(sum + (WORD)item->count * factor);
    }
    return sum;
}

int session_get_friend_count(struct Session *self) { return (int)self->friendList.size; }
int session_get_inventory_count(struct Session *self) { return (int)self->itemList.size; }
int session_get_store_setup_size(struct Session *self) { return (int)self->storeSetupItems.size; }
int session_get_cart_size(struct Session *self) { return (int)self->cartItems.size; }
int session_mirror_count(struct Session *self) { return (int)self->storeCandidateItems.size; }
int session_shop_item_count(struct Session *self) { return (int)self->ownStoreItems.size; }
int session_purchase_count(struct Session *self) { return (int)self->storePurchaseItems.size; }
int session_shop_list_count(struct Session *self) { return (int)self->otherStoreItems.size; }
int session_get_party_member_count(struct Session *self) { return (int)self->partyMembers.size; }
int session_get_shop_buy_list_size(struct Session *self) { return (int)self->purchaseItems.size; }
int session_get_shop_sell_list_size(struct Session *self) { return (int)self->sellItems.size; }
int session_sellable_count(struct Session *self) { return (int)self->shopItems.size; }
int session_storage_count(struct Session *self) { return (int)self->storageItems.size; }

struct ItemInfo *session_get_partner_trade_item(struct Session *self,
                                                      struct ItemInfo *out, int i)
{
    item_info_copy(out, &self->partnerTradeItems[i]);
    return out;
}

struct ItemInfo *session_get_viewed_equip(struct Session *self,
                                                      struct ItemInfo *out, unsigned slot)
{
    if (slot == 15)
        item_info_construct_with_arg(out);
    else if (slot <= 9)
        item_info_copy(out, &self->viewedEquips[slot]);
    else
        item_info_construct_with_arg(out);
    return out;
}

struct ItemInfo *session_sellable_at(struct Session *self,
                                                   struct ItemInfo *out, int n)
{
    copy_nth(out, &self->shopItems, n);
    return out;
}

int session_get_shop_goods_count(struct Session *self, int index)
{
    return count_by_index(&self->shopItems, index);
}

struct ItemInfo *session_get_storage_item(struct Session *self,
                                                        struct ItemInfo *out, int index)
{
    copy_by_index(out, &self->storageItems, index);
    return out;
}

struct ItemInfo *session_storage_item_at(struct Session *self,
                                                   struct ItemInfo *out, int n)
{
    copy_nth(out, &self->storageItems, n);
    return out;
}

int session_get_storage_item_count(struct Session *self, int index)
{
    return count_by_index(&self->storageItems, index);
}

BYTE session_has_shop_goods(struct Session *self, int index)
{
    return find_node_by_index(&self->shopItems, index) != NULL;
}

BYTE session_has_cart_item_id(struct Session *self, int itemId)
{
    struct ListNode *node;

    for (node = self->cartItems.head->next; node != self->cartItems.head; node = node->next)
        if (itemId == item_id(LIST_VALUE(node, struct ItemInfo)))
            break;
    return node != self->cartItems.head;
}

int session_has_shop_sell_items(struct Session *self)
{
    struct ListNode *node;

    for (node = self->sellItems.head->next; node != self->sellItems.head; node = node->next) {
        const struct ItemInfo *want = LIST_VALUE(node, struct ItemInfo);
        struct ItemInfo have;
        int ok;

        session_get_inventory_item(self, &have, want->index);
        ok = (unsigned)have.index > 0 && have.count >= want->count;
        string_free(&have.name);
        string_free(&have.id_text);
        if (!ok)
            return 0;
    }
    return 1;
}

BYTE session_is_worn_index(struct Session *self, int index)
{
    int i;

    if (index != 0 && index == self->wornArrowIndex)
        return 1;
    for (i = 0; i < EQUIP_SLOT_COUNT; i++)
        if (self->equipSlots[i].count == 1 && self->equipSlots[i].index == index)
            return 1;
    return 0;
}

BYTE session_has_inventory_index(struct Session *self, int index)
{
    return find_node_by_index(&self->itemList, index) != NULL;
}

BYTE session_is_visible_npc(int job)
{
    if (job == 0x2d || job == 0x68 || job == 0x6f)
        return 0;
    return 1;
}

BYTE session_is_wearing_item_id(struct Session *self, int itemId)
{
    struct ItemInfo arrow;
    BYTE found;
    int i;

    session_get_inventory_item(self, &arrow, self->wornArrowIndex);
    found = (unsigned)arrow.index > 0 && item_id(&arrow) == itemId;
    for (i = 0; i < 10 && !found; i++)
        found = item_id(&self->equipSlots[i]) == itemId || item_id(&self->costumeSlots[i]) == itemId;
    string_free(&arrow.name);
    string_free(&arrow.id_text);
    return found;
}

BYTE session_is_wearing_item_id_at(struct Session *self, int itemId, int min)
{
    struct ItemInfo arrow;
    BYTE found;
    int i;

    session_get_inventory_item(self, &arrow, self->wornArrowIndex);
    found = (unsigned)arrow.index > 0 && item_id(&arrow) == itemId;
    for (i = 0; i < 10 && !found; i++)
        found = (item_id(&self->equipSlots[i]) == itemId && self->equipSlots[i].refine >= min)
            || (item_id(&self->costumeSlots[i]) == itemId && self->costumeSlots[i].refine >= min);
    string_free(&arrow.name);
    string_free(&arrow.id_text);
    return found;
}

static void set_move_pending(struct List *list, int index)
{
    struct ItemInfo *item = find_by_index(list, index);
    if (item) {
        item->move_pending = 1;
        ui_window_mgr_refresh_item_windows(&g_windowMgr);
    }
}

void session_set_inventory_move_pending(struct Session *self, int index)
{
    set_move_pending(&self->itemList, index);
}

void session_set_cart_move_pending(struct Session *self, int index)
{
    set_move_pending(&self->cartItems, index);
}

void session_set_storage_move_pending(struct Session *self, int index)
{
    set_move_pending(&self->storageItems, index);
}

static void clear_move_pending(struct List *list, int index)
{
    struct ItemInfo *item = find_by_index(list, index);
    if (item)
        item->move_pending = 0;
}

void session_clear_inventory_move_pending(struct Session *self, int index)
{
    clear_move_pending(&self->itemList, index);
}

void session_clear_cart_move_pending(struct Session *self, int index)
{
    clear_move_pending(&self->cartItems, index);
}

void session_clear_storage_move_pending(struct Session *self, int index)
{
    clear_move_pending(&self->storageItems, index);
}

void session_fill_buying_store_list(struct Session *self)
{
    int count, i;

    list_clear(&self->storeCandidateItems, item_info_destroy);
    count = (int)self->itemList.size;
    for (i = 0; i < count; i++) {
        struct ItemInfo item;

        session_get_inventory_item_at(self, &item, i);
        if ((unsigned)item.index > 0) {
            int id = item_id(&item);
            const int *p;
            for (p = (const int *)self->buyingStoreItems.first; p != (const int *)self->buyingStoreItems.last; p++)
                if (*p == id) {
                    item_info_copy(list_push_back(&self->storeCandidateItems, NULL, sizeof(struct ItemInfo)), &item);
                    break;
                }
        }
        string_free(&item.name);
        string_free(&item.id_text);
    }
}

void session_fill_buying_store_list_priced(struct Session *self)
{
    struct ListNode *node;

    session_fill_buying_store_list(self);
    node = self->storeCandidateItems.head->next;
    while (node != self->storeCandidateItems.head) {
        struct ItemInfo *item = LIST_VALUE(node, struct ItemInfo);
        int id = item_id(item);
        struct ListNode *other;
        int value;

        for (other = self->otherStoreItems.head->next; other != self->otherStoreItems.head; other = other->next)
            if (item_id(LIST_VALUE(other, struct ItemInfo)) == id)
                break;
        if (other == self->otherStoreItems.head) {
            node = list_erase(&self->storeCandidateItems, node, item_info_destroy);
            continue;
        }
        value = LIST_VALUE(other, struct ItemInfo)->price;
        if (value == -1) {
            node = list_erase(&self->storeCandidateItems, node, item_info_destroy);
            continue;
        }
        item->price = value;
        item->charged_price = value;
        node = node->next;
    }
}

void session_set_store_setup_count_at(struct Session *self, int n, int count)
{
    struct ListNode *node;
    int i = 0;

    for (node = self->storeSetupItems.head->next; node != self->storeSetupItems.head; node = node->next, i++)
        if (i == n) {
            LIST_VALUE(node, struct ItemInfo)->count = count;
            return;
        }
}

void session_set_store_setup_price_at(struct Session *self, int n, int price)
{
    struct ListNode *node;
    int i = 0;

    for (node = self->storeSetupItems.head->next; node != self->storeSetupItems.head; node = node->next, i++)
        if (i == n) {
            LIST_VALUE(node, struct ItemInfo)->price = price;
            LIST_VALUE(node, struct ItemInfo)->charged_price = price;
            return;
        }
}
