#ifndef GAMEMODE_ACTOR_ENTRY_H
#define GAMEMODE_ACTOR_ENTRY_H

#include "base.h"

enum {
    ACTOR_MSG_NEW_ENTRY_OLD = 0x04,
    ACTOR_MSG_ENTRY_OLD = 0x03,
    ACTOR_MSG_NEW_ENTRY = 0x79,
    ACTOR_MSG_ENTRY = 0x7c,
};

struct ActorEntryInfo {
    unsigned aid;
    int job;
    int sex;
    int x;
    int y;
    int dir;
    int speed;
    int x_size;
    int y_size;
    int head;
    int weapon[2];
    int accessory;
    int state;
    int body_state;
    int health_state;
    int effect_state;
    int is_pk_mode;
    int shield;
    int accessory2;
    int accessory3;
    int head_palette;
    int body_palette;
    int head_dir;
    unsigned guild_id;
    int emblem_version;
    int honor;
    int virtue;
    int robe;
    int char_id;
    int body;
};
SIZE_ASSERT(ActorEntryInfo, 0x7c);
OFFSET_ASSERT(ActorEntryInfo, head, 0x24);
OFFSET_ASSERT(ActorEntryInfo, guild_id, 0x60);
OFFSET_ASSERT(ActorEntryInfo, robe, 0x70);
OFFSET_ASSERT(ActorEntryInfo, body, 0x78);

enum {
    ACTOR_MSG_MOVE_ENTRY_OLD = 0x05,
    ACTOR_MSG_NEW_MOVE_ENTRY_OLD = 0x06,
    ACTOR_MSG_NEW_MOVE_ENTRY = 0x7a,
    ACTOR_MSG_MOVE_ENTRY = 0x7b,
};

struct ActorMoveEntryInfo {
    unsigned aid;
    int job;
    int sex;
    unsigned move_start_time;
    int src_x, src_y;
    int sub_x, sub_y;
    int dst_x, dst_y;
    int speed;
    int head;
    int weapon[2];
    int accessory;
    int body_state;
    int health_state;
    int effect_state;
    int is_pk_mode;
    int shield;
    int accessory2;
    int accessory3;
    int head_palette;
    int body_palette;
    int head_dir;
    unsigned guild_id;
    int emblem_version;
    int honor;
    int virtue;
    int robe;
    unsigned char_id;
    int body;
};
SIZE_ASSERT(ActorMoveEntryInfo, 0x80);
OFFSET_ASSERT(ActorMoveEntryInfo, speed, 0x28);
OFFSET_ASSERT(ActorMoveEntryInfo, guild_id, 0x64);

#endif
