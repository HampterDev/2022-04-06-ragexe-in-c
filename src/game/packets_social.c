#include "game/packet_handlers.h"
#include "session/character_name.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include <windows.h>
#include <string.h>
#include "game/packets.h"
#include "platform/clientinfo.h"
#include "session/session_items.h"
#include "session/session_party.h"
#include "session/session_windows.h"

#define WND_EXCHANGE_REQUEST    0x20
#define WND_COUPLE_REQUEST      0x68
#define WND_BABY_REQUEST        0x69
#define WND_FRIEND_REQUEST      0x6d

#define MM_EXCHANGE_ANSWER      0x32
#define MM_FRIEND_ANSWER        0xaf

#define NAME_LENGTH             24

#pragma pack(push, 1)
struct PACKET_ZC_REQ_EXCHANGE_ITEM2 {
    unsigned short id;
    char name[NAME_LENGTH];
    int char_id;
    short level;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_REQ_EXCHANGE_ITEM2, 32);

void game_mode_zc_req_exchange_item2(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_REQ_EXCHANGE_ITEM2 *packet = (const struct PACKET_ZC_REQ_EXCHANGE_ITEM2 *)pkt;
    char name[NAME_LENGTH + 1];
    void *wnd;

    if (g_session1.refuseTrades != 0) {
        mode_send_message(client_current_mode(), MM_EXCHANGE_ANSWER, 4, 0, 0, 0);
        return;
    }
    memcpy(name, packet->name, NAME_LENGTH);
    name[NAME_LENGTH] = 0;
    wnd = ui_window_open(WND_EXCHANGE_REQUEST);
    wnd_send(wnd, 0x32, (int)name, packet->char_id, packet->level, 0);
    string_assign_cstr(&self->exchange_partner, name);
}

#pragma pack(push, 1)
struct PACKET_ZC_REQ_EXCHANGE_ITEM {
    unsigned short id;
    char name[NAME_LENGTH];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_REQ_EXCHANGE_ITEM, 26);

void game_mode_zc_req_exchange_item(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_REQ_EXCHANGE_ITEM *packet = (const struct PACKET_ZC_REQ_EXCHANGE_ITEM *)pkt;
    char name[NAME_LENGTH + 1];
    void *wnd;

    if (g_session1.refuseTrades != 0) {
        mode_send_message(client_current_mode(), MM_EXCHANGE_ANSWER, 4, 0, 0, 0);
        return;
    }
    memcpy(name, packet->name, NAME_LENGTH);
    name[NAME_LENGTH] = 0;
    wnd = ui_window_open(WND_EXCHANGE_REQUEST);
    wnd_send(wnd, 0x32, (int)name, 0, 0, 0);
    string_assign_cstr(&self->exchange_partner, name);
}

#define WND_TRADE               0x1f
enum {
    TRADE_WND_CONCLUDED_US = 0x33,
    TRADE_WND_CONCLUDED_THEM = 0x34,
};
#define MM_TRADE_OVER           0x12

#pragma pack(push, 1)
struct PACKET_ZC_CONCLUDE_EXCHANGE_ITEM {
    unsigned short id;
    BYTE who;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_CONCLUDE_EXCHANGE_ITEM, 3);

void game_mode_zc_conclude_exchange_item(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_CONCLUDE_EXCHANGE_ITEM *packet = (const struct PACKET_ZC_CONCLUDE_EXCHANGE_ITEM *)pkt;
    void *wnd = g_windowMgr.exchange_wnd;

    (void)self;
    if (!wnd)
        return;
    if (packet->who == 0)
        wnd_send(wnd, TRADE_WND_CONCLUDED_US, 0, 0, 0, 0);
    else if (packet->who == 1)
        wnd_send(wnd, TRADE_WND_CONCLUDED_THEM, 0, 0, 0, 0);
}

void game_mode_zc_cancel_exchange_item(struct GameMode *self, const unsigned char *pkt)
{
    (void)pkt;
    self->exchange_add_pending = 0;
    mode_send_msg(self, MM_TRADE_OVER, 0, 0, 0, 0);
    ui_window_mgr_post_quit1(&g_windowMgr, WND_TRADE);
    ui_window_mgr_post_quit1(&g_windowMgr, WND_EXCHANGE_REQUEST);
    self->exchange_open = 0;
    session_end_exchange(&g_session1);
    ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
    ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(0x4a), 0xff, 0, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_EXEC_EXCHANGE_ITEM {
    unsigned short id;
    BYTE result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_EXEC_EXCHANGE_ITEM, 3);

void game_mode_zc_exec_exchange_item(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_EXEC_EXCHANGE_ITEM *packet = (const struct PACKET_ZC_EXEC_EXCHANGE_ITEM *)pkt;
    mode_send_msg(self, MM_TRADE_OVER, 0, 0, 0, 0);
    ui_window_mgr_post_quit1(&g_windowMgr, WND_TRADE);
    ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
    if (packet->result == 0) {
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(0x4b), 0xffff00, 0, 0);
    } else if (packet->result == 1) {
        self->exchange_open = 0;
        session_end_exchange(&g_session1);
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(0x4c), 0xff, 0, 0);
    }
}

void game_mode_zc_exchange_item_undo(struct GameMode *self, const unsigned char *pkt)
{
    (void)pkt;
    self->exchange_open = 0;
    session_end_exchange2(&g_session1);
}

#pragma pack(push, 1)
struct PACKET_ZC_REQ_ADD_FRIENDS {
    unsigned short id;
    int aid;
    int char_id;
    char name[NAME_LENGTH];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_REQ_ADD_FRIENDS, 34);

void game_mode_zc_req_add_friends(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_REQ_ADD_FRIENDS *packet = (const struct PACKET_ZC_REQ_ADD_FRIENDS *)pkt;
    char name[NAME_LENGTH + 1];

    (void)self;
    strncpy(name, packet->name, NAME_LENGTH);
    name[NAME_LENGTH] = 0;
    if (clientinfo_is_yellow_aid(((unsigned)g_session1.ownAid)) || g_session1.refuseTrades != 0) {
        mode_send_message(client_current_mode(), MM_FRIEND_ANSWER, packet->aid, packet->char_id, 0, 0);
    } else {
        void *wnd = ui_window_open(WND_FRIEND_REQUEST);

        wnd_send(wnd, 0x5f, packet->aid, packet->char_id, (int)name, 0);
    }
}

#pragma pack(push, 1)
struct PACKET_ZC_DELETE_FRIENDS {
    unsigned short id;
    unsigned aid;
    unsigned char_id;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_DELETE_FRIENDS, 10);

void game_mode_zc_delete_friends(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_DELETE_FRIENDS *packet = (const struct PACKET_ZC_DELETE_FRIENDS *)pkt;

    (void)self;
    if (session_remove_friend(&g_session1, packet->char_id))
        session_notify_messenger_group_wnd(&g_session1);
}

#pragma pack(push, 1)
struct PACKET_ZC_REQ_COUPLE {
    unsigned short id;
    unsigned aid;
    unsigned char_id;
    char name[NAME_LENGTH];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_REQ_COUPLE, 34);

static void family_request(struct GameMode *self, const unsigned char *pkt, int wnd_id, int msg)
{
    const struct PACKET_ZC_REQ_COUPLE *packet = (const struct PACKET_ZC_REQ_COUPLE *)pkt;
    char name[NAME_LENGTH + 1];
    void *wnd;

    strncpy(name, packet->name, NAME_LENGTH);
    self->family_request_aid = packet->aid;
    self->family_request_char_id = packet->char_id;
    name[NAME_LENGTH] = 0;
    wnd = ui_window_open(wnd_id);
    wnd_send(wnd, msg, (int)packet->char_id, (int)name, 0, 0);
}

void game_mode_zc_req_baby(struct GameMode *self, const unsigned char *pkt)
{
    family_request(self, pkt, WND_BABY_REQUEST, 0x5b);
}

void game_mode_zc_req_couple(struct GameMode *self, const unsigned char *pkt)
{
    family_request(self, pkt, WND_COUPLE_REQUEST, 0x5a);
}

#define MM_EXCHANGE_BAD_AMOUNT  0x35

#pragma pack(push, 1)
struct PacketAddExchangeItemOpt {
    unsigned short packet_type;
    int item_id;
    unsigned char type;
    int amount;
    unsigned char is_identified;
    unsigned char is_damaged;
    unsigned char refining_level;
    int cards[4];
    struct ItemOption options[5];
    int location;
    unsigned short look;
};
struct PacketAddExchangeItemGrade {
    unsigned short packet_type;
    int item_id;
    unsigned char type;
    int amount;
    unsigned char is_identified;
    unsigned char is_damaged;
    int cards[4];
    struct ItemOption options[5];
    int location;
    unsigned short look;
    unsigned char refining_level;
    unsigned char grade;
};
#pragma pack(pop)

static void exchange_options(struct ItemInfo *item, const struct ItemOption *options)
{
    int i;

    item->option_count = item_option_count(options);
    for (i = 0; i < item->option_count; i++)
        item->options[i] = options[i];
}

void game_mode_on_add_exchange_item_opt(struct GameMode *self, const struct PacketAddExchangeItemOpt *packet)
{
    struct ItemInfo item, arg;

    (void)self;
    if (packet->amount < 0)
        mode_send_message(client_current_mode(), MM_EXCHANGE_BAD_AMOUNT, 0, 0, 0, 0);
    if (packet->item_id == 0) {
        session_set_their_trade_zeny(&g_session1, packet->amount);
        return;
    }
    item_info_construct(&item);
    item.type = packet->type;
    item_info_set_item_id(&item, packet->item_id);
    item.count = packet->amount;
    item.damaged = packet->is_damaged;
    item.identified = packet->is_identified;
    item.refine = packet->refining_level;
    item.slot[0] = packet->cards[0];
    item.slot[1] = packet->cards[1];
    item.slot[2] = packet->cards[2];
    item.slot[3] = packet->cards[3];
    item.index = -1;
    exchange_options(&item, packet->options);
    item.equip_location = packet->location;
    item.view = packet->look;
    item_info_copy(&arg, &item);
    session_add_their_trade_item(&g_session1, arg);
    item_info_destruct(&item);
}

void game_mode_on_add_exchange_item_grade(struct GameMode *self, const struct PacketAddExchangeItemGrade *packet)
{
    struct ItemInfo item, arg;

    (void)self;
    if (packet->amount < 0)
        mode_send_message(client_current_mode(), MM_EXCHANGE_BAD_AMOUNT, 0, 0, 0, 0);
    if (packet->item_id == 0) {
        session_set_their_trade_zeny(&g_session1, packet->amount);
        return;
    }
    item_info_construct(&item);
    item.type = packet->type;
    item_info_set_item_id(&item, packet->item_id);
    item.count = packet->amount;
    item.damaged = packet->is_damaged;
    item.identified = packet->is_identified;
    item.refine = packet->refining_level;
    item.slot[0] = packet->cards[0];
    item.slot[1] = packet->cards[1];
    item.slot[2] = packet->cards[2];
    item.slot[3] = packet->cards[3];
    item.index = -1;
    exchange_options(&item, packet->options);
    item.equip_location = packet->location;
    item.view = packet->look;
    item_info_copy(&arg, &item);
    session_add_their_trade_item(&g_session1, arg);
    item_info_destruct(&item);
}

#pragma pack(push, 1)
struct PACKET_ZC_COUPLENAME {
    unsigned short id;
    char name[];
};
#pragma pack(pop)

void game_mode_zc_couple_name(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_COUPLENAME *packet = (const struct PACKET_ZC_COUPLENAME *)pkt;

    sprintf(self->couple_name, packet->name);
}

#pragma pack(push, 1)
struct PACKET_ZC_DIVORCE {
    unsigned short id;
    char name[24];
};
#pragma pack(pop)

#define MSG_DIVORCED     0x344
#define UIM_DIALOG_CLOSE 3
#define DIVORCE_COLOR    0xffff00

void game_mode_zc_divorce(struct GameMode *self, const struct PACKET_ZC_DIVORCE *packet)
{
    char line[0x44];
    const char *our_name;

    (void)self;
    our_name = session_character_name(&g_session1);
    sprintf(line, msg_str(MSG_DIVORCED), our_name, packet->name);
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_DIALOG_CLOSE, 0, 0, 0, 0);
    ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)line, DIVORCE_COLOR, 0, 0);
}

#define BABY_MSG_COLOR          0x9b9bff
#pragma pack(push, 1)
struct PACKET_ZC_BABYMSG {
    unsigned short id;
    unsigned result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_BABYMSG, 6);

void game_mode_zc_baby_msg(const unsigned char *pkt)
{
    const struct PACKET_ZC_BABYMSG *packet = (const struct PACKET_ZC_BABYMSG *)pkt;
    static const short text[3] = { 0x383, 0x384, 0x394 };
    unsigned result = packet->result;

    if (result <= 2)
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(text[result]), BABY_MSG_COLOR, 0, 0);
}

#define CHAT_COLOR_YELLOW       0xffff
#define CHAT_COLOR_CYAN         0xffff00

#pragma pack(push, 1)
struct PACKET_ZC_ACK_GIVE_MANNER_POINT {
    unsigned short id;
    int result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_GIVE_MANNER_POINT, 6);

void game_mode_zc_ack_give_manner_point(const unsigned char *pkt)
{
    const struct PACKET_ZC_ACK_GIVE_MANNER_POINT *packet = (const struct PACKET_ZC_ACK_GIVE_MANNER_POINT *)pkt;
    int text;
    unsigned color;

    switch (packet->result) {
    case 0: text = 0x356; color = CHAT_COLOR_CYAN; break;
    case 3: text = 0x33e; color = CHAT_COLOR_YELLOW; break;
    case 4: text = 0x341; color = CHAT_COLOR_YELLOW; break;
    default: return;
    }
    ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(text), color, 0, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_NOTIFY_MANNER_POINT_GIVEN {
    unsigned short id;
    BYTE type;
    char name[24];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_NOTIFY_MANNER_POINT_GIVEN, 27);

void game_mode_zc_notify_manner_point_given(const unsigned char *pkt)
{
    const struct PACKET_ZC_NOTIFY_MANNER_POINT_GIVEN *packet = (const struct PACKET_ZC_NOTIFY_MANNER_POINT_GIVEN *)pkt;
    char name[25], line[0x400];

    memcpy(name, packet->name, 24);
    name[24] = 0;
    if (packet->type != 0)
        return;
    sprintf(line, msg_str(0x183), name);
    ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)line, CHAT_COLOR_CYAN, 0, 0);
}
