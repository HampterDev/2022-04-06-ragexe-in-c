#include "game/packet_handlers.h"
#include "actor/render_object.h"
#include "ui/window_access.h"
#include <stdlib.h>
#include <string.h>
#include "game/objects.h"
#include "game/item_info.h"
#include "game/context.h"
#include "actor/actor_common.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "session/session_items.h"
#include "ui/items/refining_internal.h"

#pragma pack(push, 1)
struct PacketItemComposition {
    unsigned short packet_type;
    short equip_index;
    short card_index;
    unsigned char result;
};
struct PacketItemRefining {
    unsigned short packet_type;
    short result;
    short index;
    short refining_level;
};
struct PacketMakingItem {
    unsigned short packet_type;
    short result;
    int item_id;
};
#pragma pack(pop)

enum {
    EF_REFINE_SUCCESS = 0x9a,
    EF_REFINE_FAILURE = 0x9b,
    ACTOR_MSG_REMOVE_EFFECT = 0x6d,
    MSG_COMPOSITION_DONE = 0x1ed,
    MSG_COMPOSITION_FAILED = 0x1ee,
    MSG_REFINE_SUCCESS = 0x1f2,
    MSG_REFINE_FAILURE = 0x1f3,
    MSG_REFINE_DOWNGRADED = 0x601,
    MSG_ITEM_CREATE_FAILED = 0x1ae,
    MSG_ITEM_CREATE_SUCCEEDED = 0x1af,
};

static void chat_line(int msg, int color)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (int)msg_str(msg), color, 0, 0);
}

static void play_on_player(int effect_id)
{
    Vec3 here = { 0.0f, 0.0f, 0.0f };
    struct GameActor *player = (struct GameActor *)game_current_player();

    if (player)
        game_actor_launch_effect(player, effect_id, here, 0.0f);
}

void game_mode_zc_ack_item_composition(struct GameMode *self, const struct PacketItemComposition *packet)
{
    struct ItemInfo card, equip;
    int slot;

    (void)self;
    if (packet->result == 1) {
        chat_line(MSG_COMPOSITION_FAILED, CHAT_COLOR_RED);
        return;
    }
    if (packet->result != 0)
        return;
    chat_line(MSG_COMPOSITION_DONE, CHAT_COLOR_CYAN);
    session_get_inventory_item(&g_session1, &card, packet->card_index);
    session_get_inventory_item(&g_session1, &equip, packet->equip_index);
    if (equip.index != 0 && card.index != 0) {
        for (slot = 0; slot < ITEM_CARD_SLOTS; slot++) {
            if (equip.slot[slot] == 0) {
                equip.slot[slot] = atoi(string_cstr(&card.id_text));
                break;
            }
        }
        session_remove_inventory_item(&g_session1, packet->card_index, 1);
        session_inventory_update(&g_session1, &equip, 1);
    }
    item_info_destruct(&equip);
    item_info_destruct(&card);
}

void game_mode_zc_ack_item_refining(struct GameMode *self, const struct PacketItemRefining *packet)
{
    struct ItemInfo item, again;
    struct UIWindow *wnd;

    (void)self;
    session_get_inventory_item(&g_session1, &item, packet->index);
    if (item.index != 0) {
        item.refine = packet->refining_level;
        session_inventory_update(&g_session1, &item, 1);
    }
    wnd = ui_window_find(WID_REFINE);
    if (wnd != 0) {
        if ((unsigned)packet->result <= 3)
            ui_refining_wnd_set_state((struct UIRefiningWnd *)wnd, 2, 3 + packet->result);
    } else {
        switch (packet->result) {
        case 0:
            play_on_player(EF_REFINE_SUCCESS);
            chat_line(MSG_REFINE_SUCCESS, CHAT_COLOR_CYAN);
            break;
        case 1:
            play_on_player(EF_REFINE_FAILURE);
            chat_line(MSG_REFINE_FAILURE, CHAT_COLOR_CYAN);
            break;
        case 2:
            play_on_player(EF_REFINE_FAILURE);
            chat_line(MSG_REFINE_DOWNGRADED, CHAT_COLOR_CYAN);
            break;
        }
        session_get_inventory_item(&g_session1, &again, packet->index);
        again.refine = packet->refining_level;
        session_inventory_update(&g_session1, &again, 1);
        item_info_destruct(&again);
    }
    item_info_destruct(&item);
}

void game_mode_zc_ack_req_making_item(struct GameMode *self, const struct PacketMakingItem *packet)
{
    static const short effect_of[8] = { EF_REFINE_SUCCESS, EF_REFINE_FAILURE, 0x131, 0x132, 0x3f7, 0x3f8,
                                        0x3f9, 0x3fa };
    static const short msg_of[8] = { MSG_ITEM_CREATE_SUCCEEDED, MSG_ITEM_CREATE_FAILED, MSG_ITEM_CREATE_SUCCEEDED, MSG_ITEM_CREATE_FAILED,
                                     MSG_ITEM_CREATE_SUCCEEDED, MSG_ITEM_CREATE_FAILED, MSG_ITEM_CREATE_FAILED, MSG_ITEM_CREATE_FAILED };
    struct ItemInfo made;
    struct String line, formatted;
    struct String name = {0};
    unsigned result;
    int effect_id;

    (void)self;
    item_info_construct(&made);
    item_info_set_item_id(&made, packet->item_id);
    string_init(&line);
    made.identified = 1;
    result = (unsigned)packet->result;
    if (result <= 7) {
        struct GameActor *player = (struct GameActor *)game_current_player();

        effect_id = effect_of[result];
        if (player && result >= 2 && able_to_make_effect_is_effect_exist(player, effect_id))
            render_object_send(player, NULL, ACTOR_MSG_REMOVE_EFFECT, effect_id, 0, 0, 0);
        play_on_player(effect_id);
        item_info_get_display_name(&made, &name);
        string_init(&formatted);
        string_format(&formatted, msg_str(msg_of[result]), string_cstr(&name));
        if (result < 2) {
            string_assign(&line, string_cstr(&formatted), formatted.size);
        } else {
            const char *text = string_cstr(&formatted);

            string_assign_cstr(&line, text);
        }
        string_free(&formatted);
        string_free(&name);
    }
    string_free(&line);
    item_info_destruct(&made);
}

#pragma pack(push, 1)
struct PacketItemRepair {
    unsigned short packet_type;
    short index;
    unsigned char result;
};
struct PacketWeaponRefine {
    unsigned short packet_type;
    int result;
    int item_id;
};
struct PacketRepairItemList {
    unsigned short packet_type;
    short length;
    unsigned char entries[1][23];
};
#pragma pack(pop)

enum {
    MSG_REPAIR_DONE = 0x32d,
    MSG_REPAIR_FAILED = 0x32e,
    MSG_WEAPON_REFINE_FIRST = 0x38f,
    MSG_NOTHING_TO_REPAIR = 0x1a8,
    WID_REPAIR = 0x6b,
    UIM_REPAIR_ADD_ITEM = 0x1f,
    REPAIR_ENTRY_SIZE = 23,
};

void game_mode_zc_ack_item_repair(struct GameMode *self, const struct PacketItemRepair *packet)
{
    (void)self;
    if (packet->result == 0)
        chat_line(MSG_REPAIR_DONE, CHAT_COLOR_CYAN);
    else if (packet->result == 1)
        chat_line(MSG_REPAIR_FAILED, CHAT_COLOR_RED);
}

void game_mode_zc_ack_weapon_refine(struct GameMode *self, const struct PacketWeaponRefine *packet)
{
    static const unsigned line_color[4] = { 0xffff00, 0xcdcd00, 0xc8c8ff, 0xc8c8ff };
    struct ItemInfo weapon;
    struct String line;
    struct String name = {0};
    unsigned result = (unsigned)packet->result;

    (void)self;
    item_info_construct(&weapon);
    item_info_set_item_id(&weapon, packet->item_id);
    weapon.refine = 0;
    weapon.identified = 1;
    if (result <= 3) {
        item_info_get_display_name(&weapon, &name);
        string_init(&line);
        string_format(&line, msg_str(MSG_WEAPON_REFINE_FIRST + result), string_cstr(&name));
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (int)string_cstr(&line), line_color[result],
                              0, 0);
        string_free(&line);
        string_free(&name);
    }
    item_info_destruct(&weapon);
}

void game_mode_zc_repair_item_list(struct GameMode *self, const struct PacketRepairItemList *packet)
{
    unsigned count = (unsigned)(packet->length - 4) / REPAIR_ENTRY_SIZE;
    struct UIWindow *wnd;
    int i;

    (void)self;
    if (count == 0) {
        ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_NOTHING_TO_REPAIR), 0, 1, 0, 0, "Message", 0x118,
                               0x78, 0);
        return;
    }
    ui_window_mgr_post_quit1(&g_windowMgr, WID_REPAIR);
    wnd = ui_window_open(WID_REPAIR);
    for (i = 0; i < (int)count; i++)
        ui_window_send(wnd, NULL, UIM_REPAIR_ADD_ITEM, (int)packet->entries[i], 0, 0, 0);
}
