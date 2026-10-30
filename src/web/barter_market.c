#include "std/string.h"
#include "ui/window_manager_state.h"
#include "core/memory.h"
#include "ui/window_access.h"
#include <string.h>
#include "web/barter_market.h"
#include "std/tree.h"

struct BarterMarketMgr *g_barter;

#define BARTER_GOODS_WND_ID 0x14e
#define BARTER_CART_WND_ID  0x14f
#define BARTER_MSG_REFRESH  0x17

static void tell_window(int id)
{
    struct UIWindow *w = ui_window_find(id);

    if (w)
        ui_window_send(w, NULL, BARTER_MSG_REFRESH, 0, 0, 0, 0);
}

static struct ListNode *find_key(struct List *list, int key)
{
    struct ListNode *node;

    for (node = list->head->next; node != list->head; node = node->next)
        if (LIST_VALUE(node, struct BarterGoods)->key == key)
            break;
    return node;
}

static struct ListNode *find_key_of_copy(struct List *list, const struct BarterGoods *goods)
{
    struct BarterGoods copy;
    struct ListNode *node;

    barter_goods_copy(&copy, goods);
    node = find_key(list, copy.key);
    barter_goods_destruct(&copy);
    return node;
}

static void destroy_goods(void *value)
{
    barter_goods_destruct(value);
}

static void mgr_init(struct BarterMarketMgr *self)
{
    list_init(&self->goods);
    list_init(&self->cart);
    tree_init(&self->pay);
}

static void barter_market_destruct(struct BarterMarketMgr *self)
{
    tree_free(&self->pay, NULL);
    list_free(&self->cart, destroy_goods);
    list_free(&self->goods, destroy_goods);
}

char barter_market_create(void)
{
    struct BarterMarketMgr *mgr;

    if (g_barter)
        return 0;
    mgr = mem_try_alloc(sizeof *mgr);
    if (mgr)
        mgr_init(mgr);
    g_barter = mgr;
    if (!mgr)
        return 0;
    return 1;
}

void barter_market_destroy(void)
{
    if (g_barter) {
        barter_market_destruct(g_barter);
        mem_free(g_barter);
        g_barter = NULL;
    }
}

struct List *barter_goods(struct BarterMarketMgr *self)
{
    return &self->goods;
}

struct List *barter_cart(struct BarterMarketMgr *self)
{
    return &self->cart;
}

struct Tree *barter_pay_map(struct BarterMarketMgr *self)
{
    return &self->pay;
}

unsigned barter_cart_size(struct BarterMarketMgr *self)
{
    return self->cart.size;
}

static struct BarterFound *find_in(struct List *list, struct BarterFound *out, int key)
{
    struct ListNode *node = find_key(list, key);
    struct BarterGoods empty;

    if (node != list->head) {
        out->found = 1;
        barter_goods_copy(&out->goods, LIST_VALUE(node, struct BarterGoods));
    } else {
        barter_goods_construct(&empty);
        out->found = 0;
        barter_goods_copy(&out->goods, &empty);
        barter_goods_destruct(&empty);
    }
    return out;
}

struct BarterFound *barter_find_goods(struct BarterMarketMgr *self, struct BarterFound *out, int key)
{
    return find_in(&self->goods, out, key);
}

struct BarterFound *barter_find_in_cart(struct BarterMarketMgr *self, struct BarterFound *out, int key)
{
    return find_in(&self->cart, out, key);
}

int barter_goods_count(struct BarterMarketMgr *self, int key)
{
    struct ListNode *node = find_key(&self->goods, key);

    return node != self->goods.head ? LIST_VALUE(node, struct BarterGoods)->count : 0;
}

int barter_cart_count(struct BarterMarketMgr *self, int key)
{
    struct ListNode *node = find_key(&self->cart, key);

    return node != self->cart.head ? LIST_VALUE(node, struct BarterGoods)->count : 0;
}

int barter_goods_weight(struct BarterMarketMgr *self, int key)
{
    struct ListNode *node = find_key(&self->goods, key);

    return node != self->goods.head ? LIST_VALUE(node, struct BarterGoods)->weight : -1;
}

float barter_cart_weight(struct BarterMarketMgr *self)
{
    struct ListNode *node;
    float total = 0.0f;

    for (node = self->cart.head->next; node != self->cart.head; node = node->next) {
        const struct BarterGoods *goods = LIST_VALUE(node, struct BarterGoods);

        total += (float)(goods->weight * goods->count);
    }
    return total;
}

char barter_cart_has_id(struct BarterMarketMgr *self, struct String id_text)
{
    struct String copy;
    struct ListNode *node;
    char found;

    string_copy(&copy, &id_text);
    for (node = self->cart.head->next; node != self->cart.head; node = node->next)
        if (string_equal(&LIST_VALUE(node, struct BarterGoods)->id_text, &copy))
            break;
    string_free(&copy);
    found = node != self->cart.head;
    string_free(&id_text);
    return found;
}

int barter_cart_count_of(struct BarterMarketMgr *self, struct String id_text)
{
    struct ListNode *node;
    const struct String *name;
    int total = 0;

    for (node = self->cart.head->next; node != self->cart.head; node = node->next) {
        name = &LIST_VALUE(node, struct BarterGoods)->id_text;
        if (name->size == id_text.size && memcmp(string_cstr(name), string_cstr(&id_text), name->size) == 0)
            total += LIST_VALUE(node, struct BarterGoods)->count;
    }
    string_free(&id_text);
    return total;
}

void barter_rebuild_pay_map(struct BarterMarketMgr *self)
{
    struct ListNode *node;

    tree_clear(&self->pay, NULL);
    for (node = self->cart.head->next; node != self->cart.head; node = node->next) {
        const struct BarterGoods *goods = LIST_VALUE(node, struct BarterGoods);
        struct BarterPay *pay = tree_get(&self->pay, &goods->pay_id, sizeof goods->pay_id, sizeof *pay,
                                         tree_less_int);

        pay->count += goods->pay_count * goods->count;
    }
}

void barter_cart_add(struct BarterMarketMgr *self, const struct BarterGoods *goods)
{
    struct ListNode *node = find_key_of_copy(&self->cart, goods);

    if (node != self->cart.head) {
        LIST_VALUE(node, struct BarterGoods)->count += goods->count;
    } else {
        struct BarterGoods copy;

        barter_goods_copy(&copy, goods);
        list_push_back(&self->cart, &copy, sizeof copy);
    }

    node = find_key(&self->goods, goods->key);
    if (node != self->goods.head) {
        struct BarterGoods *stock = LIST_VALUE(node, struct BarterGoods);

        if (!stock->no_stock) {
            stock->count -= goods->count;
            if (stock->count <= 0) {
                list_erase(&self->goods, node, destroy_goods);
                tell_window(BARTER_GOODS_WND_ID);
            }
        }
    }
    barter_rebuild_pay_map(self);
}

void barter_cart_remove(struct BarterMarketMgr *self, const struct BarterGoods *goods)
{
    struct ListNode *node = find_key(&self->cart, goods->key);

    if (node != self->cart.head) {
        struct BarterGoods *in_cart = LIST_VALUE(node, struct BarterGoods);

        in_cart->count -= goods->count;
        if (in_cart->count <= 0) {
            list_erase(&self->cart, node, destroy_goods);
            tell_window(BARTER_CART_WND_ID);
        }
    }
    node = find_key_of_copy(&self->goods, goods);
    if (node != self->goods.head) {
        if (!LIST_VALUE(node, struct BarterGoods)->no_stock)
            LIST_VALUE(node, struct BarterGoods)->count += goods->count;
    } else {
        struct BarterGoods copy;

        barter_goods_copy(&copy, goods);
        list_push_back(&self->goods, &copy, sizeof copy);
    }
    barter_rebuild_pay_map(self);
}

void barter_clear(struct BarterMarketMgr *self)
{
    list_clear(&self->cart, destroy_goods);
    list_clear(&self->goods, destroy_goods);
    tree_clear(&self->pay, NULL);
}

void barter_goods_push_back(struct BarterMarketMgr *self, const struct BarterGoods *goods)
{
    struct BarterGoods copy;

    barter_goods_copy(&copy, goods);
    list_push_back(&self->goods, &copy, sizeof copy);
}
