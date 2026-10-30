#include <string.h>
#include "contents/barter_item.h"
#include "std/std.h"

struct BarterGoods *barter_goods_construct(struct BarterGoods *self)
{
    string_init(&self->id_text);
    self->count = 0;
    self->type = 0;
    self->price = 0;
    self->identified = 1;
    self->pay_count = 0;
    self->pay_id = 0;
    self->weight = 0;
    self->no_stock = 0;
    self->key = -1;
    self->wear_location = 0;
    self->view = 0;
    return self;
}

void barter_goods_destruct(struct BarterGoods *self)
{
    string_free(&self->id_text);
}

struct ItemInfo *barter_goods_to_item_info(const struct BarterGoods *self,
                                                     struct ItemInfo *out)
{
    item_info_construct(out);
    out->type = self->type;
    out->count = self->count;
    out->price = self->price;
    string_assign(&out->id_text, string_cstr(&self->id_text), self->id_text.size);
    out->identified = 1;
    out->equip_location = self->wear_location;
    out->view = self->view;
    return out;
}

struct BarterGoodsEx *barter_goods_ex_construct(struct BarterGoodsEx *self)
{
    self->type = 0;
    self->count = 0;
    self->price = 0;
    string_init(&self->id_text);
    self->identified = 1;
    self->refine = 0;
    self->weight = 0.0f;
    self->no_stock = 0;
    self->key = 0;
    self->wear_location = 0;
    self->view = 0;
    vec_init(&self->pay);
    return self;
}

void barter_goods_ex_destruct(struct BarterGoodsEx *self)
{
    vec_free(&self->pay);
    string_free(&self->id_text);
}

struct ItemInfo *barter_goods_ex_to_item_info(const struct BarterGoodsEx *self,
                                                              struct ItemInfo *out)
{
    item_info_construct(out);
    out->type = self->type;
    out->count = self->count;
    out->price = self->price;
    string_assign(&out->id_text, string_cstr(&self->id_text), self->id_text.size);
    out->refine = self->refine;
    out->identified = 1;
    out->weight = (short)(int)self->weight;
    out->equip_location = self->wear_location;
    out->view = self->view;
    return out;
}

struct BarterGoods *barter_goods_copy(struct BarterGoods *self,
                                                const struct BarterGoods *src)
{
    string_copy(&self->id_text, &src->id_text);
    self->count = src->count;
    self->type = src->type;
    self->price = src->price;
    self->identified = src->identified;
    self->pay_count = src->pay_count;
    self->pay_id = src->pay_id;
    self->weight = src->weight;
    self->no_stock = src->no_stock;
    self->key = src->key;
    self->wear_location = src->wear_location;
    self->view = src->view;
    return self;
}

struct BarterGoodsEx *barter_goods_ex_copy(struct BarterGoodsEx *self,
                                                    const struct BarterGoodsEx *src)
{
    self->type = src->type;
    self->count = src->count;
    self->price = src->price;
    string_copy(&self->id_text, &src->id_text);
    self->identified = src->identified;
    self->refine = src->refine;
    self->weight = src->weight;
    self->no_stock = src->no_stock;
    self->key = src->key;
    self->wear_location = src->wear_location;
    self->view = src->view;
    vec_copy(&self->pay, &src->pay, sizeof(struct BarterExPay));
    return self;
}

void barter_goods_ex_vec_copy(struct Vec *dst, const struct Vec *src)
{
    const struct BarterGoodsEx *it;
    struct BarterGoodsEx *out;

    vec_init(dst);
    out = vec_add(dst, VEC_COUNT(src, struct BarterGoodsEx), sizeof *out);
    for (it = src->first; it != src->last; it++, out++)
        barter_goods_ex_copy(out, it);
}

void barter_goods_ex_vec_free(struct Vec *v)
{
    struct BarterGoodsEx *it;

    for (it = v->first; it != v->last; it++)
        barter_goods_ex_destruct(it);
    vec_free(v);
}
