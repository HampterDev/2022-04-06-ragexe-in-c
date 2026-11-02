#include "game/actor_packets.h"
#include "game/ground_item_packets.h"
#include "game/display_settings.h"
#include "world/view_api.h"
#include "world/lifecycle.h"
#include "contents/player_names.h"
#include "std/string.h"
#include "contents/frame_timer.h"
#include "render/renderer.h"
#include "game/schedules.h"
#include "session/option_pairs.h"
#include "game/talk_type.h"
#include "session/character_name.h"
#include "contents/quest.h"
#include "navi/navi_manager.h"
#include "contents/sound.h"
#include "contents/schedule.h"
#include "actor/weather.h"
#include "game/client_state.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/memory.h"
#include "game/mode_callbacks.h"
#include "game/mode_msg.h"
#include "ui/window_access.h"
#include "platform/win_types.h"
#include <string.h>
#include <time.h>
#include "game/gamemode.h"
#include "game/client.h"
#include "game/ui_bridge.h"
#include "game/session_bridge.h"
#include "game/own_states.h"
#include "game/mode_util.h"
#include "game/world_pick.h"
#include "game/camera.h"
#include "game/packet_handlers.h"
#include "game/map_entry.h"
#include "render/render_api.h"
#include "core/msg_str.h"
#include "game/connection.h"
#include "session/session_items.h"
#include "session/session_names.h"
#include "session/session_state.h"
#include "session/session_windows.h"
#include "ui/window_mgr.h"
#include "ui/wndmgr/wndmgr.h"

enum { UIWM_LOADING = 4, WID_LOADING = 7 };

enum { CZ_REQ_SCHEDULER_CASHITEM = 0x8c9 };

enum { UIWM_CHAT_LINE = 1 };

static void store_own_names(struct GameMode *self)
{
    struct String name;
    const char *text;

    text = session_character_name(&g_session1);
    string_init(&name);
    string_assign_cstr(&name, text);
    name_card_holder_set_name(&self->actor_names, g_session1.ownAid, name);
    text = session_character_name(&g_session1);
    player_names_set(g_session1.ownGid, text_view_cstr(text));
}

static void play_map_bgm(struct GameMode *self)
{
    sound_play_bgm(session_get_mp3_name(&g_session1, self->map_name));
}

void game_mode_on_init(struct Mode *mode, const char *map)
{
    struct GameMode *self = (struct GameMode *)mode;
    struct String minimap;
    struct World *world;
    struct View *view;
    char line[0x100];
    short packet;
    size_t size;
    int short_wobble;

    self->chat_history_count = 0;
    store_own_names(self);
    self->boss_x = -1;
    self->boss_y = -1;
    self->boss_shown = 0;
    self->copy_name = 0;

    session_clear_transform_jobs(&g_session1, 1, 0, 0);

    world = mem_alloc(sizeof *world);
    world = world != 0 ? world_construct(world) : 0;
    self->world = world;
    world->mode = self;
    view = mem_alloc(sizeof *view);
    view = view != 0 ? view_construct(view) : 0;
    self->view = view;
    view->world = self->world;
    strcpy(self->map_name, map);

    string_init(&minimap);
    string_assign(&minimap, "유저인터페이스\\map\\", 0x13);
    string_append_cstr(&minimap, map);
    size = minimap.size - 4;
    string_resize(&minimap, size, 0);
    string_append_cstr(&minimap, ".bmp");
    strcpy(self->minimap_texture, string_cstr(&minimap));

    renderer_clear_frame(g_renderer, 1);
    g_session1.weight = 0;
    session_refresh_basic_info(&g_session1);
    session_invalidate_windows(&g_session1);
    g_session1.maxWeight = 0;
    session_refresh_basic_info(&g_session1);
    session_invalidate_windows(&g_session1);
    ui_window_mgr_set_backdrop(&g_windowMgr, 0);
    ui_window_mgr_close_all_but_chat(&g_windowMgr);
    ui_window_mgr_hide_chat_wnd(&g_windowMgr);
    ui_window_mgr_show_windows(&g_windowMgr, 0x43, 0);
    ui_window_open(WID_LOADING);
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_LOADING, 0, 0, 0, 0);
    ui_window_mgr_render(&g_windowMgr);
    ui_window_mgr_render_windows(&g_windowMgr);
    if ((char)renderer_draw_scene(g_renderer))
        renderer_flip(g_renderer, 1);

    view_reset(self->view);
    if (!game_mode_load_world(self)) {
        string_free(&minimap);
        return;
    }
    play_map_bgm(self);
    g_weather.ground_lighting = 0;
    game_mode_apply_fog(self, g_session1.fogOn);
    session_reset_mail_items(&g_session1);
    session_clear_mail_item_counts(&g_session1);
    game_mode_initialize(self);

    frame_timer_reset();
    self->attack_repeat = 0;
    self->timer_shown_since = 0;
    self->whispers_logged = 0;
    self->flood_count = 0;
    memset(self->sit_times, 0, sizeof self->sit_times);
    if (g_session1.autoSaveChat != 0 && session_is_admin_option(&g_session1, 7) != 0) {
        sprintf(line, "Now Auto chat Save System is ENABLED.");
        ui_window_mgr_send_msg_(&g_windowMgr, UIWM_CHAT_LINE, (intptr_t)line, 0xffff, 0, 0);
    }
    game_mode_add_schedules();
    if (session_cash_shop_goods_empty(&g_session1)) {
        packet = CZ_REQ_SCHEDULER_CASHITEM;
        send_packet(CZ_REQ_SCHEDULER_CASHITEM, &packet);
    }

    if (g_wobbleMode == 2) {
        short_wobble = g_wobbleFramesLeft <= 10000 && g_wobbleFramesLeft != 0;
        render_object_send(self->world->player, NULL, 0x6e, 0x22, short_wobble, 0, 0);
    }

    if (session_get_instant_var(&g_session1, 6) > 0) {
        session_set_pair(&g_session1, 6, 0);
        memset(line, 0, sizeof line);
        _snprintf_s(line, sizeof line, (size_t)-1, msg_str(0xdfa), g_windowMgr.show_shop_titles == 1 ? "ON" : "OFF");
        ui_window_mgr_send_msg_(&g_windowMgr, UIWM_CHAT_LINE, (intptr_t)line, 0xff, 0, 0);
    }
    if (session_get_instant_var(&g_session1, 0xb) == 1) {
        game_mode_reblock_whispers(self);
        session_set_pair(&g_session1, 0xb, 0);
    }
    string_free(&minimap);
}
