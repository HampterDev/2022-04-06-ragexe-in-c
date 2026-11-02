#include "game/packet_handlers.h"
#include "ui/window_access.h"
#include <string.h>
#include "game/storage_lists.h"
#include "game/inventory_lists.h"
#include "session/session_items.h"
#include "session/session_windows.h"

#pragma pack(push, 1)
struct StoreEquipOld {
    short index;
    int item_id;
    unsigned char type;
    unsigned char is_identified;
    unsigned short location;
    unsigned short wear_state;
    unsigned char is_damaged;
    unsigned char refining_level;
    int slot[ITEM_CARD_SLOTS];
};
struct StoreEquip2 {
    struct StoreEquipOld e;
    int hire_expire_date;
};
struct StoreEquip3 {
    struct StoreEquipOld e;
    int hire_expire_date;
    unsigned short bind_on_equip_type;
    unsigned short sprite_number;
};
struct StoreEquip4 {
    short index;
    int item_id;
    unsigned char type;
    unsigned short location;
    unsigned short wear_state;
    unsigned char refining_level;
    int slot[ITEM_CARD_SLOTS];
    int hire_expire_date;
    unsigned short bind_on_equip_type;
    unsigned short sprite_number;
    unsigned char flags;
};
struct StoreEquip5 {
    short index;
    int item_id;
    unsigned char type;
    int location;
    int wear_state;
    unsigned char refining_level;
    int slot[ITEM_CARD_SLOTS];
    int hire_expire_date;
    unsigned short bind_on_equip_type;
    unsigned short sprite_number;
    unsigned char flags;
};
struct StoreNormalOld {
    short index;
    int item_id;
    unsigned char type;
    unsigned char is_identified;
    short amount;
    unsigned short wear_state;
    int slot[ITEM_CARD_SLOTS];
};
struct StoreNormal2 {
    struct StoreNormalOld e;
    int hire_expire_date;
};
struct StoreNormal3 {
    short index;
    int item_id;
    unsigned char type;
    short amount;
    unsigned short wear_state;
    int slot[ITEM_CARD_SLOTS];
    int hire_expire_date;
    unsigned char flags;
};
struct StoreNormalBare {
    short index;
    int item_id;
    unsigned char type;
    unsigned char is_identified;
    short amount;
    unsigned short wear_state;
};

struct PACKET_STORE_LIST {
    unsigned short id;
    short length;
    unsigned char entries[1];
};
struct PACKET_STORE_LIST_NAMED {
    unsigned short id;
    short length;
    char name[24];
    unsigned char entries[1];
};
#pragma pack(pop)
SIZE_ASSERT(StoreEquipOld, 0x1e);
SIZE_ASSERT(StoreEquip2, 0x22);
SIZE_ASSERT(StoreEquip3, 0x26);
SIZE_ASSERT(StoreEquip4, 0x25);
SIZE_ASSERT(StoreEquip5, 0x29);
SIZE_ASSERT(StoreNormalOld, 0x1c);
SIZE_ASSERT(StoreNormal2, 0x20);
SIZE_ASSERT(StoreNormal3, 0x20);
SIZE_ASSERT(StoreNormalBare, 0xc);
OFFSET_ASSERT(PACKET_STORE_LIST_NAMED, entries, 0x1c);

#define LIST_HEADER 4
#define NAMED_LIST_HEADER 0x1c

static unsigned entry_count(short length, unsigned header, unsigned size)
{
    return (unsigned)(length - (int)header) / size;
}

static void open_storage(const char *name)
{
    struct UIWindow *wnd = ui_window_open(WINDOW_STORAGE);
    struct String text;

    if (name == 0 || name[0] == 0)
        return;
    string_init(&text);
    string_assign_cstr(&text, name);
    ui_storage_set_title(wnd, string_cstr(&text));
    string_free(&text);
}

static void store_item(struct ItemInfo *item)
{
    session_add_storage_item(&g_session1, item, 0);
    item_info_destruct(item);
}

static void decode_equip_old(struct ItemInfo *item, const struct StoreEquipOld *e)
{
    item_info_construct(item);
    item->type = e->type;
    item->equip_location = e->location;
    item->index = e->index;
    item->worn_location = e->wear_state;
    memcpy(item->slot, e->slot, sizeof item->slot);
    item->count = 1;
    item_info_set_item_id(item, e->item_id);
    item->identified = e->is_identified;
    item->damaged = e->is_damaged;
    item->refine = e->refining_level;
}

static void decode_equip4(struct ItemInfo *item, const struct StoreEquip4 *e)
{
    item_info_construct(item);
    item->type = e->type;
    item->equip_location = e->location;
    item->index = e->index;
    item->worn_location = e->wear_state;
    item->count = 1;
    memcpy(item->slot, e->slot, sizeof item->slot);
    item_info_set_item_id(item, e->item_id);
    item->identified = e->flags & 1;
    item->damaged = (e->flags >> 1) & 1;
    item->refine = e->refining_level;
    item->expire_time = e->hire_expire_date;
    item->bind_on_equip_type = e->bind_on_equip_type;
    item->view = e->sprite_number;
    item->favorite = (e->flags >> 2) & 1;
}

static void decode_equip5(struct ItemInfo *item, const struct StoreEquip5 *e)
{
    item_info_construct(item);
    item->type = e->type;
    item->index = e->index;
    item->equip_location = e->location;
    item->worn_location = e->wear_state;
    item->count = 1;
    memcpy(item->slot, e->slot, sizeof item->slot);
    item_info_set_item_id(item, e->item_id);
    item->identified = e->flags & 1;
    item->damaged = (e->flags >> 1) & 1;
    item->refine = e->refining_level;
    item->expire_time = e->hire_expire_date;
    item->bind_on_equip_type = e->bind_on_equip_type;
    item->view = e->sprite_number;
    item->favorite = (e->flags >> 2) & 1;
    item->option_count = 0;
}

void game_mode_zc_store_equipment_item_list(struct GameMode *self, const struct PACKET_STORE_LIST *packet)
{
    const struct StoreEquipOld *e = (const void *)packet->entries;
    unsigned count = entry_count(packet->length, LIST_HEADER, sizeof *e);
    struct ItemInfo item;
    unsigned i;

    (void)self;
    open_storage(0);
    for (i = 0; i < count; i++) {
        decode_equip_old(&item, &e[i]);
        store_item(&item);
    }
    session_refresh_friend_ui(&g_session1);
}

void game_mode_zc_store_equipment_item_list2(struct GameMode *self, const struct PACKET_STORE_LIST *packet)
{
    const struct StoreEquip2 *e = (const void *)packet->entries;
    unsigned count = entry_count(packet->length, LIST_HEADER, sizeof *e);
    struct ItemInfo item;
    unsigned i;

    (void)self;
    open_storage(0);
    for (i = 0; i < count; i++) {
        decode_equip_old(&item, &e[i].e);
        item.expire_time = e[i].hire_expire_date;
        store_item(&item);
    }
    session_refresh_friend_ui(&g_session1);
}

void game_mode_zc_store_equipment_item_list3(struct GameMode *self, const struct PACKET_STORE_LIST *packet)
{
    const struct StoreEquip3 *e = (const void *)packet->entries;
    unsigned count = entry_count(packet->length, LIST_HEADER, sizeof *e);
    struct ItemInfo item;
    unsigned i;

    (void)self;
    open_storage(0);
    for (i = 0; i < count; i++) {
        decode_equip_old(&item, &e[i].e);
        item.expire_time = e[i].hire_expire_date;
        item.bind_on_equip_type = e[i].bind_on_equip_type;
        item.view = e[i].sprite_number;
        store_item(&item);
    }
    session_refresh_friend_ui(&g_session1);
}

void game_mode_zc_store_item_list_equip_v2(struct GameMode *self, const struct PACKET_STORE_LIST_NAMED *packet)
{
    const struct StoreEquip4 *e = (const void *)packet->entries;
    unsigned count;
    struct ItemInfo item;
    unsigned i;

    (void)self;
    open_storage(packet->name);
    count = entry_count(packet->length, NAMED_LIST_HEADER, sizeof *e);
    for (i = 0; i < count; i++) {
        decode_equip4(&item, &e[i]);
        store_item(&item);
    }
    session_refresh_friend_ui(&g_session1);
}

void game_mode_zc_store_item_list_equip_v5(struct GameMode *self, const struct PACKET_STORE_LIST_NAMED *packet)
{
    const struct StoreEquip5 *e = (const void *)packet->entries;
    unsigned count;
    struct ItemInfo item;
    unsigned i;

    (void)self;
    open_storage(packet->name);
    count = entry_count(packet->length, NAMED_LIST_HEADER, sizeof *e);
    for (i = 0; i < count; i++) {
        decode_equip5(&item, &e[i]);
        store_item(&item);
    }
    session_refresh_friend_ui(&g_session1);
}

void game_mode_zc_store_item_list_equip_v6(struct GameMode *self, const struct PACKET_STORE_LIST_NAMED *packet)
{
    const struct EquipItemV7 *e = (const void *)packet->entries;
    unsigned count;
    struct ItemInfo item;
    unsigned i;

    (void)self;
    open_storage(packet->name);
    count = (unsigned)((unsigned short)packet->length - NAMED_LIST_HEADER) / sizeof *e;
    for (i = 0; i < count; i++) {
        item_list_decode_equip_v7(&item, &e[i]);
        store_item(&item);
    }
    session_refresh_friend_ui(&g_session1);
}

void game_mode_zc_store_item_list_normal_v5(struct GameMode *self, const struct PACKET_STORE_LIST_NAMED *packet)
{
    const struct NormalItem *e = (const void *)packet->entries;
    unsigned count;
    struct ItemInfo item;
    unsigned i;

    (void)self;
    open_storage(packet->name);
    count = entry_count(packet->length, NAMED_LIST_HEADER, sizeof *e);
    for (i = 0; i < count; i++) {
        item_list_decode_normal(&item, &e[i]);
        store_item(&item);
    }
    session_refresh_friend_ui(&g_session1);
}
