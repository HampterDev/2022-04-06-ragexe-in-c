#include "game/packet_handlers.h"
#include "world/mode_manager.h"
#include "game/gamemode.h"
#include "game/ui_bridge.h"
#include "core/msg_str.h"

enum {
    WID_CASH_SHOP = 0x13e,
    WID_LIMITED_SALE_TOOL = 0x154,
    CASH_SHOP_MSG_TAB = 0xa4,
    CASH_SHOP_MSG_POINTS = 0x78,
    LIMITED_SALE_MSG_FOUND = 0x40,
    MODE_MSG_CASH_SHOP_FAILED = 0x143,
    MSG_LIMITED_SALE_NOT_FOUND = 0x9d1,
};
#define CHAT_COLOR_RED 0x000000ff

void game_mode_on_cash_shop_open(struct GameMode *self, int cash_points, int kafra_points, int tab)
{
    struct UIWindow *wnd;

    (void)self;
    if (ui_close_window(WID_CASH_SHOP))
        return;
    wnd = ui_make_window(WID_CASH_SHOP);
    if (wnd == 0) {
        mode_send_message(client_current_mode(), MODE_MSG_CASH_SHOP_FAILED, tab, 0, 0, 0);
        return;
    }
    ui_window_send(wnd, NULL, CASH_SHOP_MSG_TAB, tab, 0, 0, 0);
    ui_window_send(wnd, NULL, CASH_SHOP_MSG_POINTS, cash_points, kafra_points, 0, 0);
}

void game_mode_on_open_limited_sale_tool(struct GameMode *self)
{
    (void)self;
    if (ui_get_window(WID_LIMITED_SALE_TOOL) == 0)
        ui_make_window(WID_LIMITED_SALE_TOOL);
}

void game_mode_on_search_limited_sale_item(struct GameMode *self, short result, int item)
{
    struct UIWindow *wnd;

    (void)self;
    if (result == 0) {
        wnd = ui_get_window(WID_LIMITED_SALE_TOOL);
        if (wnd)
            ui_window_send(wnd, NULL, LIMITED_SALE_MSG_FOUND, 0, item, 0, 0);
    } else if (result == 1) {
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (int)msg_str(MSG_LIMITED_SALE_NOT_FOUND),
                            CHAT_COLOR_RED, 0, 0);
    }
}
