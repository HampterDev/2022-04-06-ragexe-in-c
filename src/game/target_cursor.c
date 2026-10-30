#include "game/packet_handlers.h"
#include "ui/window_manager_state.h"
#include "game/gamemode.h"
#include "game/ui_bridge.h"

enum {
    SKILL_CURSOR_PICK_TARGET = 2,
    CURSOR_CAPTURE = 10000,
    CURSOR_COUPLE = 20000,
    CURSOR_BABY = 20001,
    CURSOR_COLLECTION = 40000,
    WID_COLLECTION = 0xc4,
    WM_RESULT = 0x50,
};

static void start_cursor(struct GameMode *mode, int id, int range)
{
    mode->skill_use.skill_id = id;
    mode->skill_use.range = range;
    mode->skill_use.level = 0;
    mode->skill_use.type = SKILL_CURSOR_PICK_TARGET;
}

void game_mode_on_start_baby(struct GameMode *self, const void *packet)
{
    (void)packet;
    start_cursor(self, CURSOR_BABY, 5);
}

void game_mode_on_start_capture(struct GameMode *self, const void *packet)
{
    (void)packet;
    start_cursor(self, CURSOR_CAPTURE, 5);
}

void game_mode_on_start_collection(struct GameMode *self, const void *packet)
{
    (void)packet;
    start_cursor(self, CURSOR_COLLECTION, 1);
}

void game_mode_on_start_couple(struct GameMode *self, const void *packet)
{
    (void)packet;
    start_cursor(self, CURSOR_COUPLE, 5);
}

#pragma pack(push, 1)
struct PacketTryResult {
    unsigned short packet_type;
    unsigned char result;
};
#pragma pack(pop)

static void send_result(struct UIWindow *wnd, unsigned char result)
{
    ui_window_send(wnd, NULL, WM_RESULT, 1, result ? 7 : 3, 0, 0);
}

void game_mode_on_try_capture_monster(struct GameMode *self, const struct PacketTryResult *packet)
{
    (void)self;
    if (g_windowMgr.pet_taming_deceive_wnd)
        send_result(g_windowMgr.pet_taming_deceive_wnd, packet->result);
}

void game_mode_on_try_collection(struct GameMode *self, const struct PacketTryResult *packet)
{
    struct UIWindow *wnd = ui_get_window(WID_COLLECTION);

    (void)self;
    if (wnd)
        send_result(wnd, packet->result);
}
