#include "world/mode_actions.h"
#include "session/party_queries.h"
#include "actor/actor_ops.h"
#include "skills/skill_info.h"
#include "contents/guild_info.h"
#include "session/state.h"
#ifndef GAMEMODE_WORLD_PICK_H
#define GAMEMODE_WORLD_PICK_H

#include <windows.h>
#include "game/input.h"
#include "game/mode_msg.h"

#include "actor/render_ops.h"

static __inline unsigned char key_down(int vk)
{
    return (unsigned char)((unsigned)GetAsyncKeyState(vk) >> 8);
}

enum { CURSOR_ARROW = 0, CURSOR_ATTACK = 5, CURSOR_SKILL = 0xb };

enum {
    CURSOR_CAPTURE = 10000,
    CURSOR_COLLECTION = 40000,
    ACTOR_MSG_ATTACK = 0xa,
    ACTOR_MSG_USE_SKILL = 0x29,
    ACTOR_MSG_SKILL_LEVEL = 0x5a,
    ACTOR_MSG_USE_GROUND_SKILL = 0x41,
    ACTOR_MSG_WALK = 0x10,
    ACTOR_MSG_WALK_CHECKED = 0x11,
};

void game_mode_proceed_attack(struct GameMode *self, unsigned aid, int once);
void game_mode_process_target_actor(struct GameMode *self, int *verdict, struct GameActor *actor);
void game_mode_process_picked_actor(struct GameMode *self, int *verdict, struct PickInfo *pick,
                                            struct GameActor *actor);
char game_mode_too_heavy_to_attack(void);
extern int g_followActorGid;
int game_mode_process_obj_click(struct GameMode *self, struct PickInfo *pick, int verdict);

struct PetOrder {
    int kind;
    int x, y;
    int level, skill_id;
    int target_aid;
};
typedef void (*PetOrderFn)(struct GameActor *pet, const int *record, int is_mercenary);

static __inline void pet_submit_order(struct GameActor *pet, PetOrderFn set_order,
                                     const struct PetOrder *order, int is_mercenary)
{
    const int record[] = {order->kind, order->x, order->y, order->level,
                          order->skill_id, order->target_aid};
    set_order(pet, record, is_mercenary);
}

static __inline void pet_target_order(struct GameActor *pet, PetOrderFn set_order, int kind, int level, int skill_id,
                                      int target, int is_mercenary)
{
    struct PetOrder order;

    order.kind = kind;
    order.x = 0;
    order.y = 0;
    order.level = level;
    order.skill_id = skill_id;
    order.target_aid = target;
    pet_submit_order(pet, set_order, &order, is_mercenary);
}
enum { PET_ORDER_ATTACK = 3, PET_ORDER_SKILL_AT_ACTOR = 7 };

enum { ACTOR_STATE_DEAD = 3 };
enum {
    OBJECT_TYPE_PET = 7, OBJECT_TYPE_HOMUNCULUS = 8, OBJECT_TYPE_MERCENARY = 9,
};
enum { CURSOR_SKILL_NO_TARGET = 0xa };

void game_mode_point_at_target(struct GameMode *self);
int game_mode_process_actor_pick(struct GameMode *self, int verdict, struct PickInfo *pick);
int game_mode_process_ground_click(struct GameMode *self, int picked);

#endif
