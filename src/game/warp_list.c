#include "game/packet_handlers.h"
#include <string.h>
#include "game/gamemode.h"
#include "game/ui_bridge.h"
#include "core/msg_str.h"

enum {
    WID_WARP_PORTAL = 0x27,
    WID_WARP_LIST = 0x134,
    WM_WARP_PORTAL_SKILL = 0x3b,
    WM_WARP_PORTAL_ADD = 0x1b,
    WM_WARP_LIST_ADD = 0x22,
};

#pragma pack(push, 1)
struct WarpMapName {
    char name[16];
};
struct PacketWarpList2 {
    unsigned short packet_type;
    short packet_length;
    unsigned short skill_id;
    struct WarpMapName maps[1];
};
struct PacketWarpList {
    unsigned short packet_type;
    unsigned short skill_id;
    struct WarpMapName maps[4];
};
struct PacketShowWarpList {
    unsigned short packet_type;
    short packet_length;
    struct WarpMapName maps[1];
};
#pragma pack(pop)

static void add_maps(struct UIWindow *wnd, int msg, struct WarpMapName *map, int count)
{
    for (; count > 0; count--, map++) {
        map->name[15] = 0;
        if (strlen(map->name) != 0)
            ui_window_send(wnd, NULL, msg, (int)map->name, 0, 0, 0);
    }
}

void game_mode_on_warp_list2(struct GameMode *self, struct PacketWarpList2 *packet)
{
    int count = (packet->packet_length - 6) / 16;
    struct UIWindow *wnd;

    (void)self;
    ui_close_window(WID_WARP_PORTAL);
    wnd = ui_make_window(WID_WARP_PORTAL);
    ui_window_send(wnd, NULL, WM_WARP_PORTAL_SKILL, packet->skill_id, 0, 0, 0);
    add_maps(wnd, WM_WARP_PORTAL_ADD, packet->maps, count);
    ui_window_send(wnd, NULL, WM_WARP_PORTAL_ADD, (int)"cancel", 0, 0, 0);
}

void game_mode_on_show_warp_list(struct GameMode *self, struct PacketShowWarpList *packet)
{
    int count = (packet->packet_length - 4) / 16;
    struct UIWindow *wnd;

    (void)self;
    ui_close_window(WID_WARP_LIST);
    wnd = ui_make_window(WID_WARP_LIST);
    add_maps(wnd, WM_WARP_LIST_ADD, packet->maps, count);
}

void game_mode_on_warp_list(struct GameMode *self, const struct PacketWarpList *packet)
{
    struct UIWindow *wnd;
    int i;

    (void)self;
    ui_close_window(WID_WARP_PORTAL);
    wnd = ui_make_window(WID_WARP_PORTAL);
    ui_window_send(wnd, NULL, WM_WARP_PORTAL_SKILL, packet->skill_id, 0, 0, 0);
    for (i = 0; i < 4; i++) {
        if (strlen(packet->maps[i].name) != 0)
            ui_window_send(wnd, NULL, WM_WARP_PORTAL_ADD, (int)packet->maps[i].name, 0, 0, 0);
    }
    ui_window_send(wnd, NULL, WM_WARP_PORTAL_ADD, (int)"cancel", 0, 0, 0);
}

#define UIM_LOG_LINE           0x13
#pragma pack(push, 1)
struct PACKET_ZC_ACK_REMEMBER_WARPPOINT {
    unsigned short packet_type;
    unsigned char result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_REMEMBER_WARPPOINT, 3);
void game_mode_zc_ack_remember_warppoint(const unsigned char *pkt)
{
    static const struct { short msg; int color; } results[4] = {
        { 0xd9, 0xffff00 },
        { 0xd6, 0xff },
        { 0xd8, 0xff },
        { 0x9e7, 0xff },
    };
    int result = ((const struct PACKET_ZC_ACK_REMEMBER_WARPPOINT *)pkt)->result;

    if (result > 3)
        return;
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (int)msg_str(results[result].msg), results[result].color,
                        0, 0);
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_LOG_LINE, (int)msg_str(results[result].msg),
                        results[result].color, 0, 0);
}
