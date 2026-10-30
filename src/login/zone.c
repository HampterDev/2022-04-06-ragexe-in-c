#include "session/state.h"
#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include "login/loginmode.h"
#include "std/std.h"
#include "login/zone_addr.h"

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <winsock2.h>
#include "core/msg_str.h"
#include "session/session_items.h"
#include "session/session_state.h"
#include "ui/character/character.h"
#include "game/connection.h"
#include "login/zone.h"

struct ServerAddress g_mapServerIp;
void login_mode_enter_zone(struct LoginMode *self, DWORD gid,
                                     struct String mapName, DWORD ip, DWORD port,
                                     struct String host)
{
    struct String map;
    struct in_addr addr;
    size_t len;

    g_session1.ownGid = gid;
    CMODE_SENDMSG(client_current_mode(), 0x2719, g_loginCharInfo.slot, 0, 0, 0);
    if (g_windowMgr.new_select_char_wnd)
        ui_window_mgr_post_quit2(&g_windowMgr, g_windowMgr.new_select_char_wnd);

    string_copy(&map, &mapName);
    len = map.size - 4;
    string_resize(&map, len, 0);
    strcpy(g_session1.mapName, string_cstr(&map));

    addr.s_addr = ip;
    strcpy(g_mapServerIp.ip, inet_ntoa(addr));
    g_mapServerIp.port = port;
    memcpy_s(g_mapServerIp.host, sizeof g_mapServerIp.host, string_cstr(&host),
               host.size + 1 > 0x81 ? 0x81 : host.size + 1);

    connection_disconnect(rag_connection_transport(rag_connection_instance()));
    self->base.next_sub_mode = 0xc;
    string_free(&map);
    string_free(&mapName);
    string_free(&host);
}

BYTE g_trackPing;

static void zone_pos_dir(const unsigned char *pos_dir)
{
    session_set_player_pos_dir(&g_session1,
                              (pos_dir[0] << 2) | (pos_dir[1] >> 6),
                              ((pos_dir[1] & 0x3f) << 4) | (pos_dir[2] >> 4),
                              pos_dir[2] & 0xf);
}

static void zone_switch(void)
{
    char name[36];
    void *wnd;

    sprintf(name, "%s.rsw", g_session1.mapName);
    mode_mgr_switch(&g_modeMgr, 1, name);
    wnd = ui_window_open(6);
    ui_wait_wnd_set_text(wnd, msg_str(0x79));
    connection_set_hold_sends(rag_connection_transport(rag_connection_instance()), 0);
}

static void zone_ping_log(void)
{
    FILE *f;
    __time32_t t;

    f = fopen("PingLog.txt", "at");
    if (f) {
        _time32(&t);
        fprintf(f, "\n");
        fprintf(f, "server:%s\n", string_cstr(&g_session1.worldName));
        fprintf(f, "ip:%s\n", g_mapServerIp.ip);
        fprintf(f, "map:%s.gat\n", g_session1.mapName);
        fprintf(f, "time:%s\n\n", _ctime32(&t));
        fclose(f);
    }
    list_clear(&g_keepAliveTimes, NULL);
    g_firstKeepAliveTime = 0;
}

void login_mode_on_zone_accept(struct LoginMode *self, const BYTE *pkt)
{
    const struct PACKET_ZC_ACCEPT_ENTER2 *packet = (const struct PACKET_ZC_ACCEPT_ENTER2 *)pkt;
    struct Vec *names = &g_session1.accountCharNames;
    int i;

    session_set_server_time(&g_session1, packet->start_time);
    session_set_pair(&g_session1, 0xb, 1);
    zone_pos_dir(packet->pos_dir);
    g_session1.font = packet->font;
    zone_switch();
    session_set_pair(&g_session1, 0x11, 0);
    if (g_trackPing)
        zone_ping_log();
    session_set_my_exchange_money(&g_session1, 0);

    string_vec_clear(names);

    for (i = 0; i < LOGIN_MAX_CHARS; i++) {
        const char *name = self->chars[i].name;

        string_vec_push_text(names, name, strlen(name));
    }
    memset(self->chars, 0, sizeof self->chars);
}

void login_mode_on_zone_accept2(const BYTE *pkt)
{
    const struct PACKET_ZC_ACCEPT_ENTER *packet = (const struct PACKET_ZC_ACCEPT_ENTER *)pkt;

    session_set_server_time(&g_session1, packet->start_time);
    zone_pos_dir(packet->pos_dir);
    zone_switch();
    if (g_trackPing)
        zone_ping_log();
}
