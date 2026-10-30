#ifndef ASYNCWORK_BARTER_EXPANDED_H
#define ASYNCWORK_BARTER_EXPANDED_H

#include "contents/barter_item.h"

#include "std/std.h"
#include "iteminfo/item_api.h"
#include "std/vec.h"
#include "std/container_types.h"

struct RefineItemPair {
    int refine;
    struct ItemInfo item;
};
struct ExpandedBarter {
    struct Vec goods;
    struct Vec cart;
    struct Vec pay;
    struct Tree inventory;
    __int64 total;
};
extern struct ExpandedBarter *g_expandedBarter;

char expanded_barter_create(void);
void expanded_barter_destroy(void);
struct Vec *expanded_barter_goods(struct ExpandedBarter *self);
struct Vec *expanded_barter_cart(struct ExpandedBarter *self);
struct Vec *expanded_barter_pay_items(struct ExpandedBarter *self);
void expanded_barter_goods_add(struct ExpandedBarter *self, const struct BarterGoodsEx *goods);
void expanded_barter_cart_add_entry(struct ExpandedBarter *self, const struct BarterGoodsEx *goods);
void expanded_barter_pay_add(struct ExpandedBarter *self, struct BarterExPay pay, int times);
void expanded_barter_update_total(struct ExpandedBarter *self);
void expanded_barter_read_inventory(struct ExpandedBarter *self);
void expanded_barter_goods_take(struct ExpandedBarter *self, const struct BarterGoodsEx *goods);
void expanded_barter_cart_take(struct ExpandedBarter *self, const struct BarterGoodsEx *goods);
void expanded_barter_pay_remove(struct ExpandedBarter *self, struct BarterExPay pay, int times);
void expanded_barter_cart_remove(struct ExpandedBarter *self, const struct BarterGoodsEx *goods);
void expanded_barter_cart_add(struct ExpandedBarter *self, const struct BarterGoodsEx *goods);
char expanded_barter_cart_has_id(struct ExpandedBarter *self, struct String id_text);
int expanded_barter_goods_count(struct ExpandedBarter *self, int key);
struct BarterExFound *expanded_barter_find_goods(struct ExpandedBarter *self, struct BarterExFound *out,
                                                            int key);
struct BarterExFound *expanded_barter_find_in_cart(struct ExpandedBarter *self, struct BarterExFound *out,
                                                              int key);
float expanded_barter_goods_weight(struct ExpandedBarter *self, int key);
unsigned expanded_barter_cart_size(struct ExpandedBarter *self);
int expanded_barter_cart_count(struct ExpandedBarter *self, int key);
__int64 expanded_barter_total(const struct ExpandedBarter *self);
float expanded_barter_cart_weight(struct ExpandedBarter *self);
char expanded_barter_can_pay(struct ExpandedBarter *self);
void expanded_barter_goods_push(struct ExpandedBarter *self, const struct BarterGoodsEx *src);
void expanded_barter_clear(struct ExpandedBarter *self);

#endif
