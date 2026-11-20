#include "contents/adventurer_agency.h"
#include "world/mode_actions.h"
#include "game/net_overview.h"
#include "game/cells.h"
#include "game/connection.h"
#include "game/net_clock.h"
#include "game/mode_misc.h"
#include "game/mode_util.h"
#include "world/actor_lookup.h"
#include "ui/window_types.h"
#include "world/view_api.h"
#include "world/lifecycle.h"
#include "ui/window_access.h"
#include "contents/player_names.h"
#include "contents/frame_timer.h"
#include "render/device.h"
#include "render/renderer.h"
#include "contents/schedule.h"
#include "contents/guild_emblem_api.h"
#include "world/ground_api.h"
#include "fileio/resource_api.h"
#include "contents/sound.h"
#include "world/pick_node.h"
#include "actors/weather.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "contents/mouse.h"
#include "game/mode_callbacks.h"
#include "platform/win_types.h"
#include "platform/thread.h"
#include <string.h>
#include "game/gamemode.h"
#include "game/client.h"
#include "game/world_pick.h"
#include "render/device.h"
#include "core/app.h"
#include "effects/effect.h"
#include "contents/guild_mgr.h"
#include "effects/effect_mgr.h"
#include "game/barter_packets.h"
#include "game/drag.h"
#include "game/minimap.h"
#include "game/screen_counters.h"
#include "session/session_names.h"
#include "ui/wndmgr/wndmgr.h"
#include "world/world.h"
#include "platform/clock.h"
#include "diag/perf.h"

int g_followActorGid;
DWORD g_firstFrameTick;

int game_mode_on_run(struct Mode *mode)
{
    struct GameMode *self = (struct GameMode *)mode;
    int next;

    while (self->base.loop_cond != 0 && g_quitRequested == 0) {
        if (g_windowMgr.modal_wnd == 0) {
            if (g_windowMgr.capture_root != 0) {
                if (g_windowMgr.close_capture_root != 0)
                    ui_window_mgr_post_quit2(&g_windowMgr, g_windowMgr.capture_root);
                g_windowMgr.capture_root = 0;
                g_windowMgr.close_capture_root = 0;
            }
            next = self->base.next_sub_mode;
            if (next != -1) {
                self->base.sub_mode = next;
                self->base.sub_mode_count = 0;
                self->base.next_sub_mode = -1;
                game_mode_on_change_state(mode, next);
            }
        }

        game_mode_on_update(mode);
        platform_thread_yield();
    }
    mode_run_fade_out(&self->base, 0);

    return 1;
}

struct Once g_firstFrameTickGuard;
extern DWORD g_firstFrameTick;

enum { WID_RELEASE_CLOSE = 0x12, PLAYER_STATE_SIT = 6 };

#pragma pack(push, 1)
struct StatusChange {
    unsigned short id;
    unsigned short status;
    unsigned char amount;
};
#pragma pack(pop)
enum { CZ_STATUS_CHANGE = 0xbb };
static const unsigned short raise_status[6] = { 0xd, 0xe, 0x12, 0x10, 0xf, 0x11 };

static void follow_pointed_actor(struct GameMode *self)
{
    struct GameActor *actor;
    struct Player *player;
    int x, y, cx, cy;

    if (g_followActorGid == 0)
        return;
    if (g_mouse.button[MOUSE_LEFT] != 0) {
        g_followActorGid = 0;
        g_followLostSinceTick = 0;
        g_followLastMoveTick = 0;
        return;
    }
    actor = world_find_pc_by_char_id(self->world, g_followActorGid);
    if (actor == 0) {
        if (g_followLostSinceTick == 0)
            g_followLostSinceTick = platform_ticks_ms();
        if (platform_ticks_ms() - g_followLostSinceTick > 10000) {
            g_followActorGid = 0;
            g_followLastMoveTick = 0;
            g_followLostSinceTick = 0;
        }
        return;
    }
    g_followLostSinceTick = 0;
    if (platform_ticks_ms() - g_followLastMoveTick <= 1000)
        return;
    g_followLastMoveTick = platform_ticks_ms();
    x = actor->cell_x;
    y = actor->cell_y;
    player = self->world->player;
    if (g_windowMgr.chat_room_wnd != 0 || player->state == PLAYER_STATE_SIT) {
        render_object_send(self->world->player, NULL, ACTOR_MSG_WALK, (long long)x, (long long)y, 0, 0);
        return;
    }
    if (!game_mode_is_valid_cell(self, x, y)) {
        cx = x;
        cy = y;
        if (!game_mode_get_closest_valid_cell(self, &cx, &cy))
            return;
        x = cx;
        y = cy;
    }
    if (g_windowMgr.menu_wnd != 0)
        ui_window_mgr_post_quit1(&g_windowMgr, WID_RELEASE_CLOSE);
    render_object_send(self->world->player, NULL, ACTOR_MSG_WALK_CHECKED, (long long)x, (long long)y, 0, 0);
}

static void play_loop_sounds(struct GameMode *self)
{
    struct LoopSound *sound;

    for (sound = self->loop_sounds.first; sound != (struct LoopSound *)self->loop_sounds.last; sound++) {
        platform_ticks_ms();
        if (platform_ticks_ms() < sound->next_tick)
            continue;
        if (sound->npc_aid == 0 || world_get_game_actor_by_aid(self->world, sound->npc_aid))
            sound_play_wave(string_cstr(&sound->name), 1.0f);
        sound->next_tick = platform_ticks_ms() + sound->interval;
    }
}

static void restart_bgm(struct GameMode *self)
{
    struct String rsw;
    struct String *map;
    const char *name;

    if (!sound_stream_done() || g_sound_settings.bgm_overridden == 0)
        return;
    g_sound_settings.bgm_overridden = 0;
    map = game_mode_get_map_name(self, &rsw);
    name = session_get_mp3_name(&g_session1, string_cstr(map));
    sound_play_bgm(name);
    string_free(&rsw);
}

/* custom: the effects' and the world's objects' steps, timed for the counters (diag/perf.h) */
static void process_effects(void)
{
    int64_t start = platform_stopwatch_ns();
    effect_mgr_process(g_effectMgr);
    g_perf.effectsNs += platform_stopwatch_ns() - start;
}

static void process_actors(struct World *world)
{
    int64_t start = platform_stopwatch_ns();
    world_process_actors(world);
    g_perf.actorsNs += platform_stopwatch_ns() - start;
}

static void run_logic_steps(struct GameMode *self)
{
    int steps, n;

    if (g_isSkipFrame > 0) {
        game_mode_send_keep_alive(self, 0);
        renderer_run_every15s();
    }
    game_mode_pin_render_clock(self);
    game_mode_own_body_frame(self);
    game_mode_play_tracks(self);
    steps = frame_timer_step_count();
    g_isSkipFrame = steps;
    game_mode_process_input(self);
    res_mgr_call_residents(g_resMgr());
    if (steps > 0) {
        for (n = steps; n != 0; n--) {
            process_effects();
            process_actors(self->world);
            c3d_ground15_update_water(self->world->ground);
            view_on_calc_view_info(self->view);
        }
    } else if (steps == 0) {
        process_actors(self->world);
        view_on_calc_view_info(self->view);
    }
}

static void render_frame(struct GameMode *self)
{
    int64_t start;
    renderer_clear_scene(g_renderer);
    actor_pick_node_clear(&g_actorPickNode);
    view_on_render(self->view);
    game_mode_draw_minimap(self);
    /* custom: timed for the counters (diag/perf.h) */
    start = platform_stopwatch_ns();
    ui_window_mgr_render(&g_windowMgr);
    ui_window_mgr_render_windows(&g_windowMgr);
    g_perf.uiNs += platform_stopwatch_ns() - start;
    if (self->world->is_pvp != 0 || g_session1.map_rule_flags[3] != 0) {
        game_mode_draw_pk_ranking(self);
        game_mode_draw_ranking_time_attack(self);
    }
    game_mode_draw_shown_timer(self);
    game_mode_draw_dragged(self);
    mode_draw_mouse_cursor(&self->base);
    game_mode_draw_skill_level_at_cursor(self);
    mode_process_fade_in(&self->base);
    game_mode_draw_net_overview(self);
    if (renderer_draw_scene(g_renderer))
        renderer_draw_frame_extras(g_renderer);
    renderer_flip(g_renderer, 1);
}

static void send_stat_raises(struct GameMode *self)
{
    struct StatusChange packet;
    int i;

    for (i = 0; i < 6; i++) {
        if (self->stat_raises[i] == 0)
            continue;
        packet.id = CZ_STATUS_CHANGE;
        packet.status = raise_status[i];
        packet.amount = 1;
        if (self->player_dead != 0)
            continue;
        send_packet(CZ_STATUS_CHANGE, &packet);
        self->stat_raises[i]--;
    }
}

void game_mode_on_update(struct Mode *mode)
{
    struct GameMode *self = (struct GameMode *)mode;
    int64_t start;
    if (once_begin(&g_firstFrameTickGuard)) {
        g_firstFrameTick = platform_ticks_ms();
        once_complete(&g_firstFrameTickGuard);
    }
    adventurer_agency_tick(g_adventurerAgency);
    check_system_message();
    /* custom: timed for the counters (diag/perf.h) */
    start = platform_stopwatch_ns();
    game_mode_poll_network_msg(self);
    g_perf.packetsNs += platform_stopwatch_ns() - start;
    if (self->base.loop_cond != 0) {
        time_scheduler_tick(g_timeScheduler);
        player_names_update();
        name_card_holder_update(&self->actor_names);
        guild_mgr_update_requests(g_guildMgr);
        game_mode_process_input(self);
        follow_pointed_actor(self);
        play_loop_sounds(self);
        restart_bgm(self);
        run_logic_steps(self);
    }
    if (g_activatedNow != 0) {
        g_activatedNow = 0;
        ui_window_mgr_invalidate_all(&g_windowMgr);
    }
    weather_process(&g_weather);
    render_frame(self);
    mode_screen_shot(&self->base);
    send_stat_raises(self);
}
