#include "game/packet_handlers.h"
#include "game/gamemode.h"
#include "game/ui_bridge.h"
#include "game/session_bridge.h"
#include "core/msg_str.h"
#include "session/session_items.h"
#include "ui/shops/shops.h"

enum {
    WID_NPC_MARKET_BUY = 0xfe,
    WID_NPC_MARKET_SELL = 0xff,
    WID_NPC_MARKET_RESULT = 0x100,
    MARKET_MSG_RESIZE = 0x24,
    MARKET_MSG_REFRESH = 0x17,
    MSG_MARKET_PURCHASE_FAILED = 0xbe3,
};
#define CHAT_COLOR_RED 0x000000ff

#pragma pack(push, 1)
struct NpcMarketEntry {
    int item_id;
    unsigned char type;
    int price;
    int stock;
    unsigned short weight;
    unsigned char refine;
    int slot[4];
    unsigned short location;
};
#pragma pack(pop)
SIZE_ASSERT(NpcMarketEntry, 0x22);

static void refresh_market_window(int id)
{
    struct UIWindow *wnd = ui_make_window(id);
    if (wnd) {
        ui_window_send(wnd, NULL, MARKET_MSG_RESIZE, 0, 0, 0, 0);
        ui_window_send(wnd, NULL, MARKET_MSG_REFRESH, 0, 0, 0, 0);
    }
}

void game_mode_on_npc_market_open(struct GameMode *self, int count, const struct NpcMarketEntry *items)
{
    int i;

    (void)self;
    session_clear_lists(&g_session1);
    for (i = 0; i < count; i++) {
        const struct NpcMarketEntry *entry = &items[i];
        struct ItemInfo item;

        item_info_construct(&item);
        item_info_set_item_id(&item, entry->item_id);
        item.type = entry->type;
        item.price = entry->price;
        item.charged_price = entry->price;
        item.refine = entry->refine;
        item.slot[0] = entry->slot[0];
        item.slot[1] = entry->slot[1];
        item.slot[2] = entry->slot[2];
        item.slot[3] = entry->slot[3];
        item.count = entry->stock;
        item.weight = (unsigned short)(entry->weight / 10);
        item.index = entry->location;
        item.identified = 1;
        item.by_index = i != -1;
        if (entry->stock > 0)
            session_push_shop_goods(&g_session1, &item);
        item_info_destruct(&item);
    }
    refresh_market_window(WID_NPC_MARKET_BUY);
    refresh_market_window(WID_NPC_MARKET_SELL);
}

#pragma pack(push, 1)
struct MarketBoughtItem {
    int item_id;
    short count;
    int price;
    unsigned char refine;
    int slot[4];
    unsigned short location;
};
#pragma pack(pop)
SIZE_ASSERT(MarketBoughtItem, 0x1d);

void game_mode_on_npc_market_purchase_result(struct GameMode *self, unsigned char result, int count,
                                                   const struct MarketBoughtItem *items)
{
    struct UIWindow *wnd;
    int i, rows;

    (void)self;
    wnd = ui_make_window(WID_NPC_MARKET_RESULT);
    if (wnd == 0)
        return;
    for (i = 0; i < count; i++) {
        const struct MarketBoughtItem *entry = &items[i];
        struct ItemInfo item;

        item_info_construct(&item);
        item_info_set_item_id(&item, entry->item_id);
        item.count = entry->count;
        item.price = entry->price;
        item.charged_price = entry->price;
        item.refine = entry->refine;
        item.slot[0] = entry->slot[0];
        item.slot[1] = entry->slot[1];
        item.slot[2] = entry->slot[2];
        item.slot[3] = entry->slot[3];
        item.index = entry->location;
        item.identified = 1;
        ui_para_result_wnd_add_item((struct UIParaResultWnd *)wnd, &item);
        item_info_destruct(&item);
    }
    rows = count - 2 > 0 ? count : 0;
    ui_window_send(wnd, NULL, MARKET_MSG_RESIZE, rows * 32 + 0x78, 0, 0, 0);
    ui_window_send(wnd, NULL, MARKET_MSG_REFRESH, 0, 0, 0, 0);
    if (result == 0)
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (int)msg_str(MSG_MARKET_PURCHASE_FAILED),
                            CHAT_COLOR_RED, 0, 0);
}
