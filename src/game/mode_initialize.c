#include "contents/account_settings.h"
#include "game/connection.h"
#include "game/drag.h"
#include "core/map_sky.h"
#include "world/tooltip.h"
#include "world/view_types.h"
#include "world/pathfinder_state.h"
#include "navi/navi_manager.h"
#include "contents/sound.h"
#include "ui/misc/snap_mgr.h"
#include "world/pick_node.h"
#include "actor/weather.h"
#include "game/client_state.h"
#include "platform/client_options.h"
#include "contents/guild_info.h"
#include "platform/client_config.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "contents/mouse.h"
#include "core/memory.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "game/context.h"
#include "contents/connection_api.h"
#include <windows.h>
#include <mmsystem.h>
#include <winsock.h>
#include <string.h>
#include "game/gamemode.h"
#include "game/client.h"
#include "base.h"
#include "ui/window_types.h"
#include "platform/language.h"
#include "world/camera.h"
#include "core/msg_str.h"
#include "session/session_items.h"
#include "session/session_state.h"
#include "ui/window_mgr.h"
#include "ui/wndmgr/wndmgr.h"
#include "game/mode_initialize.h"
#include "actor/attr_state.h"
#include "game/mode_msg.h"
#include "platform/win_utf8.h"

int g_isStopByLogin = 0;

enum {
    MODE_MSG_MAP_NAME = 9,
    WND_MSG_MARK_MIRROR_RESET = 0x1de,
};

enum { UIWM_LOADED = 3, UIWM_CHAT_LINE = 1 };

enum { CZ_NOTIFY_ACTORINIT = 0x7d };

static void clear_lists(struct GameMode *self)
{
    name_card_holder_clear(&self->actor_names);
    tree_clear(&self->unit_cells, NULL);
    list_clear(&self->pickup_requests, NULL);
    self->popup_entries.last = self->popup_entries.first;
    tree_clear(&self->square_marks, NULL);
    tree_clear(&self->ping_marks, NULL);
    list_clear(&self->shown_unit_ids, NULL);
    self->own_double_aids.last = self->own_double_aids.first;
    tree_clear(&self->quest_marks, NULL);
}

static void make_windows(void)
{
    if (g_windowMgr.mini_map_wnd != 0)
        ui_window_send(g_windowMgr.mini_map_wnd, NULL, WND_MSG_MARK_MIRROR_RESET, 0, 0, 0, 0);
    snap_mgr_clear(&g_snapMgr);
    ui_window_mgr_close_all_but_chat(&g_windowMgr);
    ui_window_mgr_set_backdrop(&g_windowMgr, 0);
    ui_window_open(0);
    ui_window_mgr_show_windows(&g_windowMgr, 0x43, 1);
    ui_window_open(0xcc);
    if (g_session1.mdWndShown != 0)
        ui_window_open(0x89);
    ui_window_open(0x9f);
    if (g_session1.storeSearchOpenOnEnter != 0)
        ui_window_open(0xb5);
    if (g_session1.cameraInfoShown != 0)
        ui_window_open(0x121);
}

static void read_name_balloon_pos(struct GameMode *self)
{
    char path[0x200];
    HKEY key;
    DWORD type, size;

    strcpy(path, g_registryKeyPath);
    strcat(path, "\\NameBaloon");
    strcat(path, "\\");
    strcat(path, string_cstr(&g_session1.serverName));
    if (win_utf8_reg_open_key(HKEY_CURRENT_USER, path, 0, KEY_QUERY_VALUE, &key) == 0) {
        RegCloseKey(key);
        if (win_utf8_reg_open_key(HKEY_CURRENT_USER, path, 0, 0xf003f, &key) == 0) {
            size = 4;
            type = 4;
            win_utf8_reg_query_value(key, "POS", 0, &type, (BYTE *)&self->name_balloon_pos, &size);
            RegCloseKey(key);
        }
    }
    if (self->name_balloon_pos != 0 && self->name_balloon_pos != 1)
        self->name_balloon_pos = 0;
}

static void greet_after_login(struct GameMode *self)
{
    g_isStopByLogin = 0;
    guild_info_reset(&g_guildInfo);
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_LOADED, 0, 0, 0, 0);
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_CHAT_LINE, (int)msg_str(0x16), 0xffe6e6, 0, 0);
    if (!account_user_data_is_loaded(g_accountUserDataMgr))
        ui_window_mgr_send_msg_(&g_windowMgr, UIWM_CHAT_LINE, (int)"Local account settings could not be loaded. Using initial preferences; existing file preserved.", 0xff, 0, 0);
}

static void enter_map_server(void)
{
    short packet;

    packet = CZ_NOTIFY_ACTORINIT;
    rag_connection_send_packet(rag_connection_instance(), 2, &packet);
    connection_clear_held(rag_connection_transport(rag_connection_instance()));
    session_set_pair(&g_session1, 5, 1);
}

static void start_weather(struct GameMode *self)
{
    struct String map;

    weather_forget_effect(&g_weather);
    string_init(&map);
    string_assign_cstr(&map, self->map_name);
    weather_script_process(&g_weather, map);
    if (strcmp(self->map_name, "xmas_fild01.rsw") == 0 || strcmp(self->map_name, "xmas.rsw") == 0) {
        if (g_session1.effectOn != 0)
            weather_snow(&g_weather);
        else
            weather_stop_effect(&g_weather);
    }
    string_init(&map);
    string_assign_cstr(&map, self->map_name);
    map_sky_mgr_apply(map_sky_mgr_instance(), &map);
    string_free(&map);
}

void game_mode_initialize(struct GameMode *self)
{
    mode_hide_help_balloon(&self->base);
    clear_lists(self);
    make_windows();

    self->base.loop_cond = 1;
    g_drawnCellMinX = 0;
    g_drawnCellMaxX = 0;
    g_drawnCellMinY = 0;
    g_drawnCellMaxY = 0;
    actor_pick_node_build(&g_actorPickNode, 1, 0, 0, g_renderer->width, g_renderer->height);
    path_finder_set_attr(&g_pathFinder, self->world->attr);
    navi_manager_bind_world(&g_navi);
    g_playerBlind = 0;
    self->base.fade_in_start = 1;
    self->base.cursor_offset_x = 0;
    self->base.cursor_offset_y = 0;
    self->click_repeat_time = 0;
    self->last_mouse_x = g_mouse.x;
    self->last_mouse_y = g_mouse.y;
    self->right_press_x = g_mouse.x;
    self->right_press_y = g_mouse.y;
    self->server_move_pending = 0;
    self->npc_dialog_open = 0;
    self->player_dead = 0;
    self->name_balloon = 0;
    self->name_balloon2 = 0;
    self->notice_wnd = 0;
    self->skill_name_wnd = 0;
    self->skill_hint_wnd = 0;

    self->status_bars = 0;
    self->state_icon_tip = 0;
    self->tip_icon = -1;
    self->picked_aid = 0;
    self->name_balloon_width = 0;
    self->name_balloon_height = 0;
    self->name_balloon_stale = 1;
    self->name_balloon2_stale = 1;
    self->use_item_pending = 0;
    self->throw_item_pending = 0;
    self->status_request_pending = 0;
    game_mode_reset_drag(self);
    self->npc_dialog_id = 0;
    self->popup_target_aid = 0;
    self->pointed_player_aid = 0;
    self->balloon_follow_aid = 0;
    self->attack_target_aid = 0;
    self->held_walk_allowed = 1;
    self->chat_room_pending = 0;
    self->exchange_add_pending = 0;
    self->wear_ack_time = timeGetTime() - 2000;
    self->takeoff_ack_time = timeGetTime() - 2000;
    self->trade_add_amount = 0;
    self->exchange_open = 0;
    self->chat_repeats = 0;
    self->server_time_syncs = 0;
    string_clear(&self->last_chat_line);
    self->skill_use.type = 0;
    self->skill_use.skill_id = 0;
    g_skillAimGood = 0;
    self->name_balloon_pos = 0;
    self->composition_card_index = 0;
    self->illust_position = -1;
    self->skill_name_id = -1;
    read_name_balloon_pos(self);
    self->preferences_loaded = 1;

    mouse_reset_buttons(&g_mouse);
    self->server_time_answered = 1;
    self->server_time_requested_at = timeGetTime();
    self->initialized_at = timeGetTime();
    self->emotion_time = timeGetTime();
    self->server_move_time = timeGetTime();
    self->ui_tick_time = timeGetTime();
    self->notice_shown_at = timeGetTime();
    self->skill_shout_time = timeGetTime();
    self->chat_room_enter_time = timeGetTime();
    self->store_browse_time = timeGetTime();
    self->item_move_ready = 1;
    memset(self->stat_raises, 0, sizeof self->stat_raises);
    self->wheel_count = 0;
    self->wheel_count_max = 0;
    sound_set_listener_angle(self->view->destLongitude);
    mode_send_msg(self, MODE_MSG_TOGGLE_STATUS_BARS, 0, 0, 0, 0);
    if (g_isStopByLogin != 0)
        greet_after_login(self);

    session_clear_stat_bonuses(&g_session1);
    session_clear_storage(&g_session1);
    session_clear_lists(&g_session1);
    session_clear_store_setup(&g_session1);
    session_clear_own_store_items(&g_session1);
    session_clear_store_candidates(&g_session1);
    enter_map_server();
    game_mode_send_keep_alive(self, 1);
    start_weather(self);

    g_session1.homunAid = (int)((unsigned)-1);
    g_session1.homun_ai_flags[0] = 0;
    session_clear_homun_commands(&g_session1);
    g_session1.mercAid = (int)((unsigned)-1);
    g_session1.mercPresent = 0;
    session_clear_merc_commands(&g_session1);
    ui_window_mgr_edit_focus_to_chat_wnd(&g_windowMgr);
    navi_manager_resume(&g_navi);
    self->admin_whisper_balloon = 0;
    self->admin_whisper_time = 0;
    self->achievement_notice = 0;
    self->achievement_notice_time = 0;
    g_mapPropertyQuiet = 0;
    if (g_session1.loadingAirplane == 1)
        g_session1.loadingAirplane = 0;
    guild_info_clear_castles(&g_guildInfo);
}
