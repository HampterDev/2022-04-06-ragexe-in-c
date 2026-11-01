#include "iteminfo/item_api.h"
#include "session/state.h"
#include <stdlib.h>
#include <string.h>
#include "platform/win_types.h"
#include "game/mode_msg.h"
#include "session/session_items.h"
#include "ui/ui_std.h"
#include "ui/shops/merchant_shop_make.h"
#include "platform/clock.h"

#define CZ_REQ_OPENSTORE2               0x1b2
#define CZ_PC_PURCHASE_ITEMLIST_FROMMC2 0x801
#define CZ_REQ_BUYING_STORE_OPEN        0x811
#define CZ_REQ_CLICK_TO_BUYING_STORE    0x817
#define CZ_REQ_TRADE_BUYING_STORE       0x819
#define CZ_REQ_BUY_FROMMC               0x130
#define CZ_REQ_CREATE_ASSISTANT_VENDING 0xa7f
#define CZ_REQ_CREATE_ASSISTANT_BUYING  0xa92

#define LIST_PACKET_MAX 0x800
#define SHOP_TITLE_MAX 80

#pragma pack(push, 1)
struct OpenStoreEntry {
    unsigned short index;
    unsigned short count;
    int price;
};
struct BuyingEntry {
    int item_id;
    unsigned short count;
    int price;
};
#pragma pack(pop)

int game_mode_msg_open_vending(const char *title, int open)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        unsigned short len;
        char title[SHOP_TITLE_MAX];
        unsigned char open;
        struct OpenStoreEntry entries[(LIST_PACKET_MAX - 0x55) / sizeof(struct OpenStoreEntry)];
    } packet;
#pragma pack(pop)
    struct ItemInfo item;
    struct OpenStoreEntry *out;
    int count = session_get_store_setup_size(&g_session1);
    int i;

    packet.id = CZ_REQ_OPENSTORE2;
    packet.len = (unsigned short)(count * 8 + 0x55);
    if (open == 1) {
        strncpy(packet.title, title, 0x4f);
        packet.title[SHOP_TITLE_MAX - 1] = 0;
        packet.open = 1;
    } else {
        packet.title[0] = 0;
        packet.open = 0;
    }
    string_assign_cstr(&g_session1.shopTitle, packet.title);
    out = packet.entries;
    for (i = 0; i < count; i++) {
        session_get_store_setup_item_at(&g_session1, &item, i);
        if (item.index) {
            out->index = (unsigned short)item.index;
            out->count = (unsigned short)item.count;
            out->price = item.charged_price;
            out++;
        }
        item_info_destruct(&item);
    }
    if (count == 0)
        packet.open = 0;
    send_raw((short)packet.len, &packet);
    return 0;
}

int game_mode_msg_buy_from_vendor(int aid, int store_id)
{
#pragma pack(push, 1)
    struct PurchaseEntry {
        unsigned short count;
        unsigned short index;
    };
    struct {
        unsigned short id;
        unsigned short len;
        int aid;
        int store_id;
        struct PurchaseEntry entries[(LIST_PACKET_MAX - 0xc) / sizeof(struct PurchaseEntry)];
    } packet;
#pragma pack(pop)
    struct ItemInfo item;
    unsigned len;
    int count, i;

    session_copy_store_purchase_to_sent(&g_session1);
    count = session_purchase_count(&g_session1);
    len = count * 4 + 0xc;
    if (len > LIST_PACKET_MAX)
        return 0;
    packet.id = CZ_PC_PURCHASE_ITEMLIST_FROMMC2;
    packet.len = (unsigned short)len;
    packet.aid = aid;
    packet.store_id = store_id;
    for (i = 0; i < count; i++) {
        session_purchase_item(&g_session1, &item, i);
        if (item.index) {
            packet.entries[i].count = (unsigned short)item.count;
            packet.entries[i].index = (unsigned short)item.index;
        }
        item_info_destruct(&item);
    }
    if (count == 0)
        return 0;
    send_raw((short)len, &packet);
    return 0;
}

static int store_browse_allowed(struct GameMode *self)
{
    if (platform_ticks_ms() <= self->store_browse_time + 1000)
        return 0;
    self->store_browse_time = platform_ticks_ms();
    return 1;
}

int game_mode_msg_browse_vendor(struct GameMode *self, int aid)
{
    if (aid == g_session1.ownAid || g_windowMgr.merchant_item_shop_wnd)
        return 0;
    if (!store_browse_allowed(self))
        return 0;
    send_u32(CZ_REQ_BUY_FROMMC, aid);
    return 0;
}

int game_mode_msg_click_buying_store(struct GameMode *self, int aid)
{
    if (aid == g_session1.ownAid || g_windowMgr.buying_store_item_shop_wnd)
        return 0;
    if (!store_browse_allowed(self))
        return 0;
    send_u32(CZ_REQ_CLICK_TO_BUYING_STORE, aid);
    return 0;
}

int game_mode_msg_open_buying_store(const char *title, int open, int limit)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        unsigned short len;
        int limit;
        unsigned char open;
        char title[SHOP_TITLE_MAX];
        struct BuyingEntry entries[(LIST_PACKET_MAX - 0x59) / sizeof(struct BuyingEntry)];
    } packet;
#pragma pack(pop)
    struct ItemInfo item;
    struct BuyingEntry *out;
    int count = session_get_store_setup_size(&g_session1);
    int i;

    packet.id = CZ_REQ_BUYING_STORE_OPEN;
    packet.len = (unsigned short)(count * 10 + 0x59);
    if (open == 1) {
        strncpy(packet.title, title, 0x4f);
        packet.title[SHOP_TITLE_MAX - 1] = 0;
        packet.limit = limit;
        packet.open = 1;
    } else {
        packet.open = 0;
        packet.title[0] = 0;
        packet.limit = 0;
    }
    string_assign_cstr(&g_session1.shopTitle, packet.title);
    out = packet.entries;
    for (i = 0; i < count; i++) {
        session_get_store_setup_item_at(&g_session1, &item, i);
        if (item.index) {
            out->item_id = atoi(string_cstr(&item.id_text));
            out->count = (unsigned short)item.count;
            out->price = item.charged_price;
            out->count = (unsigned short)item.count;
            out++;
        }
        item_info_destruct(&item);
    }
    if (count == 0)
        packet.open = 0;
    send_raw((short)packet.len, &packet);
    return 0;
}

int game_mode_msg_trade_buying_store(int aid, int store_id)
{
#pragma pack(push, 1)
    struct TradeEntry {
        unsigned short index;
        int item_id;
        unsigned short count;
    };
    struct {
        unsigned short id;
        unsigned short len;
        int aid;
        int store_id;
        struct TradeEntry entries[(LIST_PACKET_MAX - 0xc) / sizeof(struct TradeEntry)];
    } packet;
#pragma pack(pop)
    struct TradeEntry entry;
    struct ItemInfo item;
    unsigned len;
    int count, i;

    session_copy_store_purchase_to_sent(&g_session1);
    count = session_purchase_count(&g_session1);
    packet.id = CZ_REQ_TRADE_BUYING_STORE;
    packet.aid = aid;
    len = count * 8 + 0xc;
    packet.len = (unsigned short)len;
    if (len > LIST_PACKET_MAX)
        return 0;
    packet.store_id = store_id;
    for (i = 0; i < count; i++) {
        session_purchase_item(&g_session1, &item, i);
        if (item.index) {
            entry.index = (unsigned short)item.index;
            entry.item_id = item_info_get_item_id(&item);
            entry.count = (unsigned short)item.count;
            packet.entries[i] = entry;
        }
        item_info_destruct(&item);
    }
    if (count == 0)
        return 0;
    send_raw((short)len, &packet);
    return 0;
}

int game_mode_msg_create_assistant_vending(const char *title)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        unsigned short len;
        char title[SHOP_TITLE_MAX];
        unsigned short cell_x;
        unsigned short cell_y;
        struct OpenStoreEntry entries[(LIST_PACKET_MAX - 0x58) / sizeof(struct OpenStoreEntry)];
    } packet;
#pragma pack(pop)
    struct ItemInfo item;
    struct OpenStoreEntry *out;
    struct UIWindow *wnd;
    int count, i;

    memset(&packet, 0, LIST_PACKET_MAX);
    count = session_get_store_setup_size(&g_session1);
    packet.id = CZ_REQ_CREATE_ASSISTANT_VENDING;
    packet.len = (unsigned short)(count * 8 + 0x58);
    strncpy(packet.title, title, 0x4f);
    packet.title[SHOP_TITLE_MAX - 1] = 0;
    wnd = g_windowMgr.merchant_shop_make_wnd;
    if (wnd) {
        int x = -1, y = -1;

        ui_merchant_shop_make_wnd_get_chosen_cell((const struct UIMerchantShopMakeWnd *)wnd, &x, &y);
        packet.cell_x = (unsigned short)x;
        packet.cell_y = (unsigned short)y;
    }
    out = packet.entries;
    for (i = 0; i < count; i++) {
        session_get_store_setup_item_at(&g_session1, &item, i);
        if (item.index) {
            out->index = (unsigned short)item.index;
            out->count = (unsigned short)item.count;
            out->price = item.charged_price;
            out++;
        }
        item_info_destruct(&item);
    }
    send_raw((short)packet.len, &packet);
    return 0;
}

int game_mode_msg_create_assistant_buying(const char *title, int limit)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        unsigned short len;
        int limit;
        int limit_high;
        char title[SHOP_TITLE_MAX];
        unsigned short cell_x;
        unsigned short cell_y;
        struct BuyingEntry entries[(LIST_PACKET_MAX - 0x60) / sizeof(struct BuyingEntry)];
        unsigned char tail[(LIST_PACKET_MAX - 0x60) % sizeof(struct BuyingEntry)];
    } packet;
#pragma pack(pop)
    struct ItemInfo item;
    struct BuyingEntry *out;
    struct UIWindow *wnd;
    int count, i;

    memset(&packet, 0, LIST_PACKET_MAX);
    count = session_get_store_setup_size(&g_session1);
    packet.limit_high = 0;
    packet.id = CZ_REQ_CREATE_ASSISTANT_BUYING;
    packet.limit = limit;
    packet.len = (unsigned short)(count * 10 + 0x60);
    strncpy(packet.title, title, 0x4f);
    packet.title[SHOP_TITLE_MAX - 1] = 0;
    wnd = g_windowMgr.buying_store_make_wnd;
    if (wnd) {
        int x = -1, y = -1;

        ui_merchant_shop_make_wnd_get_chosen_cell((const struct UIMerchantShopMakeWnd *)wnd, &x, &y);
        packet.cell_x = (unsigned short)x;
        packet.cell_y = (unsigned short)y;
    }
    string_assign_cstr(&g_session1.shopTitle, packet.title);
    out = packet.entries;
    for (i = 0; i < count; i++) {
        session_get_store_setup_item_at(&g_session1, &item, i);
        if (item.index) {
            out->item_id = atoi(string_cstr(&item.id_text));
            out->count = (unsigned short)item.count;
            out->price = item.charged_price;
            out++;
        }
        item_info_destruct(&item);
    }
    send_raw((short)packet.len, &packet);
    return 0;
}
