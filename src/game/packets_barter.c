#include "ui/window_access.h"
#include <stdlib.h>
#include <string.h>
#include "game/packets.h"
#include "game/barter_packets.h"

#include "web/barter_market.h"
#include "web/barter_expanded.h"

enum {
    WID_BARTER_MARKET = 0x14e,
    WID_BARTER_PURCHASE = 0x14f,
    WID_EXPANDED_BARTER_MARKET = 0x155,
    WID_EXPANDED_BARTER_PURCHASE = 0x156,
    UIM_LIST_CHANGED = 0x17,
};
#define BARTER_NO_STOCK (-1)

static void redraw_window(int id)
{
    void *wnd = ui_window_open(id);

    if (wnd)
        wnd_send(wnd, UIM_LIST_CHANGED, 0, 0, 0, 0);
}

static void set_id_text(struct String *text, int item_id)
{
    char buf[0x20];

    memset(buf, 0, sizeof buf);
    snprintf(buf, sizeof buf, "%d", item_id);
    string_assign_cstr(text, buf);
}

#pragma pack(push, 1)
struct BarterEntry {
    int item_id;
    unsigned char type;
    int count;
    int pay_id;
    int pay_count;
    int weight;
    int key;
    unsigned short view;
    int wear_location;
};
struct PACKET_ZC_NPC_BARTER_MARKET_ITEMINFO2 {
    unsigned short id;
    short length;
    struct BarterEntry items[1];
};
#pragma pack(pop)
SIZE_ASSERT(BarterEntry, 31);

void game_mode_zc_npc_barter_market_item_info2(struct GameMode *self, const unsigned char *data)
{
    const struct PACKET_ZC_NPC_BARTER_MARKET_ITEMINFO2 *packet =
        (const struct PACKET_ZC_NPC_BARTER_MARKET_ITEMINFO2 *)data;
    struct BarterGoods goods;
    long long count, i;

    (void)self;
    if (!packet)
        return;
    barter_clear(g_barter);
    count = ((long long)packet->length - 4) / (long long)sizeof(struct BarterEntry);
    for (i = 0; i < count; i++) {
        struct BarterEntry e;

        memcpy(&e, &packet->items[i], sizeof e);
        barter_goods_construct(&goods);
        set_id_text(&goods.id_text, e.item_id);
        goods.type = e.type;
        goods.pay_id = e.pay_id;
        goods.pay_count = e.pay_count;
        goods.count = e.count;
        goods.identified = 1;
        goods.weight = e.weight / 10;
        goods.no_stock = e.count == BARTER_NO_STOCK;
        goods.key = e.key;
        goods.wear_location = e.wear_location;
        goods.view = e.view;
        barter_goods_push_back(g_barter, &goods);
        barter_goods_destruct(&goods);
    }
    redraw_window(WID_BARTER_MARKET);
    redraw_window(WID_BARTER_PURCHASE);
}

#pragma pack(push, 1)
struct BarterMarketEntry {
    int item_id;
    unsigned char type;
    int amount;
    int currency_item_id;
    int currency_amount;
    int weight;
    int index;
};
struct PACKET_ZC_NPC_BARTER_MARKET_ITEMINFO {
    unsigned short id;
    short length;
    struct BarterMarketEntry items[];
};
#pragma pack(pop)
SIZE_ASSERT(BarterMarketEntry, 25);
SIZE_ASSERT(PACKET_ZC_NPC_BARTER_MARKET_ITEMINFO, 4);

void game_mode_zc_npc_barter_market_item_info(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_NPC_BARTER_MARKET_ITEMINFO *packet = (const struct PACKET_ZC_NPC_BARTER_MARKET_ITEMINFO *)pkt;
    unsigned n = (unsigned)(packet->length - 4) / sizeof(struct BarterMarketEntry);
    const struct BarterMarketEntry *entry = packet->items;
    void *wnd;

    (void)self;
    barter_clear(g_barter);
    for (; n != 0; n--, entry++) {
        struct BarterMarketEntry e = *entry;
        struct BarterGoods item;
        char id_text[33];

        barter_goods_construct(&item);
        memset(id_text, 0, sizeof id_text);
        snprintf(id_text, sizeof id_text, "%d", e.item_id);
        string_assign_cstr(&item.id_text, id_text);
        item.type = e.type;
        item.count = e.amount;
        item.pay_id = e.currency_item_id;
        item.pay_count = e.currency_amount;
        item.identified = 1;
        item.weight = e.weight / 10;
        item.no_stock = e.amount == -1;
        item.key = e.index;
        barter_goods_push_back(g_barter, &item);
        barter_goods_destruct(&item);
    }
    wnd = ui_window_open(WID_BARTER_MARKET);
    wnd_send(wnd, UIM_LIST_CHANGED, 0, 0, 0, 0);
    wnd = ui_window_open(WID_BARTER_PURCHASE);
    wnd_send(wnd, UIM_LIST_CHANGED, 0, 0, 0, 0);
}

#pragma pack(push, 1)
struct BarterExPayEntry {
    int id;
    unsigned short refine;
    int count;
    unsigned short type;
};
struct BarterExEntry {
    int item_id;
    unsigned short type;
    int count;
    int weight;
    int key;
    int price;
    int pay_count;
};
struct BarterExEntry2 {
    int item_id;
    unsigned short type;
    int count;
    int weight;
    int key;
    int price;
    unsigned short view;
    int wear_location;
    int pay_count;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct PACKET_ZC_NPC_EXPANDED_BARTER_MARKET_ITEMINFO {
    unsigned short id;
    short length;
    int count;
    unsigned char entries[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_NPC_EXPANDED_BARTER_MARKET_ITEMINFO, 8);
SIZE_ASSERT(BarterExPayEntry, 12);
SIZE_ASSERT(BarterExEntry, 0x1a);
SIZE_ASSERT(BarterExEntry2, 0x20);

static void add_pay(struct BarterGoodsEx *goods, const struct BarterExPayEntry *p)
{
    struct BarterExPay pay;

    pay.id = p->id;
    pay.refine = p->refine;
    pay.count = p->count;
    pay.type = p->type;
    vec_push(&goods->pay, &pay, sizeof pay);
}

void game_mode_zc_npc_expanded_barter_market_item_info(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_NPC_EXPANDED_BARTER_MARKET_ITEMINFO *packet =
        (const struct PACKET_ZC_NPC_EXPANDED_BARTER_MARKET_ITEMINFO *)pkt;
    const unsigned char *p = packet->entries;
    struct BarterGoodsEx goods;
    int n, k;

    (void)self;
    expanded_barter_clear(g_expandedBarter);
    for (n = packet->count; n > 0; n--) {
        struct BarterExEntry e;

        memcpy(&e, p, sizeof e);
        barter_goods_ex_construct(&goods);
        set_id_text(&goods.id_text, e.item_id);
        p += sizeof e;
        goods.type = e.type;
        goods.count = e.count;
        goods.weight = (float)(e.weight / 10);
        goods.key = e.key;
        goods.price = e.price;
        for (k = e.pay_count; k > 0; k--, p += sizeof(struct BarterExPayEntry)) {
            struct BarterExPayEntry pay;

            memcpy(&pay, p, sizeof pay);
            add_pay(&goods, &pay);
        }
        expanded_barter_goods_push(g_expandedBarter, &goods);
        barter_goods_ex_destruct(&goods);
    }
    wnd_send(ui_window_open(WID_EXPANDED_BARTER_MARKET), UIM_LIST_CHANGED, 0, 0, 0, 0);
    wnd_send(ui_window_open(WID_EXPANDED_BARTER_PURCHASE), UIM_LIST_CHANGED, 0, 0, 0, 0);
}

void game_mode_zc_npc_expanded_barter_market_item_info2(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_NPC_EXPANDED_BARTER_MARKET_ITEMINFO *packet =
        (const struct PACKET_ZC_NPC_EXPANDED_BARTER_MARKET_ITEMINFO *)pkt;
    const unsigned char *p;
    struct BarterGoodsEx goods;
    long long count, i, pays, k;

    (void)self;
    if (!pkt)
        return;
    expanded_barter_clear(g_expandedBarter);
    p = packet->entries;
    count = packet->count;
    for (i = 0; i < count; i++) {
        struct BarterExEntry2 e;

        memcpy(&e, p, sizeof e);
        barter_goods_ex_construct(&goods);
        set_id_text(&goods.id_text, e.item_id);
        p += sizeof e;
        goods.type = e.type;
        goods.count = e.count;
        goods.weight = (float)(e.weight / 10);
        goods.key = e.key;
        goods.price = e.price;
        goods.wear_location = e.wear_location;
        goods.view = e.view;
        pays = e.pay_count;
        for (k = 0; k < pays; k++, p += sizeof(struct BarterExPayEntry)) {
            struct BarterExPayEntry pay;

            memcpy(&pay, p, sizeof pay);
            add_pay(&goods, &pay);
        }
        expanded_barter_goods_push(g_expandedBarter, &goods);
        barter_goods_ex_destruct(&goods);
    }
    redraw_window(WID_EXPANDED_BARTER_MARKET);
    redraw_window(WID_EXPANDED_BARTER_PURCHASE);
}
