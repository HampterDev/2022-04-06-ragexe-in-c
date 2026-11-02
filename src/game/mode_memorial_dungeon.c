#include "game/actor_packets.h"
#include "game/gamemode.h"
#include "game/ground_item_packets.h"
#include <string.h>
#include "game/packet_handlers.h"
#include "game/client.h"
#include "core/msg_str.h"

enum {
    MD_QUEUED = 0,
    MD_ENTERED = 1,
    MD_MADE = 2,
};

#define WINDOW_MEMORIAL_DUNGEON 0x89
#define COLOUR_MEMORIAL_DUNGEON 0x64ffff

static int state_of(int enter_limit)
{
    return enter_limit != 0 ? MD_ENTERED : MD_MADE;
}

static void refresh_window(void)
{
    struct UIWindow *w = ui_window_open(WINDOW_MEMORIAL_DUNGEON);

    if (w != 0)
        UIWINDOW_SENDMSG(w, 0x3c, 0, 0, 0, 0);
}

static void close_with_line(int msg_id)
{
    ui_window_mgr_post_quit1(&g_windowMgr, WINDOW_MEMORIAL_DUNGEON);
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (intptr_t)msg_str(msg_id), COLOUR_MEMORIAL_DUNGEON, 0, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_MEMORIALDUNGEON_SUBSCRIPTION_INFO {
    unsigned short packet_type;
    char name[61];
    short place;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_MEMORIALDUNGEON_SUBSCRIPTION_INFO, 65);

void game_mode_memorial_dungeon_subscription_info(const unsigned char *pkt)
{
    const struct PACKET_ZC_MEMORIALDUNGEON_SUBSCRIPTION_INFO *packet =
        (const struct PACKET_ZC_MEMORIALDUNGEON_SUBSCRIPTION_INFO *)pkt;
    const char *name = packet->name;

    string_assign_cstr(&g_session1.mdName, name);
    g_session1.mdQueuePlace = packet->place;
    g_session1.mdState = MD_QUEUED;
    refresh_window();
}

#pragma pack(push, 1)
struct PACKET_ZC_MEMORIALDUNGEON_SUBSCRIPTION_NOTIFY {
    unsigned short packet_type;
    short place;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_MEMORIALDUNGEON_SUBSCRIPTION_NOTIFY, 4);

void game_mode_memorial_dungeon_subscription_notify(const unsigned char *pkt)
{
    const struct PACKET_ZC_MEMORIALDUNGEON_SUBSCRIPTION_NOTIFY *packet =
        (const struct PACKET_ZC_MEMORIALDUNGEON_SUBSCRIPTION_NOTIFY *)pkt;
    struct UIWindow *w;

    g_session1.mdQueuePlace = packet->place;
    w = ui_window_open(WINDOW_MEMORIAL_DUNGEON);
    if (w == 0)
        return;
    if (packet->place > -1) {
        g_session1.mdState = MD_QUEUED;
        UIWINDOW_SENDMSG(w, 0x3c, 0, 0, 0, 0);
    } else {
        close_with_line(0x541);
    }
}

#pragma pack(push, 1)
struct PACKET_ZC_MEMORIALDUNGEON_INFO {
    unsigned short packet_type;
    char name[61];
    int destroy_time;
    int enter_limit;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_MEMORIALDUNGEON_INFO, 71);

void game_mode_memorial_dungeon_info(const unsigned char *pkt)
{
    const struct PACKET_ZC_MEMORIALDUNGEON_INFO *packet = (const struct PACKET_ZC_MEMORIALDUNGEON_INFO *)pkt;
    const char *name = packet->name;
    int old_state;

    string_assign_cstr(&g_session1.mdName, name);
    g_session1.mdDestroyTime = packet->destroy_time;
    g_session1.mdEnterLimit = packet->enter_limit;
    old_state = g_session1.mdState;
    g_session1.mdState = state_of(packet->enter_limit);
    if (old_state == g_session1.mdState && g_session1.mdState == MD_MADE && g_session1.mdWndShown == 0)
        return;
    refresh_window();
}

#pragma pack(push, 1)
struct PACKET_ZC_MEMORIALDUNGEON_NOTIFY {
    unsigned short packet_type;
    int type;
    int enter_limit;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_MEMORIALDUNGEON_NOTIFY, 10);

void game_mode_memorial_dungeon_notify(const unsigned char *pkt)
{
    const struct PACKET_ZC_MEMORIALDUNGEON_NOTIFY *packet = (const struct PACKET_ZC_MEMORIALDUNGEON_NOTIFY *)pkt;
    int old_state = g_session1.mdState;
    int enter_limit;

    g_session1.mdEnterLimit = packet->enter_limit;
    enter_limit = packet->enter_limit;
    g_session1.mdState = state_of(enter_limit);
    if (packet->type == 0) {
        if (old_state == state_of(enter_limit) && enter_limit == 0 && g_session1.mdWndShown == 0)
            return;
        refresh_window();
        return;
    }
    switch (packet->type) {
    case 1:
        close_with_line(0x542);
        break;
    case 2:
        close_with_line(0x543);
        break;
    case 3:
        close_with_line(0x544);
        break;
    default:
        ui_window_mgr_post_quit1(&g_windowMgr, WINDOW_MEMORIAL_DUNGEON);
        break;
    }
}
