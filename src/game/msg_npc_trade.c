#include "world/mode_manager.h"
#include "game/context.h"
#include "ui/window_access.h"
#include <stdlib.h>
#include <string.h>
#include "game/mode_msg.h"
#include "web/barter_market.h"
#include "web/barter_expanded.h"
#include "session/session_items.h"

#define CZ_ACK_SELECT_DEALTYPE                  0xc5
#define CZ_PC_PURCHASE_ITEMLIST                 0xc8
#define CZ_PC_SELL_ITEMLIST                     0xc9
#define CZ_PC_BUY_CASH_POINT_ITEM               0x288
#define CZ_ITEMLISTWIN_RES                      0x7e4
#define CZ_NPC_MARKET_PURCHASE                  0x9d6
#define CZ_NPC_MARKETGEAR_PURCHASE              0xaaa
#define CZ_NPC_BARTER_MARKET_PURCHASE           0xb0f
#define CZ_NPC_EXPANDED_BARTER_MARKET_PURCHASE  0xb57

#define LIST_PACKET_MAX 0x800

enum {
    WID_ITEM_INFO = 0xc,
    WID_NPC_DIALOG = 0x10,
    WID_NPC_MENU = 0x11,
    WID_NPC_STORE = 0x16,
    WID_NPC_BUY_LIST = 0x17,
    WID_NPC_SELL_LIST = 0x18,
    WID_NPC_DEAL_TYPE = 0x19,
    WID_VENDING_SHOP = 0x2b,
    WID_VENDING_PURCHASE = 0x2c,
    WID_BUYING_SHOP = 0xb1,
    WID_BUYING_SELL = 0xb2,
    WID_BUYING_INFO = 0xb3,
    WID_NPC_MARKET_BUY = 0xfe,
    WID_NPC_MARKET_SELL = 0xff,
    WID_NPC_MARKET_RESULT = 0x100,
    WID_BARTER_MARKET = 0x14e,
    WID_BARTER_PURCHASE = 0x14f,
    WID_EXPANDED_BARTER_MARKET = 0x155,
    WID_EXPANDED_BARTER_PURCHASE = 0x156,
};

static void set_window_shown(struct UIWindow *w, int arg)
{
    ui_window_show(w, arg);
}

#pragma pack(push, 1)
struct ListHeader {
    unsigned short id;
    unsigned short len;
};
#pragma pack(pop)

int game_mode_msg_npc_sell(struct GameMode *self)
{
    struct {
        struct ListHeader h;
        struct { unsigned short index, count; } items[(LIST_PACKET_MAX - 4) / 4];
    } packet;
    struct ItemInfo item;
    int count = session_get_shop_sell_list_size(&g_session1);
    unsigned len = count * 4 + 4;
    int i;

    if (len > LIST_PACKET_MAX)
        return 0;
    packet.h.id = CZ_PC_SELL_ITEMLIST;
    packet.h.len = (unsigned short)len;
    for (i = 0; i < count; i++) {
        session_get_shop_sell_item_at(&g_session1, &item, i);
        packet.items[i].index = (unsigned short)item.index;
        packet.items[i].count = (unsigned short)item.count;
        item_info_destruct(&item);
    }
    if (count != 0 && !self->player_dead)
        send_raw((short)len, &packet);
    session_clear_lists(&g_session1);
    close_window(WID_NPC_DEAL_TYPE);
    close_window(WID_NPC_STORE);
    close_window(WID_NPC_SELL_LIST);
    return 0;
}

int game_mode_msg_item_list_win_send(struct GameMode *self, int window_type)
{
    struct {
        struct ListHeader h;
        int type;
        int action;
        struct { unsigned short index, count; } items[(LIST_PACKET_MAX - 12) / 4];
    } packet;
    struct ItemInfo item;
    int count = session_get_shop_sell_list_size(&g_session1);
    unsigned len = count * 4 + 12;
    int i;

    (void)self;
    if (len > LIST_PACKET_MAX)
        return 0;
    packet.h.id = CZ_ITEMLISTWIN_RES;
    packet.h.len = (unsigned short)len;
    packet.type = window_type;
    packet.action = 1;
    for (i = 0; i < count; i++) {
        session_get_shop_sell_item_at(&g_session1, &item, i);
        packet.items[i].index = (unsigned short)item.index;
        packet.items[i].count = (unsigned short)item.count;
        item_info_destruct(&item);
    }
    if (count != 0)
        send_raw((short)len, &packet);
    session_clear_lists(&g_session1);
    return 0;
}

int game_mode_msg_item_list_win_cancel(int window_type)
{
    struct {
        struct ListHeader h;
        int type;
        int action;
    } packet;

    packet.h.id = CZ_ITEMLISTWIN_RES;
    packet.h.len = sizeof packet;
    packet.type = window_type;
    packet.action = 0;
    send_raw(sizeof packet, &packet);
    session_clear_lists(&g_session1);
    return 0;
}

int game_mode_msg_npc_buy(struct GameMode *self)
{
#pragma pack(push, 1)
    struct {
        struct ListHeader h;
        struct { unsigned short count; int item_id; } items[(LIST_PACKET_MAX - 4) / 6];
    } packet;
#pragma pack(pop)
    struct ItemInfo item;
    int count = session_get_shop_buy_list_size(&g_session1);
    unsigned len = count * 6 + 4;
    int i;

    if (len > LIST_PACKET_MAX)
        return 0;
    packet.h.id = CZ_PC_PURCHASE_ITEMLIST;
    packet.h.len = (unsigned short)len;
    for (i = 0; i < count; i++) {
        session_get_shop_buy_item_at(&g_session1, &item, i);
        packet.items[i].count = (unsigned short)item.count;
        packet.items[i].item_id = atoi(string_cstr(&item.id_text));
        item_info_destruct(&item);
    }
    if (count != 0 && !self->player_dead)
        send_raw((short)len, &packet);
    session_clear_lists(&g_session1);
    close_window(WID_NPC_DEAL_TYPE);
    close_window(WID_NPC_STORE);
    close_window(WID_NPC_BUY_LIST);
    return 0;
}

int game_mode_msg_buy_cash_item(struct GameMode *self, int points)
{
#pragma pack(push, 1)
    struct {
        struct ListHeader h;
        int points;
        unsigned short count;
        struct { unsigned short count; int item_id; } items[(LIST_PACKET_MAX - 10) / 6];
    } packet;
#pragma pack(pop)
    struct ItemInfo item;
    int count;
    unsigned len;
    int i;

    if (session_get_shop_buy_list_size(&g_session1) == 0 || self->player_dead)
        goto done;
    count = session_get_shop_buy_list_size(&g_session1);
    len = count * 6 + 10;
    if (len > LIST_PACKET_MAX)
        return 0;
    packet.h.id = CZ_PC_BUY_CASH_POINT_ITEM;
    packet.h.len = (unsigned short)len;
    packet.points = points;
    packet.count = (unsigned short)count;
    for (i = 0; i < count; i++) {
        session_get_shop_buy_item_at(&g_session1, &item, i);
        packet.items[i].count = (unsigned short)item.count;
        packet.items[i].item_id = atoi(string_cstr(&item.id_text));
        item_info_destruct(&item);
    }
    if (count != 0 && !self->player_dead)
        send_raw((short)len, &packet);
done:
    session_clear_lists(&g_session1);
    return 0;
}

int game_mode_msg_select_deal_type(struct GameMode *self, int npc_id, int type)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        int npc_id;
        unsigned char type;
    } packet;
#pragma pack(pop)

    packet.id = CZ_ACK_SELECT_DEALTYPE;
    packet.npc_id = npc_id;
    packet.type = (unsigned char)type;
    if (self->player_dead)
        return 0;
    send_packet(CZ_ACK_SELECT_DEALTYPE, &packet);
    return 0;
}

int game_mode_msg_npc_trade_closed(struct GameMode *self)
{
    static const int trade_windows[] = {
        WID_NPC_DEAL_TYPE, WID_NPC_STORE, WID_NPC_BUY_LIST, WID_NPC_SELL_LIST,
        WID_VENDING_SHOP, WID_VENDING_PURCHASE, WID_ITEM_INFO,
        WID_BUYING_SHOP, WID_BUYING_SELL, WID_BUYING_INFO,
        WID_NPC_MARKET_BUY, WID_NPC_MARKET_RESULT, WID_NPC_MARKET_SELL,
        WID_BARTER_MARKET, WID_BARTER_PURCHASE, WID_EXPANDED_BARTER_MARKET, WID_EXPANDED_BARTER_PURCHASE,
    };
    struct GameMode *mode;
    unsigned i;

    self->npc_dialog_open = 0;
    if (g_windowMgr.choose_wnd) {
        ui_window_send(g_windowMgr.choose_wnd, NULL, 0xa6, 0, 0, 0, 0);
        close_window(WID_NPC_MENU);
    }
    if (g_windowMgr.say_dialog_wnd) {
        mode = game_current_mode();
        if (mode)
            self_msg(mode, MODE_MSG_CLOSE_NPC_DIALOG, 0, 0, 0, 0);
    }
    for (i = 0; i < sizeof trade_windows / sizeof trade_windows[0]; i++)
        close_window(trade_windows[i]);
    self_msg(self, MODE_MSG_SHOW_ILLUSTRATION, (int)"", -1, 0, 0);
    return 0;
}

int game_mode_msg_npc_store_windows_closed(struct GameMode *self)
{
    self->npc_dialog_open = 0;
    close_window(WID_VENDING_SHOP);
    close_window(WID_VENDING_PURCHASE);
    close_window(WID_ITEM_INFO);
    close_window(WID_BUYING_SHOP);
    close_window(WID_BUYING_SELL);
    close_window(WID_BUYING_INFO);
    return 0;
}

static void market_purchase_done(int count, int checked)
{
    struct UIWindow *sell;

    if (count > 0) {
        if (checked) {
            sell = ui_window_find(WID_NPC_MARKET_SELL);
            if (sell)
                set_window_shown(sell, 0);
        } else {
            sell = ui_window_find(WID_NPC_MARKET_SELL);
            set_window_shown(sell, 0);
        }
        close_window(WID_ITEM_INFO);
    } else {
        close_window(WID_NPC_MARKET_SELL);
        if (!checked || client_current_mode())
            mode_send_message(client_current_mode(), MODE_MSG_NPC_TRADE_CLOSED, 0, 0, 0, 0);
    }
    session_clear_lists(&g_session1);
    close_window(WID_NPC_MARKET_BUY);
}

int game_mode_msg_market_gear_purchase(struct GameMode *self)
{
#pragma pack(push, 1)
    struct {
        struct ListHeader h;
        unsigned char body[LIST_PACKET_MAX - 4];
    } packet;
    struct GearEntry { int item_id; int amount; unsigned short index; } *out;
#pragma pack(pop)
    struct ItemInfo item;
    int count, i;
    unsigned len;

    memset(packet.body, 0, sizeof packet.body);
    count = session_get_shop_buy_list_size(&g_session1);
    len = count * 10 + 4;
    if (len > LIST_PACKET_MAX)
        return 0;
    packet.h.id = CZ_NPC_MARKETGEAR_PURCHASE;
    packet.h.len = (unsigned short)len;
    if (count > 0) {
        out = (struct GearEntry *)packet.body;
        for (i = 0; i < count; i++, out++) {
            session_get_shop_buy_item_at(&g_session1, &item, i);
            out->item_id = atoi(string_cstr(&item.id_text));
            out->amount = item.count;
            out->index = (unsigned short)item.index;
            item_info_destruct(&item);
        }
        if (!self->player_dead)
            send_raw((short)len, &packet);
    }
    market_purchase_done(count, 1);
    return 0;
}

int game_mode_msg_market_purchase(struct GameMode *self)
{
    struct {
        struct ListHeader h;
        struct { int item_id; int amount; } items[(LIST_PACKET_MAX - 4) / 8];
        unsigned char tail[(LIST_PACKET_MAX - 4) % 8];
    } packet;
    struct ItemInfo item;
    int count, i;
    unsigned len;

    memset(packet.items, 0, LIST_PACKET_MAX - 4);
    count = session_get_shop_buy_list_size(&g_session1);
    len = count * 8 + 4;
    if (len > LIST_PACKET_MAX)
        return 0;
    packet.h.id = CZ_NPC_MARKET_PURCHASE;
    packet.h.len = (unsigned short)len;
    if (count > 0) {
        for (i = 0; i < count; i++) {
            session_get_shop_buy_item_at(&g_session1, &item, i);
            packet.items[i].item_id = atoi(string_cstr(&item.id_text));
            packet.items[i].amount = item.count;
            item_info_destruct(&item);
        }
        if (!self->player_dead)
            send_raw((short)len, &packet);
    }
    market_purchase_done(count, 0);
    return 0;
}

int game_mode_msg_barter_purchase(struct GameMode *self)
{
#pragma pack(push, 1)
    struct BarterPurchaseEntry { int item_id; int amount; unsigned short currency_index; int shop_index; } *out;
#pragma pack(pop)
    unsigned char packet[LIST_PACKET_MAX];
    struct ListHeader header;
    struct List *cart;
    struct ListNode *node;
    struct BarterGoods item;
    const struct BarterGoods *src;
    int count;
    unsigned len;

    memset(packet, 0, sizeof packet);
    count = barter_cart_size(g_barter);
    len = count * 14 + 4;
    if (len > LIST_PACKET_MAX)
        return 0;
    header.id = CZ_NPC_BARTER_MARKET_PURCHASE;
    header.len = (unsigned short)len;
    memcpy_s(packet, sizeof packet, &header, sizeof header);
    cart = barter_cart(g_barter);
    out = (struct BarterPurchaseEntry *)(packet + 4);
    for (node = cart->head->next; node != cart->head; node = node->next, out++) {
        src = LIST_VALUE(node, struct BarterGoods);
        barter_goods_copy(&item, src);
        out->item_id = atoi(string_cstr(&item.id_text));
        out->amount = item.count;
        out->currency_index = (unsigned short)session_get_inventory_index_by_id(&g_session1, item.pay_id);
        out->shop_index = item.key;
        barter_goods_destruct(&item);
    }
    if (count <= 0 || self->player_dead)
        return 0;
    send_raw((short)len, packet);
    return 0;
}

int game_mode_msg_expanded_barter_purchase(struct GameMode *self)
{
    struct ExpandedEntry { int item_id; int shop_index; int amount; } *out;
    unsigned char packet[LIST_PACKET_MAX];
    struct ListHeader header;
    struct Vec *cart;
    const struct BarterGoodsEx *src;
    struct BarterGoodsEx item;
    int count;
    unsigned len;

    memset(packet, 0, sizeof packet);
    count = expanded_barter_cart_size(g_expandedBarter);
    len = count * 12 + 4;
    if (len > LIST_PACKET_MAX)
        return 0;
    header.id = CZ_NPC_EXPANDED_BARTER_MARKET_PURCHASE;
    header.len = (unsigned short)len;
    memcpy_s(packet, sizeof packet, &header, sizeof header);
    cart = expanded_barter_cart(g_expandedBarter);
    out = (struct ExpandedEntry *)(packet + 4);
    for (src = cart->first; src != cart->last; src++, out++) {
        barter_goods_ex_copy(&item, src);
        out->item_id = atoi(string_cstr(&item.id_text));
        out->shop_index = item.key;
        out->amount = item.count;
        barter_goods_ex_destruct(&item);
    }
    if (count <= 0 || self->player_dead)
        return 0;
    send_raw((short)len, packet);
    return 0;
}
