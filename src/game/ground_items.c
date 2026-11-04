#include "game/actor_packets.h"
#include "game/gamemode.h"
#include "game/ground_item_packets.h"
#include "game/pickup_requests.h"
#include "game/ground_item.h"
#include "world/object_lookup.h"
#include <stdlib.h>
#include <string.h>
#include "game/packet_handlers.h"
#include "game/client.h"
#include "world/world.h"

static void show_ground_item(struct GameMode *self, struct ItemDropInfo *entry)
{
    struct Item *item = world_get_item_by_aid(self->world, entry->aid);

    if (item == 0) {
        item = item_create();
        if (!item) return;
        list_push_back(&self->world->gameObjects, &item, sizeof item);
        list_push_back(&self->world->items, &item, sizeof item);
    }
    item_place(item, self, entry);
}

void game_mode_item_entry(struct GameMode *self, const struct PacketGroundItemEntry *packet)
{
    struct ItemDropInfo entry = {0};
    entry.count = packet->count;
    entry.aid = packet->aid;
    entry.x = packet->x;
    entry.y = packet->y;
    entry.sub_x = packet->sub_x;
    entry.sub_y = packet->sub_y;
    snprintf(entry.item_id_text, sizeof entry.item_id_text, "%d", packet->item_id);
    entry.falls = 0;
    entry.is_identified = packet->identified;
    show_ground_item(self, &entry);
}

void game_mode_item_fall_entry4(struct GameMode *self, const struct PacketGroundItemFall4 *packet)
{
    struct ItemDropInfo entry = {0};
    entry.count = packet->count;
    entry.aid = packet->aid;
    entry.x = packet->x;
    entry.y = packet->y;
    entry.sub_x = packet->sub_x;
    entry.sub_y = packet->sub_y;
    snprintf(entry.item_id_text, sizeof entry.item_id_text, "%d", packet->item_id);
    entry.falls = 1;
    entry.is_identified = packet->identified;
    entry.type = packet->type;
    show_ground_item(self, &entry);
}

void game_mode_item_fall_entry(struct GameMode *self, const struct PacketGroundItemFall *packet)
{
    struct ItemDropInfo entry = {0};
    entry.count = packet->count;
    entry.aid = packet->aid;
    entry.x = packet->x;
    entry.y = packet->y;
    entry.sub_x = packet->sub_x;
    entry.sub_y = packet->sub_y;
    snprintf(entry.item_id_text, sizeof entry.item_id_text, "%d", packet->item_id);
    entry.falls = 1;
    entry.is_identified = packet->identified;
    show_ground_item(self, &entry);
}

int game_mode_is_exception_state(int state)
{
    return state == 0x28 || state == 0x2a;
}

#pragma pack(push, 1)
struct PACKET_ZC_ITEM_DISAPPEAR {
    unsigned short packet_type;
    int aid;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ITEM_DISAPPEAR, 6);

void game_mode_zc_item_disappear(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_ITEM_DISAPPEAR *packet = (const struct PACKET_ZC_ITEM_DISAPPEAR *)pkt;

    world_remove_item(self->world, packet->aid);
    game_mode_remove_pickup_request(self, packet->aid);
}
