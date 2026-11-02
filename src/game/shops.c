#include "game/gamemode.h"
#include "game/mode_callbacks.h"
#include "game/ground_item_packets.h"
#include "session/shop_inventory.h"
#include "ui/window_messages.h"
#include "game/actor_packets.h"
#include "iteminfo/item_table.h"
#include "contents/sound.h"
#include "ui/window_access.h"
#include <string.h>
#include "game/packet_handlers.h"
#include "game/store_entries.h"
#include "game/client.h"
#include "game/item_info.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "session/session_items.h"
#include "ui/shops/merchant_shop_make.h"

#define UIWM_REFRESH 0x17
#define UIWM_SET_ZENY 0x77
#define UIWM_SET_OWNER 0x1c

#define WINDOW_BUYING_STORE 0xb0
#define WINDOW_BUYING_STORE_ITEMS 0xb1
#define WINDOW_BUYING_STORE_SELL 0xb2
#define WINDOW_BUYING_STORE_LIST 0xb3
#define WINDOW_BUYING_STORE_MAKE 0xae
#define WINDOW_BUYING_STORE_MAKE2 0xaf

static void play_sale_sound(void)
{
    struct Vec3 here = { 0, 0, 0 };
    sound_play_wave("effect\\ef_steal.wav", here.x, here.y, here.z, 0xfa, 0x28, 1.0f);
}

static void refresh_window(struct UIWindow *w)
{
    UIWINDOW_SENDMSG(w, UIWM_REFRESH, 0, 0, 0, 0);
}

static void chat_line(const char *text, unsigned colour)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (intptr_t)text, colour, 0, 0);
}

static void chat_msg(int msg_id, unsigned colour)
{
    chat_line(msg_str(msg_id), colour);
}

static void chat_refresh(void)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_REFRESH, 0, 0, 0, 0);
}

void game_mode_shop_item_sold(int date, unsigned short index, unsigned short amount, int zeny, int buyer_cid)
{
    struct ItemInfo item;
    struct String line;
    struct String name = {0};
    struct UIWindow *w, *shop, *items;
    int sold_out;
    int i = (short)index;

    session_get_shop_item_by_index(&g_session1, &item, i);
    item.count = (short)amount;
    item.index = i;
    item.price = zeny;
    w = ui_window_open(0x101);
    UIWINDOW_SENDMSG(w, UIWM_REFRESH, (intptr_t)&item, buyer_cid, date, 0);
    session_remove_own_store_item(&g_session1, &item);
    chat_refresh();
    item_info_get_display_name(&item, &name);
    string_init(&line);
    string_format(&line, msg_str(0xe7), string_cstr(&name), item.count);
    chat_line(string_cstr(&line), 0xffff64);
    string_free(&line);
    string_free(&name);

    shop = g_windowMgr.merchant_item_wnd;
    items = g_windowMgr.merchant_item_my_shop_wnd;
    if (shop != 0)
        refresh_window(shop);
    sold_out = session_shop_item_count(&g_session1) == 0;
    if (sold_out)
        chat_msg(0x137, 0xff00);
    if (items != 0) {
        refresh_window(items);
        if (sold_out)
            UIWINDOW_SENDMSG(items, 6, 0xc1, 0, 0, 0);
        if (g_session1.noSellWndSound)
            play_sale_sound();
    }
    item_info_destruct(&item);
}

#pragma pack(push, 1)
struct PACKET_ZC_DELETEITEM_FROM_MCSTORE2 {
    unsigned short packet_type;
    unsigned short index;
    unsigned short amount;
    int buyer_cid;
    int date;
    int zeny;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_DELETEITEM_FROM_MCSTORE2, 18);

void game_mode_shop_item_sold_packet(const unsigned char *pkt)
{
    const struct PACKET_ZC_DELETEITEM_FROM_MCSTORE2 *packet = (const struct PACKET_ZC_DELETEITEM_FROM_MCSTORE2 *)pkt;

    game_mode_shop_item_sold(packet->date, packet->index, packet->amount, packet->zeny, packet->buyer_cid);
}

void game_mode_buying_store_bought(int time, int item_id, int count, int zeny, int zeny_left, int seller_cid)
{
    struct ItemInfo item;
    struct UIWindow *w, *buying, *store;

    session_get_shop_item_by_id(&g_session1, &item, item_id);
    item_info_set_item_id(&item, item_id);
    item.count = count;
    item.price = zeny;
    w = ui_window_open(0x102);
    UIWINDOW_SENDMSG(w, UIWM_REFRESH, (intptr_t)&item, seller_cid, time, 0);
    session_remove_own_store_item_by_id(&g_session1, &item);
    buying = g_windowMgr.item_wnd;
    store = g_windowMgr.buying_store_item_my_shop_wnd;
    if (buying != 0)
        refresh_window(buying);
    if (store != 0) {
        UIWINDOW_SENDMSG(store, UIWM_SET_ZENY, zeny_left, 0, 0, 0);
        refresh_window(store);
        if (g_session1.noSellWndSound)
            play_sale_sound();
        if (session_shop_item_count(&g_session1) == 0)
            UIWINDOW_SENDMSG(store, 6, 0xc1, 0, 0, 0);
    }
    item_info_destruct(&item);
}

#pragma pack(push, 1)
struct PACKET_ZC_UPDATE_ITEM_FROM_BUYING_STORE2 {
    unsigned short packet_type;
    int item_id;
    short count;
    int zeny;
    int zeny_left;
    int seller_cid;
    int time;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_UPDATE_ITEM_FROM_BUYING_STORE2, 24);

void game_mode_buying_store_bought_packet(const unsigned char *pkt)
{
    const struct PACKET_ZC_UPDATE_ITEM_FROM_BUYING_STORE2 *packet =
        (const struct PACKET_ZC_UPDATE_ITEM_FROM_BUYING_STORE2 *)pkt;

    game_mode_buying_store_bought(packet->time, packet->item_id, packet->count, packet->zeny, packet->zeny_left,
                               packet->seller_cid);
}

#pragma pack(push, 1)
struct PACKET_ZC_MYITEMLIST_BUYING_STORE {
    unsigned short packet_type;
    short packet_length;
    int aid;
    int zeny_limit;
    struct BuyingStoreEntry entries[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_MYITEMLIST_BUYING_STORE, 12);

void game_mode_my_buying_store_list(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_MYITEMLIST_BUYING_STORE *packet = (const struct PACKET_ZC_MYITEMLIST_BUYING_STORE *)pkt;
    const struct BuyingStoreEntry *e = packet->entries;
    unsigned n = ((unsigned)(int)packet->packet_length - 0xc) / sizeof *e;
    struct ItemInfo item;
    struct UIWindow *w;
    int i;

    ui_merchant_shop_make_wnd_print_buy_list((struct UIMerchantShopMakeWnd *)g_windowMgr.buying_store_make_wnd);
    session_clear_store_setup(&g_session1);
    ui_window_mgr_post_quit1(&g_windowMgr, WINDOW_BUYING_STORE_MAKE);
    ui_window_mgr_post_quit1(&g_windowMgr, WINDOW_BUYING_STORE_MAKE2);
    session_clear_own_store_items(&g_session1);
    for (i = 0; i < (int)n; i++, e++) {
        buying_store_entry_to_item(&item, e, i + 1);
        session_add_own_store_item(&g_session1, &item);
        item_info_destruct(&item);
    }
    w = ui_window_open(WINDOW_BUYING_STORE);
    refresh_window(w);
    UIWINDOW_SENDMSG(w, UIWM_SET_ZENY, packet->zeny_limit, 0, 0, 0);
    self->npc_dialog_open = 1;
}

#pragma pack(push, 1)
struct PACKET_ZC_ACK_ITEMLIST_BUYING_STORE {
    unsigned short packet_type;
    short packet_length;
    int aid;
    int store_id;
    int zeny_limit;
    struct BuyingStoreEntry entries[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_ITEMLIST_BUYING_STORE, 16);

void game_mode_ack_buying_store_list(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_ACK_ITEMLIST_BUYING_STORE *packet = (const struct PACKET_ZC_ACK_ITEMLIST_BUYING_STORE *)pkt;
    const struct BuyingStoreEntry *e = packet->entries;
    unsigned n;
    struct ItemInfo item;
    struct UIWindow *w;
    int i;

    session_clear_other_store_items(&g_session1);
    session_clear_purchase_list(&g_session1);
    n = ((unsigned)(int)packet->packet_length - 0x10) / sizeof *e;
    for (i = 0; i < (int)n; i++, e++) {
        buying_store_entry_to_item(&item, e, i + 1);
        session_add_other_store_item(&g_session1, &item);
        item_info_destruct(&item);
    }
    w = ui_window_open(WINDOW_BUYING_STORE_ITEMS);
    refresh_window(w);
    UIWINDOW_SENDMSG(w, UIWM_SET_OWNER, packet->aid, 0, 0, 0);
    UIWINDOW_SENDMSG(w, UIWM_SET_ZENY, packet->zeny_limit, 0, 0, 0);
    w = ui_window_open(WINDOW_BUYING_STORE_SELL);
    refresh_window(w);
    UIWINDOW_SENDMSG(w, UIWM_SET_OWNER, packet->aid, packet->store_id, 0, 0);
    w = g_windowMgr.buying_store_mirror_sell_list_wnd;
    if (w == 0)
        w = ui_window_open(WINDOW_BUYING_STORE_LIST);
    else
        session_fill_buying_store_list_priced(&g_session1);
    if (w != 0)
        refresh_window(w);
    self->npc_dialog_open = 1;
}

#define ACTOR_MSG_BUYING_STORE_TITLE 0x8d
#define ACTOR_MSG_BUYING_STORE_CLOSED 0x8e

void game_mode_buying_store_entry(struct GameMode *self, const struct PACKET_ZC_TALKBOX_CHATCONTENTS *pkt)
{
    char title[84];
    struct GameActor *actor;

    strncpy(title, pkt->text, 0x50);
    title[80] = 0;
    actor = world_get_game_actor_by_aid(self->world, pkt->aid);
    if (actor != 0)
        render_object_send(actor, NULL, ACTOR_MSG_BUYING_STORE_TITLE, (intptr_t)title, 0, 0, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_DISAPPEAR_BUYING_STORE_ENTRY {
    unsigned short packet_type;
    int aid;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_DISAPPEAR_BUYING_STORE_ENTRY, 6);

void game_mode_disappear_buying_store_entry(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_DISAPPEAR_BUYING_STORE_ENTRY *packet =
        (const struct PACKET_ZC_DISAPPEAR_BUYING_STORE_ENTRY *)pkt;
    struct GameActor *actor = world_get_game_actor_by_aid(self->world, packet->aid);

    if (actor != 0)
        render_object_send(actor, NULL, ACTOR_MSG_BUYING_STORE_CLOSED, 0, 0, 0, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_ITEM_DELETE_BUYING_STORE {
    unsigned short packet_type;
    short index;
    short amount;
    int price;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ITEM_DELETE_BUYING_STORE, 10);

void game_mode_item_delete_buying_store(const unsigned char *pkt)
{
    const struct PACKET_ZC_ITEM_DELETE_BUYING_STORE *packet = (const struct PACKET_ZC_ITEM_DELETE_BUYING_STORE *)pkt;
    struct ItemInfo item;
    struct String line;
    struct String name = {0};
    short amount;

    chat_refresh();
    session_get_inventory_item(&g_session1, &item, packet->index);
    item_info_get_display_name(&item, &name);
    amount = packet->amount;
    string_init(&line);
    string_format(&line, msg_str(0x6d2), string_cstr(&name), (int)amount, packet->price * amount);
    chat_line(string_cstr(&line), 0xffff64);
    string_free(&line);
    string_free(&name);
    item_info_destruct(&item);
    session_remove_inventory_item(&g_session1, packet->index, packet->amount);
}

static void message_box(const char *text)
{
    ui_window_mgr_message_box(&g_windowMgr, text, 0, 1, 0, 0, "메시지", 0x118, 0x78, 0);
}

static void close_buying_store_make(void)
{
    session_clear_store_setup(&g_session1);
    ui_window_mgr_post_quit1(&g_windowMgr, WINDOW_BUYING_STORE_MAKE);
    ui_window_mgr_post_quit1(&g_windowMgr, WINDOW_BUYING_STORE_MAKE2);
}

#pragma pack(push, 1)
struct PACKET_ZC_FAILED_OPEN_BUYING_STORE_TO_BUYER {
    unsigned short packet_type;
    short result;
    int weight;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_FAILED_OPEN_BUYING_STORE_TO_BUYER, 8);

void game_mode_failed_open_buying_store(const unsigned char *pkt)
{
    const struct PACKET_ZC_FAILED_OPEN_BUYING_STORE_TO_BUYER *packet =
        (const struct PACKET_ZC_FAILED_OPEN_BUYING_STORE_TO_BUYER *)pkt;
    char text[128];
    int weight;

    switch (packet->result) {
    case 1:
        chat_refresh();
        chat_msg(0x6cd, 0xff);
        close_buying_store_make();
        return;
    case 2:
        weight = packet->weight / 10;
        sprintf(text, msg_str(0x6ce),
                    (int)((float)(weight + g_session1.weight) - (float)g_session1.maxWeight * 0.9f));
        message_box(text);
        return;
    case 9:
        chat_refresh();
        chat_msg(0x705, 0xff);
        return;
    case 10:
        chat_refresh();
        chat_msg(0xc9d, 0xff);
        close_buying_store_make();
        return;
    }
}

#pragma pack(push, 1)
struct PACKET_ZC_FAILED_TRADE_BUYING_STORE_TO_BUYER {
    unsigned short packet_type;
    short result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_FAILED_TRADE_BUYING_STORE_TO_BUYER, 4);

void game_mode_failed_trade_buying_store_to_buyer(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_FAILED_TRADE_BUYING_STORE_TO_BUYER *packet =
        (const struct PACKET_ZC_FAILED_TRADE_BUYING_STORE_TO_BUYER *)pkt;

    switch (packet->result) {
    case 3:
        chat_refresh();
        chat_msg(0x6cf, 0xff);
        break;
    case 4:
        chat_refresh();
        chat_msg(0x6d0, 0xff);
        break;
    }
    game_mode_send_msg(&self->base, 0x28, 0, 0, 0, 0);
    ui_window_mgr_post_quit1(&g_windowMgr, WINDOW_BUYING_STORE);
}

#pragma pack(push, 1)
struct PACKET_ZC_FAILED_TRADE_BUYING_STORE_TO_SELLER {
    unsigned short packet_type;
    short result;
    int item_id;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_FAILED_TRADE_BUYING_STORE_TO_SELLER, 8);

void game_mode_failed_trade_buying_store_to_seller(const unsigned char *pkt)
{
    const struct PACKET_ZC_FAILED_TRADE_BUYING_STORE_TO_SELLER *packet =
        (const struct PACKET_ZC_FAILED_TRADE_BUYING_STORE_TO_SELLER *)pkt;
    struct ItemInfo item;
    struct String line;
    struct String name = {0};

    switch (packet->result) {
    case 5:
        chat_refresh();
        chat_msg(0x39, 0xff);
        break;
    case 6:
        chat_refresh();
        session_get_inventory_item_by_id(&g_session1, &item, packet->item_id, 0);
        item_info_get_display_name(&item, &name);
        string_init(&line);
        string_format(&line, msg_str(0x6d3), string_cstr(&name));
        chat_line(string_cstr(&line), 0xff);
        string_free(&line);
        string_free(&name);
        item_info_destruct(&item);
        break;
    case 7:
        chat_refresh();
        chat_msg(0x6d1, 0xff);
        break;
    }
    ui_window_mgr_post_quit1(&g_windowMgr, WINDOW_BUYING_STORE_ITEMS);
    ui_window_mgr_post_quit1(&g_windowMgr, WINDOW_BUYING_STORE_SELL);
    ui_window_mgr_post_quit1(&g_windowMgr, WINDOW_BUYING_STORE_LIST);
}

#pragma pack(push, 1)
struct PACKET_ZC_MCSTORE_NOTMOVEITEM_LIST {
    unsigned short packet_type;
    short packet_length;
    int item_ids[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_MCSTORE_NOTMOVEITEM_LIST, 4);

void game_mode_shop_not_movable_items(const unsigned char *pkt)
{
    const struct PACKET_ZC_MCSTORE_NOTMOVEITEM_LIST *packet = (const struct PACKET_ZC_MCSTORE_NOTMOVEITEM_LIST *)pkt;
    char text[512];
    const int *id = packet->item_ids;
    unsigned n = ((unsigned)(int)packet->packet_length - 4) >> 2;
    const char *s;
    char *d;

    s = item_table_get(*id)->name;
    d = text;
    while ((*d++ = *s++) != 0)
        ;

    id++;
    if (n > 1) {
        for (n--; n != 0; n--, id++)
            sprintf(text, "%s, %s", text, item_table_get(*id)->name);
    }
    sprintf(text, msg_str(0x827), text);
    chat_refresh();
    chat_line(text, 0xff);
}

void game_mode_shop_open_result(struct GameMode *self, unsigned char result)
{
    struct UIWindow *w;

    switch (result) {
    case 0:
        w = ui_window_open(0x2d);
        if (w != 0)
            refresh_window(w);
        w = ui_window_open(0x29);
        if (w != 0)
            ui_merchant_shop_make_wnd_print_sale_list((struct UIMerchantShopMakeWnd *)w);
        ui_window_mgr_post_quit1(&g_windowMgr, 0x29);
        self->npc_dialog_open = 1;
        return;
    case 1:
        game_mode_send_msg(&self->base, 0x28, 0, 0, 0, 0);
        ui_window_mgr_post_quit1(&g_windowMgr, 0x2b);
        ui_window_mgr_post_quit1(&g_windowMgr, 0x2c);
        session_clear_purchase_lists(&g_session1);
        session_clear_own_store_items(&g_session1);
        message_box(msg_str(0xa4f));
        ui_window_mgr_post_quit1(&g_windowMgr, 0x29);
        return;
    case 3:
        game_mode_send_msg(&self->base, 0x28, 0, 0, 0, 0);
        ui_window_mgr_post_quit1(&g_windowMgr, 0x2b);
        ui_window_mgr_post_quit1(&g_windowMgr, 0x2c);
        session_clear_purchase_lists(&g_session1);
        session_clear_own_store_items(&g_session1);
        ui_window_mgr_post_quit1(&g_windowMgr, 0x29);
        chat_msg(0xc9d, 0xff);
        return;
    }
}
