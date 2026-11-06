#include "game/barter_packets.h"
#include "game/item_list_packets.h"
#include "game/quest_packets.h"
#include "ui/window_access.h"
#include "replay/replay.h"
#include "replay/packet_reader.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#include "contents/connection_api.h"
#include <string.h>
#include <windows.h>
#include <mmsystem.h>
#include "game/gamemode.h"
#include "game/client.h"
#include "game/replay.h"
#include "game/packet_dispatch.h"
#include "login/loginmode.h"
#include "ui/window.h"
#include "core/unknown_packet.h"

#define MODE_MSG_DISCONNECTED   1
#define WND_MSG_REPLAY_TICK     0x3c
#define WND_MSG_RECORDING       0x74

void game_mode_poll_network_msg(struct GameMode *self)
{
    int replay_state;
    unsigned short packet_len;
    size_t packet_size;
    unsigned short packet_id;
    char received;

    memset(g_packetBuffer, 0, 0x10000);
    if (!g_replayPlaying) {
        if (!connection_poll(rag_connection_transport(rag_connection_instance())))
            mode_send_message(client_current_mode(), MODE_MSG_DISCONNECTED, 0, 0, 0, 0);
        if (g_mapServerAidPending == 1) {
            char account_id[4];
            if (connection_recv_bytes(rag_connection_transport(rag_connection_instance()), account_id, 4, 1))
                g_mapServerAidPending = 0;
            return;
        }
    }

    replay_state = replay_recorder_instance()->state;
    if (replay_state == REPLAY_STATE_PLAYING) {
        ui_window_send(g_windowMgr.replay_control_wnd, NULL, WND_MSG_REPLAY_TICK, 0, 0, 0, 0);
    } else if (replay_state == REPLAY_STATE_RECORDING) {
        if (!replay_recorder_on_event(replay_recorder_instance(), REPLAY_EVENT_RECORD)) {
            replay_recorder_close(replay_recorder_instance());
            replay_recorder_set_mode(replay_recorder_instance(), REPLAY_STATE_IDLE);
            replay_state = REPLAY_STATE_IDLE;
        }
        if (replay_state == REPLAY_STATE_RECORDING && g_windowMgr.replay_reccontrol_wnd)
            ui_window_send(g_windowMgr.replay_reccontrol_wnd, NULL, WND_MSG_RECORDING, 0, 0, 0, 0);
    }

    packet_len = 0;
    packet_id = 0;
    for (;;) {
        if (!g_replayPlaying) {
            received = rag_connection_get_packet(rag_connection_instance(), g_packetBuffer, &packet_len);
            packet_size = packet_len;
            if (received) {
                rag_connection_instance();
                packet_id = rag_connection_get_packet_type(g_packetBuffer);
                if (replay_state == REPLAY_STATE_RECORDING)
                    replay_recorder_write_packet(replay_recorder_instance(), (unsigned short)packet_len,
                                               g_packetBuffer, REPLAY_TIME_NOW, 0);
            }
            replay_recorder_capture_packet(replay_recorder_instance(), packet_id, (unsigned short)packet_len,
                                         (const char *)g_packetBuffer);
        } else {
            received = (char)replay_read_packet(g_packetBuffer, 0x10000, &packet_id, &packet_size);
        }
        if (!received)
            return;
        g_session1.lastPacketTime = timeGetTime();
        /* game_mode_poll_network_msg reads on to the last packet received even after one that ends
         * the mode (a map move, a return to the character select), as the client's own loop does: what
         * came in with that packet is handled by the mode it came to. The "Warped." line sent behind a move
         * to another map goes to the old world's player, so no balloon follows the player onto the new map. */
        game_mode_dispatch_packet(self, (short)packet_id, g_packetBuffer, packet_size);
    }
}
