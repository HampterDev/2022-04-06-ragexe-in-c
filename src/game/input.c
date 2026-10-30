#include "world/mode_actions.h"
#include "game/gamemode.h"
#include "world/tooltip.h"
#include "world/model_types.h"
#include "render/projection.h"
#include "replay/replay.h"
#include "world/ground_api.h"
#include "resources/attr_queries.h"
#include "world/pick_node.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "contents/mouse.h"
#include <windows.h>
#include <mmsystem.h>
#include "game/input.h"
#include "game/cells.h"
#include "game/mouse_camera.h"
#include "game/client.h"
#include "game/camera.h"
#include "game/replay.h"
#include "game/tips.h"
#include "actor/actor_common.h"
#include "game/mode_msg.h"
#include "game/name_balloon.h"
#include "game/world_pick.h"
#include "session/session_jobs.h"
#include "session/session_windows.h"
#include "ui/window_mgr.h"
#include "ui/wndmgr/wndmgr.h"

int g_shiftDown;

enum { WID_RELEASE_CLOSE = 0x12 };

enum { SKILL_AM_POTIONPITCHER = 0xe7 };


enum { CURSOR_NORMAL = 0, CURSOR_SKILL_TARGET = 0xa, CURSOR_NO_WALK = 0xd };

enum { ACTORMSG_RELEASE = 0xf };

#include "world/ray_picker.h"
#include "ui/character/popup_menu.h"

#define NO_HIT 999999.0f

__declspec(noinline) char game_mode_pick_ground_cell(struct GameMode *self, int *cell_x, int *cell_y)
{
    struct RayPicker blocks;
    struct Ray3 ray;
    Vec3 eye;
    size_t index;
    float best = NO_HIT, distance;
    char hit;
    int x, y;

    ray_picker_construct(&blocks);
    eye = (Vec3){self->view->from[0], self->view->from[1], self->view->from[2]};
    renderer_cursor_ray(g_renderer, g_mouse.x, g_mouse.y, &eye, self->view->invViewMatrix,
                        &ray);
    ray_picker_pick_ray(&blocks, &ray, &self->world->root);
    for (index = 0; index < blocks.count; ++index) {
        struct SceneGraphNode *block = blocks.hits[index];
        struct C3dAttr *ground = block->attr;
        for (y = block->attrCell[1]; y <= block->attrCell[3]; y++) {
            for (x = block->attrCell[0]; x <= block->attrCell[2]; x++) {
                distance = ground_ray_distance(ground, &ray, x, y);
                if (best > distance) {
                    best = distance;
                    *cell_x = x;
                    *cell_y = y;
                }
            }
        }
    }
    hit = NO_HIT > best;
    return hit;
}

void game_mode_cast_player_ray(struct GameMode *self)
{
    struct RayPicker blocks;
    struct Ray3 to_player;
    struct Ray3 ray;
    Vec3 eye;
    size_t index;
    struct SceneGraphNode *block;
    struct GameActor *player;
    size_t object_index;
    int sx, sy, x, y;

    if (self->world == 0 || self->world->player == 0 || g_renderer == 0)
        return;
    player = (struct GameActor *)self->world->player;
    sx = player->screen_x;
    sy = (int)((float)player->screen_y - g_renderer->scaleY * 23.0f);
    ray_picker_construct(&blocks);
    eye = (Vec3){self->view->from[0], self->view->from[1], self->view->from[2]};
    renderer_cursor_ray(g_renderer, sx, sy, &eye, self->view->invViewMatrix, &ray);
    to_player = ray;
    to_player.length = self->world->player->pos.y + self->view->distance - 16.0f;
    ray_picker_pick_ray(&blocks, &ray, &self->world->root);
    for (index = 0; index < blocks.count; ++index) {
        block = blocks.hits[index];
        for (object_index = 0; object_index < block->objects.count; ++object_index) {
            struct C3dActor *object = block->objects.data[object_index];
            c3d_actor_pick(object, &to_player);
        }
    }
    if (self->ray_tests_cells != 0) {
        for (index = 0; index < blocks.count; ++index) {
            block = blocks.hits[index];
            for (y = block->groundCell[1]; y <= block->groundCell[3]; y++)
                for (x = block->groundCell[0]; x <= block->groundCell[2]; x++)
                    c3d_ground15_check_alpha(block->ground, &ray, x, y,
                                                                       to_player.length);
        }
    }
}

static int pick_offsets_cursor(const struct GameMode *self, const struct PickInfo *pick)
{
    int offset = 0;

    if (self->skill_use.skill_id == SKILL_AM_POTIONPITCHER) {
        if (!session_is_homun_or_merc_job(&g_session1, pick->job))
            return 0;
        return pick->kind == 4;
    }
    if (g_session1.monsterSnapInEffect != 0 && job_is_monster(pick->job))
        offset = 1;
    else if (g_session1.itemSnap != 0 && pick->kind == 1)
        offset = 1;
    if ((unsigned)(pick->kind - 3) <= 1)
        return 0;
    return offset;
}

void game_mode_process_input(struct GameMode *self)
{
    struct PickInfo *pick;
    int verdict, taken, cell_x, cell_y, cursor;

    mouse_read_state(&g_mouse);
    verdict = ui_window_mgr_process_input(&g_windowMgr);
    ui_window_mgr_on_process(&g_windowMgr);
    game_mode_update_balloons(self);
    mode_process_help_balloon(&self->base);
    game_mode_process_right_button(self);
    game_mode_process_wheel(self, verdict);
    mode_nothing(&self->base);
    if (timeGetTime() > self->ui_tick_time + 1000) {
        ui_window_mgr_tick(&g_windowMgr);
        self->ui_tick_time = timeGetTime();
    }

    if (verdict == 2)
        verdict = 0;
    if (self->skill_use.type != 0) {
        if (g_session1.wheelConsumed == 0) {
            if (g_mouse.wheel > 0)
                mode_send_msg(self, MODE_MSG_SKILL_AIM_LEVEL_UP, 1, 0, 0, 0);
            else if (g_mouse.wheel < 0)
                mode_send_msg(self, MODE_MSG_SKILL_AIM_LEVEL_DOWN, 1, 0, 0, 0);
        }
        g_session1.wheelConsumed = 0;
    }

    if (g_mouse.button[MOUSE_LEFT] == BUTTON_RELEASED) {
        struct Player *player = self->world->player;
        if (player != 0)
            render_object_send(player, NULL, ACTORMSG_RELEASE, 0, 0, 0, 0);
        mode_send_msg(self, MODE_MSG_DROP_DRAGGED, g_mouse.x, g_mouse.y, 0, 0);
        if (g_windowMgr.menu_wnd != 0) {
            if (!ui_menu_take_hold(g_windowMgr.menu_wnd))
                ui_window_mgr_post_quit1(&g_windowMgr, WID_RELEASE_CLOSE);
        }
        self->held_walk_allowed = 1;
    }

    g_shiftDown = (unsigned char)((unsigned)GetAsyncKeyState(VK_SHIFT) >> 8);
    pick = actor_pick_node_find_pick_info(&g_actorPickNode, (float)g_mouse.x, (float)g_mouse.y);
    taken = g_windowMgr.exchange_wnd != NULL;
    if (taken == 0)
        game_mode_open_actor_popup_menu(self, pick, verdict);

    self->base.cursor_offset_x = 0;
    self->base.cursor_offset_y = 0;
    if (verdict == 0) {
        session_refresh_mouse_snap_state(&g_session1,
                                         self->skill_use.type == 2 || self->skill_use.type == 4);
        if (pick != 0 && pick_offsets_cursor(self, pick)) {
            self->base.cursor_offset_x = (pick->left - pick->right) * 0.5f + pick->right - (float)g_mouse.x;
            self->base.cursor_offset_y = (pick->top - pick->bottom) * 0.5f + pick->bottom - (float)g_mouse.y;
        }
    }

    game_mode_cast_player_ray(self);
    if (self->player_dead == 0 && (self->npc_dialog_open == 0 || self->world->player->recharge_gage_wnd != 0)
        && taken == 0 && g_windowMgr.item_store_wnd == 0) {
        verdict = game_mode_process_ground_click(self, game_mode_process_actor_pick(self, verdict, pick));
        game_mode_point_at_target(self);
        if (verdict != 1 && g_mouse.button[MOUSE_LEFT] == BUTTON_PRESSED) {
            ui_window_mgr_set_focus_edit(&g_windowMgr, 0);
        }
    } else if (pick != 0 && verdict == 0) {
        game_mode_process_monster_pick(self, pick->aid);
    }

    if (g_mouse.button[MOUSE_LEFT] == BUTTON_PRESSED) {
        mode_send_msg(self, MODE_MSG_CANCEL_SKILL_AIM, 0, 0, 0, 0);
        if (timeGetTime() > self->server_move_time + 5000)
            self->server_move_pending = 0;
        self->click_repeat_time = timeGetTime();
    }

    if (replay_recorder_instance()->state != REPLAY_STATE_STOPPED && g_session1.showStateInfo != 0)
        game_mode_update_state_icon_tip(self, g_mouse.x, g_mouse.y);

    if (verdict == 0) {
        if (self->skill_use.type != 0) {
            mode_set_cursor_action(&self->base, CURSOR_SKILL_TARGET);
        } else if (g_mouse.button[MOUSE_RIGHT] != BUTTON_HELD) {
            cell_x = -1;
            cell_y = -1;
            game_mode_pick_ground_cell(self, &cell_x, &cell_y);
            cursor = game_mode_is_cell_blocked(self, cell_x, cell_y) ? CURSOR_NO_WALK : CURSOR_NORMAL;
            mode_set_cursor_action(&self->base, cursor);
        }
    }

    self->last_mouse_x = g_mouse.x;
    self->last_mouse_y = g_mouse.y;
}
