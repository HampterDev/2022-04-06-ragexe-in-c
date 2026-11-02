#include "game/packet_handlers.h"
#include "session/state.h"
#include "ui/window_access.h"
#include "game/packets_item_update.h"
#include "game/packets_inventory.h"
#include "game/packets_world.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "session/session_items.h"
#include "session/session_party.h"
#include "ui/items/refine_controls.h"
#include "ui/ui_std.h"

#pragma pack(push, 1)
struct PACKET_ZC_UPDATE_CARDSLOT {
    unsigned short id;
    short index;
    short slot;
    int card;
    unsigned char action;
};
struct PACKET_ZC_GRADE_ENCHANT_ACK {
    unsigned short id;
    short index;
    unsigned short grade;
    int result;
};
struct PACKET_ZC_CHANGE_ITEM_OPTION3 {
    unsigned short id;
    unsigned short index;
    unsigned char is_damaged;
    int cards[ITEM_CARD_SLOTS];
    struct ItemOption options[ITEM_OPTION_SLOTS];
    unsigned char refine;
    unsigned char grade;
};
#pragma pack(pop)
STATIC_ASSERT(change_item_option3_grade, offsetof(struct PACKET_ZC_CHANGE_ITEM_OPTION3, grade) == 0x2f);

enum { CARDSLOT_INSERT = 1, CARDSLOT_REMOVE = 2, CARDSLOT_REMOVE2 = 3 };
enum { GRADE_SUCCESS = 0, GRADE_DOWNGRADE = 2 };
enum {
    WID_ITEM_COLLECTION = 0x0c,
    WID_GRADE_ENCHANT = 0x157,
};
enum { UIM_SHOW_ITEM = 0x18 };
enum {
    EF_GRADE_ENCHANT = 0x647,
    EF_GRADE_ENCHANT_SUCCESS = 0x648,
    EF_GRADE_ENCHANT_FAILURE = 0x649,
};

enum { IN_INVENTORY = 0, IN_EQUIP_SLOTS = 1, IN_COSTUME_SLOTS = 2 };

void game_mode_zc_update_card_slot(struct GameMode *self, const struct PACKET_ZC_UPDATE_CARDSLOT *packet)
{
    int slots[3];
    struct ItemInfo item;

    (void)self;
    session_equip_slots_of(slots, packet->index);
    session_get_equipped_item(&g_session1, &item, slots[2]);
    if (item.index != 0) {
        if (packet->action == CARDSLOT_INSERT)
            item.slot[packet->slot] = packet->card;
        else if ((unsigned char)(packet->action - CARDSLOT_REMOVE) <= 1)
            item.slot[packet->slot] = 0;
        session_set_equip_slot(&g_session1, slots[2], &item);
    }
    item_info_destruct(&item);
}

static void play_grade_effects(int result_effect)
{
    struct Vec3 at = { 0, 0, 0 };
    struct Effect *effect;

    effect_mgr_spawn(g_effectMgr, ((unsigned)g_session1.ownAid), EF_GRADE_ENCHANT, at, 0, -1, 0, 0);
    effect = effect_mgr_spawn(g_effectMgr, ((unsigned)g_session1.ownAid), result_effect, at, 0, -1, 0, 0);
    effect->ops->set_start_delay(effect, 0xaf0);
}

void game_mode_zc_grade_enchant_ack(struct GameMode *self, const struct PACKET_ZC_GRADE_ENCHANT_ACK *packet)
{
    struct ItemInfo item;
    void *wnd;
    int result;

    (void)self;
    session_get_inventory_item(&g_session1, &item, packet->index);
    if (item.index != 0) {
        result = packet->result;
        if (result == GRADE_SUCCESS)
            item.refine = result;
        if (result == GRADE_SUCCESS || result == GRADE_DOWNGRADE) {
            item.grade = packet->grade;
            session_inventory_update(&g_session1, &item, 1);
        }
    }
    wnd = ui_window_find(WID_GRADE_ENCHANT);
    if (wnd != 0) {
        result = packet->result;
        if (result == GRADE_SUCCESS) {
            grade_enchant_wnd_set_result(wnd, 2, result);
            play_grade_effects(EF_GRADE_ENCHANT_SUCCESS);
        } else if ((unsigned)(result - 1) <= 3) {
            grade_enchant_wnd_set_result(wnd, 2, result);
            play_grade_effects(EF_GRADE_ENCHANT_FAILURE);
        }
    }
    item_info_destruct(&item);
}

static void set_options(struct ItemInfo *item, const struct ItemOption *options)
{
    long long i;

    item->option_count = item_option_count(options);
    for (i = 0; i < item->option_count; i++)
        item->options[i] = options[i];
}

static void show_item(struct UIWindow *wnd, struct ItemInfo *item)
{
    if (wnd != 0)
        ui_window_send(wnd, NULL, UIM_SHOW_ITEM, (intptr_t)item, 1, 0, 0);
}

void game_mode_zc_change_item_option(struct GameMode *self, unsigned index, unsigned refine,
                                               struct ItemCards cards, struct ItemOptions options)
{
    struct ItemInfo item;
    int i;

    (void)self;
    session_get_inventory_item(&g_session1, &item, (unsigned short)index);
    item.refine = (unsigned short)refine;
    for (i = 0; i < ITEM_CARD_SLOTS; i++)
        item.slot[i] = cards.card[i];
    set_options(&item, options.option);
    session_inventory_update(&g_session1, &item, 1);
    show_item(ui_window_open(WID_ITEM_COLLECTION), &item);
    item_info_destruct(&item);
}

static int find_item(struct ItemInfo *item, int index, int *slot)
{
    struct ItemInfo found;

    session_get_inventory_item(&g_session1, item, index);
    if (item->index != 0)
        return IN_INVENTORY;
    item_info_assign(item, session_find_equipped_item(&g_session1, &found, index, slot));
    item_info_destruct(&found);
    if (item->index != 0)
        return IN_EQUIP_SLOTS;
    item_info_assign(item, session_find_costume_item(&g_session1, &found, index, slot));
    item_info_destruct(&found);
    return IN_COSTUME_SLOTS;
}

static void put_item(int where, int slot, const struct ItemInfo *item)
{
    if (where == IN_INVENTORY)
        session_inventory_update(&g_session1, item, 1);
    else if (where == IN_EQUIP_SLOTS)
        session_set_equip_slot(&g_session1, slot, item);
    else if (where == IN_COSTUME_SLOTS)
        session_set_costume_slot(&g_session1, slot, item);
}

void game_mode_zc_change_item_option2(struct GameMode *self, unsigned index, unsigned is_damaged,
                                                unsigned refine, struct ItemCards cards, struct ItemOptions options)
{
    struct ItemInfo item;
    int slot = 0, where, i;

    (void)self;
    where = find_item(&item, (unsigned short)index, &slot);
    item.damaged = (unsigned char)is_damaged;
    item.refine = (unsigned short)refine;
    for (i = 0; i < ITEM_CARD_SLOTS; i++)
        item.slot[i] = cards.card[i];
    set_options(&item, options.option);
    put_item(where, slot, &item);
    show_item(ui_window_find(WID_ITEM_COLLECTION), &item);
    item_info_destruct(&item);
}

void game_mode_zc_change_item_option3(struct GameMode *self, const struct PACKET_ZC_CHANGE_ITEM_OPTION3 *packet)
{
    struct ItemInfo item;
    int slot = 0, where, i;

    (void)self;
    where = find_item(&item, packet->index, &slot);
    item.damaged = packet->is_damaged;
    item.refine = packet->refine;
    for (i = 0; i < ITEM_CARD_SLOTS; i++)
        item.slot[i] = packet->cards[i];
    set_options(&item, packet->options);
    item.grade = packet->grade;
    put_item(where, slot, &item);
    show_item(ui_window_find(WID_ITEM_COLLECTION), &item);
    item_info_destruct(&item);
}

#pragma pack(push, 1)
struct PACKET_ZC_CHANGE_ITEM_OPTION {
    unsigned short packet_type;
    unsigned short index;
    unsigned short refine;
    struct ItemCards cards;
    struct ItemOptions options;
};
struct PACKET_ZC_CHANGE_ITEM_OPTION2 {
    unsigned short packet_type;
    unsigned short index;
    unsigned char is_damaged;
    unsigned short refine;
    struct ItemCards cards;
    struct ItemOptions options;
};
#pragma pack(pop)
OFFSET_ASSERT(PACKET_ZC_CHANGE_ITEM_OPTION, options, 0x16);
OFFSET_ASSERT(PACKET_ZC_CHANGE_ITEM_OPTION2, options, 0x17);

void game_mode_zc_change_item_option_packet(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_CHANGE_ITEM_OPTION *packet = (const struct PACKET_ZC_CHANGE_ITEM_OPTION *)pkt;
    struct ItemCards cards;
    struct ItemOptions options;

    memcpy(&cards, &packet->cards, sizeof cards);
    memcpy(&options, &packet->options, sizeof options);
    game_mode_zc_change_item_option(self, packet->index, packet->refine, cards, options);
}

void game_mode_zc_change_item_option2_packet(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_CHANGE_ITEM_OPTION2 *packet = (const struct PACKET_ZC_CHANGE_ITEM_OPTION2 *)pkt;
    struct ItemCards cards;
    struct ItemOptions options;

    memcpy(&cards, &packet->cards, sizeof cards);
    memcpy(&options, &packet->options, sizeof options);
    game_mode_zc_change_item_option2(self, packet->index, packet->is_damaged, packet->refine, cards, options);
}

#pragma pack(push, 1)
struct PACKET_ZC_EQUIPITEM_DAMAGED {
    unsigned short id;
    unsigned short location;
    unsigned aid;
};
#pragma pack(pop)

enum {
    EQUIP_HAND_R = 0x2,
    EQUIP_ARMOR = 0x10,
    EQUIP_HAND_L = 0x20,
    MSG_OUR_ITEM_DAMAGED = 0x514,
    MSG_MEMBER_ITEM_DAMAGED = 0x515,
    MSG_DAMAGED_HAND = 0x516,
    MSG_DAMAGED_ARMOR = 0x517,
};
#define DAMAGED_COLOR   0xff
#define DAMAGED_CHANNEL 0xc

void game_mode_zc_equip_item_damaged(struct GameMode *self, const struct PACKET_ZC_EQUIPITEM_DAMAGED *packet)
{
    struct String line, text;

    (void)self;
    if (packet->location == 0)
        return;
    string_init(&line);
    if ((int)packet->aid != ((unsigned)g_session1.ownAid)) {
        struct PartyMember member;

        session_get_party_member(&g_session1, &member, packet->aid);
        if (member.found != 0) {
            const char *what;

            switch (packet->location) {
            case EQUIP_HAND_L:
            case EQUIP_HAND_R:
                what = msg_str(MSG_DAMAGED_HAND);
                string_init(&text);
                string_format(&text, msg_str(MSG_MEMBER_ITEM_DAMAGED), string_cstr(&member.name), what);
                string_assign(&line, string_cstr(&text), text.size);
                string_free(&text);
                break;
            case EQUIP_ARMOR:
                what = msg_str(MSG_DAMAGED_ARMOR);
                string_format(&text, msg_str(MSG_MEMBER_ITEM_DAMAGED), string_cstr(&member.name), what);
                string_assign(&line, string_cstr(&text), text.size);
                string_free(&text);
                break;
            }
        }
        party_member_info_destroy(&member);
    } else {
        int slots[3];
        struct ItemInfo item;

        session_equip_slots_of(slots, packet->location);
        session_get_equipped_item(&g_session1, &item, slots[2]);
        if (item.index != 0) {
            struct String name = {0};

            item_info_get_display_name(&item, &name);
            string_init(&text);
            string_format(&text, msg_str(MSG_OUR_ITEM_DAMAGED), string_cstr(&name));
            string_assign(&line, string_cstr(&text), text.size);
            string_free(&text);
            string_free(&name);
        }
        item_info_destruct(&item);
    }
    if (line.size != 0) {
        ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (intptr_t)string_cstr(&line), DAMAGED_COLOR, DAMAGED_CHANNEL, 0);
    }
    string_free(&line);
}
