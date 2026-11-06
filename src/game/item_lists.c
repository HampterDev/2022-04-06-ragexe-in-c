#include "game/actor_packets.h"
#include "game/barter_packets.h"
#include "game/gamemode.h"
#include "game/ground_item_packets.h"
#include "game/quest_packets.h"
#include "session/item_predicates.h"
#include "game/item_list_packets.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "ui/window_access.h"
#include "game/packet_handlers.h"
#include "game/client.h"
#include "game/item_info.h"
#include "game/packet_dispatch.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "ui/shops/storage.h"
#include "core/msg_str.h"
#include "session/session_items.h"
#include "session/session_windows.h"
#include "ui/wndmgr/wndmgr.h"

#pragma pack(push, 1)
struct SchedulerCashItem {
    int item_id;
    int value;
    unsigned short view;
    int location;
};
struct PACKET_ZC_ACK_SCHEDULER_CASHITEM {
    unsigned short packet_type;
    unsigned short packet_length;
    short count;
    unsigned short kind;
    struct SchedulerCashItem items[];
};
#pragma pack(pop)
SIZE_ASSERT(SchedulerCashItem, 14);
OFFSET_ASSERT(PACKET_ZC_ACK_SCHEDULER_CASHITEM, items, 8);

void game_mode_item_list_by_kind(const unsigned char *pkt)
{
    const struct PACKET_ZC_ACK_SCHEDULER_CASHITEM *packet = (const struct PACKET_ZC_ACK_SCHEDULER_CASHITEM *)pkt;
    const struct SchedulerCashItem *entry = packet->items;
    struct ItemInfo item;
    const char *name;
    int i;

    for (i = 0; i < packet->count; i++, entry++) {
        int id = entry->item_id;
        int value = entry->value;
        unsigned short slot;
        short kind;

        item_info_construct(&item);
        item.index = -1;
        item.price = value;
        item.charged_price = value;
        item.type = 4;
        item.identified = 1;
        item.damaged = 0;
        item.refine = 0;
        slot = packet->kind;
        if (slot == 8)
            slot = 0xffff;
        item.bind_on_equip_type = slot;
        item_info_set_item_id(&item, id);
        name = item_info_get_name(&item);
        item.view = entry->view;
        item.equip_location = entry->location;
        item_info_set_name(&item, name);
        kind = (short)packet->kind;
        if ((unsigned)(int)kind <= 10)
            item_info_copy(list_push_back(&g_session1.cashShopGoods[kind], NULL, sizeof(struct ItemInfo)), &item);
        item_info_destruct(&item);
    }
}

#pragma pack(push, 1)
struct LimitedSaleEntry {
    int item_id;
    int amount;
    int remaining;
    int price;
    int start_time;
    int end_time;
};
struct PacketLimitedSaleList {
    unsigned short id;
    short length;
    unsigned short result;
    struct LimitedSaleEntry items[1];
};
#pragma pack(pop)
SIZE_ASSERT(LimitedSaleEntry, 24);
OFFSET_ASSERT(PacketLimitedSaleList, items, 6);

enum {
    WID_LIMITED_SALE_LIST = 0x13e,
    WID_LIMITED_SALE_TOOL = 0x154,
};
enum {
    UIM_LIMITED_SALE_LIST_REFRESH = 0x17,
    UIM_LIMITED_SALE_TOOL_RESULT = 0x40,
};
#define ITEM_LIST_LIMITED_SALE 9

static void tell_sale_tool(int what, int result)
{
    struct UIWindow *wnd = ui_window_find(WID_LIMITED_SALE_TOOL);

    if (wnd != 0)
        UIWINDOW_SENDMSG(wnd, UIM_LIMITED_SALE_TOOL_RESULT, what, result, 0, 0);
}

void game_mode_add_limited_sale_item(struct GameMode *self, unsigned short result)
{
    (void)self;
    switch ((short)result) {
    case 0: tell_sale_tool(1, 0); break;
    case 1: tell_sale_tool(1, 1); break;
    case 2: ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (int)msg_str(0x9d5), CHAT_COLOR_RED, 0, 0); break;
    case 3: tell_sale_tool(1, 3); break;
    }
}

void game_mode_del_limited_sale_item(struct GameMode *self, unsigned short result)
{
    (void)self;
    if (result == 0)
        tell_sale_tool(2, 0);
}

void game_mode_close_limited_sale_tool(struct GameMode *self)
{
    (void)self;
    ui_window_mgr_post_quit1(&g_windowMgr, WID_LIMITED_SALE_TOOL);
}

void game_mode_limited_sale_list(struct GameMode *self, const struct PacketLimitedSaleList *packet)
{
    const struct LimitedSaleEntry *src;
    struct LimitedSaleEntry entry;
    struct ItemInfo item;
    struct UIWindow *wnd;
    const char *name;
    unsigned count, i;

    (void)self;
    if (packet->result != 0)
        return;
    count = (unsigned)(packet->length - 6) / sizeof(struct LimitedSaleEntry);
    session_clear_item_list(&g_session1, ITEM_LIST_LIMITED_SALE);
    for (i = 0, src = packet->items; i < count; i++, src++) {
        entry = *src;
        item_info_construct(&item);
        item.index = -1;
        item_info_set_item_id(&item, entry.item_id);
        item.identified = 1;
        item.bind_on_equip_type = 8;
        item.sale_remaining = entry.remaining;
        item.count = entry.amount;
        item.price = entry.price;
        item.sale_start_time = entry.start_time;
        item.sale_end_time = entry.end_time;

        name = item_info_get_name(&item);
        item_info_set_name(&item, name);
        item_info_copy(list_push_back(&g_session1.cashShopGoods[ITEM_LIST_LIMITED_SALE], NULL, sizeof(struct ItemInfo)), &item);
        item_info_destruct(&item);
    }
    wnd = ui_window_find(WID_LIMITED_SALE_LIST);
    if (wnd != 0)
        UIWINDOW_SENDMSG(wnd, UIM_LIMITED_SALE_LIST_REFRESH, 0, 0, 0, 0);
}

#define WID_STORAGE             0x21

#pragma pack(push, 1)
struct NormalItemV1 {
    short index;
    int item_id;
    unsigned char type;
    unsigned char identified;
    short count;
    unsigned short wear_state;
};
struct NormalItemV3 {
    struct NormalItemV1 head;
    int cards[4];
    int expire_time;
};
struct NormalItemV5 {
    short index;
    int item_id;
    unsigned char type;
    short count;
    unsigned short wear_state;
    int cards[4];
    int expire_time;
    unsigned char flags;
};
struct EquipItemV1 {
    short index;
    int item_id;
    unsigned char type;
    unsigned char identified;
    unsigned short location;
    unsigned short wear_state;
    unsigned char damaged;
    unsigned char refine;
    int cards[4];
};
struct EquipItemV5 {
    short index;
    int item_id;
    unsigned char type;
    unsigned short location;
    unsigned short wear_state;
    unsigned char refine;
    int cards[4];
    int expire_time;
    unsigned short bind_on_equip;
    unsigned short view;
    unsigned char flags;
};
#pragma pack(pop)
SIZE_ASSERT(NormalItemV1, 12);
SIZE_ASSERT(NormalItemV3, 32);
SIZE_ASSERT(NormalItemV5, 32);
SIZE_ASSERT(EquipItemV1, 30);
SIZE_ASSERT(EquipItemV5, 37);

static unsigned list_count(const struct PACKET_ZC_ITEMLIST *packet, int header, unsigned size)
{
    return (unsigned)(packet->packet_length - header) / size;
}

static void normal_item_v1(struct ItemInfo *item, const struct NormalItemV1 *e, const int *cards)
{
    int i;

    item_info_construct(item);
    item->type = e->type;
    item->equip_location = 0;
    item->index = e->index;
    item->worn_location = e->wear_state;
    item->count = e->count;
    for (i = 0; i < 4; i++)
        item->slot[i] = cards ? cards[i] : 0;
    item_info_set_item_id(item, e->item_id);
    item->identified = e->identified;
    item->damaged = 0;
    item->refine = 0;
}

static void normal_item_v5(struct ItemInfo *item, const struct NormalItemV5 *e)
{
    int i;

    item_info_construct(item);
    item->type = e->type;
    item->equip_location = 0;
    item->index = e->index;
    item->worn_location = e->wear_state;
    item->count = e->count;
    for (i = 0; i < 4; i++)
        item->slot[i] = e->cards[i];
    item_info_set_item_id(item, e->item_id);
    item->damaged = 0;
    item->identified = e->flags & 1;
    item->refine = 0;
    item->expire_time = e->expire_time;
    item->favorite = (e->flags >> 1) & 1;
}

static void equip_item_v1(struct ItemInfo *item, const struct EquipItemV1 *e)
{
    int i;

    item_info_construct(item);
    item->type = e->type;
    item->equip_location = e->location;
    item->index = e->index;
    item->worn_location = e->wear_state;
    item->count = 1;
    for (i = 0; i < 4; i++)
        item->slot[i] = e->cards[i];
    item_info_set_item_id(item, e->item_id);
    item->identified = e->identified;
    item->damaged = e->damaged;
    item->refine = e->refine;
}

static void equip_item_v5(struct ItemInfo *item, const struct EquipItemV5 *e)
{
    int i;

    item_info_construct(item);
    item->type = e->type;
    item->equip_location = e->location;
    item->index = e->index;
    item->worn_location = e->wear_state;
    item->count = 1;
    for (i = 0; i < 4; i++)
        item->slot[i] = e->cards[i];
    item_info_set_item_id(item, e->item_id);
    item->identified = e->flags & 1;
    item->damaged = (e->flags >> 1) & 1;
    item->refine = e->refine;
    item->expire_time = e->expire_time;
    item->bind_on_equip_type = e->bind_on_equip;
    item->view = e->view;
    item->favorite = (e->flags >> 2) & 1;
}

enum { INTO_INVENTORY, INTO_CART, INTO_STORAGE };

static void put_item(int place, struct ItemInfo *item)
{
    if (place == INTO_INVENTORY)
        session_inventory_update(&g_session1, item, 0);
    else if (place == INTO_CART)
        session_add_cart_item(&g_session1, item);
    else
        session_add_storage_item(&g_session1, item, 0);
    item_info_destruct(item);
}

static void refresh_place(int place)
{
    if (place == INTO_INVENTORY)
        ui_window_mgr_refresh_item_windows(&g_windowMgr);
    else if (place == INTO_CART)
        session_refresh_party_ui(&g_session1);
    else
        session_refresh_friend_ui(&g_session1);
}

static void normal_list(const struct PACKET_ZC_ITEMLIST *packet, int place, int version)
{
    static const unsigned sizes[] = { 12, 28, 32 };
    unsigned size = sizes[version - 1];
    unsigned n = list_count(packet, 4, size);
    const unsigned char *e = packet->records;
    struct ItemInfo item;

    for (; n != 0; n--, e += size) {
        normal_item_v1(&item, (const struct NormalItemV1 *)e,
                       version >= 2 ? ((const struct NormalItemV3 *)e)->cards : 0);
        if (version == 3)
            item.expire_time = ((const struct NormalItemV3 *)e)->expire_time;
        put_item(place, &item);
    }
    refresh_place(place);
}

static void normal_records_v5(const struct NormalItemV5 *e, unsigned n, int place)
{
    struct ItemInfo item;
    unsigned i;

    for (i = 0; i < n; i++) {
        normal_item_v5(&item, &e[i]);
        put_item(place, &item);
    }
    refresh_place(place);
}

static void normal_list_v5(const struct PACKET_ZC_ITEMLIST *packet, int place)
{
    normal_records_v5((const struct NormalItemV5 *)packet->records,
                      list_count(packet, 4, sizeof(struct NormalItemV5)), place);
}

static void equip_list_v1(const struct PACKET_ZC_ITEMLIST *packet, int place)
{
    unsigned n = list_count(packet, 4, sizeof(struct EquipItemV1));
    const struct EquipItemV1 *e = (const struct EquipItemV1 *)packet->records;
    struct ItemInfo item;

    for (; n != 0; n--, e++) {
        equip_item_v1(&item, e);
        put_item(place, &item);
    }
    refresh_place(place);
}

static void equip_list_v5(const struct PACKET_ZC_ITEMLIST *packet, int place)
{
    unsigned n = list_count(packet, 4, sizeof(struct EquipItemV5));
    const struct EquipItemV5 *e = (const struct EquipItemV5 *)packet->records;
    struct ItemInfo item;
    unsigned i;

    for (i = 0; i < n; i++) {
        equip_item_v5(&item, &e[i]);
        put_item(place, &item);
    }
    refresh_place(place);
}

void game_mode_zc_normal_item_list(const unsigned char *pkt, int version)
{
    session_remove_inventory_other_types(&g_session1);
    normal_list((const struct PACKET_ZC_ITEMLIST *)pkt, INTO_INVENTORY, version);
}

void game_mode_zc_inventory_item_list_normal(const unsigned char *pkt)
{
    session_remove_inventory_other_types(&g_session1);
    normal_list_v5((const struct PACKET_ZC_ITEMLIST *)pkt, INTO_INVENTORY);
}

void game_mode_zc_equipment_item_list(const unsigned char *pkt)
{
    const struct PACKET_ZC_ITEMLIST *packet = (const struct PACKET_ZC_ITEMLIST *)pkt;
    unsigned n;
    const struct EquipItemV1 *e = (const struct EquipItemV1 *)packet->records;
    struct ItemInfo item;

    session_remove_inventory_filter_types(&g_session1);
    session_clear_worn_items(&g_session1);
    for (n = list_count(packet, 4, sizeof *e); n != 0; n--, e++) {
        equip_item_v1(&item, e);
        if (session_is_pet_egg_id(&g_session1, atoi(string_cstr(&item.id_text))))
            g_session1.hatchedEggIndex = item.damaged ? item.index : g_session1.hatchedEggIndex;
        if (item.worn_location != 0) {
            session_set_worn_item(&g_session1, &item);
        } else {
            session_add_inventory_item(&g_session1, &item, 0);
            session_inventory_update(&g_session1, &item, 0);
        }
        item_info_destruct(&item);
    }
    refresh_place(INTO_INVENTORY);
}

void game_mode_zc_cart_normal_item_list(const unsigned char *pkt, int version)
{
    normal_list((const struct PACKET_ZC_ITEMLIST *)pkt, INTO_CART, version);
}

void game_mode_zc_cart_equipment_item_list(const unsigned char *pkt)
{
    equip_list_v1((const struct PACKET_ZC_ITEMLIST *)pkt, INTO_CART);
}

void game_mode_zc_cart_item_list_normal(const unsigned char *pkt)
{
    normal_list_v5((const struct PACKET_ZC_ITEMLIST *)pkt, INTO_CART);
}

void game_mode_zc_cart_item_list_equip(const unsigned char *pkt)
{
    equip_list_v5((const struct PACKET_ZC_ITEMLIST *)pkt, INTO_CART);
}

void game_mode_zc_store_normal_item_list(const unsigned char *pkt, int version)
{
    ui_window_open(WID_STORAGE);
    normal_list((const struct PACKET_ZC_ITEMLIST *)pkt, INTO_STORAGE, version);
}

void game_mode_zc_store_item_list_normal(const unsigned char *pkt)
{
    ui_window_open(WID_STORAGE);
    normal_list_v5((const struct PACKET_ZC_ITEMLIST *)pkt, INTO_STORAGE);
}

void game_mode_zc_store_item_list_equip(const unsigned char *pkt)
{
    ui_window_open(WID_STORAGE);
    equip_list_v5((const struct PACKET_ZC_ITEMLIST *)pkt, INTO_STORAGE);
}

void game_mode_zc_store_item_list_normal_v2(const unsigned char *pkt)
{
    const struct PACKET_ZC_STORE_ITEMLIST *packet = (const struct PACKET_ZC_STORE_ITEMLIST *)pkt;
    struct UIWindow *wnd = ui_window_open(WID_STORAGE);

    if (packet->name[0] != 0) {
        const char *name = packet->name;
        struct String title;

        string_init(&title);
        string_assign_cstr(&title, name);
        ui_storage_set_title(wnd, string_cstr(&title));
        string_free(&title);
    }
    normal_records_v5((const struct NormalItemV5 *)packet->records,
                      (unsigned)(packet->packet_length - (int)sizeof *packet) / sizeof(struct NormalItemV5),
                      INTO_STORAGE);
}

#define STORAGE_WND_COUNT       0x37

void game_mode_zc_delete_item_from_store(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_DELETE_ITEM_FROM_STORE *packet = (const struct PACKET_ZC_DELETE_ITEM_FROM_STORE *)pkt;
    struct ItemInfo item;

    item_info_construct(&item);
    item.index = packet->index;
    item.count = packet->amount;
    session_remove_storage_item(&g_session1, &item);
    self->item_move_ready = 1;
    item_info_destruct(&item);
}

void game_mode_zc_close_store(void)
{
    ui_window_mgr_post_quit1(&g_windowMgr, WID_STORAGE);
    session_clear_storage(&g_session1);
}

#pragma pack(push, 1)
struct PACKET_ZC_NOTIFY_STOREITEM_COUNTINFO {
    unsigned short packet_type;
    short count;
    short capacity;
};
struct PACKET_ZC_NOTIFY_CARTITEM_COUNTINFO {
    unsigned short packet_type;
    short count;
    short capacity;
    int weight;
    int max_weight;
};
struct PACKET_ZC_ACK_ADDITEM_TO_CART {
    unsigned short packet_type;
    unsigned char result;
};
struct PACKET_ZC_ACK_ITEMIDENTIFY {
    unsigned short packet_type;
    short index;
    unsigned char result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_NOTIFY_STOREITEM_COUNTINFO, 6);
SIZE_ASSERT(PACKET_ZC_NOTIFY_CARTITEM_COUNTINFO, 14);
SIZE_ASSERT(PACKET_ZC_ACK_ADDITEM_TO_CART, 3);
SIZE_ASSERT(PACKET_ZC_ACK_ITEMIDENTIFY, 5);

void game_mode_zc_notify_store_item_count_info(const unsigned char *pkt)
{
    const struct PACKET_ZC_NOTIFY_STOREITEM_COUNTINFO *packet = (const struct PACKET_ZC_NOTIFY_STOREITEM_COUNTINFO *)pkt;
    struct UIWindow *wnd = ui_window_open(WID_STORAGE);

    ui_window_send(wnd, NULL, STORAGE_WND_COUNT, packet->count, packet->capacity, 0, 0);
}

#define CART_WND_COUNT          0x37

void game_mode_zc_notify_cart_item_count_info(const unsigned char *pkt)
{
    const struct PACKET_ZC_NOTIFY_CARTITEM_COUNTINFO *packet = (const struct PACKET_ZC_NOTIFY_CARTITEM_COUNTINFO *)pkt;

    g_session1.cartCount = packet->count;
    g_session1.cartCountMax = packet->capacity;
    g_session1.cartWeight = packet->weight / 10;
    g_session1.cartWeightMax = packet->max_weight / 10;
    if (g_windowMgr.merchant_item_wnd)
        ui_window_send(g_windowMgr.merchant_item_wnd, NULL, CART_WND_COUNT, 0, 0, 0, 0);
}

#define WID_CART                0x28

void game_mode_zc_cart_off(void)
{
    session_clear_cart(&g_session1);
    ui_window_mgr_post_quit1(&g_windowMgr, WID_CART);
}

void game_mode_zc_ack_add_item_to_cart(const unsigned char *pkt)
{
    const struct PACKET_ZC_ACK_ADDITEM_TO_CART *packet = (const struct PACKET_ZC_ACK_ADDITEM_TO_CART *)pkt;
    int text;

    switch (packet->result) {
    case 0: text = 0x38; break;
    case 1: text = 0xdd; break;
    default: return;
    }
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_REFRESH, 0, 0, 0, 0);
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (int)msg_str(text), CHAT_COLOR_RED, 0, 0);
}

void game_mode_zc_add_item_to_cart(const unsigned char *pkt)
{
    const struct PACKET_ZC_ADD_ITEM_TO_CART *packet = (const struct PACKET_ZC_ADD_ITEM_TO_CART *)pkt;
    struct ItemInfo item;

    item_info_construct(&item);
    item.index = packet->index;
    item.count = packet->amount;
    item_info_set_item_id(&item, packet->item_id);
    item.identified = packet->identified;
    item.damaged = packet->damaged;
    item.refine = packet->refine;
    memcpy(item.slot, packet->cards, sizeof item.slot);
    session_add_cart_item(&g_session1, &item);
    item_info_destruct(&item);
}

void game_mode_zc_delete_item_from_cart(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_DELETE_ITEM_FROM_CART *packet = (const struct PACKET_ZC_DELETE_ITEM_FROM_CART *)pkt;
    struct ItemInfo item;

    item_info_construct(&item);
    item.index = packet->index;
    item.count = packet->amount;
    session_remove_cart_item(&g_session1, &item);
    self->item_move_ready = 1;
    item_info_destruct(&item);
}

#define WID_ITEM_IDENTIFY       0x49
#define WID_ITEM_COMPOSITION    0x4a
#define ITEM_LIST_WND_CLEAR     0x4b
#define ITEM_LIST_WND_ADD       0x1f
#define ITEM_LIST_WND_CARD      0x4d
#define ITEM_LIST_WND_SHOW      0x3c

static void item_index_list(struct UIWindow *wnd, const struct PACKET_ZC_ITEMIDENTIFY_LIST *packet)
{
    unsigned count = (unsigned)(packet->packet_length - 4) >> 1;
    const short *index = packet->indexes;
    struct ItemInfo item;

    for (; count != 0; count--, index++) {
        session_get_inventory_item(&g_session1, &item, *index);
        if (item.index != 0)
            ui_window_send(wnd, NULL, ITEM_LIST_WND_ADD, (int)string_cstr(&item.id_text), item.index, 0, 0);
        item_info_destruct(&item);
    }
    ui_window_send(wnd, NULL, ITEM_LIST_WND_SHOW, 0, 0, 0, 0);
}

void game_mode_item_identify_list(struct GameMode *self, const unsigned char *pkt)
{
    struct UIWindow *wnd = ui_window_open(WID_ITEM_IDENTIFY);

    (void)self;

    ui_window_send(wnd, NULL, ITEM_LIST_WND_CLEAR, 0, 0, 0, 0);
    item_index_list(wnd, (const struct PACKET_ZC_ITEMIDENTIFY_LIST *)pkt);
}

void game_mode_item_composition_list(struct GameMode *self, const unsigned char *pkt)
{
    struct UIWindow *wnd = ui_window_open(WID_ITEM_COMPOSITION);

    ui_window_send(wnd, NULL, ITEM_LIST_WND_CLEAR, 0, 0, 0, 0);
    ui_window_send(wnd, NULL, ITEM_LIST_WND_CARD, self->composition_card_index, 0, 0, 0);
    item_index_list(wnd, (const struct PACKET_ZC_ITEMIDENTIFY_LIST *)pkt);
}

void game_mode_zc_ack_item_identify(const unsigned char *pkt)
{
    const struct PACKET_ZC_ACK_ITEMIDENTIFY *packet = (const struct PACKET_ZC_ACK_ITEMIDENTIFY *)pkt;
    struct ItemInfo item;

    switch (packet->result) {
    case 0:
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (int)msg_str(0x1eb), 0xffff00, 0, 0);
        session_get_inventory_item(&g_session1, &item, packet->index);
        item.identified = 1;
        session_inventory_update(&g_session1, &item, 1);
        item_info_destruct(&item);
        break;
    case 1:
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (int)msg_str(0x1ec), CHAT_COLOR_RED, 0, 0);
        break;
    }
}
