#include "game/packet_handlers.h"
#include "game/actor_packets.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type.h"
#include "ui/window_access.h"
#include "ui/shops/merchant_shop_make.h"
#include "game/context.h"
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "game/store_assistant.h"
#include "game/store_entries.h"
#include "game/actor_entry.h"
#include "actor/actor_common.h"
#include "core/msg_str.h"
#include "game/packets_entry.h"
#include "session/session_items.h"
#include "ui/shops/merchant_shop.h"
#include "ui/ui_items.h"
#include "world/world.h"

enum {
    WID_VENDING_MYSHOP = 0x2d,
    WID_VENDING_SHOP = 0x2b,
    WID_VENDING_PURCHASE = 0x2c,
    WID_BUYING_MYSHOP = 0xb0,
    WID_BUYING_SHOP = 0xb1,
    WID_BUYING_SELL = 0xb2,
    WID_BUYING_INFO = 0xb3,
};

enum {
    STORE_MSG_CLEAR = 0x17,
    STORE_MSG_SET_OWNER = 0x1c,
    STORE_MSG_SET_LIMIT = 0x77,
};

static void copy_options(struct ItemInfo *item, const struct ItemOption *options)
{
    int k;
    for (k = 0; k < item->option_count; k++)
        item->options[k] = options[k];
}

static void vending_entry_to_item(struct ItemInfo *item, const struct VendingEntry *e)
{
    item_info_construct(item);
    item->index = e->index;
    item->price = e->price;
    item->charged_price = e->price;
    item->type = e->type;
    item->count = e->count;
    item->identified = e->identified;
    item->damaged = e->damaged;
    item->refine = e->refine;
    item_info_set_item_id(item, e->item_id);
    item->slot[0] = e->slot[0];
    item->slot[1] = e->slot[1];
    item->slot[2] = e->slot[2];
    item->slot[3] = e->slot[3];
    item->option_count = item_option_count(e->options);
    copy_options(item, e->options);
    item->equip_location = e->equip_location;
    item->view = e->view;
}

static struct UIWindow *make_my_shop_window(int id)
{
    return (struct UIWindow *)object_as_type(ui_window_open(id), OBJECT_MY_SHOP_WINDOW);
}

void game_mode_on_buying_store_item_list(struct GameMode *self, int owner_aid, int store_id,
                                              char is_mine, int count, int duration,
                                              const struct BuyingStoreEntry *entries, int zeny_limit)
{
    struct ItemInfo item;
    struct UIWindow *wnd;
    int i;

    if (is_mine == 1) {
        session_clear_own_store_items(&g_session1);
        for (i = 0; i < count; i++) {
            buying_store_entry_to_item(&item, &entries[i], i + 1);
            session_add_own_store_item(&g_session1, &item);
            item_info_destruct(&item);
        }
        wnd = make_my_shop_window(WID_BUYING_MYSHOP);
        if (wnd) {
            ui_merchant_item_my_shop_wnd_add_assistant_bar((struct UIMerchantItemMyShopWnd *)wnd);
            ui_window_send(wnd, NULL, STORE_MSG_CLEAR, 0, 0, 0, 0);
            ui_window_send(wnd, NULL, STORE_MSG_SET_LIMIT, zeny_limit, 0, 0, 0);
            ui_window_send(wnd, NULL, STORE_MSG_SET_OWNER, owner_aid, 0, 0, 0);
            ui_merchant_item_my_shop_wnd_set_timer((struct UIMerchantItemMyShopWnd *)wnd, duration);
        }
    } else {
        session_clear_other_store_items(&g_session1);
        session_clear_purchase_list(&g_session1);
        for (i = 0; i < count; i++) {
            buying_store_entry_to_item(&item, &entries[i], i + 1);
            session_add_other_store_item(&g_session1, &item);
            item_info_destruct(&item);
        }
        wnd = ui_make_window(WID_BUYING_SHOP);
        ui_window_send(wnd, NULL, STORE_MSG_CLEAR, 0, 0, 0, 0);
        ui_window_send(wnd, NULL, STORE_MSG_SET_OWNER, owner_aid, 0, 0, 0);
        ui_window_send(wnd, NULL, STORE_MSG_SET_LIMIT, zeny_limit, 0, 0, 0);
        ui_merchant_item_shop_wnd_set_timer((struct UIMerchantItemShopWnd *)wnd, duration);

        wnd = ui_make_window(WID_BUYING_SELL);
        ui_window_send(wnd, NULL, STORE_MSG_CLEAR, 0, 0, 0, 0);
        ui_window_send(wnd, NULL, STORE_MSG_SET_OWNER, owner_aid, store_id, 0, 0);

        wnd = g_windowMgr.buying_store_mirror_sell_list_wnd;
        if (wnd == 0)
            wnd = ui_make_window(WID_BUYING_INFO);
        else
            session_fill_buying_store_list_priced(&g_session1);
        if (wnd)
            ui_window_send(wnd, NULL, STORE_MSG_CLEAR, 0, 0, 0, 0);
    }
    self->npc_dialog_open = 1;
}

void game_mode_on_vending_store_item_list(struct GameMode *self, int owner_aid, int store_id,
                                               char is_mine, int count, int duration,
                                               const struct VendingEntry *entries)
{
    struct ItemInfo item;
    struct UIWindow *wnd;
    int i;

    if (is_mine == 1) {
        session_clear_own_store_items(&g_session1);
        for (i = count; i > 0; i--, entries++) {
            struct VendingEntry e = *entries;
            vending_entry_to_item(&item, &e);
            session_add_own_store_item(&g_session1, &item);
            item_info_destruct(&item);
        }
        wnd = make_my_shop_window(WID_VENDING_MYSHOP);
        if (wnd) {
            ui_merchant_item_my_shop_wnd_add_assistant_bar((struct UIMerchantItemMyShopWnd *)wnd);
            ui_window_send(wnd, NULL, STORE_MSG_CLEAR, 0, 0, 0, 0);
            ui_window_send(wnd, NULL, STORE_MSG_SET_OWNER, owner_aid, 0, 0, 0);
            ui_merchant_item_my_shop_wnd_set_timer((struct UIMerchantItemMyShopWnd *)wnd, duration);
        }
        self->npc_dialog_open = 1;
        return;
    }
    session_clear_other_store_items(&g_session1);
    session_clear_purchase_list(&g_session1);
    for (i = count; i > 0; i--, entries++) {
        struct VendingEntry e = *entries;
        vending_entry_to_item(&item, &e);
        session_add_other_store_item(&g_session1, &item);
        item_info_destruct(&item);
    }
    wnd = ui_make_window(WID_VENDING_SHOP);
    ui_window_send(wnd, NULL, STORE_MSG_CLEAR, 0, 0, 0, 0);
    ui_window_send(wnd, NULL, STORE_MSG_SET_OWNER, owner_aid, 0, 0, 0);
    ui_merchant_item_shop_wnd_set_timer((struct UIMerchantItemShopWnd *)wnd, duration);
    wnd = ui_make_window(WID_VENDING_PURCHASE);
    ui_window_send(wnd, NULL, STORE_MSG_CLEAR, 0, 0, 0, 0);
    ui_window_send(wnd, NULL, STORE_MSG_SET_OWNER, owner_aid, store_id, 0, 0);
    self->npc_dialog_open = 1;
}

#pragma pack(push, 1)
struct PacketVendingList4 {
    unsigned short packet_type;
    short packet_length;
    int owner_aid;
    int store_id;
    unsigned char is_mine;
    int duration;
    struct VendingEntry4 items[1];
};
#pragma pack(pop)

static void item_copy_inline(struct ItemInfo *dst, const struct ItemInfo *src)
{
    memcpy(dst, src, offsetof(struct ItemInfo, id_text));
    string_copy(&dst->id_text, &src->id_text);
    string_copy(&dst->name, &src->name);
    memcpy(&dst->identified, &src->identified, sizeof *dst - offsetof(struct ItemInfo, identified));
}

static void items_to_session(const struct Vec *items, int mine)
{
    const struct ItemInfo *it;

    for (it = items->first; it != items->last; it++) {
        struct ItemInfo copy;

        item_copy_inline(&copy, it);
        if (mine)
            session_add_own_store_item(&g_session1, &copy);
        else
            session_add_other_store_item(&g_session1, &copy);
        item_info_destruct(&copy);
    }
}

static void open_vending_shop(int owner_aid, int store_id, int duration, int timed)
{
    struct UIWindow *wnd = ui_make_window(WID_VENDING_SHOP);

    if (wnd) {
        ui_window_send(wnd, NULL, STORE_MSG_CLEAR, 0, 0, 0, 0);
        ui_window_send(wnd, NULL, STORE_MSG_SET_OWNER, owner_aid, 0, 0, 0);
        if (timed)
            ui_merchant_item_shop_wnd_set_timer((struct UIMerchantItemShopWnd *)wnd, duration);
    }
    wnd = ui_make_window(WID_VENDING_PURCHASE);
    if (wnd) {
        ui_window_send(wnd, NULL, STORE_MSG_CLEAR, 0, 0, 0, 0);
        ui_window_send(wnd, NULL, STORE_MSG_SET_OWNER, owner_aid, store_id, 0, 0);
    }
}

void game_mode_take_vending_store_items(struct GameMode *self, const struct Vec *items,
                                    int owner_aid, int store_id, int duration, char is_mine)
{
    struct UIWindow *wnd;

    if (is_mine == 1) {
        session_clear_own_store_items(&g_session1);
        items_to_session(items, 1);
        wnd = ui_make_window(WID_VENDING_MYSHOP);
        if (wnd) {
            ui_merchant_item_my_shop_wnd_add_assistant_bar((struct UIMerchantItemMyShopWnd *)wnd);
            ui_window_send(wnd, NULL, STORE_MSG_CLEAR, 0, 0, 0, 0);
            ui_window_send(wnd, NULL, STORE_MSG_SET_OWNER, owner_aid, 0, 0, 0);
            ui_merchant_item_my_shop_wnd_set_timer((struct UIMerchantItemMyShopWnd *)wnd, duration);
        }
    } else {
        session_clear_other_store_items(&g_session1);
        session_clear_purchase_list(&g_session1);
        items_to_session(items, 0);
        open_vending_shop(owner_aid, store_id, duration, 1);
    }
    self->npc_dialog_open = 1;
}

static void items_push_copy(struct Vec *items, const struct ItemInfo *item)
{
    struct ItemInfo copy;

    item_info_copy(&copy, item);
    vec_push(items, &copy, sizeof copy);
}

static void vending_entry4_to_item(struct ItemInfo *item, const struct VendingEntry4 *e)
{
    item_info_construct(item);
    item->index = e->index;
    item->price = e->price;
    item->charged_price = e->price;
    item->type = e->type;
    item->count = e->count;
    item->identified = e->identified;
    item->damaged = e->damaged;
    item->refine = e->refine;
    item_info_set_item_id(item, e->item_id);
    item->slot[0] = e->slot[0];
    item->slot[1] = e->slot[1];
    item->slot[2] = e->slot[2];
    item->slot[3] = e->slot[3];
    item->option_count = item_option_count(e->options);
    copy_options(item, e->options);
    item->equip_location = e->equip_location;
    item->view = e->view;
    item->grade = e->grade;
}

void game_mode_on_vending_store_item_list4(struct GameMode *self, const struct PacketVendingList4 *packet)
{
    struct Vec items = { 0, 0, 0 };
    struct ItemInfo item;
    size_t count = (unsigned)(packet->packet_length - 0x11) >> 6;
    size_t i;

    vec_reserve(&items, count, sizeof(struct ItemInfo));
    for (i = 0; i < count; i++) {
        const struct VendingEntry4 *e = &packet->items[i];

        vending_entry4_to_item(&item, e);
        items_push_copy(&items, &item);
        item_info_destruct(&item);
    }
    game_mode_take_vending_store_items(self, &items, packet->owner_aid, packet->store_id, packet->duration,
                                   packet->is_mine);
    item_info_vec_free(&items);
}

#pragma pack(push, 1)
struct PacketPurchaseItemListFromMc4 {
    unsigned short packet_type;
    short packet_length;
    int owner_aid;
    int store_id;
    struct VendingEntry4 items[1];
};

struct PacketPurchaseMyItemList3 {
    unsigned short packet_type;
    short packet_length;
    int owner_aid;
    struct MyVendingEntry3 items[1];
};
#pragma pack(pop)

void game_mode_take_other_vending_store_items(struct GameMode *self, const struct Vec *items,
                                         int owner_aid, int store_id)
{
    session_clear_other_store_items(&g_session1);
    session_clear_purchase_list(&g_session1);
    items_to_session(items, 0);
    open_vending_shop(owner_aid, store_id, 0, 0);
    self->npc_dialog_open = 1;
}

void game_mode_take_my_vending_store_items(struct GameMode *self, const struct Vec *items)
{
    session_clear_own_store_items(&g_session1);
    if ((int)((char *)items->last - (char *)items->first) / (int)sizeof(struct ItemInfo) == 0) {
        mode_send_msg(self, 0x28, 0, 0, 0, 0);
        ui_close_window(WID_VENDING_SHOP);
        ui_close_window(WID_VENDING_PURCHASE);
        session_clear_purchase_lists(&g_session1);
        return;
    }
    items_to_session(items, 1);
}

void game_mode_on_purchase_item_list_from_mc4(struct GameMode *self,
                                                   const struct PacketPurchaseItemListFromMc4 *packet)
{
    struct Vec items = { 0, 0, 0 };
    size_t count = (unsigned)(packet->packet_length - 0xc) >> 6;
    size_t i;

    vec_reserve(&items, count, sizeof(struct ItemInfo));
    for (i = 0; i < count; i++) {
        struct VendingEntry4 e;
        struct ItemInfo item;

        memcpy(&e, &packet->items[i], sizeof e);
        vending_entry4_to_item(&item, &e);
        items_push_copy(&items, &item);
        item_info_destruct(&item);
    }
    game_mode_take_other_vending_store_items(self, &items, packet->owner_aid, packet->store_id);
    item_info_vec_free(&items);
}

void game_mode_on_purchase_my_item_list3(struct GameMode *self, const struct PacketPurchaseMyItemList3 *packet)
{
    struct Vec items = { 0, 0, 0 };
    size_t count = (unsigned)(packet->packet_length - 8) / sizeof(struct MyVendingEntry3);
    size_t i;

    vec_reserve(&items, count, sizeof(struct ItemInfo));
    for (i = 0; i < count; i++) {
        struct MyVendingEntry3 e;
        struct ItemInfo item;

        memcpy(&e, &packet->items[i], sizeof e);
        item_info_construct(&item);
        item.index = e.index;
        item.price = e.price;
        item.charged_price = e.price;
        item.count = e.count;
        item.type = e.type;
        item_info_set_item_id(&item, e.item_id);
        item.identified = e.identified;
        item.damaged = e.damaged;
        item.refine = e.refine;
        item.slot[0] = e.slot[0];
        item.slot[1] = e.slot[1];
        item.slot[2] = e.slot[2];
        item.slot[3] = e.slot[3];
        item.option_count = item_option_count(e.options);
        copy_options(&item, e.options);
        item.grade = e.grade;
        items_push_copy(&items, &item);
        item_info_destruct(&item);
    }
    game_mode_take_my_vending_store_items(self, &items);
    item_info_vec_free(&items);
}

enum {
    WID_BUYING_SETUP = 0xae,
    WID_VENDING_SETUP = 0x29,
    MSG_STORE_SETUP_BUSY = 0x24b,
    MODE_MSG_BUYING_SETUP_CANCELLED = 0x12b,
    MODE_MSG_VENDING_SETUP_CANCELLED = 0x129,
};

static int store_setup_refused(void)
{
    struct GameMode *gm = game_current_mode();
    return gm && gm->drag.source != 0;
}

static void store_setup_busy_box(void)
{
    ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_STORE_SETUP_BUSY), 0, 1, 0, 0, "메시지", 0x118, 0x78, 0);
}

void game_mode_on_open_ui_assistant_buying(struct GameMode *self, unsigned char slots)
{
    if (!store_setup_refused()) {
        g_session1.buyingStoreSlots = slots < 2 ? 2 : (slots > 5 ? 5 : slots);
        session_clear_store_setup(&g_session1);
        if (g_windowMgr.chat_room_wnd == 0 && g_windowMgr.chat_room_make_wnd == 0 && g_windowMgr.merchant_shop_make_wnd == 0) {
            ui_make_window(WID_BUYING_SETUP);
            if (g_windowMgr.buying_store_make_wnd)
                ui_merchant_shop_make_wnd_set_assistant_items((struct UIMerchantShopMakeWnd *)g_windowMgr.buying_store_make_wnd, 0, 0);
            return;
        }
        store_setup_busy_box();
    }
    mode_send_msg(self, MODE_MSG_BUYING_SETUP_CANCELLED, 0, 0, 0, 0);
}

void game_mode_on_open_ui_assistant_vending(struct GameMode *self, unsigned char slots, int count,
                                                  const void *items)
{
    if (!store_setup_refused()) {
        g_session1.vendingSlots = slots < 3 ? 3 : (slots > 13 ? 13 : slots);
        session_clear_store_setup(&g_session1);
        if (g_windowMgr.chat_room_wnd == 0 && g_windowMgr.chat_room_make_wnd == 0) {
            ui_make_window(WID_VENDING_SETUP);
            if (g_windowMgr.merchant_shop_make_wnd)
                ui_merchant_shop_make_wnd_set_assistant_items((struct UIMerchantShopMakeWnd *)g_windowMgr.merchant_shop_make_wnd, count, items);
            return;
        }
        store_setup_busy_box();
    }
    mode_send_msg(self, MODE_MSG_VENDING_SETUP_CANCELLED, 0, 0, 0, 0);
}

enum {
    ASSISTANT_VENDING = 100,
    ASSISTANT_BUYING = 101,
    WID_BUYING_SETUP_ITEMS = 0xaf,
};

void game_mode_on_ack_create_assistant(unsigned char kind, unsigned char result)
{
    struct UIWindow *wnd;
    int msg;

    if (kind == ASSISTANT_VENDING) {
        switch (result) {
        case 0:
            ui_close_window(WID_VENDING_SETUP);
            session_clear_store_setup(&g_session1);
            break;
        case 1: case 2: case 3:
            wnd = ui_get_window(WID_VENDING_SETUP);
            if (wnd != 0)
                ui_merchant_shop_make_wnd_set_waiting((struct UIMerchantShopMakeWnd *)wnd, 0);
            break;
        case 4: case 6: case 7:
            session_clear_store_setup(&g_session1);
            ui_close_window(WID_VENDING_SETUP);
            break;
        }
    } else if (kind == ASSISTANT_BUYING) {
        switch (result) {
        case 0:
            ui_close_window(WID_BUYING_SETUP);
            session_clear_store_setup(&g_session1);
            break;
        case 1: case 2: case 3: case 8:
            wnd = ui_get_window(WID_BUYING_SETUP);
            if (wnd != 0)
                ui_merchant_shop_make_wnd_set_waiting((struct UIMerchantShopMakeWnd *)wnd, 0);
            break;
        case 4: case 6: case 7:
            session_clear_store_setup(&g_session1);
            ui_close_window(WID_BUYING_SETUP);
            ui_close_window(WID_BUYING_SETUP_ITEMS);
            break;
        }
    }
    switch (result) {
    case 2: msg = 0xba7; break;
    case 3: msg = 0x7a2; break;
    case 8: msg = 0x38; break;
    default: return;
    }
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (int)msg_str(msg), 0xff, 0, 0);
}

struct Pc;

static int store_assistant_enter(struct GameMode *self, struct ActorEntryInfo *info, struct String *title)
{
    struct GameActor *actor;

    if (title->size != 0) {
        struct String copy;
        string_copy(&copy, title);
        name_card_holder_set_name(&self->actor_names, info->aid, copy);
    }
    actor = world_get_game_actor_by_aid(self->world, info->aid);
    if (actor) {
        render_object_send(actor, NULL, ACTOR_MSG_ENTRY, (int)info, 0, 0, 0);
    } else {
        void *block = mem_alloc(sizeof(struct Pc));
        struct Pc *pc = block ? pc_construct((struct Pc *)block) : 0;

        if (!pc)
            return 0;
        actor = (struct GameActor *)pc;
        list_push_back(&self->world->gameObjects, &pc, sizeof pc);
        actor_list_push_back(&self->world->actors, pc);
        render_object_send(actor, NULL, ACTOR_MSG_NEW_ENTRY, (int)info, 0, 0, 0);
    }
    actor->is_store_assistant = 1;
    return 1;
}

void game_mode_on_store_assistant_entry(struct GameMode *self, int aid, int job, short x, short y,
                                               unsigned char sex, short head, int weapon, int shield,
                                               short accessory, short accessory2, short accessory3,
                                               short head_palette, short body_palette, short robe,
                                               struct String title)
{
    struct ActorEntryInfo info;

    actor_entry_info_construct(&info);
    info.job = job;
    info.aid = aid;
    info.sex = game_mode_job_sex(self, job, sex);
    info.x = x;
    info.y = y;
    info.head = head;
    info.weapon[0] = weapon;
    info.weapon[1] = shield;
    info.accessory = accessory;
    info.accessory2 = accessory2;
    info.accessory3 = accessory3;
    info.head_palette = head_palette;
    info.body_palette = body_palette;
    info.dir = 4;
    info.robe = robe;
    store_assistant_enter(self, &info, &title);
    string_free(&title);
}

int game_store_assistant_restore(struct GameMode *self, const struct PacketStoreAssistantEntryV2 *packet)
{
    struct ActorEntryInfo info;
    struct String title;
    const char *end = memchr(packet->name, 0, sizeof packet->name);
    size_t length = end ? (size_t)(end - packet->name) : sizeof packet->name;
    int entered;
    actor_entry_info_construct(&info);
    info.job = packet->job;
    info.aid = packet->aid;
    info.sex = game_mode_job_sex(self, packet->job, packet->sex);
    info.x = packet->x;
    info.y = packet->y;
    info.head = packet->head;
    info.weapon[0] = packet->weapon;
    info.weapon[1] = packet->shield;
    info.accessory = packet->accessory;
    info.accessory2 = packet->accessory2;
    info.accessory3 = packet->accessory3;
    info.head_palette = packet->head_palette;
    info.body_palette = packet->body_palette;
    info.robe = packet->robe;
    info.dir = 4;
    info.body = packet->body;
    string_init(&title);
    string_assign(&title, packet->name, length);
    entered = store_assistant_enter(self, &info, &title);
    string_free(&title);
    return entered;
}

void game_mode_on_store_assistant_disappear(struct GameMode *self, int aid)
{
    if (world_remove_actor_by_aid(self->world, aid) == 1)
        mode_send_msg(self, MODE_MSG_ACTOR_REMOVED, aid, 0, 0, 0);
}

#pragma pack(push, 1)
struct PacketStoreAssistantEntry {
    unsigned short packet_type;
    int aid;
    int job;
    unsigned short x;
    unsigned short y;
    unsigned char sex;
    unsigned short head;
    int weapon;
    int shield;
    unsigned short accessory;
    unsigned short accessory2;
    unsigned short accessory3;
    unsigned short head_palette;
    unsigned short body_palette;
    unsigned short robe;
    char name[24];
};
#pragma pack(pop)
SIZE_ASSERT(PacketStoreAssistantEntry, 61);

void game_mode_zc_store_assistant_entry(struct GameMode *self, const unsigned char *pkt)
{
    const struct PacketStoreAssistantEntry *packet = (const struct PacketStoreAssistantEntry *)pkt;
    struct String title;

    string_init(&title);
    string_assign_cstr(&title, packet->name);
    game_mode_on_store_assistant_entry(self, packet->aid, packet->job, packet->x, packet->y, packet->sex, packet->head,
                                   packet->weapon, packet->shield, packet->accessory, packet->accessory2,
                                   packet->accessory3, packet->head_palette, packet->body_palette, packet->robe,
                                   title);
}

void game_mode_zc_store_assistant_entry_v2(struct GameMode *self, const unsigned char *pkt)
{
    game_store_assistant_restore(self, (const struct PacketStoreAssistantEntryV2 *)pkt);
}
