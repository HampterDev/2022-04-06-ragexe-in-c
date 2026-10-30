#ifndef CONTENTS_BARTER_ITEM_H
#define CONTENTS_BARTER_ITEM_H

#include "base.h"
#include "iteminfo/item_api.h"
#include "std/vec.h"

struct BarterGoods {
    struct String id_text;
    int count;
    int type;
    int price;
    char identified;
    int pay_count;
    int pay_id;
    int weight;
    char no_stock;
    int key;
    int wear_location;
    int view;
};

struct BarterExPay {
    int id;
    int refine;
    int count;
    int type;
};

struct BarterGoodsEx {
    int type;
    int count;
    int price;
    struct String id_text;
    char identified;
    int refine;
    float weight;
    char no_stock;
    int key;
    int wear_location;
    int view;
    struct Vec pay;
};

struct BarterFound {
    char found;
    struct BarterGoods goods;
};
struct BarterExFound {
    char found;
    struct BarterGoodsEx goods;
};

struct BarterGoods *barter_goods_construct(struct BarterGoods *self);
void barter_goods_destruct(struct BarterGoods *self);
struct BarterGoodsEx *barter_goods_ex_construct(struct BarterGoodsEx *self);
void barter_goods_ex_destruct(struct BarterGoodsEx *self);

struct BarterGoods *barter_goods_copy(struct BarterGoods *self, const struct BarterGoods *src);
struct BarterGoodsEx *barter_goods_ex_copy(struct BarterGoodsEx *self, const struct BarterGoodsEx *src);
void barter_goods_ex_vec_copy(struct Vec *dst, const struct Vec *src);
void barter_goods_ex_vec_free(struct Vec *v);

struct ItemInfo *barter_goods_to_item_info(const struct BarterGoods *self, struct ItemInfo *out);
struct ItemInfo *barter_goods_ex_to_item_info(const struct BarterGoodsEx *self, struct ItemInfo *out);

#endif
