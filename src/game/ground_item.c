#include "game/packet_handlers.h"
#include "world/object_lookup.h"
#include "session/state.h"
#include <stdlib.h>
#include <string.h>
#include "game/ground_item.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "session/session_party.h"
#include "world/world.h"

#pragma pack(push, 1)
struct PacketItemFallEntry5 {
    unsigned short packet_type;
    int aid;
    int item_id;
    unsigned short type;
    unsigned char is_identified;
    short x, y;
    unsigned char sub_x, sub_y;
    short count;
    unsigned char show_drop_effect;
    unsigned short drop_effect_mode;
};
#pragma pack(pop)

void game_mode_on_item_fall_entry5(struct GameMode *self, const struct PacketItemFallEntry5 *packet)
{
    struct ItemDropInfo info;
    struct Item *item;

    memset(&info, 0, sizeof info);
    _itoa(packet->item_id, info.item_id_text, 10);
    info.x = packet->x;
    info.y = packet->y;
    info.sub_x = packet->sub_x;
    info.sub_y = packet->sub_y;
    info.count = packet->count;
    info.is_identified = packet->is_identified;
    info.type = packet->type;
    info.show_drop_effect = packet->show_drop_effect;
    info.drop_effect_mode = packet->drop_effect_mode;
    info.aid = packet->aid;
    info.falls = 1;

    item = world_get_item_by_aid(self->world, packet->aid);
    if (item == 0) {
        item = item_create();
        if (!item) return;
        list_push_back(&self->world->gameObjects, &item, sizeof item);
        list_push_back(&self->world->items, &item, sizeof item);
    }
    item_place(item, self, &info);
}

#pragma pack(push, 1)
struct PacketItemPickupParty2 {
    unsigned short packet_type;
    int aid;
    int item_id;
    unsigned char is_identified;
    unsigned char is_damaged;
    int slot[4];
    unsigned short location;
    unsigned char type;
    unsigned char refining_level;
    unsigned char grade;
};
#pragma pack(pop)

enum { MSG_PARTY_MEMBER_GOT_ITEM = 0x4fc };

void game_mode_on_item_pickup_party2(struct GameMode *self, const struct PacketItemPickupParty2 *packet)
{
    struct PartyMember member;

    (void)self;
    session_get_party_member(&g_session1, &member, packet->aid);
    if (packet->aid != g_session1.ownAid && member.found != 0) {
        struct ItemInfo item;
        struct String line;
        struct String item_name = {0};
        struct String *name;

        item_info_construct(&item);
        item.type = packet->type;
        item.equip_location = packet->location;
        item.index = packet->item_id;
        item.worn_location = 0;
        item_info_set_item_id(&item, packet->item_id);
        item.slot[0] = packet->slot[0];
        item.slot[1] = packet->slot[1];
        item.slot[2] = packet->slot[2];
        item.slot[3] = packet->slot[3];
        item.identified = packet->is_identified;
        item.damaged = packet->is_damaged;
        item.refine = packet->refining_level;
        item.grade = packet->grade;

        name = item_info_get_display_name(&item, &item_name);
        string_init(&line);
        string_format(&line, msg_str(MSG_PARTY_MEMBER_GOT_ITEM), string_cstr(&member.name),
                          string_cstr(name));
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)string_cstr(&line), CHAT_COLOR_YELLOW,
                            8, 0);
        string_free(&line);
        string_free(&item_name);
        item_info_destruct(&item);
    }
    party_member_info_destroy(&member);
}
