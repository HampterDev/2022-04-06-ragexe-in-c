#include "game/packet_handlers.h"
#include "ui/window_access.h"
#include "game/objects.h"
#include "game/item_info.h"
#include "core/msg_str.h"

#pragma pack(push, 1)
struct MakableItem {
    int item_id;
    unsigned char materials[12];
};
struct PacketMakableItemList {
    unsigned short packet_type;
    short length;
    struct MakableItem items[1];
};
struct WeaponItem {
    short index;
    int item_id;
    unsigned char refine;
    int cards[ITEM_CARD_SLOTS];
};
struct PacketWeaponItemList {
    unsigned short packet_type;
    short length;
    struct WeaponItem items[1];
};
#pragma pack(pop)
SIZE_ASSERT(MakableItem, 16);
SIZE_ASSERT(WeaponItem, 23);

enum {
    MSG_NOTHING_IN_LIST = 0x1a8,
    WID_MAKABLE_ITEMS = 0x4f,
    WID_WEAPON_REFINE = 0x6f,
    WMSG_ADD_ITEM = 0x1f,
};

static void nothing_to_choose(void)
{
    ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_NOTHING_IN_LIST), 0, 1, 0, 0, "Message", 0x118, 0x78, 0);
}

void game_mode_zc_makable_item_list(struct GameMode *self, const struct PacketMakableItemList *packet)
{
    unsigned count = (unsigned)(packet->length - 4) / sizeof(struct MakableItem);
    struct UIWindow *wnd;
    unsigned i;

    (void)self;
    if (count == 0) {
        nothing_to_choose();
        return;
    }
    ui_window_mgr_post_quit1(&g_windowMgr, WID_MAKABLE_ITEMS);
    wnd = ui_window_open(WID_MAKABLE_ITEMS);
    for (i = 0; i < count; i++)
        ui_window_send(wnd, NULL, WMSG_ADD_ITEM, packet->items[i].item_id, 0, 0, 0);
}

void game_mode_zc_notify_weapon_item_list(struct GameMode *self, const struct PacketWeaponItemList *packet)
{
    int count = (int)((unsigned)(packet->length - 4) / sizeof(struct WeaponItem));
    struct UIWindow *wnd;
    int i;

    (void)self;
    if (count == 0) {
        nothing_to_choose();
        return;
    }
    if (ui_window_find(WID_WEAPON_REFINE) != 0)
        ui_window_mgr_post_quit1(&g_windowMgr, WID_WEAPON_REFINE);
    wnd = ui_window_open(WID_WEAPON_REFINE);
    for (i = 0; i < count; i++)
        ui_window_send(wnd, NULL, WMSG_ADD_ITEM, packet->items[i].item_id, packet->items[i].index, 0, 0);
}
