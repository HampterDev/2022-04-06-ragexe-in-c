#include "game/packet_handlers.h"
#include "ui/window_manager_state.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include "game/packets_roulette.h"
#include "core/msg_str.h"
#include "ui/wndmgr/wndmgr.h"
#include "ui/shops/roulette.h"

#pragma pack(push, 1)
struct PACKET_ZC_ACK_OPEN_ROULETTE {
    unsigned short id;
    unsigned char result;
    int serial;
    unsigned char step;
    unsigned char index;
    int addition_item_id;
    int gold;
    int silver;
    int bronze;
};
struct PACKET_ZC_ACK_ROULEITTE_INFO {
    unsigned short id;
    short length;
    int serial;
    struct RoulettePrize prizes[ROULETTE_PRIZES];
};
struct PACKET_ZC_ACK_CLOSE_ROULETTE {
    unsigned short id;
    unsigned char result;
};
struct PACKET_ZC_ACK_GENERATE_ROULETTE {
    unsigned short id;
    unsigned char result;
    unsigned short step;
    unsigned short index;
    int addition_item_id;
    int gold;
    int silver;
    int bronze;
};
#pragma pack(pop)
SIZE_ASSERT(RoulettePrize, 12);
OFFSET_ASSERT(PACKET_ZC_ACK_OPEN_ROULETTE, bronze, 0x15);
OFFSET_ASSERT(PACKET_ZC_ACK_ROULEITTE_INFO, prizes, 0x08);
OFFSET_ASSERT(PACKET_ZC_ACK_GENERATE_ROULETTE, bronze, 0x13);

#define WID_ROULETTE 0x10d
#define CZ_REQ_ROULETTE_INFO 0xa1b
#define ROULETTE_REFUSED_COLOR 0x6464ff

static int screen_left(void)
{
    return (int)(g_renderer->width * 0.5 - 320.0);
}

static int screen_top(void)
{
    return (int)(g_renderer->height * 0.5 - 240.0);
}

void game_mode_zc_ack_open_roulette(struct GameMode *self, const struct PACKET_ZC_ACK_OPEN_ROULETTE *packet)
{
    struct RagConnection *conn;
    unsigned short request;
    unsigned char index;
    int top, length;

    (void)self;
    switch (packet->result) {
    case 0:
        break;
    case 1:
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (int)msg_str(0xa48), ROULETTE_REFUSED_COLOR, 0, 0);
        return;
    default:
        return;
    }
    g_roulette->serial = packet->serial;
    g_roulette->step = packet->step;
    roulette_set_addition_item(g_roulette, packet->addition_item_id);
    g_roulette->coins[ROULETTE_GOLD] = packet->gold;
    g_roulette->coins[ROULETTE_SILVER] = packet->silver;
    g_roulette->coins[ROULETTE_BRONZE] = packet->bronze;
    index = packet->index;
    roulette_set_slot(g_roulette, index, index != 0xff, 0);
    if (index != 0xff) {
        top = screen_top();
        roulette_place_marker(g_roulette, (packet->index + packet->step) * 36 + 0xad, 0x155 - packet->step * 40,
                   screen_left(), top);
    }
    if (g_roulette->prizes.size == 0) {
        request = CZ_REQ_ROULETTE_INFO;
        length = rag_connection_get_packet_length(rag_connection_instance(), CZ_REQ_ROULETTE_INFO);
        conn = rag_connection_instance();
        rag_connection_send_packet(conn, length, &request);
        return;
    }
    if (g_windowMgr.roullette_wnd == 0)
        ui_window_open(WID_ROULETTE);
}

void game_mode_zc_ack_roulette_info(struct GameMode *self, const struct PACKET_ZC_ACK_ROULEITTE_INFO *packet)
{
    struct RoulettePrize prize;
    int i;

    (void)self;
    for (i = 0; i < ROULETTE_PRIZES; i++) {
        prize.row = packet->prizes[i].row;
        prize.position = packet->prizes[i].position;
        prize.item_id = packet->prizes[i].item_id;
        prize.count = packet->prizes[i].count;
        roulette_add_prize(g_roulette, prize);
    }
    roulette_set_addition_item(g_roulette, g_roulette->addition_item_id);
    if (g_roulette->has_slot == 1)
        roulette_set_slot(g_roulette, (unsigned short)g_roulette->slot, 1, 0);
    if (g_windowMgr.roullette_wnd == 0)
        ui_window_open(WID_ROULETTE);
}

void game_mode_zc_ack_close_roulette(struct GameMode *self, const struct PACKET_ZC_ACK_CLOSE_ROULETTE *packet)
{
    (void)self;
    switch (packet->result) {
    case 0:
        roulette_reset(g_roulette);
        if (g_windowMgr.roullette_wnd != 0)
            ui_window_mgr_post_quit1(&g_windowMgr, WID_ROULETTE);
        break;
    case 1:
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (int)msg_str(0xa49), ROULETTE_REFUSED_COLOR, 0, 0);
        break;
    }
}

static void spin_refused(int msg)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (int)msg_str(msg), 0x64ffff, 0, 0);
    if (g_windowMgr.roullette_wnd != 0)
        ui_window_mgr_allow_roulette_spin(&g_windowMgr);
}

void game_mode_zc_ack_generate_roulette(struct GameMode *self,
                                                  const struct PACKET_ZC_ACK_GENERATE_ROULETTE *packet)
{
    int flag, top;

    (void)self;
    switch (packet->result) {
    case 0: flag = 0; break;
    case 3: flag = 1; break;
    case 1: spin_refused(0xa4a); return;
    case 2: spin_refused(0xa4b); return;
    case 4: spin_refused(0xa85); return;
    case 5: spin_refused(0xa8c); return;
    default: return;
    }
    g_roulette->step = packet->step;
    roulette_set_addition_item(g_roulette, packet->addition_item_id);
    g_roulette->coins[ROULETTE_BRONZE] = packet->bronze;
    g_roulette->coins[ROULETTE_SILVER] = packet->silver;
    g_roulette->coins[ROULETTE_GOLD] = packet->gold;
    roulette_set_slot(g_roulette, packet->index, 0, flag);
    top = screen_top();
    roulette_begin_spin(g_roulette, 0x1cd, 0x155 - packet->step * 40, screen_left(), top);
}
