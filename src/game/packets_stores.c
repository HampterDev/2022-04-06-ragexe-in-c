#include "game/packet_handlers.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "ui/window_access.h"
#include "game/context.h"
#include "platform/win_types.h"
#include <string.h>
#include <time.h>
#include "game/packets.h"
#include "game/store_entries.h"
#include "core/text_util.h"
#include "game/store_search_items.h"
#include "session/session_items.h"
#include "ui/shops/merchant_shop_make.h"

#define WND_BUYING_STORE_OPEN   0xae
#define MM_STORE_REFUSED        0x51
#define MM_BUYING_STORE_CANCEL  0x10e

#pragma pack(push, 1)
struct PACKET_ZC_OPEN_BUYING_STORE {
    unsigned short packet_type;
    unsigned char slots;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_OPEN_BUYING_STORE, 3);

void game_mode_zc_open_buying_store(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_OPEN_BUYING_STORE *packet = (const struct PACKET_ZC_OPEN_BUYING_STORE *)pkt;
    struct GameMode *game = game_current_mode();
    unsigned slots;

    if (game && game->drag.source != 0) {
        mode_send_msg(self, MM_STORE_REFUSED, 0, 0, 0, 0);
        return;
    }
    slots = packet->slots;
    g_session1.buyingStoreSlots = slots < 2 ? 2 : slots > 5 ? 5 : slots;
    session_clear_store_setup(&g_session1);
    if (g_windowMgr.chat_room_wnd || g_windowMgr.chat_room_make_wnd || g_windowMgr.merchant_shop_make_wnd) {
        message_box(0x24b);
        mode_send_msg(self, MM_BUYING_STORE_CANCEL, 0, 0, 0, 0);
    } else {
        void *wnd = ui_window_open(WND_BUYING_STORE_OPEN);

        if (wnd)
            ui_merchant_shop_make_wnd_add_load_backup_button(wnd);
    }
}

#pragma pack(push, 1)
struct PACKET_ZC_UPDATE_ITEM_FROM_BUYING_STORE {
    unsigned short packet_type;
    int item_id;
    short count;
    int zeny;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_UPDATE_ITEM_FROM_BUYING_STORE, 12);

void game_mode_zc_update_item_from_buying_store(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_UPDATE_ITEM_FROM_BUYING_STORE *packet =
        (const struct PACKET_ZC_UPDATE_ITEM_FROM_BUYING_STORE *)pkt;
    int now = (int)_time32(NULL);

    (void)self;
    game_mode_buying_store_bought(now, packet->item_id, packet->count, packet->zeny, 0, 0);
}

#define WND_STORE_SEARCH        0xb5

#pragma pack(push, 1)
struct StoreSearchEntry {
    int store_id;
    unsigned aid;
    char store_name[80];
    int item_id;
    unsigned char item_type;
    int price;
    short amount;
    unsigned char refining_level;
    int cards[ITEM_CARD_SLOTS];
    struct ItemOption options[ITEM_OPTION_SLOTS];
};
struct StoreSearchEntry2 {
    int store_id;
    unsigned aid;
    char store_name[80];
    int item_id;
    unsigned char item_type;
    int price;
    short amount;
    int cards[ITEM_CARD_SLOTS];
    struct ItemOption options[ITEM_OPTION_SLOTS];
    unsigned char refining_level;
    unsigned char grade;
};
#pragma pack(pop)
SIZE_ASSERT(StoreSearchEntry, 0x8d);
SIZE_ASSERT(StoreSearchEntry2, 0x8e);

#pragma pack(push, 1)
struct PACKET_ZC_OPEN_SEARCH_STORE_INFO {
    unsigned short packet_type;
    unsigned short item_limit;
    unsigned char searches_left;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_OPEN_SEARCH_STORE_INFO, 5);

void game_mode_zc_open_search_store_info(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_OPEN_SEARCH_STORE_INFO *packet = (const struct PACKET_ZC_OPEN_SEARCH_STORE_INFO *)pkt;

    (void)self;
    g_session1.storeSearchesLeft = packet->searches_left;
    g_session1.storeSearchItemLimit = packet->item_limit;
    ui_window_open(WND_STORE_SEARCH);
}

#pragma pack(push, 1)
struct StoreSearchHeader {
    unsigned short packet_type;
    short packet_length;
    unsigned char first_page;
    unsigned char has_next_page;
    unsigned char searches_left;
};
struct PACKET_ZC_SEARCH_STORE_INFO_ACK {
    struct StoreSearchHeader header;
    struct StoreSearchEntry entries[];
};
struct PACKET_ZC_SEARCH_STORE_INFO_ACK2 {
    struct StoreSearchHeader header;
    struct StoreSearchEntry2 entries[];
};
#pragma pack(pop)
SIZE_ASSERT(StoreSearchHeader, 7);
SIZE_ASSERT(PACKET_ZC_SEARCH_STORE_INFO_ACK, 7);
SIZE_ASSERT(PACKET_ZC_SEARCH_STORE_INFO_ACK2, 7);

static void *store_search_begin(const struct StoreSearchHeader *header)
{
    void *wnd = ui_window_find(WND_STORE_SEARCH);

    if (header->first_page) {
        if (wnd)
            wnd_send(wnd, 0x3c, 1, 0x17b, 0, 0);
        store_search_items_destroy(g_session1.storeSearchResults.first, g_session1.storeSearchResults.last);
        vec_clear(&g_session1.storeSearchResults);
    }
    g_session1.storeSearchHasNextPage = header->has_next_page;
    g_session1.storeSearchesLeft = header->searches_left;
    return wnd;
}

static void store_search_item_fill(struct StoreSearchItem *result, int store_id, unsigned aid,
                                   const char *store_name, int price, short amount, int item_id)
{
    item_info_construct(&result->item);
    result->store_id = store_id;
    result->aid = aid;
    memcpy(result->store_name, store_name, sizeof result->store_name);
    result->price = price;
    result->amount = amount;
    item_info_set_item_id(&result->item, item_id);
}

static void store_search_add(struct StoreSearchItem *result)
{
    struct StoreSearchItem copy;

    store_search_item_copy(&copy, result);
    vec_push(&g_session1.storeSearchResults, &copy, sizeof copy);
    item_info_destruct(&result->item);
}

static void store_search_options(struct ItemInfo *item, const struct ItemOption *options)
{
    int i;

    item->option_count = item_option_count(options);
    for (i = 0; i < item->option_count; i++)
        item->options[i] = options[i];
}

void game_mode_zc_search_store_info_ack2(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_SEARCH_STORE_INFO_ACK2 *packet = (const struct PACKET_ZC_SEARCH_STORE_INFO_ACK2 *)pkt;
    const struct StoreSearchEntry2 *entries = packet->entries;
    void *wnd = store_search_begin(&packet->header);
    unsigned count = (unsigned)(packet->header.packet_length - 7) / sizeof *entries;
    unsigned i;

    (void)self;
    for (i = 0; i < count; i++) {
        struct StoreSearchEntry2 entry = entries[i];
        struct StoreSearchItem result;

        store_search_item_fill(&result, entry.store_id, entry.aid, entry.store_name, entry.price, entry.amount,
                               entry.item_id);
        result.item.refine = entry.refining_level;
        result.item.type = entry.item_type;
        result.item.identified = 1;
        store_search_options(&result.item, entry.options);
        memcpy(result.item.slot, entry.cards, sizeof result.item.slot);
        result.item.grade = entry.grade;
        store_search_add(&result);
    }
    if (wnd)
        wnd_send(wnd, 0x3c, 1, 0xb0, 0, 0);
}

void game_mode_zc_search_store_info_ack(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_SEARCH_STORE_INFO_ACK *packet = (const struct PACKET_ZC_SEARCH_STORE_INFO_ACK *)pkt;
    const struct StoreSearchEntry *entry = packet->entries;
    void *wnd = store_search_begin(&packet->header);
    int count = (int)((unsigned)(packet->header.packet_length - 7) / sizeof *entry);

    (void)self;
    for (; count > 0; count--, entry++) {
        struct StoreSearchEntry copy = *entry;
        struct StoreSearchItem result;

        store_search_item_fill(&result, copy.store_id, copy.aid, copy.store_name, copy.price, copy.amount,
                               copy.item_id);
        result.item.refine = copy.refining_level;
        result.item.type = copy.item_type;
        result.item.identified = 1;
        store_search_options(&result.item, copy.options);
        memcpy(result.item.slot, copy.cards, sizeof result.item.slot);
        store_search_add(&result);
    }
    if (wnd)
        wnd_send(wnd, 0x3c, 1, 0xb0, 0, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_SEARCH_STORE_INFO_FAILED {
    unsigned short packet_type;
    unsigned char reason;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_SEARCH_STORE_INFO_FAILED, 3);

void game_mode_zc_search_store_info_failed(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_SEARCH_STORE_INFO_FAILED *packet = (const struct PACKET_ZC_SEARCH_STORE_INFO_FAILED *)pkt;
    void *wnd = ui_window_find(WND_STORE_SEARCH);

    (void)self;
    if (wnd)
        wnd_send(wnd, 0x3c, 1, packet->reason, 0, 0);
}

#define WND_VENDING_LIST        0x2b
#define WND_VENDING_CART        0x2c
#define MM_VENDING_CLOSED       0x28
#define MM_VENDING_OPENED       0x52
#define CHAT_COLOR_RED          0xff
#define CHAT_COLOR_SOLD         0x6464ff

static void vending_options(struct ItemInfo *item, const struct ItemOption *options)
{
    int i;

    item->option_count = item_option_count(options);
    for (i = 0; i < item->option_count; i++)
        item->options[i] = options[i];
}

#pragma pack(push, 1)
struct PACKET_ZC_PC_PURCHASE_ITEMLIST_FROMMC2 {
    unsigned short packet_type;
    short packet_length;
    int aid;
    int store_id;
    struct VendingEntry entries[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_PC_PURCHASE_ITEMLIST_FROMMC2, 12);

void game_mode_zc_pc_purchase_itemlist_from_mc(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_PC_PURCHASE_ITEMLIST_FROMMC2 *packet =
        (const struct PACKET_ZC_PC_PURCHASE_ITEMLIST_FROMMC2 *)pkt;
    const struct VendingEntry *entry = packet->entries;
    int count;
    void *wnd;

    session_clear_other_store_items(&g_session1);
    session_clear_purchase_list(&g_session1);
    count = (int)((unsigned)(packet->packet_length - 0xc) / sizeof *entry);
    for (; count > 0; count--, entry++) {
        struct VendingEntry copy = *entry;
        struct ItemInfo item;

        item_info_construct(&item);
        item.index = copy.index;
        item.price = copy.price;
        item.charged_price = copy.price;
        item.type = copy.type;
        item.count = copy.count;
        item.identified = copy.identified;
        item.damaged = copy.damaged;
        item.refine = copy.refine;
        item_info_set_item_id(&item, copy.item_id);
        memcpy(item.slot, copy.slot, sizeof item.slot);
        vending_options(&item, copy.options);
        item.equip_location = copy.equip_location;
        item.view = copy.view;
        session_add_other_store_item(&g_session1, &item);
        item_info_destruct(&item);
    }
    wnd = ui_window_open(WND_VENDING_LIST);
    wnd_send(wnd, 0x17, 0, 0, 0, 0);
    wnd_send(wnd, 0x1c, packet->aid, 0, 0, 0);
    wnd = ui_window_open(WND_VENDING_CART);
    wnd_send(wnd, 0x17, 0, 0, 0, 0);
    wnd_send(wnd, 0x1c, packet->aid, packet->store_id, 0, 0);
    self->npc_dialog_open = 1;
}

#pragma pack(push, 1)
struct PACKET_ZC_PC_PURCHASE_MYITEMLIST {
    unsigned short packet_type;
    short packet_length;
    int aid;
    struct MyVendingEntry entries[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_PC_PURCHASE_MYITEMLIST, 8);

void game_mode_zc_pc_purchase_my_itemlist(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_PC_PURCHASE_MYITEMLIST *packet = (const struct PACKET_ZC_PC_PURCHASE_MYITEMLIST *)pkt;
    const struct MyVendingEntry *entry = packet->entries;
    unsigned count;

    session_clear_own_store_items(&g_session1);
    count = (unsigned)(packet->packet_length - 8) / sizeof *entry;
    if (count == 0) {
        mode_send_msg(self, MM_VENDING_CLOSED, 0, 0, 0, 0);
        ui_window_mgr_post_quit1(&g_windowMgr, WND_VENDING_LIST);
        ui_window_mgr_post_quit1(&g_windowMgr, WND_VENDING_CART);
        session_clear_purchase_lists(&g_session1);
        return;
    }
    for (; (int)count > 0; count--, entry++) {
        struct MyVendingEntry copy = *entry;
        struct ItemInfo item;

        item_info_construct(&item);
        item.price = copy.price;
        item.charged_price = copy.price;
        item.index = copy.index;
        item.count = copy.count;
        item.type = copy.type;
        item_info_set_item_id(&item, copy.item_id);
        item.identified = copy.identified;
        item.damaged = copy.damaged;
        item.refine = copy.refine;
        memcpy(item.slot, copy.slot, sizeof item.slot);
        vending_options(&item, copy.options);
        session_add_own_store_item(&g_session1, &item);
        item_info_destruct(&item);
    }
}

static void vending_sold_line(struct ItemInfo *item, int msg, int what_msg, int amount)
{
    struct String line;

    struct String name = {0};

    item_info_get_display_name(item, &name);
    if (what_msg >= 0) {
        const char *what = msg_str(what_msg);

        string_init(&line);
        string_format(&line, msg_str(msg), string_cstr(&name), what);
    } else {
        string_init(&line);
        string_format(&line, msg_str(msg), string_cstr(&name), amount);
    }
    ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)string_cstr(&line), CHAT_COLOR_SOLD, 0, 0);
    string_free(&line);
    string_free(&name);
}

#pragma pack(push, 1)
struct PACKET_ZC_PC_PURCHASE_RESULT_FROMMC {
    unsigned short packet_type;
    short index;
    short amount;
    unsigned char result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_PC_PURCHASE_RESULT_FROMMC, 7);

void game_mode_zc_pc_purchase_result_from_mc(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_PC_PURCHASE_RESULT_FROMMC *packet = (const struct PACKET_ZC_PC_PURCHASE_RESULT_FROMMC *)pkt;
    unsigned result = packet->result;
    struct ItemInfo item;

    (void)self;
    ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
    switch (result) {
    case 6:
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(0x69f), CHAT_COLOR_SOLD, 0, 0);
        return;
    case 7:
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(0x705), CHAT_COLOR_RED, 0, 0);
        return;
    case 15:
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(0xe75), CHAT_COLOR_RED, 0, 0);
        return;
    }
    session_get_sent_store_purchase_item(&g_session1, &item, packet->index);
    if (item.index != 0) {
        switch (result) {
        case 1: vending_sold_line(&item, 0xe5, 0x37, 0); break;
        case 2: vending_sold_line(&item, 0xe5, 0x38, 0); break;
        case 4: vending_sold_line(&item, 0xf0, -1, packet->amount); break;
        case 5: vending_sold_line(&item, 0x119, -1, packet->amount); break;
        case 8: vending_sold_line(&item, 0xe5, 0x756, 0); break;
        }
    }
    item_info_destruct(&item);
}

#define WND_VENDING_OPEN        0x29
#define ACTOR_MSG_STORE_SIGN    0x37
#define ACTOR_MSG_STORE_CLOSED  0x36
#define STORE_TITLE_MAX         0x50

#pragma pack(push, 1)
struct PACKET_ZC_OPENSTORE {
    unsigned short packet_type;
    short slots;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_OPENSTORE, 4);

void game_mode_zc_open_store(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_OPENSTORE *packet = (const struct PACKET_ZC_OPENSTORE *)pkt;
    struct GameMode *game = game_current_mode();
    int slots;

    if (game && game->drag.source != 0) {
        mode_send_msg(self, MM_STORE_REFUSED, 0, 0, 0, 0);
        return;
    }
    slots = packet->slots;
    g_session1.vendingSlots = slots < 3 ? 3 : slots > 13 ? 13 : slots;
    session_clear_store_setup(&g_session1);
    if (g_windowMgr.chat_room_wnd || g_windowMgr.chat_room_make_wnd) {
        message_box(0x24b);
        mode_send_msg(self, MM_STORE_REFUSED, 0, 0, 0, 0);
    } else {
        void *wnd = ui_window_open(WND_VENDING_OPEN);

        if (wnd)
            ui_merchant_shop_make_wnd_add_load_backup_button(wnd);
    }
}

#pragma pack(push, 1)
struct PACKET_ZC_STORE_ENTRY {
    unsigned short packet_type;
    unsigned aid;
    char title[STORE_TITLE_MAX];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_STORE_ENTRY, 86);

void game_mode_zc_store_entry(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_STORE_ENTRY *packet = (const struct PACKET_ZC_STORE_ENTRY *)pkt;
    char title[STORE_TITLE_MAX + 1];
    struct GameActor *actor;

    strncpy(title, packet->title, STORE_TITLE_MAX);
    title[STORE_TITLE_MAX] = 0;
    actor = world_get_game_actor_by_aid(self->world, packet->aid);
    if (actor)
        actor_send(actor, ACTOR_MSG_STORE_SIGN, (int)title, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_DISAPPEAR_ENTRY {
    unsigned short packet_type;
    unsigned aid;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_DISAPPEAR_ENTRY, 6);

void game_mode_zc_disappear_entry(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_DISAPPEAR_ENTRY *packet = (const struct PACKET_ZC_DISAPPEAR_ENTRY *)pkt;
    struct GameActor *actor = world_get_game_actor_by_aid(self->world, packet->aid);

    if (actor)
        actor_send(actor, ACTOR_MSG_STORE_CLOSED, 0, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_DELETEITEM_FROM_MCSTORE {
    unsigned short packet_type;
    unsigned short index;
    unsigned short amount;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_DELETEITEM_FROM_MCSTORE, 6);

void game_mode_zc_delete_item_from_mc_store(const unsigned char *pkt)
{
    const struct PACKET_ZC_DELETEITEM_FROM_MCSTORE *packet = (const struct PACKET_ZC_DELETEITEM_FROM_MCSTORE *)pkt;
    int now = (int)_time32(NULL);

    game_mode_shop_item_sold(now, packet->index, packet->amount, 0, 0);
}
