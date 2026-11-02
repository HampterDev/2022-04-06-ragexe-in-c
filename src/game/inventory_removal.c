#include "game/packet_handlers.h"
#include "iteminfo/item_table.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "game/context.h"
#include <stdlib.h>
#include "game/client.h"
#include "game/gamemode.h"
#include "game/item_info.h"
#include "game/item_list_packets.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "session/session_items.h"
#include "session/session_state.h"

#define WND_RODEX_COMPOSE   0x108
#define MM_RODEX_SENT       0x125

static void chat(const char *text, int color, int channel)
{
    ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)text, color, channel, 0);
}

static void end_exchange(struct GameMode *self)
{
    if (self->exchange_open != 0) {
        self->exchange_open = 0;
        session_end_exchange2(&g_session1);
    }
}

#pragma pack(push, 1)
struct PACKET_ZC_DELETE_ITEM_FROM_BODY {
    unsigned short packet_type;
    unsigned short delete_type;
    unsigned short index;
    short count;
};
struct PACKET_ZC_CASH_ITEM_DELETE {
    unsigned short packet_type;
    short index;
    int item_id;
};
struct PACKET_ZC_CASH_TIME_COUNTER {
    unsigned short packet_type;
    int item_id;
    unsigned seconds;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_DELETE_ITEM_FROM_BODY, 8);
SIZE_ASSERT(PACKET_ZC_CASH_ITEM_DELETE, 8);
SIZE_ASSERT(PACKET_ZC_CASH_TIME_COUNTER, 10);

void game_mode_delete_item_from_body(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_DELETE_ITEM_FROM_BODY *packet = (const struct PACKET_ZC_DELETE_ITEM_FROM_BODY *)pkt;
    unsigned short index = packet->index;

    if (g_windowMgr.rodex_compose_wnd != 0) {
        chat(msg_str(0xaae), 0x64ffff, 0);
        game_send_message(MM_RODEX_SENT, 0, 0, 0, 0);
        ui_window_mgr_post_quit1(&g_windowMgr, WND_RODEX_COMPOSE);
    }
    end_exchange(self);
    session_remove_inventory_item(&g_session1, index, packet->count);
    self->throw_item_pending = 0;
    self->item_move_ready = 1;
    if (g_session1.petJob != -1 && g_session1.hatchedEggIndex == index)
        session_reset_pet(&g_session1);
}

static void rental_line(const char *text)
{
    ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
    ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)text, 0xffff, 0, 0);
    ui_window_mgr_send_msg_(&g_windowMgr, 0x13, (int)text, 0xffff, 0, 0);
}

static const char *item_name(int item_id)
{
    char text[12];

    snprintf(text, sizeof text, "%d", item_id);
    return item_table_get(atoi(text))->name;
}

void game_mode_cash_item_delete(const unsigned char *pkt)
{
    const struct PACKET_ZC_CASH_ITEM_DELETE *packet = (const struct PACKET_ZC_CASH_ITEM_DELETE *)pkt;
    char line[260];
    const char *name;

    session_remove_inventory_item(&g_session1, packet->index, 1);
    name = item_name(packet->item_id);
    sprintf(line, msg_str(0x4ea), name);
    rental_line(line);
}

void game_mode_cash_time_counter(const unsigned char *pkt)
{
    const struct PACKET_ZC_CASH_TIME_COUNTER *packet = (const struct PACKET_ZC_CASH_TIME_COUNTER *)pkt;
    char line[260];
    const char *name = item_name(packet->item_id);
    unsigned seconds = packet->seconds;
    int minutes = (int)(seconds / 60);

    if (minutes > 0)
        sprintf(line, msg_str(0x4e8), name, minutes);
    else
        sprintf(line, msg_str(0x4f6), name, seconds);
    rental_line(line);
    if (minutes == 1) {
        sprintf(line, msg_str(0x4e9), name);
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)line, 0xffff, 0, 0);
        ui_window_mgr_send_msg_(&g_windowMgr, 0x13, (int)line, 0xffff, 0, 0);
    }
}
