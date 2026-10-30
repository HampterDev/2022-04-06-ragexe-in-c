#include "game/cells.h"
#include "actor/actor_ops.h"
#include "world/actor_lookup.h"
#include "world/ground_api.h"
#include "world/view_types.h"
#include "world/hover_cell.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "contents/mouse.h"
#include "ui/window_access.h"
#include "ui/shops/merchant_shop_make.h"
#include <string.h>
#include <windows.h>
#include <mmsystem.h>
#include "game/world_pick.h"
#include "ui/chat/macro_reporter.h"
#include "actor/job.h"
#include "game/name_balloon.h"
#include "session/session_jobs.h"
#include "session/session_skills.h"
#include "session/session_state.h"
#include "ui/chat/chat.h"
#include "ui/wndmgr/wndmgr.h"

int g_hoverCellX = 0;

int g_hoverCellY = 0;

int g_hoverCellHidden = 1;
unsigned g_hoverCellColor = 0xaa55ffffu;

struct ViewMatrix {
    float m[12];
};

static void draw_cell(struct GameMode *self, int x, int y, unsigned color)
{
    struct ViewMatrix matrix;
    struct C3dGround15 *ground;

    memcpy(&matrix, self->view->viewMatrix, sizeof matrix);
    ground = self->world->ground;
    c3d_ground15_render_attr_tile(ground, matrix.m, x, y, color);
}

enum { MSG_TOO_HEAVY_TO_ATTACK = 0x133 };
enum { WINDOWMGR_MSG_CLEAR_CHAT = 3, WINDOWMGR_MSG_CHAT_LINE = 1 };

char game_mode_too_heavy_to_attack(void)
{
    int max = g_session1.maxWeight > 1 ? g_session1.maxWeight : 1;

    if (g_session1.weight * 100 / max < 90)
        return 0;
    ui_window_mgr_send_msg_(&g_windowMgr, WINDOWMGR_MSG_CLEAR_CHAT, 0, 0, 0, 0);
    ui_window_mgr_send_msg_(&g_windowMgr, WINDOWMGR_MSG_CHAT_LINE, (int)msg_str(MSG_TOO_HEAVY_TO_ATTACK), 0xff, 0,
                          0);
    return 1;
}

void game_mode_proceed_attack(struct GameMode *self, unsigned aid, int once)
{
    struct Player *player;

    if (self->skill_use.skill_id != CURSOR_CAPTURE && game_mode_too_heavy_to_attack()
        && self->skill_use.skill_id != CURSOR_COLLECTION)
        return;
    switch (self->skill_use.type) {
    case 0:
        render_object_send(self->world->player, NULL, ACTOR_MSG_ATTACK, aid, (long long)once, 0, 0);
        break;
    case 2:
    case 4:
        player = self->world->player;
        render_object_send(player, NULL, ACTOR_MSG_USE_SKILL, aid, (long long)self->skill_use.skill_id, 0, 0);
        player = self->world->player;
        render_object_send(player, NULL, ACTOR_MSG_SKILL_LEVEL, (long long)self->skill_use.level, 0, 0, 0);
        break;
    }
}

enum { WND_MACRO_REPORTER = 0x11e, WND_COLLECTION_SLOT_MACHINE = 0xc4 };
enum { PICK_PET = 3, PICK_COMPANION = 4 };

int game_mode_process_actor_pick(struct GameMode *self, int verdict, struct PickInfo *pick)
{
    struct UIWindow *wnd;
    struct GameActor *actor;

    if (g_mouse.button[MOUSE_LEFT] == BUTTON_PRESSED && pick != 0) {
        wnd = ui_window_find(WND_MACRO_REPORTER);
        if (wnd != 0 && ui_macro_reporter_picking(wnd) && ui_macro_reporter_area(wnd) == 0) {
            ui_macro_reporter_wnd_add_player((struct UIMacroReporterWnd *)wnd, pick->aid);
            return -1;
        }
    }
    if (ui_window_find(WND_COLLECTION_SLOT_MACHINE) != 0 || self->skill_use.type == 1)
        return verdict;

    if (self->base.attack_in_progress != 0) {
        mode_set_cursor_action(&self->base, CURSOR_ATTACK);
        if (self->attack_repeat != 0) {
            if (g_mouse.button[MOUSE_LEFT] == BUTTON_PRESSED) {
                mode_send_msg(self, MODE_MSG_SET_ATTACK_IN_PROGRESS, 0, 0, 0, 0);
            } else {
                mode_set_cursor_action(&self->base, CURSOR_ARROW);
                if (pick == 0)
                    return verdict;
                verdict = 0;
            }
        } else {
            if (g_mouse.button[MOUSE_LEFT] == BUTTON_HELD)
                return 1;
            if (pick == 0 && self->balloon_follow_aid == 0 && self->pointed_player_aid == 0)
                self->base.attack_in_progress = 0;
        }
    } else if (pick == 0) {
        if (self->attack_repeat != 0 || g_mouse.button[MOUSE_LEFT] == BUTTON_PRESSED)
            return verdict;
        if (self->last_mouse_x == g_mouse.x && self->last_mouse_y == g_mouse.y)
            return verdict;
        self->pointed_player_aid = 0;
        self->balloon_follow_aid = 0;
        return verdict;
    }

    if (self->attack_repeat == 0) {
        if (self->balloon_follow_aid != 0) {
            if (pick != 0) {
                if (pick->kind == PICK_PET) {
                    mode_set_cursor_action(&self->base, CURSOR_ARROW);
                    return 0;
                }
                if (pick->kind == PICK_COMPANION) {
                    mode_set_cursor_action(&self->base, CURSOR_ARROW);
                    game_mode_process_monster_pick(self, pick->aid);
                    return game_mode_process_obj_click(self, pick, verdict);
                }
            }
            actor = world_get_game_actor_by_aid(self->world, self->balloon_follow_aid);
            if (actor == 0) {
                self->balloon_follow_aid = 0;
                return verdict;
            }
            game_mode_process_target_actor(self, &verdict, actor);
            return verdict;
        }
        if (self->pointed_player_aid != 0) {
            actor = world_get_game_actor_by_aid(self->world, self->pointed_player_aid);
            if (actor == 0) {
                self->pointed_player_aid = 0;
                return verdict;
            }
            if (self->attack_repeat != 0 && actor->state == 3)
                self->attack_repeat = 0;
            game_mode_process_picked_actor(self, &verdict, pick, actor);
            return verdict;
        }
    }
    if (pick != 0 && verdict == 0) {
        game_mode_process_monster_pick(self, pick->aid);
        verdict = game_mode_process_obj_click(self, pick, 0);
    }
    return verdict;
}

enum { JOB_WARP_PORTAL = 0x2d };

static int actor_on_screen(struct GameActor *actor)
{
    return actor->visible == 1 && actor->revealed == 1 && !is_effect_state_star2(actor_get_effect_state(actor));
}

void game_mode_point_at_target(struct GameMode *self)
{
    struct GameActor *actor;

    if (g_followActorGid != 0) {
        actor = (struct GameActor *)world_find_pc_by_char_id(self->world, g_followActorGid);
        if (actor == 0)
            return;
        if (actor->visible == 1 && actor->revealed == 1 && !is_effect_state_star2(actor_get_effect_state(actor)))
            game_mode_show_name_balloon2(self, actor);
    } else {
        if (self->base.attack_in_progress == 0) {
            mode_send_msg(self, MODE_MSG_HIDE_MOUSE_POINTER, 0, 0, 0, 0);
            return;
        }
        actor = world_get_game_actor_by_aid(self->world, self->attack_target_aid);
        if (actor == 0)
            return;
        if (actor->object_type == 0 && actor_on_screen(actor)) {
            self->name_balloon2_stale = 0;
            game_mode_show_name_balloon2(self, actor);
        }
    }
    if (game_actor_get_job(actor) != JOB_WARP_PORTAL && (actor->object_type != 0 || actor_on_screen(actor))) {
        mode_send_msg(self, MODE_MSG_POINT_MOUSE_AT, (int)actor->pos.x, (int)actor->pos.z, 0, 0);
        return;
    }
    mode_send_msg(self, MODE_MSG_HIDE_MOUSE_POINTER, 0, 0, 0, 0);
}

enum { MSG_PLACE_ASSISTANT_HERE = 0xb63, MSGBOX_YES = 0xb3 };

static void pet_order(struct GameActor *pet, PetOrderFn set_order, int kind, int x, int y, int level, int skill_id,
                      int is_mercenary)
{
    struct PetOrder order;

    order.kind = kind;
    order.x = x;
    order.y = y;
    order.level = level;
    order.skill_id = skill_id;
    order.target_aid = 0;
    pet_submit_order(pet, set_order, &order, is_mercenary);
}

enum { WID_RELEASE_CLOSE = 0x12, PLAYER_STATE_SIT = 6 };

int game_mode_process_ground_click(struct GameMode *self, int picked)
{
    struct UIWindow *wnd;
    struct UIMerchantShopMakeWnd *shop;
    struct GameActor *pet;
    struct Player *player;
    int x = -1, y = -1, i, j, radius, placing, due;

    wnd = ui_window_find(WND_MACRO_REPORTER);
    if (wnd != 0 && ui_macro_reporter_picking(wnd) && ui_macro_reporter_area(wnd) == 1) {
        game_mode_pick_ground_cell(self, &x, &y);
        radius = ui_macro_reporter_radius(wnd);
        for (i = -radius; i <= radius; i++)
            for (j = -radius; j <= radius; j++)
                draw_cell(self, x + i, y + j, 0x660000ff);
        if (g_mouse.button[MOUSE_RIGHT] == BUTTON_PRESSED) {
            macro_reporter_request_players_in_range((WORD)x, (WORD)y, (BYTE)radius);
            return -1;
        }
    }

    wnd = g_windowMgr.merchant_shop_make_wnd;
    if (wnd == 0 && g_windowMgr.buying_store_make_wnd != 0)
        wnd = g_windowMgr.buying_store_make_wnd;
    placing = 0;
    shop = (struct UIMerchantShopMakeWnd *)wnd;
    if (shop != NULL) {
        if (ui_merchant_shop_make_wnd_is_waiting(shop)) {
            placing = 1;
            x = -1;
            y = -1;
            game_mode_pick_ground_cell(self, &x, &y);
            draw_cell(self, x, y, 0x66ff0000);
            if (g_mouse.button[MOUSE_LEFT] == BUTTON_PRESSED) {
                ui_merchant_shop_make_wnd_set_waiting(shop, 0);
                ui_merchant_shop_make_wnd_set_chosen_cell(shop, x, y);
                if (ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_PLACE_ASSISTANT_HERE), 2, 1, 0, 0,
                                           "메시지", 0x118, 0x78, 0)
                    == MSGBOX_YES) {
                    ui_merchant_shop_make_wnd_request_assistant(shop);
                    return -1;
                }
                ui_merchant_shop_make_wnd_set_waiting(shop, 1);
            }
        } else if (ui_merchant_shop_make_wnd_get_chosen_cell(shop, &x, &y)) {
            draw_cell(self, x, y, 0x66ff0000);
        }
    }

    if (self->skill_use.type == 2 || self->skill_use.type == 3 || self->skill_use.type == 4)
        return picked;
    x = -1;
    y = -1;
    if (!game_mode_pick_ground_cell(self, &x, &y))
        return picked;

    if (g_session1.homun_ai_flags[0] != 0) {
        if (key_down(VK_MENU) && g_mouse.button[MOUSE_RIGHT] == BUTTON_PRESSED) {
            if (g_session1.homun_command[4] != 0) {
                g_session1.homun_command[4] = 0;
                return picked;
            }
            pet = world_get_game_actor_by_aid(self->world, ((unsigned)g_session1.homunAid));
            if (key_down(VK_SHIFT)) {
                if (pet != 0 && actor_homun_ai_running(pet))
                    pet_order(pet, game_actor_set_request2, 1, x, y, 0, 0, 0);
                return picked;
            }
            g_session1.homun_command[1] = x;
            g_session1.homun_command[2] = y;
            g_session1.homun_command[0] = 0;
            g_session1.homun_command[5] = 0;
            if (pet != 0 && actor_homun_ai_running(pet))
                pet_order(pet, game_actor_set_request, 1, x, y, 0, 0, 0);
            g_session1.homun_command[3] = 1;
            return picked;
        }
        if (g_session1.homun_ai_flags[0] != 0 && g_mouse.button[MOUSE_RIGHT] == BUTTON_PRESSED)
            session_clear_homun_commands(&g_session1);
    }

    if (g_session1.mercPresent != 0) {
        if (key_down(VK_MENU) && g_mouse.button[MOUSE_LEFT] == BUTTON_PRESSED) {
            if (g_session1.merc_command[5] != 0) {
                g_session1.merc_command[5] = 0;
                return picked;
            }
            pet = world_get_game_actor_by_aid(self->world, ((unsigned)g_session1.mercAid));
            if (key_down(VK_SHIFT)) {
                if (pet != 0 && actor_merc_ai_running(pet))
                    pet_order(pet, game_actor_set_request2, 1, x, y, 0, 0, 1);
                return picked;
            }
            g_session1.merc_command[1] = x;
            g_session1.merc_command[2] = y;
            g_session1.merc_command[0] = 0;
            g_session1.merc_command[6] = 0;
            if (pet != 0 && actor_merc_ai_running(pet))
                pet_order(pet, game_actor_set_request, 1, x, y, 0, 0, 1);
            g_session1.merc_command[3] = 1;
            return picked;
        }
        if (g_session1.mercPresent != 0 && g_mouse.button[MOUSE_LEFT] == BUTTON_PRESSED) {
            session_clear_merc_commands(&g_session1);
            if (self->skill_use.type == 1 && session_is_merc_or_elemental_skill(&g_session1, self->skill_use.skill_id) != 0) {
                pet = world_get_game_actor_by_aid(self->world, ((unsigned)g_session1.mercAid));
                if (pet != 0 && actor_merc_ai_running(pet) && (pet->state == 1 || pet->state == 0 || pet->state == 4)) {
                    pet_order(pet, game_actor_set_request, 8, x, y, self->skill_use.level, self->skill_use.skill_id, 1);
                    return 1;
                }
            }
        }
    }

    if (self->skill_use.type == 1 && g_mouse.button[MOUSE_LEFT] == BUTTON_PRESSED && (picked == 0 || picked == 2)) {
        picked = 1;
        if (session_is_homunculus_skill(&g_session1, self->skill_use.skill_id) == 1 && g_session1.homun_ai_flags[0] == 1) {
            pet = world_get_game_actor_by_aid(self->world, ((unsigned)g_session1.homunAid));
            if (pet != 0 && actor_homun_ai_running(pet) == 1 && pet->state >= 0
                && (pet->state <= 1 || pet->state == 4)) {
                pet_order(pet, game_actor_set_request, 8, x, y, self->skill_use.level, self->skill_use.skill_id, 0);
                return 1;
            }
        }
        if (self->world->player != 0) {
            render_object_send(self->world->player, NULL, ACTOR_MSG_USE_GROUND_SKILL, (long long)self->skill_use.skill_id, (long long)x, (long long)y, 0);
            render_object_send(self->world->player, NULL, ACTOR_MSG_SKILL_LEVEL, (long long)self->skill_use.level, 0, 0, 0);
        }
    }

    if (key_down(VK_CONTROL) || self->server_move_pending != 0 || (picked != 0 && picked != 2)
        || self->drag.source != 0)
        return picked;
    if (g_session1.showCellInfo != 0) {
        const struct C3dAttr *attr;
        const struct AttrCell *cell = 0;
        int show = 0;
        unsigned color;

        g_hoverCellX = x;
        g_hoverCellY = y;
        g_hoverCellHidden = 0;
        attr = self->world->attr;
        cell = c3d_attr_get_cell(attr, x, y);
        if (cell == 0 || cell->type == ATTR_CELL_BLOCKED) {
            color = 0xfeff0000;
            show = 1;
        } else if (cell->type == 5) {
            color = 0xfeffff00;
            show = 1;
        } else {
            color = 0x6600ffaa;
        }
        g_hoverCellColor = color;
        if (placing == 1)
            show = 1;
        g_hoverCellHidden = show;
    }

    due = timeGetTime() > self->click_repeat_time + 600;
    if (self->world->player->pending == 0 && g_mouse.button[MOUSE_LEFT] == BUTTON_HELD) {
        if (!due || self->held_walk_allowed == 0)
            return picked;
    } else {
        if (g_mouse.button[MOUSE_LEFT] != BUTTON_PRESSED)
            return picked;
        if (g_mouse.button[MOUSE_RIGHT] == BUTTON_HELD)
            return picked;
    }

    ui_window_mgr_navigation_help(&g_windowMgr);
    self->click_repeat_time = timeGetTime();
    if (key_down(VK_SHIFT) || g_windowMgr.chat_room_wnd != 0 || self->world->player->state == PLAYER_STATE_SIT) {
        render_object_send(self->world->player, NULL, ACTOR_MSG_WALK, (long long)x, (long long)y, 0, 0);
        return picked;
    }
    if (!game_mode_is_valid_cell(self, x, y) && !game_mode_get_closest_valid_cell(self, &x, &y))
        return picked;
    if (g_windowMgr.menu_wnd != 0)
        ui_window_mgr_post_quit1(&g_windowMgr, WID_RELEASE_CLOSE);
    player = self->world->player;
    if (player != 0)
        render_object_send(player, NULL, ACTOR_MSG_WALK_CHECKED, (long long)x, (long long)y, 0, 0);
    return picked;
}
