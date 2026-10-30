#include "game/packet_handlers.h"
#include "game/client.h"
#include "game/gamemode.h"
#include "game/item_records.h"
#include "game/item_list_packets.h"
#include "session/session_items.h"
#include "ui/wndmgr/wndmgr.h"

static void put(struct ItemInfo *item)
{
    session_inventory_update(&g_session1, item, 0);
    item_info_destruct(item);
}

void game_mode_inventory_item_list_normal_v5(const unsigned char *pkt)
{
    const struct PACKET_ZC_ITEMLIST *packet = (const struct PACKET_ZC_ITEMLIST *)pkt;
    int n;
    const unsigned char *r = packet->records;
    struct ItemInfo item;

    session_remove_inventory_other_types(&g_session1);
    for (n = (int)((unsigned)(packet->packet_length - 4) / 34); n > 0; n--, r += 34) {
        item_from_normal_record(&item, r, 1);
        put(&item);
    }
    ui_window_mgr_refresh_item_windows(&g_windowMgr);
}

#pragma pack(push, 1)
struct PACKET_ZC_INVENTORY_TAB {
    unsigned short packet_type;
    unsigned short index;
    unsigned char tab;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_INVENTORY_TAB, 5);

void game_mode_inventory_tab(const unsigned char *pkt)
{
    const struct PACKET_ZC_INVENTORY_TAB *packet = (const struct PACKET_ZC_INVENTORY_TAB *)pkt;
    struct ItemInfo item;

    session_get_inventory_item(&g_session1, &item, packet->index);
    if (item.index != 0) {
        item.favorite = packet->tab == 0;
        session_inventory_update(&g_session1, &item, 1);
    }
    item_info_destruct(&item);
}
