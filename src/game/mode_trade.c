#include "game/packet_handlers.h"
#include "iteminfo/item_api.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include <string.h>
#include "game/packets.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "session/session_items.h"
#include "session/session_windows.h"

#define MM_TRADE_STARTED        0x28
#define MM_TRADE_OVERWEIGHT     0x35
#define WND_TRADE               0x1f
#define UIM_REFRESH             3
#define UIM_CHAT_LINE           1
#define COLOUR_RED              0xff

static void chat_msg(int msg_id)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_REFRESH, 0, 0, 0, 0);
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (intptr_t)msg_str(msg_id), COLOUR_RED, 0, 0);
}

static void open_trade(struct GameMode *self, int cid, int level)
{
    void *wnd;

    mode_send_msg(self, MM_TRADE_STARTED, 0, 0, 0, 0);
    self->exchange_open = 1;
    session_clear_my_trade(&g_session1);
    session_clear_their_trade(&g_session1);
    ui_window_mgr_post_quit1(&g_windowMgr, WND_TRADE);
    wnd = ui_window_open(WND_TRADE);
    wnd_send(wnd, 0x32, (intptr_t)string_cstr(&self->exchange_partner), cid, level, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_ACK_EXCHANGE_ITEM {
    unsigned short packet_type;
    unsigned char result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_EXCHANGE_ITEM, 3);

void game_mode_ack_exchange_item(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_ACK_EXCHANGE_ITEM *packet = (const struct PACKET_ZC_ACK_EXCHANGE_ITEM *)pkt;

    switch (packet->result) {
    case 0: chat_msg(0x46); break;
    case 1: chat_msg(0x47); break;
    case 2: chat_msg(0x48); break;
    case 3: open_trade(self, 0, 0); break;
    case 4: chat_msg(0x45); break;
    }
}

#pragma pack(push, 1)
struct PACKET_ZC_ACK_EXCHANGE_ITEM2 {
    unsigned short packet_type;
    unsigned char result;
    int cid;
    short level;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_EXCHANGE_ITEM2, 9);

void game_mode_ack_exchange_item2(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_ACK_EXCHANGE_ITEM2 *packet = (const struct PACKET_ZC_ACK_EXCHANGE_ITEM2 *)pkt;
    struct String line;

    switch (packet->result) {
    case 0: chat_msg(0x46); break;
    case 1: chat_msg(0x47); break;
    case 2: chat_msg(0x48); break;
    case 3: open_trade(self, packet->cid, packet->level); break;
    case 4: chat_msg(0x45); break;
    case 5:
        string_init(&line);
        string_format(&line, msg_str(0x6ac), string_cstr(&self->exchange_partner));
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_REFRESH, 0, 0, 0, 0);
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (intptr_t)string_cstr(&line), COLOUR_RED, 0, 0);
        string_free(&line);
        break;
    }
}

#pragma pack(push, 1)
struct PACKET_ZC_ACK_ADD_EXCHANGE_ITEM {
    unsigned short packet_type;
    short index;
    unsigned char result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_ADD_EXCHANGE_ITEM, 5);

void game_mode_ack_add_exchange_item(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_ACK_ADD_EXCHANGE_ITEM *packet = (const struct PACKET_ZC_ACK_ADD_EXCHANGE_ITEM *)pkt;
    struct ItemInfo item, arg;
    short index;

    self->exchange_add_pending = 0;
    self->item_move_ready = 1;
    switch (packet->result) {
    case 0:
        index = packet->index;
        if (index != 0) {
            session_get_inventory_item(&g_session1, &item, index);
            item.count = self->trade_add_amount;
            item_info_copy(&arg, &item);
            session_add_my_trade_item(&g_session1, arg);
            item_info_destruct(&item);
        } else {
            g_session1.zeny -= self->trade_add_amount;
            session_refresh_basic_info(&g_session1);
            session_invalidate_windows(&g_session1);
            session_set_other_exchange_money(&g_session1, self->trade_add_amount + g_session1.myTradeZeny);
        }
        break;
    case 1: chat_msg(0x49); break;
    case 3: chat_msg(0x792); break;
    case 4: chat_msg(0x793); break;
    }
}

static void their_item(int item_id, int type, int amount, int identified, int damaged, int refine,
                       const int *cards, const struct ItemOption *options)
{
    struct ItemInfo item, arg;
    long long i;

    if (amount < 0)
        mode_send_message(client_current_mode(), MM_TRADE_OVERWEIGHT, 0, 0, 0, 0);
    if (item_id == 0) {
        session_set_their_trade_zeny(&g_session1, amount);
        return;
    }
    item_info_construct(&item);
    item.type = type;
    item_info_set_item_id(&item, item_id);
    item.damaged = (char)damaged;
    item.identified = (char)identified;
    item.refine = refine;
    memcpy(item.slot, cards, sizeof item.slot);
    item.index = -1;
    item.count = amount;
    item.option_count = item_option_count(options);
    for (i = 0; i < item.option_count; i++)
        item.options[i] = options[i];
    item_info_copy(&arg, &item);
    session_add_their_trade_item(&g_session1, arg);
    item_info_destruct(&item);
}

#pragma pack(push, 1)
struct PACKET_ZC_ADD_EXCHANGE_ITEM2 {
    unsigned short packet_type;
    int item_id;
    unsigned char type;
    int amount;
    unsigned char identified;
    unsigned char damaged;
    unsigned char refine;
    int cards[ITEM_CARD_SLOTS];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ADD_EXCHANGE_ITEM2, 30);

void game_mode_add_exchange_item2(const unsigned char *pkt)
{
    const struct PACKET_ZC_ADD_EXCHANGE_ITEM2 *packet = (const struct PACKET_ZC_ADD_EXCHANGE_ITEM2 *)pkt;
    struct ItemOption none[ITEM_OPTION_SLOTS];

    memset(none, 0, sizeof none);
    their_item(packet->item_id, packet->type, packet->amount, packet->identified, packet->damaged, packet->refine,
               packet->cards, none);
}

#pragma pack(push, 1)
struct PACKET_ZC_ADD_EXCHANGE_ITEM3 {
    unsigned short packet_type;
    int item_id;
    unsigned char type;
    int amount;
    unsigned char identified;
    unsigned char damaged;
    unsigned char refine;
    int cards[ITEM_CARD_SLOTS];
    struct ItemOption options[ITEM_OPTION_SLOTS];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ADD_EXCHANGE_ITEM3, 55);

void game_mode_add_exchange_item3(const unsigned char *pkt)
{
    const struct PACKET_ZC_ADD_EXCHANGE_ITEM3 *packet = (const struct PACKET_ZC_ADD_EXCHANGE_ITEM3 *)pkt;
    struct ItemOption options[ITEM_OPTION_SLOTS];

    memcpy(options, packet->options, sizeof options);
    their_item(packet->item_id, packet->type, packet->amount, packet->identified, packet->damaged, packet->refine,
               packet->cards, options);
}
