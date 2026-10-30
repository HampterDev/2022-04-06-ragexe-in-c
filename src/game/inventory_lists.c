#include "game/packet_handlers.h"
#include "session/item_predicates.h"
#include "contents/pet_evolution.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "ui/window_access.h"
#include <stdlib.h>
#include <string.h>
#include "game/inventory_lists.h"
#include "session/session_items.h"
#include "session/session_jobs.h"
#include "session/session_state.h"
#include "session/session_windows.h"
#include "ui/wndmgr/wndmgr.h"
#include "core/unknown_packet.h"

enum {
    LIST_INVENTORY = 0,
    LIST_CART = 1,
    LIST_STORAGE = 2,
    LIST_STORAGE2 = 3,
};

#pragma pack(push, 1)
struct EquipItemV8 {
    short index;
    int item_id;
    unsigned char type;
    int location;
    int wear_state;
    int slot[ITEM_CARD_SLOTS];
    int hire_expire_date;
    unsigned short bind_on_equip_type;
    unsigned short sprite_number;
    unsigned char option_count;
    struct ItemOption options[ITEM_OPTION_SLOTS];
    unsigned char refining_level;
    unsigned char grade;
    unsigned char flags;
};
struct PACKET_ZC_SPLIT_SEND_ITEMLIST_EQUIPV8 {
    unsigned short id;
    short length;
    unsigned char type;
    struct EquipItemV8 items[1];
};
struct PACKET_ZC_SPLIT_SEND_ITEMLIST_SET {
    unsigned short id;
    short length;
    unsigned char type;
    char name[1];
};
#pragma pack(pop)
SIZE_ASSERT(EquipItemV8, 0x44);

enum {
    ITEM_FLAG_IDENTIFIED = 0x01,
    ITEM_FLAG_DAMAGED = 0x02,
};

#define STORAGE_NAME_MAX 0x18

void storage_wnd_set_name(struct UIWindow *wnd, struct String name)
{
    ui_storage_set_title(wnd, string_cstr(&name));
    string_free(&name);
}

static void set_options(struct ItemInfo *item, const struct ItemOption *options, unsigned short grade)
{
    int i;

    item->grade = grade;
    item->option_count = item_option_count(options);
    for (i = 0; i < item->option_count; i++)
        item->options[i] = options[i];
}

void item_list_decode_equip_v7(struct ItemInfo *item, const struct EquipItemV7 *e)
{
    item_info_construct(item);
    item->type = e->type;
    item->index = e->index;
    item->equip_location = e->location;
    item->worn_location = e->wear_state;
    item->count = 1;
    memcpy(item->slot, e->slot, sizeof item->slot);
    item_info_set_item_id(item, e->item_id);
    item->identified = e->flags & ITEM_FLAG_IDENTIFIED;
    item->damaged = (e->flags >> 1) & 1;
    item->refine = e->refining_level;
    item->expire_time = e->hire_expire_date;
    item->bind_on_equip_type = e->bind_on_equip_type;
    item->view = e->sprite_number;
    item->favorite = (e->flags >> 2) & 1;
    set_options(item, e->options, 0);
}

static void equip_v8_to_item(struct ItemInfo *item, const struct EquipItemV8 *e)
{
    item_info_construct(item);
    item->type = e->type;
    item->index = e->index;
    item->equip_location = e->location;
    item->worn_location = e->wear_state;
    item->count = 1;
    memcpy(item->slot, e->slot, sizeof item->slot);
    item_info_set_item_id(item, e->item_id);
    item->identified = e->flags & ITEM_FLAG_IDENTIFIED;
    item->damaged = (e->flags >> 1) & 1;
    item->refine = e->refining_level;
    item->expire_time = e->hire_expire_date;
    item->bind_on_equip_type = e->bind_on_equip_type;
    item->view = e->sprite_number;
    item->favorite = (e->flags >> 2) & 1;
    set_options(item, e->options, e->grade);
}

void item_list_decode_normal(struct ItemInfo *item, const struct NormalItem *e)
{
    item_info_construct(item);
    item->type = e->type;
    item->index = e->index;
    item->worn_location = e->wear_state;
    item->equip_location = 0;
    item->count = e->amount;
    memcpy(item->slot, e->slot, sizeof item->slot);
    item_info_set_item_id(item, e->item_id);
    item->damaged = 0;
    item->identified = e->flags & ITEM_FLAG_IDENTIFIED;
    item->expire_time = e->hire_expire_date;
    item->refine = 0;
    item->favorite = (e->flags >> 1) & 1;
}

static int egg_id(const struct ItemInfo *item)
{
    return session_is_pet_egg_id(&g_session1, atoi(string_cstr(&item->id_text)));
}

static void inventory_put_equip(struct ItemInfo *item)
{
    struct ItemInfo egg;
    int id;

    if (egg_id(item) != 0)
        g_session1.hatchedEggIndex = item->damaged ? item->index : g_session1.hatchedEggIndex;
    if (item->worn_location != 0) {
        if (session_is_costume_or_shadow_location(&g_session1, item->worn_location))
            session_set_worn_costume(&g_session1, item);
        else
            session_set_worn_item(&g_session1, item);
    } else {
        session_add_inventory_item(&g_session1, item, 0);
        session_inventory_update(&g_session1, item, 0);
    }
    if (egg_id(item) != 0 && item->damaged) {
        session_get_inventory_item(&g_session1, &egg, g_session1.hatchedEggIndex);
        id = atoi(string_cstr(&egg.id_text));
        item_info_destruct(&egg);
        if (!pet_evolution_is_auto_feeding(g_petTable, id) && g_session1.petAutoFeed == 1)
            session_send_pet_auto_feed(&g_session1, 0);
    }
}

void game_mode_zc_split_send_item_list_equip(struct GameMode *self, unsigned char type, int count,
                                                      const struct EquipItemV7 *items)
{
    struct ItemInfo item;
    int i;

    (void)self;
    switch (type) {
    case LIST_INVENTORY:
        for (i = 0; i < count; i++) {
            item_list_decode_equip_v7(&item, &items[i]);
            inventory_put_equip(&item);
            item_info_destruct(&item);
        }
        break;
    case LIST_CART:
        for (i = 0; i < count; i++) {
            item_list_decode_equip_v7(&item, &items[i]);
            session_add_cart_item(&g_session1, &item);
            item_info_destruct(&item);
        }
        break;
    case LIST_STORAGE:
    case LIST_STORAGE2:
        for (i = 0; i < count; i++) {
            item_list_decode_equip_v7(&item, &items[i]);
            session_add_storage_item(&g_session1, &item, 0);
            item_info_destruct(&item);
        }
        break;
    }
}

void game_mode_zc_split_send_item_list_equip_v8(struct GameMode *self,
                                                        const struct PACKET_ZC_SPLIT_SEND_ITEMLIST_EQUIPV8 *packet)
{
    unsigned count = (unsigned)(packet->length - 5) / sizeof(struct EquipItemV8);
    struct ItemInfo item;
    unsigned i;

    (void)self;
    switch (packet->type) {
    case LIST_INVENTORY:
        for (i = 0; i < count; i++) {
            equip_v8_to_item(&item, &packet->items[i]);
            inventory_put_equip(&item);
            item_info_destruct(&item);
        }
        break;
    case LIST_CART:
        for (i = 0; i < count; i++) {
            equip_v8_to_item(&item, &packet->items[i]);
            session_add_cart_item(&g_session1, &item);
            item_info_destruct(&item);
        }
        break;
    case LIST_STORAGE:
    case LIST_STORAGE2:
        for (i = 0; i < count; i++) {
            equip_v8_to_item(&item, &packet->items[i]);
            session_add_storage_item(&g_session1, &item, 0);
            item_info_destruct(&item);
        }
        break;
    }
}

void game_mode_zc_split_send_item_list_normal(struct GameMode *self, unsigned char type, int count,
                                                       const struct NormalItem *items)
{
    struct ItemInfo item;
    const struct NormalItem *e;
    int i;

    (void)self;
    for (i = 0; i < count; i++) {
        e = &items[i];
        item_list_decode_normal(&item, e);
        switch (type) {
        case LIST_INVENTORY:
            session_inventory_update(&g_session1, &item, 0);
            break;
        case LIST_CART:
            session_add_cart_item(&g_session1, &item);
            break;
        case LIST_STORAGE:
        case LIST_STORAGE2:
            session_add_storage_item(&g_session1, &item, 0);
            break;
        }
        item_info_destruct(&item);
    }
}

void game_mode_zc_split_send_item_list_result(struct GameMode *self, unsigned char type,
                                                       unsigned char result)
{
    struct UIWindow *wnd;

    (void)self;
    if (result != 0)
        return;
    switch (type) {
    case LIST_INVENTORY:
        ui_window_mgr_refresh_item_windows(&g_windowMgr);
        break;
    case LIST_CART:
        session_refresh_party_ui(&g_session1);
        break;
    case LIST_STORAGE:
    case LIST_STORAGE2:
        wnd = g_windowMgr.item_store_wnd;
        if (wnd != 0) {
            ui_window_send(wnd, NULL, UIM_STORAGE_REFRESH, 0, 0, 0, 0);
            ui_storage_set_list_received(wnd);
        }
        break;
    }
}

void game_mode_zc_split_send_item_list_set(struct GameMode *self,
                                                    const struct PACKET_ZC_SPLIT_SEND_ITEMLIST_SET *packet)
{
    const struct PACKET_ZC_SPLIT_SEND_ITEMLIST_SET *buffer = (const void *)g_packetBuffer;
    int name_length = buffer->length - 5;
    char name[STORAGE_NAME_MAX];
    struct UIWindow *wnd;
    struct String text;
    size_t length;

    (void)self;
    if (name_length > STORAGE_NAME_MAX)
        return;
    switch (buffer->type) {
    case LIST_INVENTORY:
        session_clear_inventory(&g_session1);
        session_clear_worn_items(&g_session1);
        session_clear_worn_costumes(&g_session1);
        break;
    case LIST_CART:
        session_clear_cart(&g_session1);
        break;
    case LIST_STORAGE:
    case LIST_STORAGE2:
        memset(name, 0, sizeof name);
        memcpy_s(name, sizeof name, packet->name, name_length);
        wnd = ui_window_open(WINDOW_STORAGE);
        if (wnd == 0)
            break;
        length = strlen(name);
        if (length == 0)
            break;
        string_init(&text);
        string_assign(&text, name, length);
        storage_wnd_set_name(wnd, text);
        break;
    }
}
