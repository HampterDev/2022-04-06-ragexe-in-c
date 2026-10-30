#include "game/packet_handlers.h"
#include "ui/window_manager_state.h"
#include <windows.h>
#include <mmsystem.h>
#include "game/packets.h"
#include "session/session_items.h"
#include "session/session_jobs.h"

#define EQUIP_ACK_TIME_BACK     2000
#define CHAT_COLOR_RED          0xff

static void equip_failed(int msg)
{
    ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(msg), CHAT_COLOR_RED, 6, 0);
}

static void take_off(int wear_location)
{
    if (wear_location == 0)
        return;
    if (session_is_costume_or_shadow_location(&g_session1, wear_location))
        session_take_off_costume(&g_session1, wear_location);
    else
        session_take_off_equip(&g_session1, wear_location, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_REQ_WEAR_EQUIP_ACK {
    unsigned short packet_type;
    unsigned short index;
    unsigned short wear_location;
    unsigned short view;
    unsigned char result;
};
struct PACKET_ZC_REQ_TAKEOFF_EQUIP_ACK {
    unsigned short packet_type;
    unsigned short index;
    unsigned short wear_location;
    unsigned char result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_REQ_WEAR_EQUIP_ACK, 9);
SIZE_ASSERT(PACKET_ZC_REQ_TAKEOFF_EQUIP_ACK, 7);

static void wear(const struct PACKET_ZC_REQ_WEAR_EQUIP_ACK *packet)
{
    struct ItemInfo item;

    session_get_inventory_item(&g_session1, &item, packet->index);
    if (item.index != 0) {
        item.view = packet->view;
        item.worn_location = packet->wear_location;
        session_put_on_equip(&g_session1, &item);
    }
    item_info_destruct(&item);
}

void game_mode_zc_req_takeoff_equip_ack2(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_REQ_TAKEOFF_EQUIP_ACK *packet = (const struct PACKET_ZC_REQ_TAKEOFF_EQUIP_ACK *)pkt;

    self->takeoff_ack_time = timeGetTime() - EQUIP_ACK_TIME_BACK;
    switch (packet->result) {
    case 0:
        take_off(packet->wear_location);
        break;
    case 1:
        equip_failed(0x7a1);
        break;
    }
}

void game_mode_ack_takeoff_equip_v5(struct GameMode *self, unsigned char result, int wear_location,
                                           unsigned short index)
{
    self->takeoff_ack_time = timeGetTime() - EQUIP_ACK_TIME_BACK;
    switch (result) {
    case 0:
        if (wear_location == 0)
            break;
        if (session_is_costume_or_shadow_location(&g_session1, wear_location))
            session_take_off_costume(&g_session1, wear_location);
        else
            session_take_off_equip(&g_session1, wear_location, index);
        break;
    case 1:
        equip_failed(0x7a1);
        break;
    }
}

void game_mode_zc_req_takeoff_equip_ack(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_REQ_TAKEOFF_EQUIP_ACK *packet = (const struct PACKET_ZC_REQ_TAKEOFF_EQUIP_ACK *)pkt;

    self->takeoff_ack_time = timeGetTime() - EQUIP_ACK_TIME_BACK;
    if (packet->result != 0)
        take_off(packet->wear_location);
}

void game_mode_zc_req_wear_equip_ack2(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_REQ_WEAR_EQUIP_ACK *packet = (const struct PACKET_ZC_REQ_WEAR_EQUIP_ACK *)pkt;

    self->wear_ack_time = timeGetTime() - EQUIP_ACK_TIME_BACK;
    switch (packet->result) {
    case 0:
        wear(packet);
        break;
    case 1:
        equip_failed(0x7a1);
        break;
    }
}

void game_mode_zc_req_wear_equip_ack(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_REQ_WEAR_EQUIP_ACK *packet = (const struct PACKET_ZC_REQ_WEAR_EQUIP_ACK *)pkt;

    self->wear_ack_time = timeGetTime() - EQUIP_ACK_TIME_BACK;
    switch (packet->result) {
    case 0:
    case 2:
        equip_failed(0x174);
        break;
    case 1:
        wear(packet);
        break;
    }
}

void game_mode_zc_ack_wear_equip_v5(struct GameMode *self, unsigned char result, unsigned short index,
                                              unsigned short view, int wear_location)
{
    struct ItemInfo item;

    self->wear_ack_time = timeGetTime() - EQUIP_ACK_TIME_BACK;
    switch (result) {
    case 0:
        session_get_inventory_item(&g_session1, &item, index);
        if (item.index != 0) {
            item.view = view;
            item.worn_location = wear_location;
            session_put_on_equip(&g_session1, &item);
        }
        item_info_destruct(&item);
        break;
    case 1:
        equip_failed(0x7a1);
        break;
    case 2:
        equip_failed(0x174);
        break;
    }
}
