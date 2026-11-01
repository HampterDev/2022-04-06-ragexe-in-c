#include "game/barter_packets.h"
#include "game/item_list_packets.h"
#include "game/quest_packets.h"
#include "ui/window_access.h"
#include "ui/chat/balloon.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#include "contents/connection_api.h"
#include <string.h>
#include <windows.h>
#include <mmsystem.h>
#include "game/gamemode.h"
#include "game/client.h"
#include "game/packet_dispatch.h"
#include "login/loginmode.h"
#include "ui/window.h"
#include "core/unknown_packet.h"

int g_playTimeHoursNoticed;

#define MS_PER_HOUR             3600000u
#define NOTICE_COLOR            0x62b0ff
#define MODE_MSG_DISCONNECTED   1

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
    unsigned short packet_len = 0;
    unsigned short packet_id;

    memset(g_packetBuffer, 0, 0x10000);
    notice_play_time(self);
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
        g_session1.lastPacketTime = timeGetTime();
        game_mode_dispatch_packet(self, (short)packet_id, g_packetBuffer, packet_len);
        if (!self->base.loop_cond)
            return;
    }
}
