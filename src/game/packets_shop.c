#include "game/packet_handlers.h"
#include "ui/window_types.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include "platform/win_types.h"
#include <stdlib.h>
#include <string.h>
#include "game/packets.h"
#include "game/actor_packets.h"
#include "session/session_items.h"
#include "ui/ui_std.h"
#include "ui/wndmgr/wndmgr.h"
#include "game/store_entries.h"

#define WND_SHOP_BUY            0x16
#define WND_SHOP_BUY_CART       0x17
#define WND_SHOP_SELL           0x18
#define WND_CASH_SHOP           0x13e
#define SHOP_MSG_SET_MODE         0x24
#define WND_MSG_SHOW            0x17

#define CHAT_COLOR_RED          0xff
#define CHAT_COLOR_YELLOW       0xffff00

static void chat_result(int msg, int color)
{
    ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
    ui_window_mgr_send_msg_(&g_windowMgr, 1, (intptr_t)msg_str(msg), color, 0, 0);
}

static void shop_window_refresh(void *wnd, int a, int b)
{
    wnd_send(wnd, SHOP_MSG_SET_MODE, a, b, 0, 0);
    wnd_send(wnd, WND_MSG_SHOW, 0, 0, 0, 0);
}

#pragma pack(push, 1)
struct ShopEntry2 {
    int item_id;
    int price;
    int discount_price;
    unsigned char type;
    unsigned short view;
    int location;
};
struct ShopEntry {
    int price;
    int discount_price;
    unsigned char type;
    int item_id;
};
#pragma pack(pop)
SIZE_ASSERT(ShopEntry2, 19);
SIZE_ASSERT(ShopEntry, 13);

#pragma pack(push, 1)
struct PACKET_ZC_PC_PURCHASE_ITEMLIST2 {
    unsigned short packet_type;
    short packet_length;
    struct ShopEntry2 entries[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_PC_PURCHASE_ITEMLIST2, 4);

void game_mode_zc_pc_purchase_itemlist2(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_PC_PURCHASE_ITEMLIST2 *packet = (const struct PACKET_ZC_PC_PURCHASE_ITEMLIST2 *)pkt;
    const struct ShopEntry2 *entries = packet->entries;
    int count, i;
    void *wnd;

    (void)self;
    session_clear_lists(&g_session1);
    count = (packet->packet_length - 4) / (int)sizeof *entries;
    for (i = 0; i < count; i++) {
        struct ShopEntry2 entry = entries[i];
        struct ItemRecordA item;

        item_record_a_clear(&item);
        _itoa(entry.item_id, item.id_text, 10);
        item.price = entry.price;
        item.charged_price = entry.discount_price;
        item.type = entry.type;
        item.equip_location = entry.location;
        item.view = entry.view;
        item.index = -1;
        item.identified = 1;
        session_append_shop_goods_record(&g_session1, &item, 0);
    }
    wnd = ui_window_open(WND_SHOP_BUY);
    if (wnd)
        shop_window_refresh(wnd, 0, 0);
    wnd = ui_window_open(WND_SHOP_BUY_CART);
    if (wnd)
        shop_window_refresh(wnd, 0, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_PC_PURCHASE_ITEMLIST {
    unsigned short packet_type;
    short packet_length;
    struct ShopEntry entries[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_PC_PURCHASE_ITEMLIST, 4);

void game_mode_zc_pc_purchase_itemlist(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_PC_PURCHASE_ITEMLIST *packet = (const struct PACKET_ZC_PC_PURCHASE_ITEMLIST *)pkt;
    const struct ShopEntry *entry = packet->entries;
    int count;

    (void)self;
    session_clear_lists(&g_session1);
    count = (int)((unsigned)(packet->packet_length - 4) / sizeof *entry);
    for (; count > 0; count--, entry++) {
        struct ShopEntry copy = *entry;
        struct ItemRecordA item;

        item_record_a_clear(&item);
        _itoa(copy.item_id, item.id_text, 10);
        item.price = copy.price;
        item.charged_price = copy.discount_price;
        item.type = copy.type;
        item.index = -1;
        item.identified = 1;
        session_append_shop_goods_record(&g_session1, &item, 0);
    }
    shop_window_refresh(ui_window_open(WND_SHOP_BUY), 0, 0);
    shop_window_refresh(ui_window_open(WND_SHOP_BUY_CART), 0, 0);
}

void game_mode_zc_pc_purchase_result(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_PC_PURCHASE_RESULT *packet = (const struct PACKET_ZC_PC_PURCHASE_RESULT *)pkt;

    (void)self;
    switch (packet->result) {
    case 0: chat_result(0x36, CHAT_COLOR_YELLOW); break;
    case 1: chat_result(0x37, CHAT_COLOR_RED); break;
    case 2: chat_result(0x38, CHAT_COLOR_RED); break;
    case 3: chat_result(0xdd, CHAT_COLOR_RED); break;
    case 9: chat_result(0x794, CHAT_COLOR_RED); break;
    case 10: chat_result(0xb65, CHAT_COLOR_YELLOW); break;
    case 11: chat_result(0xde2, CHAT_COLOR_RED); break;
    case 12: chat_result(0xde3, CHAT_COLOR_YELLOW); break;
    case 13: chat_result(0xde5, CHAT_COLOR_RED); break;
    case 14: chat_result(0xde4, CHAT_COLOR_RED); break;
    }
    ui_window_mgr_post_quit1(&g_windowMgr, WND_SHOP_BUY);
    ui_window_mgr_post_quit1(&g_windowMgr, WND_SHOP_BUY_CART);
}

#pragma pack(push, 1)
struct SellEntry {
    short index;
    int price;
    int overcharge_price;
};
#pragma pack(pop)
SIZE_ASSERT(SellEntry, 10);

#pragma pack(push, 1)
struct PACKET_ZC_PC_SELL_ITEMLIST {
    unsigned short packet_type;
    short packet_length;
    struct SellEntry entries[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_PC_SELL_ITEMLIST, 4);

void game_mode_zc_pc_sell_itemlist(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_PC_SELL_ITEMLIST *packet = (const struct PACKET_ZC_PC_SELL_ITEMLIST *)pkt;
    const struct SellEntry *entry = packet->entries;
    int count;

    (void)self;
    session_clear_lists(&g_session1);
    count = (int)((unsigned)(packet->packet_length - 4) / sizeof *entry);
    for (; count > 0; count--, entry++) {
        struct SellEntry copy = *entry;
        struct ItemInfo owned;

        session_get_inventory_item(&g_session1, &owned, copy.index);
        if (owned.index != 0 && !(owned.favorite && g_session1.blockPrivateTab)) {
            struct ItemRecordA item;

            item_record_a_clear(&item);
            item.index = copy.index;
            strcpy_s(item.id_text, sizeof item.id_text, string_cstr(&owned.id_text));
            item.price = copy.price;
            item.charged_price = copy.overcharge_price;
            item.type = owned.type;
            item.identified = owned.identified;
            session_append_shop_goods_record(&g_session1, &item, 1);
        }
        item_info_destruct(&owned);
    }
    shop_window_refresh(ui_window_open(WND_SHOP_BUY), 1, 0);
    {
        void *sell = ui_window_open(WND_SHOP_SELL);

        wnd_send(sell, WND_MSG_SHOW, 0, 0, 0, 0);
    }
}

#pragma pack(push, 1)
struct PACKET_ZC_PC_SELL_RESULT {
    unsigned short packet_type;
    unsigned char result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_PC_SELL_RESULT, 3);

void game_mode_zc_pc_sell_result(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_PC_SELL_RESULT *packet = (const struct PACKET_ZC_PC_SELL_RESULT *)pkt;

    (void)self;
    if (packet->result == 0)
        chat_result(0x36, CHAT_COLOR_YELLOW);
    else if (packet->result == 1)
        chat_result(0x39, CHAT_COLOR_RED);
    session_clear_lists(&g_session1);
    ui_window_mgr_post_quit1(&g_windowMgr, WND_SHOP_BUY);
    ui_window_mgr_post_quit1(&g_windowMgr, WND_SHOP_SELL);
}

#pragma pack(push, 1)
struct PACKET_ZC_PC_CASH_POINT_ITEMLIST {
    unsigned short packet_type;
    short packet_length;
    int cash_points;
    int kafra_points;
    struct ShopEntry entries[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_PC_CASH_POINT_ITEMLIST, 12);

void game_mode_zc_pc_cash_point_itemlist(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_PC_CASH_POINT_ITEMLIST *packet = (const struct PACKET_ZC_PC_CASH_POINT_ITEMLIST *)pkt;
    const struct ShopEntry *entry = packet->entries;
    int count;
    void *wnd;

    if (g_windowMgr.n_cash_wnd != 0 || ui_window_find(WND_CASH_SHOP))
        return;
    session_clear_lists(&g_session1);
    if (g_session1.channelCopId == 1) {
        ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
        ui_window_mgr_send_msg_(&g_windowMgr, 1,
            (intptr_t)"한게임 유저는 12월 3일부로 "
                 "유료아이템 구매가 불가능합니다.",
            CHAT_COLOR_RED, 0, 0);
        return;
    }
    count = (int)((unsigned)(packet->packet_length - 12) / sizeof *entry);
    for (; count > 0; count--, entry++) {
        struct ShopEntry copy = *entry;
        struct ItemRecordA item;

        item_record_a_clear(&item);
        _itoa(copy.item_id, item.id_text, 10);
        item.price = copy.price;
        item.charged_price = copy.discount_price;
        item.type = copy.type;
        item.index = -1;
        item.identified = 1;
        session_append_shop_goods_record(&g_session1, &item, 0);
    }
    g_session1.cashPoints = packet->cash_points;
    g_session1.freePoints = packet->kafra_points;
    self->npc_dialog_open = 1;
    wnd = ui_window_open(WND_SHOP_BUY);
    shop_window_refresh(wnd, 0, 1);
    wnd = ui_window_open(WND_SHOP_BUY_CART);
    shop_window_refresh(wnd, 1, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_PC_CASH_POINT_UPDATE {
    unsigned short packet_type;
    int cash_points;
    int kafra_points;
    short result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_PC_CASH_POINT_UPDATE, 12);

void game_mode_zc_pc_cash_point_update(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_PC_CASH_POINT_UPDATE *packet = (const struct PACKET_ZC_PC_CASH_POINT_UPDATE *)pkt;
    struct String line;
    int color = CHAT_COLOR_RED;
    const char *text = 0;
    short result = packet->result;

    (void)self;
    string_init(&line);
    switch (result) {
    case 0:
    case 8:
        g_session1.cashPoints = packet->cash_points;
        g_session1.freePoints = packet->kafra_points;
        if (g_windowMgr.item_shop_wnd)
            ui_window_invalidate(g_windowMgr.item_shop_wnd);
        if (result == 8) {
            text = msg_str(0x716);
        } else {
            text = msg_str(0x36);
            color = CHAT_COLOR_YELLOW;
        }
        break;
    case 1: text = msg_str(0x4cb); break;
    case 2: text = msg_str(0x4cc); break;
    case 3: text = msg_str(0x38); break;
    case 4: text = msg_str(0x4cd); break;
    case 5: text = msg_str(0x4ce); break;
    case 6: text = msg_str(0x4e6); break;
    case 7: text = msg_str(0x715); break;
    case 9: text = msg_str(0x624); break;
    }
    if (text)
        string_assign_cstr(&line, text);
    if (line.size != 0) {
        ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (intptr_t)string_cstr(&line), color, 0, 0);
    }
    string_free(&line);
}

static void cash_error_line(struct String *line, int code, int msg)
{
    string_format(line, "- %d : %s", code, msg_str(msg));
}

#pragma pack(push, 1)
struct PACKET_ZC_SE_PC_BUY_CASHITEM_RESULT {
    unsigned short packet_type;
    int item_id;
    short result;
    int cash_points;
    int kafra_points;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_SE_PC_BUY_CASHITEM_RESULT, 16);

void game_mode_zc_se_pc_buy_cashitem_result(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_SE_PC_BUY_CASHITEM_RESULT *packet = (const struct PACKET_ZC_SE_PC_BUY_CASHITEM_RESULT *)pkt;
    struct String line;
    int result = packet->result;

    (void)self;
    string_init(&line);
    if (result == 0 || result == 8) {
        void *shop = ui_window_find(WND_CASH_SHOP);

        if (shop)
            wnd_send(shop, 0x7a, packet->cash_points, packet->kafra_points, 0, 0);
        if (packet->result == 8) {
            const char *text = msg_str(0x716);

            string_assign_cstr(&line, text);
        }
    } else if (result >= 9 && result <= 12) {
        string_assign_cstr(&line, msg_str(0x798 + (result - 9)));
    } else if (result == 0x12f || result == 0x130) {
        cash_error_line(&line, result, result == 0x12f ? 0xf64 : 0xf65);
        ui_window_mgr_message_box(&g_windowMgr, string_cstr(&line), 0, 0, 0, 0, "Error", 0x118, 0x78, 0);
    } else {
        const char *text = msg_str(0x39);

        string_assign_cstr(&line, text);
    }
    if (line.size != 0) {
        ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (intptr_t)string_cstr(&line), CHAT_COLOR_RED, 0, 0);
    }
    string_free(&line);
}

#define CZ_SE_CASHSHOP_CLOSE    0x844

#pragma pack(push, 1)
struct PACKET_ZC_SE_CASHSHOP_OPEN {
    unsigned short packet_type;
    int cash_points;
    int kafra_points;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_SE_CASHSHOP_OPEN, 10);

void game_mode_zc_se_cashshop_open(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_SE_CASHSHOP_OPEN *packet = (const struct PACKET_ZC_SE_CASHSHOP_OPEN *)pkt;
    void *shop;
    unsigned short close_packet;
    int length;

    (void)self;
    if (ui_window_mgr_post_quit1(&g_windowMgr, WND_CASH_SHOP))
        return;
    shop = ui_window_open(WND_CASH_SHOP);
    if (shop) {
        wnd_send(shop, 0x78, packet->cash_points, packet->kafra_points, 0, 0);
        return;
    }
    close_packet = CZ_SE_CASHSHOP_CLOSE;
    length = rag_connection_get_packet_length(rag_connection_instance(), CZ_SE_CASHSHOP_CLOSE);
    rag_connection_send_packet(rag_connection_instance(), length, &close_packet);
}

#pragma pack(push, 1)
struct PACKET_ZC_SE_CASHSHOP_POINTS {
    unsigned short packet_type;
    int cash_points;
    int kafra_points;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_SE_CASHSHOP_POINTS, 10);

void game_mode_zc_se_cashshop_points(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_SE_CASHSHOP_POINTS *packet = (const struct PACKET_ZC_SE_CASHSHOP_POINTS *)pkt;
    void *shop = ui_window_find(WND_CASH_SHOP);

    (void)self;
    if (shop)
        wnd_send(shop, 0x7a, packet->cash_points, packet->kafra_points, 0, 0);
}
