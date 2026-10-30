#include "session/state.h"
#include "core/memory.h"
#include <stdlib.h>
#include <string.h>
#include "web/barter_expanded.h"
#include "std/tree.h"
#include "session/session_items.h"

struct ExpandedBarter *g_expandedBarter;

#define GOODS_FIRST(v) ((struct BarterGoodsEx *)(v)->first)
#define GOODS_LAST(v) ((struct BarterGoodsEx *)(v)->last)
#define PAY_FIRST(v) ((struct BarterExPay *)(v)->first)
#define PAY_LAST(v) ((struct BarterExPay *)(v)->last)

static struct BarterGoodsEx *find_key(const struct Vec *v, int key)
{
    struct BarterGoodsEx *p;

    for (p = GOODS_FIRST(v); p != GOODS_LAST(v); p++)
        if (p->key == key)
            break;
    return p;
}

static void goods_push_copy(struct Vec *v, const struct BarterGoodsEx *goods)
{
    struct BarterGoodsEx copy;

    barter_goods_ex_copy(&copy, goods);
    vec_push(v, &copy, sizeof copy);
}

static struct BarterExPay *find_pay(const struct Vec *v, int id, int refine)
{
    struct BarterExPay *p;

    for (p = PAY_FIRST(v); p != PAY_LAST(v); p++)
        if (p->id == id && p->refine == refine)
            break;
    return p;
}

static void destroy_refine_item(void *value)
{
    item_info_destruct(&((struct RefineItemPair *)value)->item);
}

static void mgr_init(struct ExpandedBarter *self)
{
    vec_init(&self->goods);
    vec_init(&self->cart);
    vec_init(&self->pay);
    tree_init(&self->inventory);
    self->total = 0;
}

static void expanded_barter_destruct(struct ExpandedBarter *self)
{
    tree_free(&self->inventory, destroy_refine_item);
    vec_free(&self->pay);
    barter_goods_ex_vec_free(&self->cart);
    barter_goods_ex_vec_free(&self->goods);
}

char expanded_barter_create(void)
{
    struct ExpandedBarter *mgr;

    if (g_expandedBarter)
        return 0;
    mgr = mem_try_alloc(sizeof *mgr);
    if (mgr)
        mgr_init(mgr);
    g_expandedBarter = mgr;
    if (!mgr)
        return 0;
    return 1;
}

void expanded_barter_destroy(void)
{
    if (g_expandedBarter) {
        expanded_barter_destruct(g_expandedBarter);
        mem_free(g_expandedBarter);
        g_expandedBarter = NULL;
    }
}

struct Vec *expanded_barter_goods(struct ExpandedBarter *self)
{
    return &self->goods;
}

struct Vec *expanded_barter_cart(struct ExpandedBarter *self)
{
    return &self->cart;
}

struct Vec *expanded_barter_pay_items(struct ExpandedBarter *self)
{
    return &self->pay;
}

void expanded_barter_goods_add(struct ExpandedBarter *self, const struct BarterGoodsEx *goods)
{
    struct BarterGoodsEx *p = find_key(&self->goods, goods->key);

    if (p == GOODS_LAST(&self->goods))
        goods_push_copy(&self->goods, goods);
    else if (!p->no_stock)
        p->count += goods->count;
}

void expanded_barter_cart_add_entry(struct ExpandedBarter *self, const struct BarterGoodsEx *goods)
{
    struct BarterGoodsEx *p = find_key(&self->cart, goods->key);

    if (p == GOODS_LAST(&self->cart))
        goods_push_copy(&self->cart, goods);
    else
        p->count += goods->count;
}

void expanded_barter_pay_add(struct ExpandedBarter *self, struct BarterExPay pay, int times)
{
    struct BarterExPay *p = find_pay(&self->pay, pay.id, pay.refine);
    struct BarterExPay value;

    if (p != PAY_LAST(&self->pay)) {
        p->count += pay.count * times;
        return;
    }
    value.id = pay.id;
    value.refine = pay.refine;
    value.count = pay.count * times;
    value.type = pay.type;
    vec_push(&self->pay, &value, sizeof value);
}

void expanded_barter_update_total(struct ExpandedBarter *self)
{
    struct BarterGoodsEx *p;

    self->total = 0;
    for (p = GOODS_FIRST(&self->cart); p != GOODS_LAST(&self->cart); p++)
        self->total += (int)((unsigned)p->price * (unsigned)p->count);
}

void expanded_barter_read_inventory(struct ExpandedBarter *self)
{
    struct Tree *map = &self->inventory;
    struct RefineItemPair pair;
    struct ItemInfo item;
    int count, i;

    tree_clear(map, destroy_refine_item);
    count = session_get_inventory_count(&g_session1);
    for (i = 0; i < count; i++) {
        session_get_inventory_item_at(&g_session1, &item, i);
        if (item.identified) {
            pair.refine = item.refine;
            item_info_copy(&pair.item, &item);
            tree_add_multi(map, &pair, sizeof pair, tree_less_int);
        }
        item_info_destruct(&item);
    }
}

void expanded_barter_goods_take(struct ExpandedBarter *self, const struct BarterGoodsEx *goods)
{
    struct BarterGoodsEx *p;

    for (p = GOODS_FIRST(&self->goods); p != GOODS_LAST(&self->goods); p++) {
        if (p->key == goods->key && !p->no_stock) {
            p->count -= goods->count;
            if (p->count < 0)
                p->count = 0;
            return;
        }
    }
}

void expanded_barter_cart_take(struct ExpandedBarter *self, const struct BarterGoodsEx *goods)
{
    struct Vec *cart = &self->cart;
    struct BarterGoodsEx *p = find_key(cart, goods->key);

    if (p == GOODS_LAST(cart))
        return;
    p->count -= goods->count;
    if (p->count > 0)
        return;
    barter_goods_ex_destruct(p);
    vec_erase(cart, p, p + 1);
}

void expanded_barter_pay_remove(struct ExpandedBarter *self, struct BarterExPay pay, int times)
{
    struct Vec *v = &self->pay;
    struct BarterExPay *p = find_pay(v, pay.id, pay.refine);

    if (p == PAY_LAST(v))
        return;
    p->count -= pay.count * times;
    if (p->count > 0)
        return;
    vec_erase(v, p, p + 1);
}

void expanded_barter_cart_remove(struct ExpandedBarter *self, const struct BarterGoodsEx *goods)
{
    const struct BarterExPay *pay;

    expanded_barter_cart_take(self, goods);
    expanded_barter_goods_add(self, goods);
    for (pay = PAY_FIRST(&goods->pay); pay != PAY_LAST(&goods->pay); pay++)
        expanded_barter_pay_remove(self, *pay, goods->count);
    expanded_barter_update_total(self);
}

void expanded_barter_cart_add(struct ExpandedBarter *self, const struct BarterGoodsEx *goods)
{
    const struct BarterExPay *pay;

    expanded_barter_cart_add_entry(self, goods);
    expanded_barter_goods_take(self, goods);
    for (pay = PAY_FIRST(&goods->pay); pay != PAY_LAST(&goods->pay); pay++)
        expanded_barter_pay_add(self, *pay, goods->count);
    expanded_barter_update_total(self);
}

char expanded_barter_cart_has_id(struct ExpandedBarter *self, struct String id_text)
{
    struct String key;
    struct BarterGoodsEx copy;
    struct BarterGoodsEx *p, *last = GOODS_LAST(&self->cart);
    char found = 0, same;

    string_copy(&key, &id_text);
    for (p = GOODS_FIRST(&self->cart); p != last; p++) {
        barter_goods_ex_copy(&copy, p);
        same = copy.id_text.size == key.size
            && memcmp(string_cstr(&key), string_cstr(&copy.id_text), key.size) == 0;
        found = same;
        barter_goods_ex_destruct(&copy);
        if (same)
            break;
    }
    string_free(&key);
    string_free(&id_text);
    return found;
}

int expanded_barter_goods_count(struct ExpandedBarter *self, int key)
{
    struct BarterGoodsEx *p = find_key(&self->goods, key);

    return p == GOODS_LAST(&self->goods) ? 0 : p->count;
}

static struct BarterExFound *found_copy(struct BarterExFound *out, const struct Vec *v, int key)
{
    struct BarterGoodsEx *p = find_key(v, key);
    struct BarterGoodsEx empty;

    if (p != GOODS_LAST(v)) {
        out->found = 1;
        barter_goods_ex_copy(&out->goods, p);
        return out;
    }
    barter_goods_ex_construct(&empty);
    out->found = 0;
    barter_goods_ex_copy(&out->goods, &empty);
    barter_goods_ex_destruct(&empty);
    return out;
}

struct BarterExFound *expanded_barter_find_goods(struct ExpandedBarter *self, struct BarterExFound *out,
                                                            int key)
{
    return found_copy(out, &self->goods, key);
}

struct BarterExFound *expanded_barter_find_in_cart(struct ExpandedBarter *self, struct BarterExFound *out,
                                                              int key)
{
    return found_copy(out, &self->cart, key);
}

float expanded_barter_goods_weight(struct ExpandedBarter *self, int key)
{
    struct BarterGoodsEx *p = find_key(&self->goods, key);

    return p == GOODS_LAST(&self->goods) ? -1.0f : p->weight;
}

unsigned expanded_barter_cart_size(struct ExpandedBarter *self)
{
    return (unsigned)(GOODS_LAST(&self->cart) - GOODS_FIRST(&self->cart));
}

int expanded_barter_cart_count(struct ExpandedBarter *self, int key)
{
    struct BarterGoodsEx *p = find_key(&self->cart, key);

    return p == GOODS_LAST(&self->cart) ? 0 : p->count;
}

__int64 expanded_barter_total(const struct ExpandedBarter *self)
{
    return self->total;
}

float expanded_barter_cart_weight(struct ExpandedBarter *self)
{
    struct BarterGoodsEx *p;
    float total = 0.0f;

    for (p = GOODS_FIRST(&self->cart); p != GOODS_LAST(&self->cart); p++)
        total += (float)p->count * p->weight;
    return total;
}

static char take_from_inventory(struct Tree *map, const struct BarterExPay *pay)
{
    struct TreeNode *node = tree_first(map);
    struct ItemInfo *item;
    int needed = pay->count;

    while (node != map->head) {
        item = &TREE_VALUE(node, struct RefineItemPair)->item;
        if (atoi(string_cstr(&item->id_text)) == pay->id && (pay->refine <= 0 || item->refine >= pay->refine)) {
            if (item->count > needed) {
                item->count -= needed;
                return 1;
            }
            if (item->count == needed) {
                tree_erase(map, node, destroy_refine_item);
                return 1;
            }
            needed -= item->count;
            node = tree_erase(map, node, destroy_refine_item);
        } else {
            node = tree_next(node);
        }
    }
    return 0;
}

char expanded_barter_can_pay(struct ExpandedBarter *self)
{
    const struct BarterExPay *pay;

    expanded_barter_read_inventory(self);
    for (pay = PAY_FIRST(&self->pay); pay != PAY_LAST(&self->pay); pay++)
        if (!take_from_inventory(&self->inventory, pay))
            return 0;
    return 1;
}

void expanded_barter_goods_push(struct ExpandedBarter *self, const struct BarterGoodsEx *src)
{
    struct BarterGoodsEx goods;
    const struct BarterExPay *pay;

    barter_goods_ex_construct(&goods);
    goods.count = src->count;
    goods.type = src->type;
    string_assign(&goods.id_text, string_cstr(&src->id_text), src->id_text.size);
    goods.wear_location = src->wear_location;
    goods.view = src->view;
    goods.identified = 1;
    for (pay = PAY_FIRST(&src->pay); pay != PAY_LAST(&src->pay); pay++)
        vec_push(&goods.pay, pay, sizeof *pay);
    goods.key = src->key;
    goods.no_stock = src->count == -1;
    goods.weight = src->weight;
    goods.price = src->price;
    vec_push(&self->goods, &goods, sizeof goods);
}

void expanded_barter_clear(struct ExpandedBarter *self)
{
    struct BarterGoodsEx *p;

    self->total = 0;
    for (p = GOODS_FIRST(&self->cart); p != GOODS_LAST(&self->cart); p++)
        barter_goods_ex_destruct(p);
    vec_clear(&self->cart);
    for (p = GOODS_FIRST(&self->goods); p != GOODS_LAST(&self->goods); p++)
        barter_goods_ex_destruct(p);
    vec_clear(&self->goods);
    vec_clear(&self->pay);
}
