#ifndef GAMEMODE_PACKETS_ENTRY_H
#define GAMEMODE_PACKETS_ENTRY_H

#include "game/packet_handlers.h"
#include "game/world_objects.h"
#include "game/entry_packet_handlers.h"
#include "actor/render_object.h"
#include "session/state.h"

#include "game/packets.h"
#include "game/actor_entry.h"

enum {
    ENTRY_OBJ_PC = 0,
    ENTRY_OBJ_PET = 7,
    ENTRY_OBJ_HOMUN = 8,
    ENTRY_OBJ_MERC = 9,
};
#define ENTRY_HEAD_PET 0x2b

#pragma pack(push, 1)

struct PacketPosDir { unsigned char b[3]; };

struct PacketNewEntry {
    unsigned short id;
    unsigned aid;
    short speed, body_state, health_state, effect_state;
    short job;
    short head, weapon, accessory, shield, accessory2, accessory3;
    short head_palette, body_palette, head_dir;
    unsigned guild_id;
    short emblem_version, honor, virtue;
    unsigned char is_pk_mode, sex;
    struct PacketPosDir pos;
    unsigned char x_size, y_size;
    short clevel;
};
SIZE_ASSERT(PacketNewEntry, 0x35);

struct PacketNewEntry2 {
    unsigned short id;
    unsigned aid;
    short speed, body_state, health_state, effect_state;
    short job;
    short head;
    int weapon, shield;
    short accessory, accessory2, accessory3;
    short head_palette, body_palette, head_dir;
    unsigned guild_id;
    short emblem_version, honor, virtue;
    unsigned char is_pk_mode, sex;
    struct PacketPosDir pos;
    unsigned char x_size, y_size;
    short clevel;
};
SIZE_ASSERT(PacketNewEntry2, 0x39);

struct PacketNewEntry4 {
    unsigned short id;
    unsigned aid;
    short speed, body_state, health_state;
    int effect_state;
    short job;
    short head;
    int weapon, shield;
    short accessory, accessory2, accessory3;
    short head_palette, body_palette, head_dir;
    unsigned guild_id;
    short emblem_version, honor;
    int virtue;
    unsigned char is_pk_mode, sex;
    struct PacketPosDir pos;
    unsigned char x_size, y_size;
    short clevel;
    short font;
};
SIZE_ASSERT(PacketNewEntry4, 0x3f);

struct PacketStandEntry {
    unsigned short id;
    unsigned char object_type;
    unsigned aid;
    short speed, body_state, health_state, effect_state;
    short job;
    short head, weapon, accessory, shield, accessory2, accessory3;
    short head_palette, body_palette, head_dir;
    unsigned guild_id;
    short emblem_version, honor, virtue;
    unsigned char is_pk_mode, sex;
    struct PacketPosDir pos;
    unsigned char x_size, y_size;
    unsigned char state;
    short clevel;
};
SIZE_ASSERT(PacketStandEntry, 0x37);

struct PacketStandEntryNpc {
    unsigned short id;
    unsigned char object_type;
    unsigned aid;
    short speed, body_state, health_state, effect_state;
    short head, weapon, accessory;
    short job;
    short shield, accessory2, accessory3;
    short head_palette, body_palette, head_dir;
    unsigned char is_pk_mode, sex;
    struct PacketPosDir pos;
    unsigned char x_size, y_size;
    short clevel;
};
SIZE_ASSERT(PacketStandEntryNpc, 0x2c);

struct PacketStandEntry2 {
    unsigned short id;
    unsigned aid;
    short speed, body_state, health_state, effect_state;
    short job;
    short head;
    int weapon, shield;
    short accessory, accessory2, accessory3;
    short head_palette, body_palette, head_dir;
    unsigned guild_id;
    short emblem_version, honor, virtue;
    unsigned char is_pk_mode, sex;
    struct PacketPosDir pos;
    unsigned char x_size, y_size;
    unsigned char state;
    short clevel;
};
SIZE_ASSERT(PacketStandEntry2, 0x3a);

struct PacketStandEntry4 {
    unsigned short id;
    unsigned aid;
    short speed, body_state, health_state;
    int effect_state;
    short job;
    short head;
    int weapon, shield;
    short accessory, accessory2, accessory3;
    short head_palette, body_palette, head_dir;
    unsigned guild_id;
    short emblem_version, honor;
    int virtue;
    unsigned char is_pk_mode, sex;
    struct PacketPosDir pos;
    unsigned char x_size, y_size;
    unsigned char state;
    short clevel;
    short font;
};
SIZE_ASSERT(PacketStandEntry4, 0x40);

struct PacketStandEntry5 {
    unsigned short id;
    short length;
    unsigned char object_type;
    unsigned aid;
    short speed, body_state, health_state;
    int effect_state;
    short job;
    short head;
    int weapon, shield;
    short accessory, accessory2, accessory3;
    short head_palette, body_palette, head_dir, robe;
    unsigned guild_id;
    short emblem_version, honor;
    int virtue;
    unsigned char is_pk_mode, sex;
    struct PacketPosDir pos;
    unsigned char x_size, y_size;
    unsigned char state;
    short clevel;
    short font;
    char name[1];
};
OFFSET_ASSERT(PacketStandEntry5, name, 0x45);

struct PacketNewEntry7 {
    unsigned short id;
    short length;
    unsigned char object_type;
    unsigned aid;
    short speed, body_state, health_state;
    int effect_state;
    short job;
    short head;
    int weapon, shield;
    short accessory, accessory2, accessory3;
    short head_palette, body_palette, head_dir, robe;
    unsigned guild_id;
    short emblem_version, honor;
    int virtue;
    unsigned char is_pk_mode, sex;
    struct PacketPosDir pos;
    unsigned char x_size, y_size;
    short clevel;
    short font;
    int max_hp, hp;
    unsigned char is_boss;
    char name[1];
};
OFFSET_ASSERT(PacketNewEntry7, name, 0x4d);

struct PacketStandEntry8 {
    unsigned short id;
    short length;
    unsigned char object_type;
    unsigned aid;
    short speed, body_state, health_state;
    int effect_state;
    short job;
    short head;
    int weapon, shield;
    short accessory, accessory2, accessory3;
    short head_palette, body_palette, head_dir, robe;
    unsigned guild_id;
    short emblem_version, honor;
    int virtue;
    unsigned char is_pk_mode, sex;
    struct PacketPosDir pos;
    unsigned char x_size, y_size;
    unsigned char state;
    short clevel;
    short font;
    int max_hp, hp;
    unsigned char is_boss;
    char name[1];
};
OFFSET_ASSERT(PacketStandEntry8, name, 0x4e);

struct PacketMoveEntry9 {
    unsigned short id;
    short length;
    unsigned char object_type;
    unsigned aid;
    short speed, body_state, health_state;
    int effect_state;
    short job;
    short head;
    int weapon, shield;
    short accessory;
    unsigned move_tick;
    short accessory2, accessory3;
    short head_palette, body_palette, head_dir, robe;
    unsigned guild_id;
    short emblem_version, honor;
    int virtue;
    unsigned char is_pk_mode, sex;
    unsigned char move_data[6];
    unsigned char x_size, y_size;
    short clevel;
    short font;
    int max_hp, hp;
    unsigned char is_boss;
    char name[1];
};
OFFSET_ASSERT(PacketMoveEntry9, name, 0x54);

struct PacketMoveEntry10 {
    unsigned short id;
    short length;
    unsigned char object_type;
    unsigned aid;
    unsigned char_id;
    short speed, body_state, health_state;
    int effect_state;
    short job;
    short head;
    int weapon, shield;
    short accessory;
    unsigned move_tick;
    short accessory2, accessory3;
    short head_palette, body_palette, head_dir, robe;
    unsigned guild_id;
    short emblem_version, honor;
    int virtue;
    unsigned char is_pk_mode, sex;
    unsigned char move_data[6];
    unsigned char x_size, y_size;
    short clevel;
    short font;
    int max_hp, hp;
    unsigned char is_boss;
};
SIZE_ASSERT(PacketMoveEntry10, 0x58);
struct PacketMoveEntry11 { struct PacketMoveEntry10 base; short body; };
SIZE_ASSERT(PacketMoveEntry11, 0x5a);

struct PacketNewEntry10 {
    unsigned short id;
    short length;
    unsigned char object_type;
    unsigned aid;
    unsigned char_id;
    short speed, body_state, health_state;
    int effect_state;
    short job;
    short head;
    int weapon, shield;
    short accessory, accessory2, accessory3;
    short head_palette, body_palette, head_dir, robe;
    unsigned guild_id;
    short emblem_version, honor;
    int virtue;
    unsigned char is_pk_mode, sex;
    struct PacketPosDir pos;
    unsigned char x_size, y_size;
    short clevel;
    short font;
    int max_hp, hp;
    unsigned char is_boss;
};
SIZE_ASSERT(PacketNewEntry10, 0x51);
struct PacketNewEntry11 { struct PacketNewEntry10 base; short body; };
SIZE_ASSERT(PacketNewEntry11, 0x53);

struct PacketStandEntry10 {
    unsigned short id;
    short length;
    unsigned char object_type;
    unsigned aid;
    unsigned char_id;
    short speed, body_state, health_state;
    int effect_state;
    short job;
    short head;
    int weapon, shield;
    short accessory, accessory2, accessory3;
    short head_palette, body_palette, head_dir, robe;
    unsigned guild_id;
    short emblem_version, honor;
    int virtue;
    unsigned char is_pk_mode, sex;
    struct PacketPosDir pos;
    unsigned char x_size, y_size;
    unsigned char state;
    short clevel;
    short font;
    int max_hp, hp;
    unsigned char is_boss;
};
SIZE_ASSERT(PacketStandEntry10, 0x52);
struct PacketStandEntry11 { struct PacketStandEntry10 base; short body; };
SIZE_ASSERT(PacketStandEntry11, 0x54);

struct PacketNewEntry5 {
    unsigned short id;
    short length;
    unsigned char object_type;
    unsigned aid;
    short speed, body_state, health_state;
    int effect_state;
    short job;
    short head;
    int weapon, shield;
    short accessory, accessory2, accessory3;
    short head_palette, body_palette, head_dir, robe;
    unsigned guild_id;
    short emblem_version, honor;
    int virtue;
    unsigned char is_pk_mode, sex;
    struct PacketPosDir pos;
    unsigned char x_size, y_size;
    short clevel;
    short font;
    char name[1];
};
OFFSET_ASSERT(PacketNewEntry5, name, 0x44);

struct PacketMoveEntry {
    unsigned short id;
    unsigned aid;
    short speed, body_state, health_state, effect_state;
    short job;
    short head, weapon, accessory;
    unsigned move_start_time;
    short shield, accessory2, accessory3;
    short head_palette, body_palette, head_dir;
    unsigned guild_id;
    short emblem_version, honor, virtue;
    unsigned char is_pk_mode, sex;
    unsigned char move_data[6];
    unsigned char x_size, y_size;
    short clevel;
};
SIZE_ASSERT(PacketMoveEntry, 0x3c);

struct PacketMoveEntry2 {
    unsigned short id;
    unsigned aid;
    short speed, body_state, health_state, effect_state;
    short job;
    short head;
    int weapon, shield;
    short accessory;
    unsigned move_start_time;
    short accessory2, accessory3;
    short head_palette, body_palette, head_dir;
    unsigned guild_id;
    short emblem_version, honor, virtue;
    unsigned char is_pk_mode, sex;
    unsigned char move_data[6];
    unsigned char x_size, y_size;
    short clevel;
};
SIZE_ASSERT(PacketMoveEntry2, 0x40);

struct PacketMoveEntry3 {
    unsigned short id;
    unsigned char object_type;
    unsigned aid;
    short speed, body_state, health_state;
    int effect_state;
    short job;
    short head;
    int weapon, shield;
    short accessory;
    unsigned move_start_time;
    short accessory2, accessory3;
    short head_palette, body_palette, head_dir;
    unsigned guild_id;
    short emblem_version, honor;
    int virtue;
    unsigned char is_pk_mode, sex;
    unsigned char move_data[6];
    unsigned char x_size, y_size;
    short clevel;
};
SIZE_ASSERT(PacketMoveEntry3, 0x45);

struct PacketMoveEntry6 {
    unsigned short id;
    short length;
    unsigned char object_type;
    unsigned aid;
    short speed, body_state, health_state;
    int effect_state;
    short job;
    short head;
    int weapon, shield;
    short accessory;
    unsigned move_start_time;
    short accessory2, accessory3;
    short head_palette, body_palette, head_dir, robe;
    unsigned guild_id;
    short emblem_version, honor;
    int virtue;
    unsigned char is_pk_mode, sex;
    unsigned char move_data[6];
    unsigned char x_size, y_size;
    short clevel;
    short font;
    char name[1];
};
OFFSET_ASSERT(PacketMoveEntry6, name, 0x4b);

struct PacketNotifyRanking {
    unsigned short id;
    unsigned aid;
    int rank;
    int total;
};

#pragma pack(pop)

struct ActorEntryInfo *actor_entry_info_construct(struct ActorEntryInfo *info);
void entry_set_pos(struct ActorEntryInfo *info, const struct PacketPosDir *pos);
void entry_siege_emblem(struct GameMode *self, struct GameActor *actor, int is_pet,
                        unsigned guild_id, int emblem_version);
int entry_aid_listed(struct GameMode *self, const unsigned *aid);
void entry_unit_value(struct GameActor *actor, short accessory2, short accessory3, int shield,
                      int keep_existing);
void world_add_actor(struct GameMode *self, struct GameActor *actor);

int game_mode_job_sex(struct GameMode *self, int job, int sex);

#endif
