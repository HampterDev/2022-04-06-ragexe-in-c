#include "game/gamemode.h"
#include "game/mode_util.h"
#include "contents/guild_info.h"
#include "session/state.h"
#include "contents/mouse.h"
#include <windows.h>
#include "game/world_pick.h"
#include "game/cells.h"
#include "actor/actor_common.h"
#include "session/session_jobs.h"
#include "ui/wndmgr/wndmgr.h"

#define CZ_CONTACTNPC 0x90

enum {
    PICK_ACTOR = 0,
    PICK_GROUND_ITEM = 1,
    PICK_SKILL_UNIT = 2,
    PICK_PET = 3,
    PICK_COMPANION = 4,
};
enum {
    CURSOR_TALK = 1,
    CURSOR_WARP = 7,
    CURSOR_PICK_UP = 9,
};
enum { JOB_WARP_PORTAL = 0x2d };
enum { PLAYER_STATE_SIT = 6 };
enum { SKILL_USE_ON_UNIT = 5 };
enum { SKILL_AM_POTIONPITCHER = 0xe7 };
enum {
    ACTOR_MSG_PICK_UP = 0x12,
    ACTOR_MSG_UNIT_SKILL = 0x67,
};

static int held_or(int result)
{
    return g_mouse.button[MOUSE_LEFT] == BUTTON_HELD ? 2 : result;
}

static int aiming_at_actor(struct GameMode *self)
{
    return self->skill_use.type == 2 || self->skill_use.type == 4;
}

static int point_at_player(struct GameMode *self, struct PickInfo *pick)
{
    struct GameActor *actor;
    struct World *world;
    int aiming, shift, good, attack, state;

    aiming = aiming_at_actor(self);
    shift = key_down(VK_SHIFT);
    attack = shift;
    good = 0;
    if (aiming) {
        good = (unsigned char)skill_info_is_good_skill_for_actor(self->skill_use.skill_id);
        if (good != 0)
            attack = shift == 0;
    }
    actor = world_get_game_actor_by_aid(self->world, pick->aid);
    if (self->world->is_battleground != 0) {
        if (actor != 0 && g_session1.battlegroundCamp != actor->camp)
            attack = 1;
    } else {
        if (session_map_is_pvp(&g_session1) != 0) {
            if (session_is_party_member_aid(&g_session1, pick->aid, 1) == 0)
                attack = 1;
        } else if (session_map_is_gvg(&g_session1) != 0 && actor != 0) {
            if (session_is_own_guild_id(&g_session1, game_actor_get_guild_id(actor)) == 0)
                attack = 1;
        }
        world = self->world;
        if (world->is_siege != 0) {
            if (actor != 0) {
                if (session_is_own_guild_id(&g_session1, game_actor_get_guild_id(actor)) == 0
                    && !guild_info_has_kind0_relation(&g_guildInfo, game_actor_get_guild_id(actor)))
                    attack = 1;
                else if (!aiming)
                    attack = 0;
                else if (skill_info_may_harm(self->skill_use.skill_id) == 1 && (g_session1.noShift != 0 || shift != 0))
                    attack = 1;
                else
                    attack = good;
            }
        } else if (pick->attackable_as_pvp != 0 || world->is_pvp != 0 || world->is_event_pvp != 0 || g_session1.map_rule_flags[2] != 0) {
            if (session_is_party_member_aid(&g_session1, pick->aid, 1) == 0)
                attack = 1;
        }
    }

    state = actor != 0 ? actor->state : ACTOR_STATE_DEAD;
    if (attack != 0 && state != ACTOR_STATE_DEAD) {
        mode_set_cursor_action(&self->base, aiming ? CURSOR_SKILL : CURSOR_ATTACK);
        self->pointed_player_aid = pick->aid;
        return 1;
    }
    mode_set_cursor_action(&self->base, aiming && state != ACTOR_STATE_DEAD ? CURSOR_SKILL_NO_TARGET : CURSOR_ARROW);
    if (g_mouse.button[MOUSE_LEFT] == 0 && self->base.attack_in_progress == 0 && self->balloon_follow_aid == 0)
        self->pointed_player_aid = pick->aid;
    return 0;
}

static int point_at_monster(struct GameMode *self, struct PickInfo *pick)
{
    struct GameActor *actor;
    int aiming, shift, good, attack, cursor;

    aiming = aiming_at_actor(self);
    shift = key_down(VK_SHIFT);
    good = 0;
    if (aiming)
        good = (unsigned char)skill_info_is_good_skill_for_actor(self->skill_use.skill_id);
    attack = good != 0 ? shift : shift == 0;
    if (g_session1.noShift == 1)
        attack = 1;
    actor = world_get_game_actor_by_aid(self->world, pick->aid);
    if (session_map_is_gvg(&g_session1) != 0 && actor != 0 && session_is_own_guild_id(&g_session1, game_actor_get_guild_id(actor)) != 0)
        attack = 0;
    if (self->world->is_siege != 0 && actor != 0 && session_is_own_guild_id(&g_session1, game_actor_get_guild_id(actor)) != 0)
        attack = 0;

    if (self->held_walk_allowed != 0) {
        if (attack != 0) {
            if (aiming)
                cursor = CURSOR_SKILL;
            else if (actor != 0 && actor->object_type == OBJECT_TYPE_PET)
                cursor = CURSOR_ARROW;
            else
                cursor = CURSOR_ATTACK;
        } else {
            cursor = aiming ? CURSOR_SKILL_NO_TARGET : CURSOR_ARROW;
        }
        mode_set_cursor_action(&self->base, cursor);
    }
    if (g_mouse.button[MOUSE_LEFT] == 0 && self->base.attack_in_progress == 0)
        self->balloon_follow_aid = pick->aid;
    if (g_mouse.button[MOUSE_LEFT] == BUTTON_HELD || g_mouse.button[MOUSE_RIGHT] == BUTTON_HELD)
        return 2;
    return 1;
}

static int point_at_actor(struct GameMode *self, struct PickInfo *pick)
{
    if (pick->job != JOB_WARP_PORTAL && session_is_npc(&g_session1, pick->aid, pick->job) == 1) {
        mode_set_cursor_action(&self->base, CURSOR_TALK);
        return held_or(1);
    }
    if (pick->job == JOB_WARP_PORTAL) {
        mode_set_cursor_action(&self->base, CURSOR_WARP);
        return 2;
    }
    if (session_is_pc_job(&g_session1, pick->job) != 0)
        return point_at_player(self, pick);
    if (session_get_pet_acc_act_name(&g_session1, pick->job) == 1) {
        mode_set_cursor_action(&self->base, CURSOR_TALK);
        return held_or(1);
    }
    if (job_is_monster(pick->job))
        return point_at_monster(self, pick);
    return 1;
}

static int unit_is_attackable(int unit)
{
    switch (unit) {
    case 0x8d: case 0x8f: case 0x98: case 0xda: case 0xde: case 0xe6:
    case 0x5174: case 0x5175: case 0x5176: case 0x5177:
        return 1;
    }
    return 0;
}

static int unit_is_attackable_in_siege(int unit)
{
    switch (unit) {
    case 0x8d: case 0x8f: case 0x90: case 0x91: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97: case 0x98:
    case 0x99: case 0xbe: case 0xbf: case 0xc0: case 0xc1: case 0xc2: case 0xcb: case 0xd2: case 0xd3: case 0xd4:
    case 0xd5: case 0xd6: case 0xd7: case 0xd8: case 0xd9: case 0xda: case 0xde: case 0xe5: case 0xe6: case 0xe7:
    case 0xe8: case 0xe9: case 0xf0: case 0x108:
    case 0x5174: case 0x5175: case 0x5176: case 0x5177:
        return 1;
    }
    return 0;
}

static int point_at_skill_unit(struct GameMode *self, struct PickInfo *pick)
{
    int attackable, cursor;

    if (session_is_siege_mode(&g_session1) != 0)
        attackable = unit_is_attackable_in_siege(pick->job);
    else
        attackable = unit_is_attackable(pick->job);
    if (self->skill_use.type == SKILL_USE_ON_UNIT)
        cursor = CURSOR_SKILL_NO_TARGET;
    else
        cursor = attackable ? CURSOR_ATTACK : CURSOR_ARROW;
    mode_set_cursor_action(&self->base, cursor);
    return held_or(1);
}

static int point_at_companion(struct GameMode *self, struct PickInfo *pick)
{
    struct GameActor *actor, *owner;
    struct World *world;

    mode_set_cursor_action(&self->base, CURSOR_ARROW);
    actor = world_get_game_actor_by_aid(self->world, pick->aid);
    if (actor == 0 || actor->state == ACTOR_STATE_DEAD)
        return 0;
    if (g_session1.ownAid == actor->owner_aid) {
        if (actor->object_type != OBJECT_TYPE_HOMUNCULUS || self->skill_use.skill_id != SKILL_AM_POTIONPITCHER)
            return 0;
        mode_set_cursor_action(&self->base, CURSOR_SKILL);
        self->balloon_follow_aid = pick->aid;
        return 1;
    }
    world = self->world;
    if (world->is_pvp != 1 && g_session1.map_rule_flags[2] != 1 && world->is_siege != 1)
        return 0;
    if (world->is_siege == 1) {
        owner = world_get_game_actor_by_aid(world, actor->owner_aid);
        if (owner == 0 || session_is_own_guild_id(&g_session1, game_actor_get_guild_id(owner)) == 1
            || guild_info_has_kind0_relation(&g_guildInfo, game_actor_get_guild_id(owner)) == 1)
            return 0;
    }
    mode_set_cursor_action(&self->base, CURSOR_ATTACK);
    self->balloon_follow_aid = pick->aid;
    return 1;
}

static void talk_to_npc(struct GameMode *self, struct PickInfo *pick)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        int npc_id;
        unsigned char kind;
    } packet;
#pragma pack(pop)
    struct GameActor *npc;
    int state, x, y;

    state = self->world->player->state;
    if (state != 0 && state != PLAYER_STATE_SIT)
        return;
    if (game_mode_is_blocking_window_open() == 1)
        return;
    npc = world_get_game_actor_by_aid(self->world, pick->aid);
    game_mode_world_to_cell(self, npc->pos.x, npc->pos.z, &x, &y);
    render_object_send(self->world->player, NULL, ACTOR_MSG_WALK, (long long)x, (long long)y, 0, 0);
    packet.id = CZ_CONTACTNPC;
    packet.npc_id = pick->aid;
    packet.kind = 1;
    send_packet(CZ_CONTACTNPC, &packet);
}

static void attack_actor(struct GameMode *self, int aid, int repeat_as_once)
{
    int repeat, once;

    mode_send_msg(self, MODE_MSG_SET_ATTACK_IN_PROGRESS, 1, 0, 0, 0);
    self->attack_target_aid = aid;
    self->balloon_follow_aid = aid;
    self->held_walk_allowed = 0;
    if (!session_is_siege_mode(&g_session1) && (key_down(VK_CONTROL) || g_session1.noCtrl == 1)) {
        repeat = 1;
        once = repeat_as_once;
    } else {
        repeat = 0;
        once = 1;
    }
    game_mode_proceed_attack(self, aid, once);
    self->attack_repeat = repeat;
}

static void click_monster(struct GameMode *self, struct PickInfo *pick)
{
    struct GameActor *pet;
    int aiming, shift, good, attack;

    aiming = aiming_at_actor(self);
    shift = key_down(VK_SHIFT);
    good = 0;
    if (aiming)
        good = (unsigned char)skill_info_is_good_skill_for_actor(self->skill_use.skill_id);
    attack = good != 0 ? shift : shift == 0;
    if (g_session1.noShift == 1)
        attack = 1;

    if (g_session1.homun_ai_flags[0] != 0) {
        if (g_mouse.button[MOUSE_RIGHT] == BUTTON_PRESSED && key_down(VK_MENU)) {
            if (key_down(VK_SHIFT)) {
                pet = world_get_game_actor_by_aid(self->world, ((unsigned)g_session1.homunAid));
                if (pet != 0 && actor_homun_ai_running(pet))
                    pet_target_order(pet, game_actor_set_request2, PET_ORDER_ATTACK, 0, 0, pick->aid, 0);
            } else if (g_session1.homun_command[5] != 0 && g_session1.homun_command[0] == pick->aid) {
                pet = world_get_game_actor_by_aid(self->world, ((unsigned)g_session1.homunAid));
                if (pet != 0 && actor_homun_ai_running(pet))
                    pet_target_order(pet, game_actor_set_request, PET_ORDER_ATTACK, 0, 0, g_session1.homun_command[0], 0);
                g_session1.homun_command[5] = 0;
            } else {
                mode_set_cursor_action(&self->base, CURSOR_SKILL);
                g_session1.homun_command[0] = pick->aid;
                g_session1.homun_command[1] = 0;
                g_session1.homun_command[2] = 0;
                g_session1.homun_command[3] = 0;
                g_session1.homun_command[5] = 1;
            }
            g_session1.homun_command[4] = 1;
        }
        if (g_session1.homun_ai_flags[0] != 0 && g_mouse.button[MOUSE_LEFT] == BUTTON_PRESSED
            && session_is_homunculus_skill(&g_session1, self->skill_use.skill_id) != 0) {
            pet = world_get_game_actor_by_aid(self->world, ((unsigned)g_session1.homunAid));
            if (pet != 0 && actor_homun_ai_running(pet))
                pet_target_order(pet, game_actor_set_request, PET_ORDER_SKILL_AT_ACTOR, self->skill_use.level,
                                 self->skill_use.skill_id, pick->aid, 0);
            return;
        }
    }

    if (g_session1.mercPresent != 0 && g_mouse.button[MOUSE_LEFT] == BUTTON_PRESSED && key_down(VK_MENU)) {
        if (key_down(VK_SHIFT)) {
            pet = world_get_game_actor_by_aid(self->world, ((unsigned)g_session1.mercAid));
            if (pet != 0 && actor_merc_ai_running(pet))
                pet_target_order(pet, game_actor_set_request2, PET_ORDER_ATTACK, 0, 0, pick->aid, 1);
        } else if (g_session1.merc_command[6] != 0 && g_session1.merc_command[0] == pick->aid) {
            pet = world_get_game_actor_by_aid(self->world, ((unsigned)g_session1.mercAid));
            if (pet != 0 && actor_merc_ai_running(pet))
                pet_target_order(pet, game_actor_set_request, PET_ORDER_ATTACK, 0, 0, g_session1.merc_command[0], 1);
            g_session1.merc_command[6] = 0;
        } else {
            mode_set_cursor_action(&self->base, CURSOR_SKILL);
            g_session1.merc_command[0] = pick->aid;
            g_session1.merc_command[1] = 0;
            g_session1.merc_command[2] = 0;
            g_session1.merc_command[3] = 0;
            g_session1.merc_command[6] = 1;
        }
        g_session1.merc_command[5] = 1;
    }
    if (attack != 0 && g_session1.mercPresent != 0) {
        if (g_mouse.button[MOUSE_LEFT] != BUTTON_PRESSED)
            goto not_attacked;
        if (session_is_merc_or_elemental_skill(&g_session1, self->skill_use.skill_id) != 0) {
            pet = world_get_game_actor_by_aid(self->world, ((unsigned)g_session1.mercAid));
            if (pet != 0 && actor_merc_ai_running(pet))
                pet_target_order(pet, game_actor_set_request, PET_ORDER_SKILL_AT_ACTOR, self->skill_use.level,
                                 self->skill_use.skill_id, pick->aid, 1);
            return;
        }
    }

    if (g_mouse.button[MOUSE_LEFT] == BUTTON_PRESSED && attack != 0 && !key_down(VK_MENU)) {
        attack_actor(self, pick->aid, 0);
        return;
    }
not_attacked:
    if (self->attack_repeat == 0 && g_mouse.button[MOUSE_LEFT] == BUTTON_RELEASED)
        self->balloon_follow_aid = 0;
}

static void click_player(struct GameMode *self, struct PickInfo *pick)
{
    struct GameActor *pet;
    struct World *world;
    int aiming, attack, is_mercenary;

    aiming = aiming_at_actor(self);
    attack = key_down(VK_SHIFT);
    if (aiming && skill_info_is_good_skill_for_actor(self->skill_use.skill_id))
        attack = attack == 0;
    world = self->world;
    if (pick->attackable_as_pvp != 0 || world->is_pvp != 0 || g_session1.map_rule_flags[2] != 0)
        attack = 1;
    if (g_session1.noShift == 1)
        attack = 1;

    if (attack != 0 && g_mouse.button[MOUSE_LEFT] == BUTTON_PRESSED) {
        is_mercenary = 0;
        if (session_is_homunculus_skill(&g_session1, self->skill_use.skill_id) != 0 && g_session1.homun_ai_flags[0] != 0) {
            pet = world_get_game_actor_by_aid(self->world, ((unsigned)g_session1.homunAid));
            if (pet != 0 && actor_homun_ai_running(pet))
                goto skill_order;
        } else if (session_is_merc_or_elemental_skill(&g_session1, self->skill_use.skill_id) != 0 && g_session1.mercPresent != 0) {
            pet = world_get_game_actor_by_aid(self->world, ((unsigned)g_session1.mercAid));
            if (pet != 0 && actor_merc_ai_running(pet)) {
                is_mercenary = 1;
                goto skill_order;
            }
        }
    }
    if (pick->aid == g_session1.ownAid && self->skill_use.type != 4)
        return;
    if (attack == 0)
        return;
    attack_actor(self, pick->aid, 0);
    return;

skill_order:
    pet_target_order(pet, game_actor_set_request, PET_ORDER_SKILL_AT_ACTOR, self->skill_use.level, self->skill_use.skill_id,
                     pick->aid, is_mercenary);
}

static void click_actor(struct GameMode *self, struct PickInfo *pick)
{
    if (pick->job == JOB_WARP_PORTAL)
        return;
    if (session_is_npc(&g_session1, pick->aid, pick->job) == 1)
        talk_to_npc(self, pick);
    else if (job_is_monster(pick->job))
        click_monster(self, pick);
    else if (session_is_pc_job(&g_session1, pick->job) != 0)
        click_player(self, pick);
}

static void click_skill_unit(struct GameMode *self, struct PickInfo *pick)
{
    int skill = self->skill_use.skill_id;

    if (!unit_is_attackable(pick->job)) {
        if (self->skill_use.type == SKILL_USE_ON_UNIT)
            render_object_send(self->world->player, NULL, ACTOR_MSG_UNIT_SKILL, (long long)pick->aid, (long long)(INT_PTR)&self->skill_use, 0, 0);
        return;
    }
    if (skill == 0xd7 || skill == 0xd8 || skill == 0xd9 || skill == 0xda)
        return;
    mode_send_msg(self, MODE_MSG_SET_ATTACK_IN_PROGRESS, 1, 0, 0, 0);
    self->held_walk_allowed = 0;
    if (key_down(VK_CONTROL) || g_session1.noCtrl == 1) {
        render_object_send(self->world->player, NULL, ACTOR_MSG_UNIT_SKILL, (long long)pick->aid, (long long)(INT_PTR)&self->skill_use, 0, 0);
        self->attack_repeat = 1;
    } else {
        render_object_send(self->world->player, NULL, ACTOR_MSG_UNIT_SKILL, (long long)pick->aid, (long long)(INT_PTR)&self->skill_use, 1, 0);
        self->attack_repeat = 0;
    }
}

static int click_companion(struct GameMode *self, struct PickInfo *pick, int result)
{
    struct World *world;
    int aiming, attack, is_mercenary;

    mode_set_cursor_action(&self->base, CURSOR_ARROW);
    if (session_is_homunculus_job(&g_session1, pick->job) != 0) {
        if (self->skill_use.skill_id != SKILL_AM_POTIONPITCHER) {
            world = self->world;
            if (world->is_pvp == 0 && world->is_siege == 0 && g_session1.map_rule_flags[2] == 0)
                return result;
            if (session_is_homunculus_aid(&g_session1, pick->aid) != 0)
                return result;
        }
        world = self->world;
        if ((world->is_pvp != 0 || world->is_siege != 0 || g_session1.map_rule_flags[2] != 0)
            && session_is_homunculus_aid(&g_session1, pick->aid) == 0)
            mode_set_cursor_action(&self->base, CURSOR_ATTACK);
        game_mode_proceed_attack(self, pick->aid, 1);
        self->attack_repeat = 0;
        return 0;
    }

    aiming = aiming_at_actor(self);
    attack = key_down(VK_SHIFT);
    if (aiming && skill_info_is_good_skill_for_actor(self->skill_use.skill_id))
        attack = attack == 0;
    world = self->world;
    if (pick->attackable_as_pvp != 0 || world->is_pvp != 0 || world->is_siege != 0 || g_session1.map_rule_flags[2] != 0)
        attack = 1;
    is_mercenary = pick->aid == (int)((unsigned)g_session1.mercAid);
    if (is_mercenary && self->skill_use.type != 4)
        return 0;
    if (attack != 0)
        game_mode_proceed_attack(self, pick->aid, 1);
    return 0;
}

int game_mode_process_obj_click(struct GameMode *self, struct PickInfo *pick, int verdict)
{
    int result;

    if (pick->aid == g_session1.ownAid || (unsigned)pick->kind > PICK_COMPANION) {
        result = verdict;
    } else {
        switch (pick->kind) {
        case PICK_ACTOR:
            result = point_at_actor(self, pick);
            break;
        case PICK_GROUND_ITEM:
            mode_set_cursor_action(&self->base, CURSOR_PICK_UP);
            result = held_or(1);
            break;
        case PICK_SKILL_UNIT:
            result = point_at_skill_unit(self, pick);
            break;
        case PICK_PET:
            mode_set_cursor_action(&self->base, CURSOR_ARROW);
            result = 0;
            break;
        default:
            result = point_at_companion(self, pick);
            break;
        }
    }

    if (g_mouse.button[MOUSE_LEFT] != BUTTON_PRESSED && g_mouse.button[MOUSE_RIGHT] != BUTTON_PRESSED)
        return result;
    switch (pick->kind) {
    case PICK_ACTOR:
        click_actor(self, pick);
        return result;
    case PICK_GROUND_ITEM:
        if (g_mouse.button[MOUSE_LEFT] == BUTTON_PRESSED && self->skill_use.type == 0)
            render_object_send(self->world->player, NULL, ACTOR_MSG_PICK_UP, (long long)pick->aid, 0, 0, 0);
        return result;
    case PICK_SKILL_UNIT:
        click_skill_unit(self, pick);
        return result;
    case PICK_PET:
        mode_set_cursor_action(&self->base, CURSOR_ARROW);
        return 0;
    case PICK_COMPANION:
        return click_companion(self, pick, result);
    }
    return result;
}
