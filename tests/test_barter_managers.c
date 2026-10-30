#include "web/barter_market.h"
#include "web/barter_expanded.h"
#include "session/state.h"
#include "session/session_items.h"
#include "ui/window_access.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Session g_session1;
static struct Block { unsigned char *raw; size_t size; } blocks[256];
static unsigned allocations, notifications;
static int fail_manager, windows_present = 1;
static struct ItemInfo inventory[4];
static const char owned_name[] = "70000000000000000001";

void *mem_alloc(size_t bytes)
{
    unsigned i;
    unsigned char *raw;
    assert(bytes && bytes < 100000);
    for (i = 0; i < 256 && blocks[i].raw; i++) {}
    assert(i < 256);
    raw = malloc(bytes + 32);
    assert(raw);
    memset(raw, 0xa5, bytes + 32);
    blocks[i].raw = raw;
    blocks[i].size = bytes;
    allocations++;
    return raw + 16;
}
void *mem_try_alloc(size_t bytes)
{
    if (fail_manager) { fail_manager = 0; return NULL; }
    return mem_alloc(bytes);
}
void mem_free(void *memory)
{
    unsigned i, j;
    if (!memory) return;
    for (i = 0; i < 256 && (!blocks[i].raw || blocks[i].raw + 16 != memory); i++) {}
    assert(i < 256 && allocations);
    for (j = 0; j < 16; j++) {
        assert(blocks[i].raw[j] == 0xa5);
        assert(blocks[i].raw[16 + blocks[i].size + j] == 0xa5);
    }
    free(blocks[i].raw);
    blocks[i].raw = NULL;
    allocations--;
}
void fatal_error(const char *message) { (void)message; abort(); }

struct UIWindow *ui_window_find(int id)
{
    assert(id == 0x14e || id == 0x14f);
    return windows_present ? (struct UIWindow *)(uintptr_t)id : NULL;
}
int ui_window_send(struct UIWindow *window, struct UIWindow *sender, int message,
    int a, int b, int c, int d)
{
    assert((uintptr_t)window == 0x14e || (uintptr_t)window == 0x14f);
    assert(!sender && message == 0x17 && !a && !b && !c && !d);
    notifications++;
    return 0;
}
int session_get_inventory_count(struct Session *self)
{
    assert(self == &g_session1);
    return 4;
}
struct ItemInfo *session_get_inventory_item_at(struct Session *self, struct ItemInfo *out, int index)
{
    assert(self == &g_session1 && index >= 0 && index < 4);
    return item_info_copy(out, &inventory[index]);
}

static void check_payment(const struct Tree *map, int id, int count)
{
    struct TreeNode *node;
    for (node = tree_first(map); node != map->head; node = tree_next(node)) {
        const struct BarterPay *pay = TREE_VALUE(node, const struct BarterPay);
        if (pay->id == id) { assert(pay->count == count); return; }
    }
    assert(0);
}

static void destroy_goods(void *value)
{
    barter_goods_destruct(value);
}

static void copy_goods(struct List *out, const struct List *from)
{
    const struct ListNode *node;
    list_init(out);
    for (node = from->head->next; node != from->head; node = node->next) {
        struct BarterGoods copy;
        barter_goods_copy(&copy, LIST_VALUE(node, const struct BarterGoods));
        list_push_back(out, &copy, sizeof copy);
    }
}

static void ordinary(void)
{
    struct BarterGoods goods, unlimited, extra;
    struct BarterFound found;
    struct List snapshot;
    unsigned before;

    assert(!g_barter);
    fail_manager = 1;
    assert(!barter_market_create() && !g_barter && !allocations);
    assert(barter_market_create() == 1 && g_barter);
    before = allocations;
    assert(!barter_market_create() && allocations == before);
    assert(!barter_goods(g_barter)->size && !barter_cart(g_barter)->size);
    copy_goods(&snapshot, barter_cart(g_barter));
    assert(!snapshot.size && snapshot.head->next == snapshot.head && snapshot.head->prev == snapshot.head);
    list_free(&snapshot, destroy_goods);
    barter_goods_construct(&goods);
    string_assign(&goods.id_text, owned_name, sizeof owned_name - 1);
    goods.count = 10; goods.key = 11; goods.weight = 3; goods.pay_id = 501; goods.pay_count = 2;
    barter_goods_push_back(g_barter, &goods);
    goods.count = 3;
    barter_cart_add(g_barter, &goods);
    assert(barter_goods_count(g_barter, 11) == 7 && barter_cart_count(g_barter, 11) == 3);
    assert(barter_goods_weight(g_barter, 11) == 3 && barter_cart_weight(g_barter) == 9.0f);
    check_payment(barter_pay_map(g_barter), 501, 6);
    goods.count = 2;
    barter_cart_add(g_barter, &goods);
    assert(barter_goods_count(g_barter, 11) == 5 && barter_cart_count(g_barter, 11) == 5);
    check_payment(barter_pay_map(g_barter), 501, 10);
    barter_goods_construct(&unlimited);
    string_assign_cstr(&unlimited.id_text, "701");
    unlimited.key = 12; unlimited.count = -1; unlimited.no_stock = 1;
    unlimited.pay_id = 501; unlimited.pay_count = 7;
    barter_goods_push_back(g_barter, &unlimited);
    unlimited.count = 1;
    barter_cart_add(g_barter, &unlimited);
    assert(barter_goods_count(g_barter, 12) == -1 && barter_cart_size(g_barter) == 2);
    check_payment(barter_pay_map(g_barter), 501, 17);
    barter_goods_copy(&extra, &unlimited);
    extra.key = 13; extra.pay_id = 400; extra.pay_count = 3; extra.count = 5; extra.no_stock = 0;
    string_assign_cstr(&extra.id_text, "702");
    barter_goods_push_back(g_barter, &extra);
    extra.count = 1;
    barter_cart_add(g_barter, &extra);
    assert(barter_pay_map(g_barter)->size == 2);
    check_payment(barter_pay_map(g_barter), 400, 3);
    check_payment(barter_pay_map(g_barter), 501, 17);
    assert(TREE_VALUE(tree_first(barter_pay_map(g_barter)), struct BarterPay)->id == 400);
    barter_cart_remove(g_barter, &extra);
    assert(barter_pay_map(g_barter)->size == 1 && barter_goods_count(g_barter, 13) == 5);
    barter_goods_destruct(&extra);

    barter_find_goods(g_barter, &found, 11);
    assert(found.found && found.goods.count == 5);
    assert(!strcmp(string_cstr(&found.goods.id_text), owned_name));
    barter_goods_destruct(&found.goods);
    barter_find_in_cart(g_barter, &found, 999);
    assert(!found.found && found.goods.key == -1 && !found.goods.id_text.size);
    barter_goods_destruct(&found.goods);
    copy_goods(&snapshot, barter_cart(g_barter));
    assert(snapshot.head != barter_cart(g_barter)->head && snapshot.size == 2);
    assert(snapshot.head->next->prev == snapshot.head && snapshot.head->prev->next == snapshot.head);
    assert(LIST_VALUE(snapshot.head->next, struct BarterGoods)->key == 11);
    assert(LIST_VALUE(snapshot.head->prev, struct BarterGoods)->key == 12);
    barter_cart_remove(g_barter, &goods);
    assert(barter_cart_count(g_barter, 11) == 3 && barter_goods_count(g_barter, 11) == 7);
    check_payment(barter_pay_map(g_barter), 501, 13);
    before = notifications;
    barter_cart_remove(g_barter, &unlimited);
    assert(notifications == before + 1 && barter_cart_size(g_barter) == 1);
    assert(barter_goods_count(g_barter, 12) == -1);
    windows_present = 0;
    goods.count = 3;
    barter_cart_remove(g_barter, &goods);
    assert(notifications == before + 1 && !barter_cart_size(g_barter));
    assert(barter_goods_count(g_barter, 11) == 10 && !barter_pay_map(g_barter)->size);
    windows_present = 1;
    barter_goods_destruct(&goods); barter_goods_destruct(&unlimited);
    barter_market_destroy();
    assert(!g_barter && !strcmp(string_cstr(&LIST_VALUE(snapshot.head->next, struct BarterGoods)->id_text), owned_name));
    assert(LIST_VALUE(snapshot.head->next, struct BarterGoods)->count == 5);
    list_clear(&snapshot, destroy_goods);
    assert(!snapshot.size && snapshot.head->next == snapshot.head && snapshot.head->prev == snapshot.head);
    list_free(&snapshot, destroy_goods);
    barter_market_destroy();
    assert(!allocations);
}

static void expanded(void)
{
    struct BarterGoodsEx goods, unlimited;
    struct BarterExFound found;
    struct BarterExPay pay = {501, 7, 2, 4};
    struct Vec snapshot;
    const struct BarterGoodsEx *saved;
    unsigned i, before;

    assert(!g_expandedBarter);
    fail_manager = 1;
    assert(!expanded_barter_create() && !g_expandedBarter && !allocations);
    assert(expanded_barter_create() == 1);
    before = allocations;
    assert(!expanded_barter_create() && allocations == before);
    barter_goods_ex_vec_copy(&snapshot, expanded_barter_cart(g_expandedBarter));
    assert(!snapshot.first && !snapshot.last && !snapshot.end);
    barter_goods_ex_vec_free(&snapshot);
    for (i = 0; i < 4; i++) {
        item_info_construct(&inventory[i]);
        string_assign_cstr(&inventory[i].id_text, "501");
        inventory[i].identified = i != 0;
        inventory[i].refine = i == 1 ? 3 : 7;
        inventory[i].count = i < 2 ? 100 : i == 2 ? 3 : 1;
    }
    barter_goods_ex_construct(&goods);
    string_assign(&goods.id_text, owned_name, sizeof owned_name - 1);
    goods.count = 4; goods.key = 21; goods.weight = 2.5f; goods.price = 2000000000;
    vec_push(&goods.pay, &pay, sizeof pay);
    expanded_barter_goods_push(g_expandedBarter, &goods);
    goods.count = 2;
    expanded_barter_cart_add(g_expandedBarter, &goods);
    assert(expanded_barter_goods_count(g_expandedBarter, 21) == 2);
    assert(expanded_barter_cart_count(g_expandedBarter, 21) == 2);
    assert(expanded_barter_cart_weight(g_expandedBarter) == 5.0f);
    assert(expanded_barter_total(g_expandedBarter) == -294967296LL);
    assert(((struct BarterExPay *)expanded_barter_pay_items(g_expandedBarter)->first)->count == 4);
    assert(expanded_barter_can_pay(g_expandedBarter) == 1);
    inventory[3].count = 0;
    assert(!expanded_barter_can_pay(g_expandedBarter));
    inventory[3].count = 1;
    assert(expanded_barter_can_pay(g_expandedBarter) == 1);
    barter_goods_ex_construct(&unlimited);
    unlimited.key = 22; unlimited.count = -1; unlimited.price = 7;
    string_assign_cstr(&unlimited.id_text, "701");
    expanded_barter_goods_push(g_expandedBarter, &unlimited);
    unlimited.count = 1; unlimited.no_stock = 1;
    expanded_barter_cart_add(g_expandedBarter, &unlimited);
    assert(expanded_barter_goods_count(g_expandedBarter, 22) == -1);
    assert(expanded_barter_total(g_expandedBarter) == -294967289LL);
    barter_goods_ex_vec_copy(&snapshot, expanded_barter_cart(g_expandedBarter));
    assert(snapshot.first != expanded_barter_cart(g_expandedBarter)->first);
    assert(vec_count(&snapshot, sizeof goods) == 2);
    saved = snapshot.first;
    assert(saved[0].key == 21 && saved[1].key == 22);
    expanded_barter_find_in_cart(g_expandedBarter, &found, 999);
    assert(!found.found && !found.goods.id_text.size && !found.goods.pay.first);
    barter_goods_ex_destruct(&found.goods);
    goods.count = 1;
    expanded_barter_cart_remove(g_expandedBarter, &goods);
    assert(expanded_barter_total(g_expandedBarter) == 2000000007LL);
    assert(((struct BarterExPay *)expanded_barter_pay_items(g_expandedBarter)->first)->count == 2);
    expanded_barter_cart_remove(g_expandedBarter, &goods);
    assert(expanded_barter_total(g_expandedBarter) == 7);
    assert(expanded_barter_goods_count(g_expandedBarter, 21) == 4);
    assert(!vec_count(expanded_barter_pay_items(g_expandedBarter), sizeof pay));
    expanded_barter_clear(g_expandedBarter);
    assert(!expanded_barter_cart_size(g_expandedBarter) && !expanded_barter_total(g_expandedBarter));
    barter_goods_ex_destruct(&goods); barter_goods_ex_destruct(&unlimited);
    expanded_barter_destroy();
    assert(!g_expandedBarter && !strcmp(string_cstr(&saved[0].id_text), owned_name));
    assert(((struct BarterExPay *)saved[0].pay.first)->id == 501 && saved[0].count == 2);
    barter_goods_ex_vec_free(&snapshot);
    assert(!snapshot.first && !snapshot.last && !snapshot.end);
    barter_goods_ex_vec_free(&snapshot);
    expanded_barter_destroy();
    for (i = 0; i < 4; i++) item_info_destruct(&inventory[i]);
    assert(!allocations);
}

int main(void)
{
    ordinary();
    expanded();
    puts("barter managers: guarded lifetime, stock/cart/payment updates, snapshots, notifications and inventory checks passed");
    return 0;
}
