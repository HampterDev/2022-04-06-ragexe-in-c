#include "game/packet_handlers.h"
#include "contents/item_reform.h"
#include "session/state.h"
#include "core/memory.h"
#include "game/gamemode.h"
#include "game/ui_bridge.h"
#include "game/session_bridge.h"
#include "core/msg_str.h"
#include "session/session_items.h"

enum {
    WID_ITEM_REFORM = 0x15c,
    WID_ITEM_INFO = 0x0c,
    MSG_REFORM_FAILED = 0xf08,
};

enum {
    REFORM_RESULT_SUCCESS = 0,
    REFORM_RESULT_FAILED = 3,
};

enum {
    ITEM_INFO_MSG_SHOW = 0x18,
    REFORM_MSG_ERROR = 0x1d,
};

enum { EF_ITEM_REFORM_SUCCESS = 0x648 };

void game_mode_on_item_reform_ack(struct GameMode *self, unsigned char result, unsigned short index)
{
    (void)self;
    if (result == REFORM_RESULT_SUCCESS) {
        Vec3 at;
        struct ItemInfo item;

        ui_close_window(WID_ITEM_REFORM);
        at.x = 0.0f;
        at.y = 0.0f;
        at.z = 0.0f;
        effect_mgr_spawn(g_effectMgr, g_session1.ownAid, EF_ITEM_REFORM_SUCCESS, at, 0, -1, 0, 0);
        session_get_inventory_item(&g_session1, &item, (short)index);
        if ((unsigned)item.index > 0) {
            struct UIWindow *wnd = ui_make_window(WID_ITEM_INFO);
            if (wnd)
                ui_window_send(wnd, NULL, ITEM_INFO_MSG_SHOW, (int)&item, 1, 0, 0);
        }
        item_info_destruct(&item);
    } else if (result == REFORM_RESULT_FAILED) {
        struct UIWindow *wnd = ui_get_window(WID_ITEM_REFORM);
        if (wnd)
            ui_window_send(wnd, NULL, REFORM_MSG_ERROR, (int)msg_str(MSG_REFORM_FAILED), 0, 0, 0);
    }
}

enum { REFORM_MSG_OPEN = 0x17 };

void game_mode_on_open_reform_ui(struct GameMode *self, int item_id)
{
    struct UIWindow *wnd;
    struct ItemReformMgr *table;

    (void)self;
    wnd = ui_make_window(WID_ITEM_REFORM);
    table = item_reform_mgr();
    if (item_reform_is_reform_item(table, item_id) != 1) {
        ui_close_window(WID_ITEM_REFORM);
        return;
    }
    if (wnd)
        ui_window_send(wnd, NULL, REFORM_MSG_OPEN, item_id, 0, 0, 0);
}
