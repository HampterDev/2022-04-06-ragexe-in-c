#include "game/barter_packets.h"
#include "game/item_list_packets.h"
#include "game/quest_packets.h"
#include "ui/window_access.h"
#include "ui/chat/balloon.h"
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

int g_playTimeHoursNoticed;

#define MS_PER_HOUR             3600000u
#define NOTICE_COLOR            0x62b0ff
#define MODE_MSG_DISCONNECTED   1
#define WND_MSG_REPLAY_TICK     0x3c
#define WND_MSG_RECORDING       0x74

static void notice_play_time(struct GameMode *self)
{
    char text[100];
    int hours = (int)((timeGetTime() - g_session1.loginTick) / MS_PER_HOUR);
    struct UIWindow *wnd;
    struct UIBalloonText *balloon;
    int x;

    if (hours <= 0 || g_playTimeHoursNoticed == hours)
        return;
    self->notice_shown_at = timeGetTime();
    if (self->notice_wnd == 0) {
        void *block = mem_alloc(sizeof(struct UITransBalloonText));
        self->notice_wnd = block ? ui_trans_balloon_text_construct(block) : 0;
        ui_window_create(self->notice_wnd, 8, 8);
        ui_window_mgr_add_to_window_list(&g_windowMgr, self->notice_wnd);
    }
    memset(text, 0, sizeof text);
    sprintf(text, "%d 시간이 경과했습니다.", hours);
    ui_balloon_text_set_text((struct UIBalloonText *)self->notice_wnd, text, 0x48);
    sprintf(text, "과도한 게임이용은 정상적"
                      "인 일상생활에 지장을 줄 "
                      "수 있습니다.");
    ui_trans_balloon_text_add_line((struct UITransBalloonText *)self->notice_wnd, text);
    wnd = self->notice_wnd;
    x = (g_renderer->width - 640) / 2 - wnd->width / 2 + 320;
    ui_window_move_safe(wnd, x, 0x33);
    balloon = (struct UIBalloonText *)self->notice_wnd;
    balloon->colour = NOTICE_COLOR;
    balloon->outline = 0;
    g_playTimeHoursNoticed = hours;
}

void game_mode_poll_network_msg(struct GameMode *self)
{
    int replay_state;
    unsigned short packet_len;
    size_t packet_size;
    unsigned short packet_id;
    char received;

    memset(g_packetBuffer, 0, 0x10000);
    if (!g_replayPlaying) {
        notice_play_time(self);
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
        game_mode_dispatch_packet(self, (short)packet_id, g_packetBuffer, packet_size);
        if (!self->base.loop_cond)
            return;
    }
}
