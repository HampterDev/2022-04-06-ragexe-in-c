#include "game/actor_packets.h"
#include "game/gamemode.h"
#include "game/ground_item_packets.h"
#include "actor/map_type_table.h"
#include "game/client_state.h"
#include "platform/language.h"
#include "actor/pc.h"
#include "core/msg_str.h"
#include "game/packet_data.h"
#include "session/session_jobs.h"
#include "session/session_windows.h"

unsigned char g_mapPropertyQuiet;
#include "contents/connection_api.h"
#include "game/packet_handlers.h"
#include "game/client.h"

#define CZ_REQUEST_AFTER_MAPPROPERTY 0x447
#define COLOUR_NOTICE 0xffff64

static const int kR2Flags[] = { 7, 8, 9, 0xf, 0x10, 0x11, 0x12, 0x13, 0x14, 0x28, 0x29 };

#pragma pack(push, 1)
struct PacketMapProperty {
    unsigned short header;
    unsigned short type;
};
struct PacketMapPropertyR2 {
    unsigned short header;
    short type;
    unsigned flags;
};
#pragma pack(pop)

static void say_notice(int msg_id)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (int)msg_str(msg_id), COLOUR_NOTICE, 0, 0);
}

static void apply_flag_f(void)
{
    int value = g_session1.map_rule_flags[0];

    if (session_map_uses_simple_effect(&g_session1))
        value = 1;
    g_session1.map_rule_flags[0] = value;
}

void game_mode_notify_map_property(struct GameMode *self, const struct PacketMapProperty *p)
{
    g_session1.map_rule_flags[0] = g_session1.map_rule_flags[1];
    switch (p->type) {
    case 1:
        self->world->is_pvp = 1;
        break;
    case 2:
        self->world->is_event_pvp = 1;
        break;
    case 4:
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (int)msg_str(0x357), 0x9b9bff, 0, 0);
        break;
    case 5:
        self->world->is_pvp = 1;
        break;
    case 3:
        self->world->is_siege = 1;
        g_session1.map_rule_flags[0] = 1;
        break;
    }
    apply_flag_f();
    map_property_mgr_clear(g_mapFlags);
}

void game_mode_notify_map_property2(struct GameMode *self, const struct PacketMapProperty *p)
{
    unsigned short type;

    g_session1.map_rule_flags[0] = g_session1.map_rule_flags[1];
    g_session1.mapType = (short)p->type;
    type = p->type;
    if (type != 0 && type != 1) {
        session_set_hide_other_players(&g_session1, 0);
        type = p->type;
    }
    if (type == 5 || type == 6) {
        self->world->is_pvp = 1;
        self->attack_repeat = 1;
    } else if (type != 0xb && type != 0xc) {
        if (session_map_is_siege(&g_session1) || p->type == 0x12 || p->type == 8) {
            self->world->is_siege = 1;
            g_session1.map_rule_flags[0] = 1;
        } else if (p->type == 0x13) {
            self->world->is_battleground = 1;
            self->world->is_pvp = 1;
        }
    }
    apply_flag_f();
    if (g_mapPropertyQuiet == 0 && self->world && self->world->player)
        player_refresh_status_effects(self->world->player);
}

static void notice_if_no_miracle(void)
{
    if (map_property_mgr_get(g_mapFlags, g_session1.mapType, 0x29) == 0)
        say_notice(0x94d);
}

void game_mode_map_property_r2(struct GameMode *self, const struct PacketMapPropertyR2 *p)
{
    struct RagConnection *conn;
    unsigned short request;
    int i, len;

    for (i = 0; i < (int)(sizeof kR2Flags / sizeof kR2Flags[0]); i++)
        map_property_mgr_set(g_mapFlags, p->type, kR2Flags[i], (p->flags >> i) & 1);

    g_session1.mapType = p->type;
    g_session1.map_rule_flags[0] = g_session1.map_rule_flags[1];
    g_session1.map_rule_flags[2] = 0;
    g_session1.map_rule_flags[3] = 0;
    g_session1.map_rule_flags[4] = 0;
    apply_flag_f();
    if (session_map_disables_lock_on(&g_session1))
        g_session1.map_rule_flags[2] = 1;
    if (session_map_counts_pk(&g_session1))
        g_session1.map_rule_flags[3] = 1;
    if (session_map_locks_party(&g_session1))
        g_session1.map_rule_flags[4] = 1;
    if (session_map_is_siege(&g_session1))
        self->world->is_siege = 1;
    if (session_map_is_battlefield(&g_session1))
        self->world->is_battleground = 1;

    if (job_get_base_job(session_get_drawn_job(&g_session1)) == 0xfcf)
        notice_if_no_miracle();
    if (job_get_base_job(session_get_drawn_job(&g_session1)) == 0x1082)
        notice_if_no_miracle();
    if (map_property_mgr_get(g_mapFlags, p->type, 0x14))
        say_notice(0x6ec);

    request = CZ_REQUEST_AFTER_MAPPROPERTY;
    conn = rag_connection_instance();
    len = rag_connection_get_packet_length(conn, CZ_REQUEST_AFTER_MAPPROPERTY);
    rag_connection_send_packet(rag_connection_instance(), len, &request);
}

#pragma pack(push, 1)
struct PACKET_ZC_NOTIFY_MAPINFO {
    unsigned short id;
    short type;
};
#pragma pack(pop)

enum {
    UIM_NOTICE_LOG = 0x13,
    MSG_CANNOT_TELEPORT = 0x1f4,
    MSG_CANNOT_MEMO = 0x1f5,
    MSG_MAPINFO_2 = 0x536,
    MSG_MAPINFO_3 = 0x537,
};

static void map_info_line(int msg_id, unsigned colour)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (int)msg_str(msg_id), colour, 0, 0);
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_NOTICE_LOG, (int)msg_str(msg_id), colour, 0, 0);
}

void game_mode_zc_notify_map_info(struct GameMode *self, const struct PACKET_ZC_NOTIFY_MAPINFO *packet)
{
    switch (packet->type) {
    case 0:
        map_info_line(MSG_CANNOT_TELEPORT, 0xff);
        break;
    case 1:
        map_info_line(MSG_CANNOT_MEMO, 0xffff00);
        break;
    case 2:
        map_info_line(MSG_MAPINFO_2, 0xffff00);
        break;
    case 3:
        self->use_item_pending = 0;
        map_info_line(MSG_MAPINFO_3, 0xffff00);
        break;
    }
}
