#include "game/actor_actions.h"
#include "actor/actor_ops.h"
#include "contents/guild_info.h"
#include "session/state.h"
#include "contents/mouse.h"
#include "game/world_pick.h"
#include "game/name_balloon.h"
#include "session/session_jobs.h"
#include "ui/wndmgr/wndmgr.h"

void game_mode_process_target_actor(struct GameMode *self, int *verdict, struct GameActor *actor)
{
    struct GameActor *pet;
    int aiming, shift, attack, repeat, once;

    aiming = self->skill_use.type == 2 || self->skill_use.type == 4;
    shift = key_down(VK_SHIFT);
    attack = shift == 0;
    if (aiming) {
        if (skill_info_is_good_skill_for_actor(self->skill_use.skill_id))
            attack = shift;
        mode_set_cursor_action(&self->base, CURSOR_SKILL);
    } else if (actor->object_type == OBJECT_TYPE_PET) {
        mode_set_cursor_action(&self->base, CURSOR_ARROW);
    } else if (session_map_is_gvg(&g_session1) == 0 && self->world->is_siege == 0) {
        mode_set_cursor_action(&self->base, CURSOR_ATTACK);
    } else if (session_is_own_guild_id(&g_session1, game_actor_get_guild_id(actor)) != 0) {
        attack = 0;
        mode_set_cursor_action(&self->base, CURSOR_ARROW);
    }
    *verdict = 1;
    if (g_session1.noShift == 1)
        attack = 1;

    if (g_session1.homun_ai_flags[0] != 0) {
        if (g_mouse.button[MOUSE_RIGHT] == BUTTON_PRESSED && key_down(VK_MENU)) {
            if (key_down(VK_SHIFT)) {
                pet = world_get_game_actor_by_aid(self->world, ((unsigned)g_session1.homunAid));
                if (pet != 0 && actor_homun_ai_running(pet))
                    pet_target_order(pet, game_actor_set_request2, PET_ORDER_ATTACK, 0, 0, actor->aid, 0);
            } else if (g_session1.homun_command[5] != 0 && g_session1.homun_command[0] == actor->aid) {
                pet = world_get_game_actor_by_aid(self->world, ((unsigned)g_session1.homunAid));
                if (pet != 0 && actor_homun_ai_running(pet))
                    pet_target_order(pet, game_actor_set_request, PET_ORDER_ATTACK, 0, 0, g_session1.homun_command[0], 0);
                g_session1.homun_command[5] = 0;
            } else {
                mode_set_cursor_action(&self->base, CURSOR_SKILL);
                g_session1.homun_command[0] = actor->aid;
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
            if (pet == 0 || !actor_homun_ai_running(pet))
                return;
            pet_target_order(pet, game_actor_set_request, PET_ORDER_SKILL_AT_ACTOR, self->skill_use.level, self->skill_use.skill_id,
                             actor->aid, 0);
            return;
        }
    }

    if (g_session1.mercPresent != 0 && g_mouse.button[MOUSE_LEFT] == BUTTON_PRESSED && key_down(VK_MENU)) {
        if (key_down(VK_SHIFT)) {
            pet = world_get_game_actor_by_aid(self->world, ((unsigned)g_session1.mercAid));
            if (pet != 0 && actor_merc_ai_running(pet))
                pet_target_order(pet, game_actor_set_request2, PET_ORDER_ATTACK, 0, 0, actor->aid, 1);
        } else if (g_session1.merc_command[6] != 0 && g_session1.merc_command[0] == actor->aid) {
            pet = world_get_game_actor_by_aid(self->world, ((unsigned)g_session1.mercAid));
            if (pet != 0 && actor_merc_ai_running(pet))
                pet_target_order(pet, game_actor_set_request, PET_ORDER_ATTACK, 0, 0, g_session1.merc_command[0], 1);
            g_session1.merc_command[6] = 0;
        } else {
            mode_set_cursor_action(&self->base, CURSOR_SKILL);
            g_session1.merc_command[0] = actor->aid;
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
        if (session_is_mercenary_skill(&g_session1, self->skill_use.skill_id) != 0) {
            pet = world_get_game_actor_by_aid(self->world, ((unsigned)g_session1.mercAid));
            if (pet == 0 || !actor_merc_ai_running(pet))
                return;
            pet_target_order(pet, game_actor_set_request, PET_ORDER_SKILL_AT_ACTOR, self->skill_use.level, self->skill_use.skill_id,
                             actor->aid, 1);
            return;
        }
    }

    if (g_mouse.button[MOUSE_LEFT] == BUTTON_PRESSED && attack != 0 && !key_down(VK_MENU)) {
        mode_send_msg(self, MODE_MSG_SET_ATTACK_IN_PROGRESS, 1, 0, 0, 0);
        self->attack_target_aid = self->balloon_follow_aid;
        self->held_walk_allowed = 0;
        if (!session_is_siege_mode(&g_session1) && (key_down(VK_CONTROL) || g_session1.noCtrl == 1)) {
            repeat = 1;
            once = 0;
        } else {
            repeat = 0;
            once = 1;
        }
        game_mode_proceed_attack(self, self->balloon_follow_aid, once);
        self->attack_repeat = repeat;
    } else {
not_attacked:
        if (self->attack_repeat == 0 && g_mouse.button[MOUSE_LEFT] == BUTTON_RELEASED)
            self->balloon_follow_aid = 0;
    }
    if ((self->last_mouse_x != g_mouse.x || self->last_mouse_y != g_mouse.y) && self->attack_repeat == 0
        && g_mouse.button[MOUSE_LEFT] == 0)
        self->balloon_follow_aid = 0;
    game_mode_process_monster_pick(self, self->balloon_follow_aid);
    if (actor->state == ACTOR_STATE_DEAD)
        self->balloon_follow_aid = 0;
}

static int enemy_guild(struct GameActor *actor)
{
    return session_is_own_guild_id(&g_session1, game_actor_get_guild_id(actor)) == 0
           && !guild_info_has_kind0_relation(&g_guildInfo, game_actor_get_guild_id(actor));
}

static int picked_is_foe(struct GameMode *self, struct PickInfo *pick, struct GameActor *actor, int aiming,
                         int shift, int good, unsigned char harm, int *attack)
{
    struct GameActor *owner, *judged;

    if (pick == 0)
        return 0;
    if (self->world->is_battleground) {
        if (actor != 0 && g_session1.battlegroundCamp != actor->camp) {
            *attack = 1;
            return 1;
        }
        return 0;
    }
    if (session_map_is_pvp(&g_session1) != 0) {
        if (session_is_party_member_aid(&g_session1, actor->aid, 1) == 0)
            *attack = 1;
    } else if (session_map_is_gvg(&g_session1) != 0) {
        if (session_is_own_guild_id(&g_session1, game_actor_get_guild_id(actor)) == 0)
            *attack = 1;
    }
    if (self->world->is_siege == 0) {
        if (pick->attackable_as_pvp != 0 || self->world->is_pvp != 0 || self->world->is_event_pvp != 0 || g_session1.map_rule_flags[2] != 0) {
            if (session_is_party_member_aid(&g_session1, actor->aid, 1) == 0) {
                *attack = 1;
                return 1;
            }
        }
        return 0;
    }

    if (actor == 0)
        return 0;
    judged = actor;
    switch (actor->object_type) {
    case OBJECT_TYPE_HOMUNCULUS:
    case OBJECT_TYPE_MERCENARY:
        owner = world_get_game_actor_by_aid(self->world, actor->owner_aid);
        if (owner != 0)
            judged = owner;
        else if (self->world->player->aid == actor->owner_aid)
            judged = 0;
        else
            return 0;
        break;
    }
    if (judged != 0 && enemy_guild(judged)) {
        *attack = 1;
        return 1;
    }
    *attack = 0;
    if (aiming) {
        if ((g_session1.noShift != 0 || shift != 0) && harm == 1) {
            *attack = 1;
            return 1;
        }
        *attack = good;
    }
    return 0;
}

void game_mode_process_picked_actor(struct GameMode *self, int *verdict, struct PickInfo *pick,
                                            struct GameActor *actor)
{
    struct GameActor *pet;
    int aiming, shift, good, attack, foe, may_order;
    unsigned char harm;

    aiming = self->skill_use.type == 2 || self->skill_use.type == 4;
    shift = key_down(VK_SHIFT);
    good = 0;
    harm = 0;
    attack = shift;
    if (aiming) {
        harm = skill_info_may_harm(self->skill_use.skill_id);
        good = (unsigned char)skill_info_is_good_skill_for_actor(self->skill_use.skill_id);
        if (good != 0)
            attack = shift == 0;
    }
    foe = picked_is_foe(self, pick, actor, aiming, shift, good, harm, &attack);

    if ((foe || attack != 0) && actor->state != ACTOR_STATE_DEAD) {
        mode_set_cursor_action(&self->base, aiming ? CURSOR_SKILL : CURSOR_ATTACK);
        *verdict = 1;
    } else {
        mode_set_cursor_action(&self->base, aiming && actor->state != ACTOR_STATE_DEAD ? CURSOR_SKILL_NO_TARGET
                                                                                    : CURSOR_ARROW);
    }

    may_order = (attack != 0 || self->world->is_siege != 0) && actor->state != ACTOR_STATE_DEAD && !aiming;
    if (may_order && g_session1.homun_ai_flags[0] != 0 && g_mouse.button[MOUSE_RIGHT] == BUTTON_PRESSED && key_down(VK_MENU)) {
        if (key_down(VK_SHIFT)) {
            pet = world_get_game_actor_by_aid(self->world, ((unsigned)g_session1.homunAid));
            if (pet != 0 && actor_homun_ai_running(pet))
                pet_target_order(pet, game_actor_set_request2, PET_ORDER_ATTACK, 0, 0, actor->aid, 0);
        } else if (g_session1.homun_command[5] != 0 && g_session1.homun_command[0] == actor->aid) {
            pet = world_get_game_actor_by_aid(self->world, ((unsigned)g_session1.homunAid));
            if (pet != 0 && actor_homun_ai_running(pet))
                pet_target_order(pet, game_actor_set_request, PET_ORDER_ATTACK, 0, 0, g_session1.homun_command[0], 0);
            g_session1.homun_command[5] = 0;
        } else {
            mode_set_cursor_action(&self->base, CURSOR_SKILL);
            g_session1.homun_command[0] = actor->aid;
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
        if (pet == 0 || !actor_homun_ai_running(pet))
            return;
        pet_target_order(pet, game_actor_set_request, PET_ORDER_SKILL_AT_ACTOR, self->skill_use.level, self->skill_use.skill_id,
                         actor->aid, 0);
        return;
    }

    may_order = (attack != 0 || self->world->is_siege != 0) && actor->state != ACTOR_STATE_DEAD && !aiming;
    if (may_order && g_session1.mercPresent != 0 && g_mouse.button[MOUSE_LEFT] == BUTTON_PRESSED && key_down(VK_MENU)) {
        if (key_down(VK_SHIFT)) {
            pet = world_get_game_actor_by_aid(self->world, ((unsigned)g_session1.mercAid));
            if (pet != 0 && actor_merc_ai_running(pet))
                pet_target_order(pet, game_actor_set_request2, PET_ORDER_ATTACK, 0, 0, actor->aid, 1);
        } else if (g_session1.merc_command[6] != 0 && g_session1.merc_command[0] == actor->aid) {
            pet = world_get_game_actor_by_aid(self->world, ((unsigned)g_session1.mercAid));
            if (pet != 0 && actor_merc_ai_running(pet))
                pet_target_order(pet, game_actor_set_request, PET_ORDER_ATTACK, 0, 0, g_session1.merc_command[0], 1);
            g_session1.merc_command[6] = 0;
        } else {
            mode_set_cursor_action(&self->base, CURSOR_SKILL);
            g_session1.merc_command[0] = actor->aid;
            g_session1.merc_command[1] = 0;
            g_session1.merc_command[2] = 0;
            g_session1.merc_command[3] = 0;
            g_session1.merc_command[6] = 1;
        }
        g_session1.merc_command[5] = 1;
    }
    if (attack != 0 && g_session1.mercPresent != 0) {
        if (g_mouse.button[MOUSE_LEFT] != BUTTON_PRESSED)
            goto right_button;
        if (session_is_mercenary_skill(&g_session1, self->skill_use.skill_id) != 0) {
            pet = world_get_game_actor_by_aid(self->world, ((unsigned)g_session1.mercAid));
            if (pet == 0 || !actor_merc_ai_running(pet))
                return;
            pet_target_order(pet, game_actor_set_request, PET_ORDER_SKILL_AT_ACTOR, self->skill_use.level, self->skill_use.skill_id,
                             actor->aid, 1);
            return;
        }
    }

    if (g_mouse.button[MOUSE_LEFT] == BUTTON_PRESSED && attack != 0 && !key_down(VK_MENU)) {
        mode_send_msg(self, MODE_MSG_SET_ATTACK_IN_PROGRESS, 1, 0, 0, 0);
        self->attack_target_aid = actor->aid;
        self->held_walk_allowed = 0;
        if (!session_is_siege_mode(&g_session1) && (key_down(VK_CONTROL) || g_session1.noCtrl == 1)) {
            game_mode_proceed_attack(self, actor->aid, 0);
            if (!aiming)
                self->attack_repeat = 1;
        } else {
            game_mode_proceed_attack(self, actor->aid, 1);
            self->attack_repeat = 0;
        }
    }

right_button:
    if (g_mouse.button[MOUSE_RIGHT] == BUTTON_PRESSED && shift != 0) {
        if (game_mode_too_heavy_to_attack())
            return;
        mode_send_msg(self, MODE_MSG_SET_ATTACK_IN_PROGRESS, 1, 0, 0, 0);
        g_followActorGid = actor_get_char_id(actor);
        self->held_walk_allowed = 0;
    }
    if (self->last_mouse_x == g_mouse.x && self->last_mouse_y == g_mouse.y) {
        game_mode_process_monster_pick(self, self->pointed_player_aid);
        return;
    }
    self->pointed_player_aid = 0;
    game_mode_process_monster_pick(self, 0);
}

void game_mode_click_actor(struct GameMode *self, unsigned aid)
{
    struct GameActor *actor;
    int attack, shift, aiming, allowed;

    if (self->player_dead != 0)
        return;
    shift = key_down(VK_SHIFT);
    attack = shift;
    aiming = self->skill_use.type == 2 || self->skill_use.type == 4;
    allowed = aid != (unsigned)g_session1.ownAid || self->skill_use.type == 4;
    if (aiming) {
        unsigned char good = (unsigned char)skill_info_is_good_skill_for_actor(self->skill_use.skill_id);
        unsigned char harm = skill_info_may_harm(self->skill_use.skill_id);

        if (good == 1)
            attack = 1;
        if ((g_session1.noShift != 0 || shift != 0) && harm == 1)
            attack = 1;
        if (self->world->is_pvp != 0 || self->world->is_event_pvp != 0) {
            if (session_is_party_member_aid(&g_session1, aid, 1) == 0)
                attack = 1;
        } else if (self->world->is_siege != 0) {
            actor = world_get_game_actor_by_aid(self->world, aid);
            if (actor != 0 && enemy_guild(actor))
                attack = 1;
        }
    }
    if (!allowed || g_mouse.button[MOUSE_LEFT] != BUTTON_PRESSED || attack == 0 || key_down(VK_MENU))
        return;
    mode_send_msg(self, MODE_MSG_SET_ATTACK_IN_PROGRESS, 1, 0, 0, 0);
    self->held_walk_allowed = 0;
    if (!session_is_siege_mode(&g_session1) && (key_down(VK_CONTROL) || g_session1.noCtrl == 1)) {
        game_mode_proceed_attack(self, aid, 0);
        self->attack_repeat = 1;
    } else {
        game_mode_proceed_attack(self, aid, 1);
        self->attack_repeat = 0;
    }
}
