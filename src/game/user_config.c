#include "game/actor_packets.h"
#include "game/gamemode.h"
#include "game/ground_item_packets.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type.h"
#include "ui/window_access.h"
#include "game/packet_handlers.h"
#include "game/user_config.h"
#include "game/ui_bridge.h"
#include "game/session_bridge.h"
#include "core/msg_str.h"
#include "session/session_state.h"

#define CONFIG_COLOR 0x64ffff

enum {
    WID_GRAPHIC_SETTING = 0x131,
};
enum {
    UIM_EQUIP_CONFIG_CHANGED = 0x3c,
    UIM_GRAPHIC_SETTING_SET = 6,
    UIM_GRAPHIC_SETTING_REFRESH = 0x17,
};
#define USER_CONFIG_ENTRY1 1

static void config_line(int msg_id)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)msg_str(msg_id), CONFIG_COLOR, 0, 0);
}

static void config_notify(unsigned char open_equip_window, unsigned char call_allowed)
{
    g_session1.equipOpenToOthers = open_equip_window;
    config_line(0x54e + open_equip_window);
    if (g_windowMgr.equip_wnd != 0)
        ui_window_send(g_windowMgr.equip_wnd, NULL, UIM_EQUIP_CONFIG_CHANGED, 0, 0, 0, 0);
    g_session1.call_allowed = call_allowed;
    config_line(0xba2 + call_allowed);
}

void game_mode_config_notify2(struct GameMode *self, unsigned char open_equip_window,
                                       unsigned char call_allowed)
{
    (void)self;
    config_notify(open_equip_window, call_allowed);
}

void game_mode_config_notify3(struct GameMode *self, unsigned char open_equip_window,
                                       unsigned char call_allowed, unsigned char pet_auto_feed)
{
    (void)self;
    config_notify(open_equip_window, call_allowed);
    session_set_pet_auto_feed(&g_session1, pet_auto_feed);
}

void game_mode_config_notify4(struct GameMode *self, unsigned char open_equip_window,
                                       unsigned char call_allowed, unsigned char pet_auto_feed,
                                       unsigned char homun_auto_feed)
{
    (void)self;
    config_notify(open_equip_window, call_allowed);
    session_set_pet_auto_feed(&g_session1, pet_auto_feed);
    session_set_homun_auto_feed(&g_session1, homun_auto_feed);
}

static void config_equip_window(int open)
{
    g_session1.equipOpenToOthers = open;
    config_line(0x54e + (open != 0));
    if (g_windowMgr.equip_wnd != 0)
        ui_window_send(g_windowMgr.equip_wnd, NULL, UIM_EQUIP_CONFIG_CHANGED, 0, 0, 0, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_CONFIG_NOTIFY {
    unsigned short packet_type;
    unsigned char open_equip_window;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_CONFIG_NOTIFY, 3);

void game_mode_zc_config_notify(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_CONFIG_NOTIFY *packet = (const struct PACKET_ZC_CONFIG_NOTIFY *)pkt;

    (void)self;
    config_equip_window(packet->open_equip_window);
}

enum {
    CONFIG_OPEN_EQUIP_WINDOW = 0,
    CONFIG_CALL = 1,
    CONFIG_PET_AUTO_FEED = 2,
    CONFIG_HOMUN_AUTO_FEED = 3,
};

#pragma pack(push, 1)
struct PACKET_ZC_CONFIG {
    unsigned short packet_type;
    unsigned type;
    int value;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_CONFIG, 10);

void game_mode_zc_config(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_CONFIG *packet = (const struct PACKET_ZC_CONFIG *)pkt;
    int value = packet->value;

    (void)self;
    switch (packet->type) {
    case CONFIG_OPEN_EQUIP_WINDOW:
        config_equip_window(value);
        break;
    case CONFIG_CALL:
        g_session1.call_allowed = value;
        config_line(value == 1 ? 0xba3 : 0xba2);
        break;
    case CONFIG_PET_AUTO_FEED:
        session_set_pet_auto_feed(&g_session1, value > 0);
        break;
    case CONFIG_HOMUN_AUTO_FEED:
        session_set_homun_auto_feed(&g_session1, value > 0);
        break;
    }
}

void game_mode_get_user_config_list(struct GameMode *self, int count, const struct UserConfigEntry *entries)
{
    int i;

    (void)self;
    for (i = 0; i < count; i++)
        if (entries[i].type == USER_CONFIG_ENTRY1)
            g_session1.userConfigEntry1 = entries[i].value != 0;
}

void game_mode_set_user_config_ack(struct GameMode *self, int type, int result, int value)
{
    struct UIWindow *wnd;

    (void)self;
    if (result != 1)
        return;
    if (type == USER_CONFIG_ENTRY1)
        config_line(0xf15);
    g_session1.userConfigEntry1 = value != 0;
    wnd = object_as_type(ui_window_find(WID_GRAPHIC_SETTING), OBJECT_GRAPHICS_SETTINGS_WINDOW);
    if (wnd != 0) {
        ui_window_send(wnd, NULL, UIM_GRAPHIC_SETTING_SET, 0xcd, value, 0, 0);
        ui_window_send(wnd, NULL, UIM_GRAPHIC_SETTING_REFRESH, 0, 0, 0, 0);
    }
    config_line(g_session1.userConfigEntry1 ? 0xf17 : 0xf16);
}
