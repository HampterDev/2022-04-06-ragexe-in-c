#ifndef WEB_BARTER_MARKET_H
#define WEB_BARTER_MARKET_H

#include "contents/barter_item.h"

#include "std/std.h"
#include "std/list.h"

struct BarterPay {
    int id;
    int count;
};

struct BarterMarketMgr {
    struct List goods;
    struct List cart;
    struct Tree pay;
};
extern struct BarterMarketMgr *g_barter;

char barter_market_create(void);
void barter_market_destroy(void);
struct List *barter_goods(struct BarterMarketMgr *self);
struct List *barter_cart(struct BarterMarketMgr *self);
struct Tree *barter_pay_map(struct BarterMarketMgr *self);
struct BarterFound *barter_find_goods(struct BarterMarketMgr *self, struct BarterFound *out, int key);
struct BarterFound *barter_find_in_cart(struct BarterMarketMgr *self, struct BarterFound *out, int key);
void barter_cart_add(struct BarterMarketMgr *self, const struct BarterGoods *goods);
void barter_cart_remove(struct BarterMarketMgr *self, const struct BarterGoods *goods);
void barter_clear(struct BarterMarketMgr *self);
char barter_cart_has_id(struct BarterMarketMgr *self, struct String id_text);
int barter_cart_count_of(struct BarterMarketMgr *self, struct String id_text);
int barter_goods_weight(struct BarterMarketMgr *self, int key);
float barter_cart_weight(struct BarterMarketMgr *self);
void barter_rebuild_pay_map(struct BarterMarketMgr *self);
int barter_goods_count(struct BarterMarketMgr *self, int key);
unsigned barter_cart_size(struct BarterMarketMgr *self);
int barter_cart_count(struct BarterMarketMgr *self, int key);
void barter_goods_push_back(struct BarterMarketMgr *self, const struct BarterGoods *goods);

#endif
