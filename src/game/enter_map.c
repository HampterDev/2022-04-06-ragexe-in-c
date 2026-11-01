#include "platform/filesystem.h"
#include "game/actor_packets.h"
#include "game/gamemode.h"
#include "game/ground_item_packets.h"
#include "session/option_pairs.h"
#include "world/mode_manager.h"
#include "contents/connection_api.h"
#include <stdio.h>
#include <time.h>
#include "game/packet_handlers.h"
#include "game/client.h"
#include "login/zone_addr.h"
#include "session/session_state.h"
#include "ui/items/styling_shop.h"
#include "game/connection.h"
#include "login/zone.h"

#define MODE_GAME 1

static void set_position(const unsigned char *pos_dir)
{
    session_set_player_pos_dir(&g_session1, (pos_dir[0] << 2) | (pos_dir[1] >> 6),
                              ((pos_dir[1] & 0x3f) << 4) | (pos_dir[2] >> 4), pos_dir[2] & 0xf);
}

static void switch_to_map(void)
{
    char world[36];

    sprintf(world, "%s.rsw", g_session1.mapName);
    mode_mgr_switch(&g_modeMgr, MODE_GAME, world);
}

static void log_entry(void)
{
    FILE *f;
    __time32_t now;

    f = platform_file_open("PingLog.txt", "at");
    if (f != 0) {
        _time32(&now);
        fprintf(f, "\n");
        fprintf(f, "server:%s\n", string_cstr(&g_session1.worldName));
        fprintf(f, "ip:%s\n", g_mapServerIp.ip);
        fprintf(f, "map:%s.gat\n", g_session1.mapName);
        fprintf(f, "time:%s\n\n", _ctime32(&now));
        fclose(f);
    }
    list_clear(&g_keepAliveTimes, NULL);
    g_firstKeepAliveTime = 0;
}

void game_mode_accept_enter2(const unsigned char *pkt)
{
    const struct PACKET_ZC_ACCEPT_ENTER2 *packet = (const struct PACKET_ZC_ACCEPT_ENTER2 *)pkt;
    session_set_server_time(&g_session1, packet->start_time);
    set_position(packet->pos_dir);
    g_session1.font = packet->font;
    switch_to_map();
    session_set_pair(&g_session1, 0x11, 0);
    connection_set_hold_sends(rag_connection_transport(rag_connection_instance()), 0);
    if (g_trackPing)
        log_entry();
}

void game_mode_accept_enter(const unsigned char *pkt)
{
    const struct PACKET_ZC_ACCEPT_ENTER *packet = (const struct PACKET_ZC_ACCEPT_ENTER *)pkt;

    session_set_server_time(&g_session1, packet->start_time);
    set_position(packet->pos_dir);
    switch_to_map();
    connection_set_hold_sends(rag_connection_transport(rag_connection_instance()), 0);
    if (g_trackPing)
        log_entry();
}
