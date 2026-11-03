#include "game/actor_packets.h"
#include "game/gamemode.h"
#include "game/ground_item_packets.h"
#include "session/state.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include <string.h>
#include <stdlib.h>
#include "game/packet_handlers.h"
#include "game/client.h"
#include "game/item_info.h"
#include "core/msg_str.h"
#include "game/mode_util.h"
#include "session/session_items.h"

enum {
    UI_OPEN_BANK = 0,
    UI_OPEN_STYLIST = 1,
};
#define WINDOW_BANK 0xfb
#define HEADER_CZ_REQ_BANKING_CHECK 0x9ab

#pragma pack(push, 1)
struct PacketReqBankingCheck {
    unsigned short id;
    int aid;
};
#pragma pack(pop)

void game_mode_ui_open(unsigned char type)
{
    struct PacketReqBankingCheck req;

    if (type == UI_OPEN_BANK) {
        if (ui_window_find(WINDOW_BANK) == 0) {
            req.id = HEADER_CZ_REQ_BANKING_CHECK;
            req.aid = g_session1.ownAid;
            rag_connection_send_packet(rag_connection_instance(), sizeof req, &req);
        }
    } else if (type == UI_OPEN_STYLIST) {
        if (ui_window_find(0x119) == 0)
            ui_window_open(0x119);
    }
}

#define WINDOW_ITEM_LIST 0xa0
#define WINDOW_ITEM_LIST_BUTTONS 0xa1
enum {
    ITEMLISTWIN_994_997 = 1,
    ITEMLISTWIN_990_993 = 2,
};

static int item_listed(int type, struct ItemInfo *item)
{
    if ((unsigned)item->index == 0)
        return 0;
    if (type == ITEMLISTWIN_994_997 && !item_id_is994_to997(atoi(string_cstr(&item->id_text))))
        return 0;
    if (type == ITEMLISTWIN_990_993 && !item_id_is990_to993(atoi(string_cstr(&item->id_text))))
        return 0;
    return item->index != g_session1.wornArrowIndex;
}

#pragma pack(push, 1)
struct PACKET_ZC_ITEMLISTWIN_OPEN {
    unsigned short packet_type;
    int type;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ITEMLISTWIN_OPEN, 6);

void game_mode_item_list_win_open(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_ITEMLISTWIN_OPEN *packet = (const struct PACKET_ZC_ITEMLISTWIN_OPEN *)pkt;
    int type = packet->type;
    struct ItemInfo item;
    struct ItemRecordA entry;
    struct UIWindow *w;
    int count, i;

    session_clear_lists(&g_session1);
    count = session_get_inventory_count(&g_session1);
    for (i = 0; i < count; i++) {
        session_get_inventory_item_at(&g_session1, &item, i);
        if (item_listed(type, &item)) {
            item_record_a_clear(&entry);
            entry.index = item.index;
            strcpy_s(entry.id_text, sizeof entry.id_text, string_cstr(&item.id_text));
            entry.type = item.type;
            entry.identified = item.identified;
            entry.price = 1;
            entry.charged_price = 2;
            session_append_shop_goods_record(&g_session1, &entry, 1);
        }
        item_info_destruct(&item);
    }
    w = ui_window_open(WINDOW_ITEM_LIST);
    UIWINDOW_SENDMSG(w, 0x24, 1, 0, 0, 0);
    UIWINDOW_SENDMSG(w, 0x17, 0, 0, 0, 0);
    w = ui_window_open(WINDOW_ITEM_LIST_BUTTONS);
    UIWINDOW_SENDMSG(w, 0x17, type, 0, 0, 0);
}

enum {
    WINDOW_SELECT_CART = 0x5f,
    WINDOW_STYLING_SHOP = 0x119,
    WINDOW_QUEST = 0x141,
    WINDOW_ATTENDANCE = 0x148,
};
#define CHAT_COLOR_SYSTEM_RED 0xff

void game_mode_ui_open_v2(struct GameMode *self, unsigned char type, int value)
{
    struct UIWindow *w;

    (void)self;
    switch (type) {
    case 0: case 1: case 2: case 3: case 4:
        game_mode_ui_open(type);
        break;
    case 6:
        w = ui_window_open(WINDOW_QUEST);
        if (w != 0)
            UIWINDOW_SENDMSG(w, 0x17, 0, value, 0, 0);
        break;
    case 7:
        w = ui_window_open(WINDOW_ATTENDANCE);
        if (w != 0)
            UIWINDOW_SENDMSG(w, 0x22, value, 0, 0, 0);
        break;
    }
}

#pragma pack(push, 1)
struct PACKET_ZC_SELECTCART {
    unsigned short packet_type;
    short length;
    int value;
    char types[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_SELECTCART, 8);

void game_mode_select_cart(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_SELECTCART *packet = (const struct PACKET_ZC_SELECTCART *)pkt;
    struct Vec carts = { 0, 0, 0 };
    int count = packet->length - 8;
    struct UIWindow *w;

    (void)self;
    if (count == 0)
        return;
    vec_insert_range(&carts, carts.last, packet->types, packet->types + count, 1);
    w = ui_window_open(WINDOW_SELECT_CART);
    if (w != 0)
        UIWINDOW_SENDMSG(w, 0x22, (int)carts.first, count, packet->value, 0);
    vec_free(&carts);
}

#define HEADER_CZ_STYLINGSHOP_OK 0xa48

#pragma pack(push, 1)
struct PACKET_ZC_ACK_BUY_STYLINGSHOP {
    unsigned short packet_type;
    unsigned char result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_BUY_STYLINGSHOP, 3);

void game_mode_ack_buy_styling_shop(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_ACK_BUY_STYLINGSHOP *packet = (const struct PACKET_ZC_ACK_BUY_STYLINGSHOP *)pkt;
    unsigned short header;

    (void)self;
    switch (packet->result) {
    case 0:
        header = HEADER_CZ_STYLINGSHOP_OK;
        send_packet(HEADER_CZ_STYLINGSHOP_OK, &header);
        ui_window_mgr_message_box(&g_windowMgr, msg_str(0xb14), 0, 1, 0, 0, UI_MSGBOX_TITLE, 0x118, 0x78, 0);
        ui_window_mgr_post_quit1(&g_windowMgr, WINDOW_STYLING_SHOP);
        break;
    case 1:
    case 2:
        ui_window_mgr_message_box(&g_windowMgr, msg_str(0x627), 0, 1, 0, 0, UI_MSGBOX_TITLE, 0x118, 0x78, 0);
        break;
    }
}

void game_mode_ack_check_attendance(struct GameMode *self, int result, int day)
{
    struct UIWindow *w;

    (void)self;
    if (result == 0) {
        w = ui_window_find(WINDOW_ATTENDANCE);
        if (w != 0)
            UIWINDOW_SENDMSG(w, 0x22, day * 10 + 1, 0, 0, 0);
    } else if (result == 1) {
        ui_window_mgr_send_msg_(&g_windowMgr, UIWM_CHAT_PRINT, (int)msg_str(0xd90), CHAT_COLOR_SYSTEM_RED, 0, 0);
        ui_window_mgr_post_quit1(&g_windowMgr, WINDOW_ATTENDANCE);
    }
}

void game_mode_zc_dressroom_open(void)
{
    ui_window_open(0x103);
}
