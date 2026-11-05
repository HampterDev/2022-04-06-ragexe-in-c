#include "game/barter_packets.h"
#include "game/item_list_packets.h"
#include "game/quest_packets.h"
#include "ui/window_access.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#include "contents/connection_api.h"
#include <string.h>
#include "platform/win_types.h"
#include "game/gamemode.h"
#include "game/client.h"
#include "game/packet_dispatch.h"
#include "login/loginmode.h"
#include "ui/window.h"
#include "core/unknown_packet.h"
#include "platform/clock.h"

#define MODE_MSG_DISCONNECTED   1

void game_mode_poll_network_msg(struct GameMode *self)
{
    unsigned short packet_len = 0;
    unsigned short packet_id;

    memset(g_packetBuffer, 0, 0x10000);
    if (!connection_poll(rag_connection_transport(rag_connection_instance())))
        mode_send_message(client_current_mode(), MODE_MSG_DISCONNECTED, 0, 0, 0, 0);
    if (g_mapServerAidPending == 1) {
        char account_id[4];
        if (connection_recv_bytes(rag_connection_transport(rag_connection_instance()), account_id, 4, 1))
            g_mapServerAidPending = 0;
        return;
    }
    for (;;) {
        if (!rag_connection_get_packet(rag_connection_instance(), g_packetBuffer, &packet_len))
            return;
        packet_id = rag_connection_get_packet_type(g_packetBuffer);
        g_session1.lastPacketTime = platform_ticks_ms();
        /* game_mode_poll_network_msg reads on to the last packet received even after one that ends
         * the mode (a map move, a return to the character select), as the client's own loop does: what
         * came in with that packet is handled by the mode it came to. The "Warped." line sent behind a move
         * to another map goes to the old world's player, so no balloon follows the player onto the new map. */
        game_mode_dispatch_packet(self, (short)packet_id, g_packetBuffer, packet_len);
    }
}
