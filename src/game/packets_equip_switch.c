#include "game/packet_handlers.h"
#include "iteminfo/item_api.h"
#include "iteminfo/item_table.h"
#include "world/mode_manager.h"
#include <stdlib.h>
#include "game/objects.h"
#include "game/item_info.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "session/session_items.h"
#include "session/session_jobs.h"

#define CHAT_COLOR_RED 0xff
#define CHAT_COLOR_EQUIP 0xc8ff00
#define UIWM_CHAT_PRINT 1

enum {
    MSG_ALT_EQUIP_FAILED = 0xbd9,
    MSG_ALT_EQUIP_BUSY = 0xbc1,
    MSG_ALT_EQUIP_ADDED = 0xc47,
    MSG_ALT_EQUIP_REMOVED = 0xc48,
    MSG_ALT_SWITCH_FAILED = 0xbe0,
    MODE_MSG_REQUEST_ACT = 0x89,
};

#pragma pack(push, 1)
struct PacketAltEquipment {
    unsigned short packet_type;
    unsigned short index;
    unsigned position;
    unsigned short result;
};
struct AltEquipmentEntry {
    unsigned short index;
    unsigned position;
};
struct PacketAltEquipmentItems {
    short packet_type;
    short length;
    struct AltEquipmentEntry items[1];
};
struct PacketAltEquipmentSwitchAck {
    unsigned short packet_type;
    unsigned short result;
};
#pragma pack(pop)

static void put_into_set(const struct ItemInfo *item, unsigned position)
{
    if (session_is_costume_or_shadow_location(&g_session1, (int)position))
        session_put_switch_costume(&g_session1, item);
    else
        session_put_switch_equip(&g_session1, item);
}

static void take_out_of_set(const struct ItemInfo *item, unsigned position)
{
    if (session_is_costume_or_shadow_location(&g_session1, (int)position))
        session_take_out_switch_costume(&g_session1, item);
    else
        session_take_out_switch_equip(&g_session1, item);
}

static void item_chat_line(const struct ItemInfo *item, int msg_id)
{
    struct String line;

    struct String name = {0};

    item_info_get_display_name(item, &name);
    string_init(&line);
    string_format(&line, "%s%s", string_cstr(&name), msg_str(msg_id));
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_CHAT_PRINT, (int)string_cstr(&line), CHAT_COLOR_EQUIP, 6, 0);
    string_free(&line);
    string_free(&name);
}

void game_mode_on_alt_equipment_equip(struct GameMode *self, const struct PacketAltEquipment *packet)
{
    struct ItemInfo item;

    if (packet->result != 0) {
        const char *text = msg_str(packet->result == 3 ? MSG_ALT_EQUIP_BUSY : MSG_ALT_EQUIP_FAILED);

        ui_window_mgr_send_msg_(&g_windowMgr, UIWM_CHAT_PRINT, (int)text, CHAT_COLOR_RED, 0, 0);
        return;
    }
    session_find_item_for_switch(&g_session1, &item, packet->index, packet->position);
    item.worn_location = (int)packet->position;
    item.view = item_table_get(atoi(string_cstr(&item.id_text)))->class_num;
    put_into_set(&item, packet->position);

    item_chat_line(&item, MSG_ALT_EQUIP_ADDED);

    item_info_destruct(&item);
}

void game_mode_on_alt_equipment_remove(struct GameMode *self, const struct PacketAltEquipment *packet)
{
    struct ItemInfo item;

    session_get_inventory_item(&g_session1, &item, packet->index);
    item.worn_location = (int)packet->position;
    switch (packet->result) {
    case 0:
        take_out_of_set(&item, packet->position);
        item_chat_line(&item, MSG_ALT_EQUIP_REMOVED);
        break;
    case 1:
        ui_window_mgr_send_msg_(&g_windowMgr, UIWM_CHAT_PRINT, (int)msg_str(MSG_ALT_EQUIP_BUSY), CHAT_COLOR_RED, 0, 0);
        take_out_of_set(&item, packet->position);
        break;
    }
    item_info_destruct(&item);
}

void game_mode_on_alt_equipment_items(struct GameMode *self, const struct PacketAltEquipmentItems *packet)
{
    int count = (int)((unsigned)(packet->length - 4) / sizeof(struct AltEquipmentEntry));
    int i;

    session_clear_switch_equips(&g_session1);
    session_clear_switch_costumes(&g_session1);
    for (i = 0; i < count; i++) {
        const struct AltEquipmentEntry *entry = &packet->items[i];
        struct ItemInfo item;

        session_find_item_for_switch(&g_session1, &item, entry->index, entry->position);
        item.worn_location = (int)entry->position;
        if (item.index == 0) {
            item_info_destruct(&item);
            return;
        }
        put_into_set(&item, entry->position);
        item_info_destruct(&item);
    }
}

void game_mode_on_alt_equipment_switch_ack(struct GameMode *self, const struct PacketAltEquipmentSwitchAck *packet)
{
    if (packet->result == 0) {
        struct Mode *cur_mode = client_current_mode();
        struct Player *player = self->world ? self->world->player : NULL;

        if (player && player->state != 0 && cur_mode)
            mode_send_message(cur_mode, MODE_MSG_REQUEST_ACT, 3, (int)player->target_aid, 0, 0);
        return;
    }
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_CHAT_PRINT,
                        (int)msg_str(packet->result == 1 ? MSG_ALT_EQUIP_BUSY : MSG_ALT_SWITCH_FAILED),
                        CHAT_COLOR_RED, 0, 0);
}

void game_mode_on_alt_equipment_clear(struct GameMode *self, const void *packet)
{
    session_clear_switch_equips(&g_session1);
    session_clear_switch_costumes(&g_session1);
}
