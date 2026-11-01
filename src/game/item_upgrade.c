#include "session/state.h"
#include "game/item_upgrade.h"
#include "contents/lapine_box.h"
#include "core/msg_str.h"
#include "ui/items/refining_internal.h"

enum {
    WID_REPAIR = 0x6b,
    WID_RANDOM_COMBINE = 0x122,
    WID_RANDOM_UPGRADE = 0x12e,
    WM_RANDOM_UPGRADE_OPEN = 0x17,
    WM_REPAIR_ADD_ITEM = 0x1f,
    MSG_NOTHING_TO_REPAIR = 0x1a8,
};

void game_mode_on_random_combine_item_ui_open(struct GameMode *self, int value)
{
    struct UIWindow *wnd;

    (void)self;
    if (ui_get_window(WID_RANDOM_COMBINE))
        return;
    wnd = ui_make_window(WID_RANDOM_COMBINE);
    if (wnd)
        lapine_box_set_item((struct LapineBox *)wnd, value);
}

void game_mode_on_random_upgrade_item_ui_open(struct GameMode *self, int value)
{
    struct UIWindow *wnd;

    (void)self;
    if (ui_get_window(WID_RANDOM_UPGRADE))
        return;
    wnd = ui_make_window(WID_RANDOM_UPGRADE);
    if (wnd)
        ui_window_send(wnd, NULL, WM_RANDOM_UPGRADE_OPEN, value, 0, 0, 0);
}

void game_mode_on_refining_material_list(struct GameMode *self, short index, unsigned char count,
                                                const void *materials, unsigned char blessing_count)
{
    struct UIWindow *wnd = ui_get_window(WID_REFINE);

    (void)self;
    if (wnd)
        ui_refining_wnd_set_item((struct UIRefiningWnd *)wnd, index, count, materials, blessing_count);
}

#pragma pack(push, 1)
struct PacketRepairItemList2 {
    unsigned short packet_type;
    short packet_length;
    struct RepairItem items[1];
};
#pragma pack(pop)

void game_mode_on_repair_item_list2(struct GameMode *self, const struct PacketRepairItemList2 *packet)
{
    unsigned count = (unsigned)(packet->packet_length - 4) / sizeof(struct RepairItem);
    const struct RepairItem *item = packet->items;
    struct UIWindow *wnd;
    unsigned i;

    (void)self;
    if (count == 0) {
        ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_NOTHING_TO_REPAIR), 0, 1, 0, 0, "메시지", 0x118, 0x78, 0);
        return;
    }
    ui_close_window(WID_REPAIR);
    wnd = ui_make_window(WID_REPAIR);
    if (wnd == 0)
        return;
    for (i = 0; i < count; i++, item++)
        ui_window_send(wnd, NULL, WM_REPAIR_ADD_ITEM, (int)item, 0, 0, 0);
}
