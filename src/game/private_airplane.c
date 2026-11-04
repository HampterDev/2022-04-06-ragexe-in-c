#include "game/packet_handlers.h"
#include "game/connection.h"
#include "session/state.h"
#include <string.h>
#include "game/gamemode.h"
#include "game/client.h"
#include "core/msg_str.h"

#pragma pack(push, 1)
struct PacketNpcAckMapMove {
    unsigned short packet_type;
    char map_name[16];
    unsigned short x;
    unsigned short y;
};
#pragma pack(pop)
enum { HEADER_ZC_NPCACK_MAPMOVE = 0x91 };

/* map_name is the 16-byte field of ZC_MAPMOVE_PRIVATE_AIRPLANE and
   ZC_SERVERMOVE_PRIVATE_AIRPLANE; a full field has no NUL. */
#define AIRPLANE_MAP_NAME 16

void game_mode_on_map_move_private_airplane(struct GameMode *self, const char *map_name,
                                                 unsigned short x, unsigned short y)
{
    struct PacketNpcAckMapMove move;

    g_session1.loadingAirplane = 1;
    move.packet_type = HEADER_ZC_NPCACK_MAPMOVE;
    /* Field to field, as ZC_NPCACK_MAPMOVE carries it: a full name fills it without a NUL. */
    strncpy(move.map_name, map_name, sizeof move.map_name);
    move.x = x;
    move.y = y;
    game_mode_zc_npcack_mapmove(self, (const unsigned char *)&move);
}

void game_mode_on_server_move_private_airplane(struct GameMode *self, const char *map_name, short x, short y,
                                                    int ip, short port)
{
    struct String extra, map;

    g_session1.loadingAirplane = 1;
    string_init(&extra);
    string_init(&map);
    string_assign(&map, map_name, strnlen(map_name, AIRPLANE_MAP_NAME));
    game_mode_move_to_server(self, map, x, y, ip, port, extra);
}

void game_mode_on_ack_private_airplane_call(struct GameMode *self, int result)
{
    static const int refusal[] = { 0, 0xb09, 0xd05, 0xd04, 0xd03, 0xd18 };
    struct UIWindow *w = g_windowMgr.private_airplane_wnd;

    (void)self;
    if (w == 0 || (unsigned)result > 5)
        return;
    if (result == 0) {
        if (g_windowMgr.modal_wnd != 0)
            g_windowMgr.modal_result = 0xb4;
        return;
    }
    UIWINDOW_SENDMSG(w, 0x9a, (intptr_t)msg_str(refusal[result]), 0, 0, 0);
}
