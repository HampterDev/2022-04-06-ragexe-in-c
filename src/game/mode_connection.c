#include "game/packet_handlers.h"
#include "game/gamemode.h"
#include "game/mode_callbacks.h"
#include "std/string.h"
#include "game/own_states.h"
#include "navi/navi_manager.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "world/mode_dispatch.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include <stdio.h>
#include <windows.h>
#include <mmsystem.h>
#include <winsock.h>
#include "game/connection.h"
#include "game/client.h"
#include "login/zone_addr.h"
#include "render/render_api.h"
#include "platform/language.h"
#include "std/std.h"
#include "core/msg_str.h"
#include "session/session_options.h"
#include "session/session_party.h"
#include "session/session_state.h"
#include "session/session_talk.h"
#include "ui/character/character.h"
#include "ui/items/styling_shop.h"
#include "ui/ui_std.h"
#include "ui/wndmgr/wndmgr.h"
#include "core/app.h"
#include "core/text_util.h"
#include "login/zone.h"

static unsigned int g_keepAliveInterval = 12000u;

#define CZ_KEEP_ALIVE       0x360
#define CZ_ENTER            0x436

struct List g_keepAliveTimes;
DWORD g_firstKeepAliveTime;

#include "actor/render_ops.h"

#define WND_LOADING          6
#define MSGBOX_RESULT_CLOSED 0x105
#define MODE_MSG_TO_LOGIN    0x8d

#pragma pack(push, 1)
struct PacketKeepAlive {
    unsigned short id;
    DWORD time;
};
struct PacketEnter {
    unsigned short id;
    int account_id;
    int char_id;
    int auth_code;
    int reserved;
    DWORD time;
    char sex;
};
#pragma pack(pop)

void game_mode_send_keep_alive(struct GameMode *self, char force)
{
    struct PacketKeepAlive packet;

    if (!force && timeGetTime() <= self->server_time_requested_at + g_keepAliveInterval)
        return;

    self->server_time_answered = 0;
    self->server_time_requested_at = timeGetTime();
    packet.id = CZ_KEEP_ALIVE;
    packet.time = timeGetTime();
    send_packet((short)packet.id, &packet);

    if (g_trackPing) {
        DWORD now = timeGetTime();

        list_push_back(&g_keepAliveTimes, &now, sizeof now);
        if (g_firstKeepAliveTime == 0)
            g_firstKeepAliveTime = *LIST_VALUE(g_keepAliveTimes.head->next, DWORD);
    }
}

#pragma pack(push, 1)
struct PACKET_ZC_NOTIFY_TIME {
    unsigned short packet_type;
    DWORD server_time;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_NOTIFY_TIME, 6);

void game_mode_zc_notify_time(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_NOTIFY_TIME *packet = (const struct PACKET_ZC_NOTIFY_TIME *)pkt;

    if (g_trackPing) {
        FILE *log = fopen("PingLog.txt", "at");

        if (log) {
            DWORD sent = *LIST_VALUE(g_keepAliveTimes.head->next, DWORD);
            DWORD round_trip = timeGetTime() - sent;

            fprintf(log, "%10.2f\t%10d\n", (double)((float)(double)(sent - g_firstKeepAliveTime) / 1000.0f),
                        round_trip);
            fclose(log);
        }
        list_erase(&g_keepAliveTimes, g_keepAliveTimes.head->next, NULL);
    }
    if (self->server_time_syncs == 0)
        session_set_server_time(&g_session1, packet->server_time);
    else
        session_sync_server_time(&g_session1, packet->server_time);
    self->server_time_syncs++;
    self->server_time_answered = 1;
    session_push_ping_sample(&g_session1, timeGetTime() - self->server_time_requested_at);
}

static int enter_zone(struct GameMode *self)
{
    struct PacketEnter packet = {0};
    struct RagConnection *connection;

    self->base.connected = (unsigned char)connection_connect(rag_connection_transport(rag_connection_instance()), &g_mapServerIp);
    if (!self->base.connected) {
        connection_disconnect(rag_connection_transport(rag_connection_instance()));
        if (ui_window_mgr_message_box(&g_windowMgr, msg_str(1), 1, 1, 1, 0, UI_MSGBOX_TITLE, 0x118, 0x78, 0)
            != MSGBOX_RESULT_CLOSED)
            game_mode_send_msg(&self->base, MODE_MSG_TO_LOGIN, 0, 0, 0, 0);
        return 0;
    }

    connection = rag_connection_instance();
    connection_delay_sends(rag_connection_transport(connection), 1000);
    packet.id = CZ_ENTER;
    packet.time = timeGetTime();
    packet.account_id = g_session1.ownAid;
    packet.char_id = g_session1.ownGid;
    packet.auth_code = ((int)g_session1.authCode);
    packet.sex = session_get_sex(&g_session1);
    send_packet((short)packet.id, &packet);
    session_set_pair(&g_session1, 0x11, 1);
    connection_set_hold_sends(rag_connection_transport(rag_connection_instance()), 1);
    return 1;
}

void game_mode_move_to_server(struct GameMode *self, struct String map_name, int x, int y,
                                      unsigned long ip, int port, struct String server_map_name)
{
    struct String name;
    void *player, *loading;

    name_card_holder_clear(&self->actor_names);
    session_reset_store_search(&g_session1);
    session_clear_hotkey_tab0(&g_session1);
    session_clear_hotkey_tab1(&g_session1);
    g_session1.ownDressUp = 0;
    session_clear_items_and_equipment(&g_session1);
    session_clear_party_members(&g_session1);

    string_copy(&name, &map_name);
    text_drop_map_extension(&name);
    _snprintf_s((*(char (*)[20])&g_session1.mapName), 0x14, _TRUNCATE, "%s", string_cstr(&name));
    g_session1.playerPosX = x;
    g_session1.playerPosY = y;
    _snprintf_s(g_mapServerIp.ip, sizeof g_mapServerIp.ip, _TRUNCATE, inet_ntoa(*(struct in_addr *)&ip));
    g_mapServerIp.port = port;
    _snprintf_s(g_mapServerIp.host, sizeof g_mapServerIp.host, _TRUNCATE, "%s", string_cstr(&server_map_name));
    connection_disconnect(rag_connection_transport(rag_connection_instance()));

    own_states_clear();

    self->server_move_pending = 1;
    self->server_move_time = timeGetTime();
    player = self->world->player;
    render_object_send(player, NULL, 8, 0, 0, 0, 0);
    if (g_wobbleFramesLeft > 0)
        g_wobbleFramesLeft = g_wobbleMode == 2 ? 10001 : g_wobbleFramesLeft;

    if (enter_zone(self)) {
        loading = ui_window_open(WND_LOADING);
        ui_wait_wnd_set_text(loading, msg_str(0x7a));
        g_windowMgr.modal_result = 0xb1;
    }
    string_free(&name);
    string_free(&map_name);
    string_free(&server_map_name);
}

enum { WID_MEMORIAL_DUNGEON = 0x89 };
enum { MODE_REQ_LOGIN_WINDOW = 0x8d };
enum { MODE_REQ_QUIT = 2 };

static int ban_message(unsigned code)
{
    switch (code) {
    case 0:    return 3;
    case 1:    return 4;
    case 2:    return 5;
    case 3:    return 0xf1;
    case 4:    return 0x108;
    case 5:    return 0x131;
    case 9:    return 0x211;
    case 10:   return 0x212;
    case 15:   return 0;
    case 0x6e: return 0x635;
    case 0x71: return 0x7e5;
    case 0x73: return 0x92b;
    default:   return 3;
    }
}

void game_mode_notify_ban(struct GameMode *self, const struct PACKET_SC_NOTIFY_BAN *packet)
{
    unsigned code = packet->code;
    struct String text;
    int msg;

    connection_disconnect(rag_connection_transport(rag_connection_instance()));
    string_init(&text);
    msg = ban_message(code);
    if (msg != 0) {
        struct String code_text, with_msg, with_open, with_code, line;

        string_init(&code_text);
        string_format(&code_text, "%d", (int)code);
        string_copy(&with_msg, &text);
        string_append_cstr(&with_msg, msg_str(msg));
        string_append_cstr(&with_msg, "(");
        string_move(&with_open, &with_msg);
        string_append(&with_open, string_cstr(&code_text), code_text.size);
        string_move(&with_code, &with_open);
        string_append_cstr(&with_code, ")");
        string_move(&line, &with_code);
        string_free(&text);
        string_move(&text, &line);
        string_free(&with_code);
        string_free(&with_open);
        string_free(&with_msg);
        string_free(&code_text);
    }
    if (text.size == 0) {
        const char *fallback = msg_str(0x718);
        string_assign_cstr(&text, fallback);
    }
    ui_window_mgr_message_box(&g_windowMgr, string_cstr(&text), 0, 1, 1, 0, UI_MSGBOX_TITLE, 0x118, 0x78, 0);
    mode_send_msg(self, MODE_REQ_QUIT, 0, 0, 0, 0);
    string_free(&text);
}

void game_mode_isvr_disconnect(struct GameMode *self)
{
    if (g_windowMgr.memorial_dun_wnd == 0)
        return;
    ui_window_mgr_post_quit1(&g_windowMgr, WID_MEMORIAL_DUNGEON);
    connection_disconnect(rag_connection_transport(rag_connection_instance()));
    if (ui_window_mgr_message_box(&g_windowMgr, msg_str(0x545), 0, 1, 1, 0, UI_MSGBOX_TITLE, 0x118, 0x78, 0)
        != MSGBOX_RESULT_CLOSED)
        mode_send_msg(self, MODE_REQ_LOGIN_WINDOW, 0, 0, 0, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_ACK_REQ_DISCONNECT {
    unsigned short id;
    short result;
};
#pragma pack(pop)
enum { MSG_CANNOT_QUIT_NOW = 0x1f6 };

void game_mode_ack_req_disconnect(struct GameMode *self, const struct PACKET_ZC_ACK_REQ_DISCONNECT *packet)
{
    if (packet->result == 0) {
        connection_disconnect(rag_connection_transport(rag_connection_instance()));
        mode_send_msg(self, MODE_REQ_QUIT, 0, 0, 0, 0);
    } else if (packet->result == 1) {
        ui_window_mgr_send_msg_(&g_windowMgr, UIWM_CHAT_PRINT, (int)msg_str(MSG_CANNOT_QUIT_NOW), CHAT_COLOR_RED,
                              0, 0);
    }
}

#define LOGIN_START_RESTART     5
#define MODE_LOGIN              0

#pragma pack(push, 1)
struct PACKET_ZC_RESTART_ACK {
    unsigned short packet_type;
    unsigned char result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_RESTART_ACK, 3);

void game_mode_zc_restart_ack(const unsigned char *pkt)
{
    const struct PACKET_ZC_RESTART_ACK *packet = (const struct PACKET_ZC_RESTART_ACK *)pkt;

    navi_manager_reset(&g_navi, 1);
    navi_manager_clear_target_and_marker(&g_navi);
    g_navi.windowPage = 0;
    if (packet->result == 1) {
        connection_disconnect(rag_connection_transport(rag_connection_instance()));
        g_loginStartMode = LOGIN_START_RESTART;
        mode_mgr_switch(&g_modeMgr, MODE_LOGIN, "login.rsw");
    }
}
