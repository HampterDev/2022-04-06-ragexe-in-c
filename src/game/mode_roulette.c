#include "game/packet_handlers.h"
#include "ui/window_manager_state.h"
#include "game/gamemode.h"
#include "game/ui_bridge.h"
#include "game/roulette_state.h"
#include "core/msg_str.h"
#include "ui/wndmgr/wndmgr.h"
#include "ui/shops/roulette.h"

#define CHAT_COLOR_ROULETTE 0x006464ff

#pragma pack(push, 1)
struct PacketRecvRouletteItem {
    unsigned short packet_type;
    unsigned char result;
    int item;
};
struct PacketUpdateRouletteCoin {
    unsigned short packet_type;
    unsigned char type;
    int amount;
};
#pragma pack(pop)

void game_mode_on_recv_roulette_item(struct GameMode *self, const struct PacketRecvRouletteItem *packet)
{
    (void)self;
    switch (packet->result) {
    case 0:
        roulette_set_addition_item(g_roulette, packet->item);
        roulette_set_slot(g_roulette, 0, 0, 0);
        roulette_reset(g_roulette);
        if (g_windowMgr.roullette_wnd)
            session_refresh_merchant_mirror_item_wnd(&g_windowMgr);
        break;
    case 1:
    case 2:
    case 3:
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (int)msg_str(0xa4b + packet->result),
                            CHAT_COLOR_ROULETTE, 0, 0);
        break;
    }
}

void game_mode_on_update_roulette_coin(struct GameMode *self, const struct PacketUpdateRouletteCoin *packet)
{
    (void)self;
    if (packet->type <= 2)
        g_roulette->coins[packet->type] = packet->amount;
    if (g_windowMgr.roullette_wnd)
        session_refresh_merchant_mirror_item_wnd(&g_windowMgr);
}
