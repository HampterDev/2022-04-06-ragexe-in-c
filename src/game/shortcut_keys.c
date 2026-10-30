#include "game/packet_handlers.h"
#include "ui/window_manager_state.h"
#include "game/shortcut_keys.h"
#include "game/client.h"
#include "session/session_options.h"

#pragma pack(push, 1)
struct PACKET_ZC_SHORTCUT_KEY_LIST_V4 {
    unsigned short id;
    signed char rotate;
    unsigned short tab;
    unsigned char keys[1];
};
#pragma pack(pop)

enum { MSG_SHORTCUT_ROTATE = 0x8f };

void session_set_shortcut_keys(int rotate, const unsigned char *keys, unsigned short tab)
{
    int base = 0;

    if (tab == 0) {
        session_clear_hotkey_tab0(&g_session1);
        memcpy(g_session1.hotkeys0, keys, sizeof g_session1.hotkeys0);
        session_clear_quick_slots(&g_session1);
        memcpy(g_session1.quickSlots, keys + sizeof g_session1.hotkeys0, sizeof g_session1.quickSlots);
        base = g_session1.hotkeyPage0;
    } else if (tab == 1) {
        session_clear_hotkey_tab1(&g_session1);
        memcpy(g_session1.hotkeys1, keys, sizeof g_session1.hotkeys1);
        base = g_session1.hotkeyPage1;
    }
    if (g_windowMgr.short_cut_wnd != 0)
        ui_window_send(g_windowMgr.short_cut_wnd, NULL, MSG_SHORTCUT_ROTATE, rotate - base + 4, (short)tab, 0, 0);
}

void game_mode_zc_short_cut_key_list_v4(struct GameMode *self, const struct PACKET_ZC_SHORTCUT_KEY_LIST_V4 *packet)
{
    (void)self;
    session_set_shortcut_keys(packet->rotate, packet->keys, packet->tab);
}
