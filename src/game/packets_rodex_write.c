#include "game/packet_handlers.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "ui/window_access.h"
#include "game/context.h"
#include "game/packets_rodex.h"
#include "session/session_items.h"
#include "session/session_windows.h"
#include "ui/mail/rodex_compose.h"

#pragma pack(push, 1)
struct PACKET_ZC_ACK_ADD_ITEM_RODEX {
    unsigned short id;
    unsigned char result;
    short index;
    short count;
    int item_id;
    unsigned char type;
    unsigned char is_identified;
    unsigned char is_damaged;
    unsigned char refine;
    int cards[ITEM_CARD_SLOTS];
    struct ItemOption options[ITEM_OPTION_SLOTS];
    unsigned short weight;
    unsigned char favorite;
    int item_index;
};
struct PACKET_ZC_ACK_ADD_ITEM_RODEX2 {
    unsigned short id;
    unsigned char result;
    short index;
    short count;
    int item_id;
    unsigned char type;
    unsigned char is_identified;
    unsigned char is_damaged;
    int cards[ITEM_CARD_SLOTS];
    struct ItemOption options[ITEM_OPTION_SLOTS];
    unsigned short weight;
    unsigned char favorite;
    int item_index;
    unsigned char refine;
    unsigned char grade;
};
struct PACKET_ZC_ACK_REMOVE_RODEX_ITEM {
    unsigned short id;
    unsigned char result;
    short index;
    unsigned short count;
    unsigned short weight;
};
struct PACKET_ZC_ACK_OPEN_WRITE_RODEX {
    unsigned short id;
    char receiver[24];
    unsigned char result;
};
struct PACKET_ZC_ACK_SEND_RODEX {
    unsigned short id;
    unsigned char result;
};
#pragma pack(pop)
OFFSET_ASSERT(PACKET_ZC_ACK_ADD_ITEM_RODEX, item_index, 0x3b);
OFFSET_ASSERT(PACKET_ZC_ACK_ADD_ITEM_RODEX2, grade, 0x3f);

int item_options_count(const struct ItemOption *options)
{
    return item_option_count(options);
}

void game_mode_zc_ack_add_item_rodex(struct GameMode *self, const struct PACKET_ZC_ACK_ADD_ITEM_RODEX *packet)
{
    struct ItemInfo item, arg;
    int k;

    (void)self;
    switch (packet->result) {
    case 0:
        item_info_construct(&item);
        item.index = packet->index;
        item.count = packet->count;
        item_info_set_item_id(&item, packet->item_id);
        item.type = packet->type;
        item.identified = packet->is_identified;
        item.damaged = packet->is_damaged;
        item.refine = packet->refine;
        item.slot[0] = packet->cards[0];
        item.slot[1] = packet->cards[1];
        item.slot[2] = packet->cards[2];
        item.slot[3] = packet->cards[3];
        item.option_count = item_options_count(packet->options);
        item.equip_location = packet->item_index;
        for (k = 0; k < item.option_count; k++)
            item.options[k] = packet->options[k];
        item.favorite = packet->favorite;
        item_info_copy(&arg, &item);
        session_add_mail_item(&g_session1, arg);
        session_remove_inventory_item(&g_session1, packet->index, packet->count);
        g_session1.mailWeight = packet->weight;
        rodex_chat(0xa22);
        item_info_destruct(&item);
        break;
    case 1: rodex_chat(0xa46); break;
    case 2: rodex_chat(0xa45); break;
    case 3: rodex_chat(0xa89); break;
    case 4: rodex_chat(0xa8b); break;
    }
}

void game_mode_zc_ack_add_item_rodex2(struct GameMode *self,
                                                const struct PACKET_ZC_ACK_ADD_ITEM_RODEX2 *packet)
{
    struct ItemInfo item, copy, arg;
    unsigned short weight;
    unsigned k;

    (void)self;
    item_info_construct(&item);
    if (packet->result == 0) {
        item.index = packet->index;
        item.count = packet->count;
        item_info_set_item_id(&item, packet->item_id);
        item.type = packet->type;
        item.identified = packet->is_identified;
        item.damaged = packet->is_damaged;
        item.refine = packet->refine;
        item.slot[0] = packet->cards[0];
        item.slot[1] = packet->cards[1];
        item.slot[2] = packet->cards[2];
        item.slot[3] = packet->cards[3];
        item.option_count = item_option_count(packet->options);
        item.equip_location = packet->item_index;
        for (k = 0; k < (unsigned)item.option_count; k++)
            item.options[k] = packet->options[k];
        item.favorite = packet->favorite;
    }
    weight = packet->weight;
    item_info_copy(&copy, &item);
    switch (packet->result) {
    case 0:
        item_info_copy(&arg, &copy);
        session_add_mail_item(&g_session1, arg);
        session_remove_inventory_item(&g_session1, copy.index, copy.count);
        g_session1.mailWeight = weight;
        rodex_chat(0xa22);
        break;
    case 1: rodex_chat(0xa46); break;
    case 2: rodex_chat(0xa45); break;
    case 3: rodex_chat(0xa89); break;
    case 4: rodex_chat(0xa8b); break;
    }
    item_info_destruct(&copy);
    item_info_destruct(&item);
}

void game_mode_zc_ack_remove_rodex_item(struct GameMode *self,
                                                  const struct PACKET_ZC_ACK_REMOVE_RODEX_ITEM *packet)
{
    (void)self;
    if (packet->result == 1) {
        session_take_back_mail_item(&g_session1, packet->index, packet->count);
        g_session1.mailWeight = packet->weight;
        return;
    }
    rodex_chat(0xa2a);
}

void game_mode_zc_ack_open_write_rodex(struct GameMode *self,
                                                 const struct PACKET_ZC_ACK_OPEN_WRITE_RODEX *packet)
{
    const char *receiver = packet->receiver;

    (void)self;
    if (packet->result != 1) {
        rodex_chat(0xa2c);
        return;
    }
    if (g_windowMgr.rodex_compose_wnd != 0)
        return;
    session_reset_mail_items(&g_session1);
    session_clear_mail_item_counts(&g_session1);
    ui_window_open(WID_RODEX_COMPOSE);
    if (receiver != 0)
        rodex_compose_wnd_set_receiver((struct UIRodexComposeWnd *)g_windowMgr.rodex_compose_wnd, receiver);
}

static void send_refused(int msg)
{
    game_send_message(MM_RODEX_SENT, 0, 0, 0, 0);
    rodex_chat(msg);
}

void game_mode_zc_ack_send_rodex(struct GameMode *self, const struct PACKET_ZC_ACK_SEND_RODEX *packet)
{
    (void)self;
    switch (packet->result) {
    case 0: rodex_chat(0xa1b); break;
    case 1: send_refused(0xa25); break;
    case 2: send_refused(0xa2b); break;
    case 3: send_refused(0xa26); break;
    case 4: send_refused(0xa27); break;
    }
    if (g_windowMgr.rodex_inbox_wnd != 0)
        ui_window_send(g_windowMgr.rodex_inbox_wnd, NULL, UIM_RODEX_INBOX_REFRESH, 0, 0, 0, 0);
}
