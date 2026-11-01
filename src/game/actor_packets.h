#ifndef GAMEMODE_ACTOR_PACKETS_H
#define GAMEMODE_ACTOR_PACKETS_H
#include <stdint.h>
#include "base.h"
struct GameMode;

#pragma pack(push, 1)
struct PacketRoomEntry {
    uint16_t id;
    int16_t length;
    uint32_t aid;
    int32_t room_id;
    int16_t max_users, users;
    uint8_t type;
};
struct PACKET_ZC_TALKBOX_CHATCONTENTS {
    uint16_t id;
    uint32_t aid;
    char text[80];
};
struct PacketActorProgress {
    uint16_t id;
    uint32_t color, seconds;
};
struct PacketEfstSetEnter2 {
    uint16_t packet_type;
    int32_t aid;
    int16_t index;
    int32_t total_ms, remain_ms, val1, val2, val3;
};
struct PacketStoreAssistantEntryV2 {
    uint16_t id;
    int32_t aid, job;
    int16_t x, y;
    uint8_t sex;
    int16_t head;
    int32_t weapon, shield;
    int16_t accessory, accessory2, accessory3, head_palette, body_palette, robe;
    char name[24];
    int16_t body;
};
#pragma pack(pop)
SIZE_ASSERT(PacketRoomEntry, 17);
SIZE_ASSERT(PACKET_ZC_TALKBOX_CHATCONTENTS, 86);
SIZE_ASSERT(PacketActorProgress, 10);
SIZE_ASSERT(PacketEfstSetEnter2, 28);
SIZE_ASSERT(PacketStoreAssistantEntryV2, 63);

void game_mode_zc_room_newentry(struct GameMode *mode, const struct PacketRoomEntry *packet);

void game_mode_buying_store_entry(struct GameMode *mode, const struct PACKET_ZC_TALKBOX_CHATCONTENTS *packet);

void game_mode_on_efst_set_enter2(struct GameMode *mode, const struct PacketEfstSetEnter2 *packet);
int game_store_assistant_restore(struct GameMode *mode, const struct PacketStoreAssistantEntryV2 *packet);

#endif
