#include "actor/actor_ops.h"
#ifndef ACTOR_APPEARANCE_H
#define ACTOR_APPEARANCE_H

#include <stdint.h>

struct Player;
struct GameActor;

struct ActorScreenPosition {
    int x, y;
    float scale;
};

int actor_get_screen_position(const struct GameActor *actor, struct ActorScreenPosition *position);

struct PlayerAppearance {
    int job;
    int look_job;
    int sex;
    int head;
    int body_palette;
    int head_palette;
    int accessory[3];
    int robe;
    int body_style;
    unsigned char mounted;
};

int player_get_appearance(struct Player *player, struct PlayerAppearance *appearance);
int player_state(const struct Player *player);
int player_has_cast_gauge(const struct Player *player);

struct ActorLook {
    int job, sex, head;
    int64_t weapon;
    int accessory[3];
    int shield, head_palette, body_palette;
};

static __inline int64_t actor_weapon_pair(int primary, int secondary)
{
    return (int64_t)((uint64_t)(uint32_t)primary |
                     ((uint64_t)(uint32_t)secondary << 32));
}

struct ActorEntryInfo;
struct ActorMoveEntryInfo;
struct ActorLook actor_look_from_entry(const struct ActorEntryInfo *entry);
struct ActorLook actor_look_from_move_entry(const struct ActorMoveEntryInfo *entry);

#endif
