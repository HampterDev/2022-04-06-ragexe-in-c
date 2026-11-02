#include "contents/barter_item.h"
#include "game/drag.h"
#include "game/client.h"
#include "std/std.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/game/packets_barter.c"

struct UIWindowMgr g_windowMgr;
static struct BarterMarketMgr ordinary_manager;
static struct ExpandedBarter expanded_manager;
struct BarterMarketMgr *g_barter = &ordinary_manager;
struct ExpandedBarter *g_expandedBarter = &expanded_manager;
static struct BarterGoods ordinary_received[4];
static struct BarterGoodsEx expanded_received[4];
static unsigned ordinary_count, expanded_count, opens, messages, closes;
static struct Block { unsigned char *raw; size_t size; } blocks[128];
static unsigned allocations;
static int display_identified;
static const char long_id[] = "0123456789abcdefghijkl\0trailing";

void *mem_alloc(size_t bytes)
{
    unsigned i;
    unsigned char *raw;
    assert(bytes && bytes < 100000);
    for (i = 0; i < 128 && blocks[i].raw; i++) {}
    assert(i < 128);
    raw = malloc(bytes + 32);
    assert(raw);
    memset(raw, 0xa5, bytes + 32);
    blocks[i].raw = raw;
    blocks[i].size = bytes;
    allocations++;
    return raw + 16;
}

void mem_free(void *memory)
{
    unsigned i, j;
    if (!memory) return;
    for (i = 0; i < 128 && (!blocks[i].raw || blocks[i].raw + 16 != memory); i++) {}
    assert(i < 128 && allocations);
    for (j = 0; j < 16; j++) {
        assert(blocks[i].raw[j] == 0xa5);
        assert(blocks[i].raw[16 + blocks[i].size + j] == 0xa5);
    }
    free(blocks[i].raw);
    blocks[i].raw = NULL;
    allocations--;
}

void fatal_error(const char *message) { (void)message; abort(); }

void barter_clear(struct BarterMarketMgr *self)
{
    assert(self == g_barter);
    while (ordinary_count) barter_goods_destruct(&ordinary_received[--ordinary_count]);
}
void barter_goods_push_back(struct BarterMarketMgr *self, const struct BarterGoods *goods)
{
    assert(self == g_barter && ordinary_count < 4);
    barter_goods_copy(&ordinary_received[ordinary_count++], goods);
}
void expanded_barter_clear(struct ExpandedBarter *self)
{
    assert(self == g_expandedBarter);
    while (expanded_count) barter_goods_ex_destruct(&expanded_received[--expanded_count]);
}
void expanded_barter_goods_push(struct ExpandedBarter *self, const struct BarterGoodsEx *goods)
{
    assert(self == g_expandedBarter && expanded_count < 4);
    barter_goods_ex_copy(&expanded_received[expanded_count++], goods);
}
struct UIWindow *ui_window_open(int id)
{
    assert(id == 0x14e || id == 0x14f || id == 0x155 || id == 0x156);
    opens++;
    return (struct UIWindow *)(uintptr_t)id;
}
int ui_window_send(struct UIWindow *w, struct UIWindow *sender, int msg, int a, int b, int c, int d)
{
    assert(w && !sender && msg == 0x17 && !a && !b && !c && !d);
    messages++;
    return 0;
}
void ui_window_mgr_show_option_window(struct UIWindowMgr *self, int id, int arg)
{
    assert(self == &g_windowMgr && id == 15 && !arg);
    closes++;
}
const char *session_get_item_display_name(const char *name, int identified)
{
    assert(!strcmp(name, "501"));
    display_identified = identified;
    return "Red Potion";
}

static void ownership_and_conversion(void)
{
    struct BarterGoods goods, copy;
    struct BarterGoodsEx expanded, expanded_copy;
    struct ItemInfo item;
    struct BarterExPay pay = {501, 7, 3, 4};
    unsigned i;

    memset(&goods, 0xcc, sizeof goods);
    assert(barter_goods_construct(&goods) == &goods);
    assert(goods.key == -1 && goods.identified == 1 && !goods.count && !goods.price);
    assert(!goods.id_text.size && !goods.no_stock && !goods.pay_id && !goods.pay_count);
    assert(!goods.weight && !goods.type && !goods.view && !goods.wear_location);
    string_assign(&goods.id_text, long_id, sizeof long_id - 1);
    goods.type = 4; goods.count = 9; goods.price = 123; goods.identified = 0;
    goods.pay_count = 5; goods.pay_id = 501; goods.weight = 12;
    goods.no_stock = 1; goods.key = 7; goods.wear_location = 0x4000; goods.view = 42;
    barter_goods_copy(&copy, &goods);
    assert(string_cstr(&copy.id_text) != string_cstr(&goods.id_text));
    barter_goods_destruct(&goods);
    assert(copy.id_text.size == sizeof long_id - 1);
    assert(!memcmp(string_cstr(&copy.id_text), long_id, sizeof long_id));
    assert(copy.type == 4 && copy.count == 9 && copy.price == 123 && !copy.identified);
    assert(copy.pay_count == 5 && copy.pay_id == 501 && copy.weight == 12);
    assert(copy.no_stock == 1 && copy.key == 7 && copy.wear_location == 0x4000 && copy.view == 42);
    assert(barter_goods_to_item_info(&copy, &item) == &item);
    assert(item.type == 4 && item.count == 9 && item.price == 123 && item.identified == 1);
    assert(!item.weight && !item.refine && item.equip_location == 0x4000 && item.view == 42);
    barter_goods_destruct(&copy);
    assert(item.id_text.size == sizeof long_id - 1);
    assert(!memcmp(string_cstr(&item.id_text), long_id, sizeof long_id));
    item_info_destruct(&item);

    memset(&expanded, 0xcc, sizeof expanded);
    barter_goods_ex_construct(&expanded);
    assert(!expanded.key && expanded.identified == 1 && !expanded.refine && expanded.weight == 0.0f);
    assert(!expanded.type && !expanded.count && !expanded.price && !expanded.no_stock);
    assert(!expanded.view && !expanded.wear_location && !expanded.id_text.size);
    assert(!expanded.pay.first && !expanded.pay.last && !expanded.pay.end);
    barter_goods_ex_copy(&expanded_copy, &expanded);
    assert(!expanded_copy.pay.first && !expanded_copy.pay.last && !expanded_copy.pay.end);
    barter_goods_ex_destruct(&expanded_copy);
    string_assign(&expanded.id_text, long_id, sizeof long_id - 1);
    expanded.type = 5; expanded.count = 6; expanded.price = 77; expanded.identified = 0;
    expanded.refine = 8; expanded.weight = 16.75f; expanded.no_stock = 1;
    expanded.key = 123; expanded.wear_location = 0x2000; expanded.view = 56;
    for (i = 0; i < 37; i++) {
        pay.count = (int)i + 1;
        vec_push(&expanded.pay, &pay, sizeof pay);
    }
    barter_goods_ex_copy(&expanded_copy, &expanded);
    assert(expanded_copy.pay.first != expanded.pay.first);
    ((struct BarterExPay *)expanded.pay.first)[0].id = -1;
    barter_goods_ex_destruct(&expanded);
    assert(vec_count(&expanded_copy.pay, sizeof pay) == 37);
    for (i = 0; i < 37; i++) {
        const struct BarterExPay *p = (struct BarterExPay *)expanded_copy.pay.first + i;
        assert(p->id == 501 && p->refine == 7 && p->count == (int)i + 1 && p->type == 4);
    }
    assert(expanded_copy.no_stock == 1 && expanded_copy.key == 123 && !expanded_copy.identified);
    barter_goods_ex_to_item_info(&expanded_copy, &item);
    assert(item.type == 5 && item.count == 6 && item.price == 77 && item.identified == 1);
    assert(item.refine == 8 && item.weight == 16 && item.equip_location == 0x2000 && item.view == 56);
    assert(!memcmp(string_cstr(&item.id_text), long_id, sizeof long_id));
    item_info_destruct(&item);
    expanded_copy.weight = -3.75f;
    barter_goods_ex_to_item_info(&expanded_copy, &item);
    assert(item.weight == (unsigned short)-3);
    barter_goods_ex_destruct(&expanded_copy);
    item_info_destruct(&item);
    assert(!allocations);
}

static void put16(unsigned char *p, unsigned value)
{ p[0] = (unsigned char)value; p[1] = (unsigned char)(value >> 8); }
static void put32(unsigned char *p, unsigned value)
{ put16(p, value); put16(p + 2, value >> 16); }

static void packets(void)
{
    unsigned char data[256];
    unsigned version, i;
    memset(data, 0, sizeof data);
    put16(data, 0xb78); put16(data + 2, 66);
    for (i = 0; i < 2; i++) {
        unsigned char *p = data + 4 + i * 31;
        put32(p, 501 + i); p[4] = 4; put32(p + 5, i ? 8 : (unsigned)-1);
        put32(p + 9, 700); put32(p + 13, 3); put32(p + 17, 127);
        put32(p + 21, 22 + i); put16(p + 25, 0x5678); put32(p + 27, 0x12345678);
    }
    game_mode_zc_npc_barter_market_item_info2(NULL, (const void *)data);
    assert(ordinary_count == 2 && opens == 2 && messages == 2);
    assert(!strcmp(string_cstr(&ordinary_received[0].id_text), "501"));
    assert(ordinary_received[0].count == -1 && ordinary_received[0].no_stock == 1);
    assert(ordinary_received[1].count == 8 && !ordinary_received[1].no_stock);
    assert(ordinary_received[1].weight == 12 && ordinary_received[1].pay_count == 3);
    assert(ordinary_received[1].pay_id == 700 && ordinary_received[1].key == 23);
    assert(ordinary_received[1].view == 0x5678 && ordinary_received[1].wear_location == 0x12345678);
    barter_clear(g_barter);
    for (version = 1; version <= 2; version++) {
        unsigned char *p = data + 8;
        unsigned entry_size = version == 1 ? 26 : 32;
        memset(data, 0, sizeof data);
        put32(data + 4, 2);
        for (i = 0; i < 2; i++) {
            unsigned j, count = i ? 0 : 3;
            put32(p, 501 + i); put16(p + 4, 4); put32(p + 6, 8 + i);
            put32(p + 10, 137); put32(p + 14, 30 + i); put32(p + 18, 1000 + i);
            if (version == 2) { put16(p + 22, 600); put32(p + 24, 0x43210000); }
            put32(p + entry_size - 4, count); p += entry_size;
            for (j = 0; j < count; j++, p += 12) {
                put32(p, 700 + j); put16(p + 4, 5 + j);
                put32(p + 6, 9 + j); put16(p + 10, 4);
            }
        }
        if (version == 1) game_mode_zc_npc_expanded_barter_market_item_info(NULL, data);
        else game_mode_zc_npc_expanded_barter_market_item_info2(NULL, data);
        assert(expanded_count == 2);
        assert(expanded_received[0].count == 8 && expanded_received[0].price == 1000);
        assert(expanded_received[0].weight == 13.0f && expanded_received[0].key == 30);
        assert(expanded_received[0].view == (version == 2 ? 600 : 0));
        assert(expanded_received[0].wear_location == (version == 2 ? 0x43210000 : 0));
        assert(vec_count(&expanded_received[0].pay, sizeof(struct BarterExPay)) == 3);
        for (i = 0; i < 3; i++) {
            const struct BarterExPay *pay = (struct BarterExPay *)expanded_received[0].pay.first + i;
            assert(pay->id == 700 + (int)i && pay->refine == 5 + (int)i);
            assert(pay->count == 9 + (int)i && pay->type == 4);
        }
        assert(expanded_received[1].key == 31 && expanded_received[1].count == 9);
        assert(!expanded_received[1].pay.first);
        expanded_barter_clear(g_expandedBarter);
    }
    memset(data, 0, sizeof data);
    put16(data, 0xb0e); put16(data + 2, 54);
    for (i = 0; i < 2; i++) {
        unsigned char *p = data + 4 + i * 25;
        put32(p, 503 + i); p[4] = 5; put32(p + 5, i ? 7 : (unsigned)-1);
        put32(p + 9, 701); put32(p + 13, 4); put32(p + 17, 157);
        put32(p + 21, 24 + i);
    }
    game_mode_zc_npc_barter_market_item_info(NULL, data);
    assert(ordinary_count == 2 && ordinary_received[0].no_stock == 1);
    assert(ordinary_received[0].count == -1 && !ordinary_received[1].no_stock);
    assert(ordinary_received[1].count == 7 && ordinary_received[1].type == 5);
    assert(ordinary_received[1].pay_id == 701 && ordinary_received[1].pay_count == 4);
    assert(ordinary_received[1].weight == 15 && ordinary_received[1].key == 25);
    assert(!ordinary_received[1].view && !ordinary_received[1].wear_location);
    assert(!strcmp(string_cstr(&ordinary_received[1].id_text), "504"));
    barter_clear(g_barter);
    assert(opens == 8 && messages == 8 && !allocations);
}

static void dragging(void)
{
    struct GameMode *mode = calloc(1, sizeof *mode);
    struct BarterGoods goods;
    struct BarterGoodsEx expanded;
    struct DragInfo *drag;
    assert(mode);
    drag = &mode->drag;
    string_init(&drag->display_name); string_init(&drag->name); string_init(&drag->name2);
    barter_goods_construct(&goods);
    string_assign_cstr(&goods.id_text, "501");
    goods.type = 4; goods.price = 25; goods.key = 72; goods.identified = (char)0x80;
    game_mode_drag_barter_item(mode, 31, &goods, 3, -1);
    assert(display_identified == 128 && drag->identified == (char)0x80);
    assert(drag->source == 31 && drag->drop_count == 3 && drag->count == -1);
    assert(drag->type == 4 && drag->price == 25 && drag->shop_index == 72 && !drag->refine);
    assert(!strcmp(string_cstr(&drag->name), "501"));
    assert(!strcmp(string_cstr(&drag->display_name), "Red Potion"));
    barter_goods_ex_construct(&expanded);
    string_assign_cstr(&expanded.id_text, "501");
    expanded.type = 5; expanded.price = 99; expanded.key = 73; expanded.refine = 9;
    game_mode_drag_expanded_barter_item(mode, 32, &expanded, 7, 2);
    assert(drag->source == 32 && drag->drop_count == 7 && drag->count == 2 && drag->refine == 9);
    assert(drag->type == 5 && drag->price == 99 && drag->shop_index == 73 && display_identified == 1);
    assert(closes == 2 && !drag->index && !drag->named);
    barter_goods_ex_destruct(&expanded); barter_goods_destruct(&goods);
    string_free(&drag->name2); string_free(&drag->name); string_free(&drag->display_name);
    free(mode);
    assert(!allocations);
}

int main(void)
{
    ownership_and_conversion();
    packets();
    dragging();
    puts("barter records: ownership, conversion, packets and dragging passed");
    return 0;
}
