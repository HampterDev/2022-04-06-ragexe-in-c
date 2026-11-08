#include "game/gamemode.h"
#include "game/context.h"
#include "game/camera.h"
#include "world/mode_manager.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static struct GameMode *current;
static int messages;

intptr_t game_mode_send_msg(struct Mode *mode, int message, intptr_t a, intptr_t b, intptr_t c, intptr_t d)
{
    assert(mode == (struct Mode *)current);
    assert(message == 7 && a == 1 && b == 2 && c == 3 && d == 4);
    ++messages;
    return 91;
}

int main(void)
{
    static struct GameMode mode;
    static struct World world;
    static struct Player player;
    static struct View view;
    static struct C3dAttr attr;
    int width = -1, height = -1, drag_index = -42;
    struct GameNoticeWindows windows;
    int notice, whisper, achievement;

    assert(g_modeMgr.loopCond == 1 && !client_current_mode());
    assert(!game_current_mode() && !game_current_world() && !game_current_player());
    assert(!game_current_drag() && !game_current_chat_room());
    assert(!game_inventory_drag_index(&drag_index) && drag_index == -42);
    game_get_notice_windows(&windows);
    assert(!windows.notice && !windows.admin_whisper && !windows.achievement);
    assert(!game_battleground_state() && !game_siege_state());
    assert(game_camera_scale() == 0.0f);
    assert(!game_current_view() && !game_current_map_attr());
    assert(!game_camera_is_free() && !game_player_is_dead());
    assert(!game_map_has_party_marks());
    game_complete_item_use();
    assert(game_camera_pitch() == 0.0f);
    assert(game_camera_heading() == 0.0f);
    assert(!game_map_preserves_party_effects());
    assert(!game_map_dimensions(&width, &height) && width == 0 && height == 0);
    assert(strlen(game_current_map_name()) == 0);
    assert(strcmp(game_current_map_name(), "new_event.rsw") != 0);
    assert(!game_send_message(7, 1, 2, 3, 4) && messages == 0);

    current = &mode;
    g_modeMgr.curMode = (struct Mode *)&mode;
    g_modeMgr.curModeType = MODE_TYPE_LOGIN;
    assert(client_current_mode() == (struct Mode *)&mode);
    assert(!game_current_mode() && !game_current_base_mode());
    g_modeMgr.curModeType = MODE_TYPE_GAME;
    assert(game_current_mode() == &mode);
    assert(game_current_base_mode() == (struct Mode *)&mode);
    assert(game_current_drag() == &mode.drag);
    assert(game_current_chat_room() == &mode.own_chat_room);
    assert(!game_inventory_drag_index(&drag_index) && drag_index == -42);
    mode.drag.source = 2;
    mode.drag.index = 77;
    assert(!game_inventory_drag_index(&drag_index) && drag_index == -42);
    mode.drag.source = 1;
    assert(game_inventory_drag_index(&drag_index) && drag_index == 77);
    mode.notice_wnd = (struct UIWindow *)&notice;
    mode.admin_whisper_balloon = (struct UIWindow *)&whisper;
    mode.achievement_notice = (struct UIWindow *)&achievement;
    game_get_notice_windows(&windows);
    assert(windows.notice == (struct UIWindow *)&notice);
    assert(windows.admin_whisper == (struct UIWindow *)&whisper);
    assert(windows.achievement == (struct UIWindow *)&achievement);
    assert(strlen(game_current_map_name()) == 0);
    memcpy(mode.map_name, "new_event.rsw", sizeof "new_event.rsw");
    assert(game_current_map_name() == mode.map_name);
    assert(strcmp(game_current_map_name(), "new_event.rsw") == 0);
    memcpy(mode.map_name, "prontera.rsw", sizeof "prontera.rsw");
    assert(strcmp(game_current_map_name(), "prontera.rsw") == 0);
    assert(game_current_mode() == &mode && !game_current_world());
    mode.player_dead = 1;
    mode.use_item_pending = 1;
    game_complete_item_use();
    assert(mode.use_item_pending == 0);
    mode.camera_free = 1;
    assert(game_camera_is_free() && game_player_is_dead());
    mode.player_dead = 0;
    mode.camera_free = 0;
    assert(!game_camera_is_free() && !game_player_is_dead());
    assert(!game_current_player() && game_camera_scale() == 0.0f);
    assert(game_send_message(7, 1, 2, 3, 4) == 91 && messages == 1);
    mode.world = &world;
    assert(game_current_world() == &world && !game_current_player());
    assert(!game_map_has_party_marks());
    world.is_pvp = 1;
    assert(game_map_has_party_marks());
    assert(game_map_preserves_party_effects());
    world.is_pvp = 2;
    assert(!game_map_preserves_party_effects());
    world.is_pvp = 0;
    world.is_event_pvp = 1;
    assert(game_map_has_party_marks());
    assert(game_map_preserves_party_effects());
    world.is_event_pvp = 2;
    assert(!game_map_preserves_party_effects());
    world.is_event_pvp = 0;
    assert(!game_current_map_attr());
    world.attr = &attr;
    attr.width = 150;
    attr.height = 90;
    attr.zoom = 5;
    assert(game_current_map_attr() == &attr);
    assert(game_map_dimensions(&width, &height) && width == 750 && height == 450);
    world.player = &player;
    world.is_battleground = 2;
    world.is_siege = 3;
    assert(game_current_player() == &player);
    assert(game_battleground_state() == 2 && game_siege_state() == 3);
    mode.view = &view;
    assert(game_current_view() == &view);
    view.latitude = -45.0f;
    assert(game_camera_pitch() == -45.0f);
    view.longitude = 30.0f;
    assert(game_camera_heading() == 30.0f);
    assert(game_camera_scale() == 0.0f);
    view.destDistance = 800.0f;
    assert(game_camera_scale() == 0.5f);
    view.destDistance = -1.0f;
    assert(game_camera_scale() == 0.0f);
    current = NULL;
    g_modeMgr.curMode = NULL;
    assert(!game_current_mode() && !game_current_base_mode());
    assert(!game_current_drag() && !game_current_chat_room());
    assert(!game_inventory_drag_index(&drag_index) && drag_index == 77);
    game_get_notice_windows(&windows);
    assert(!windows.notice && !windows.admin_whisper && !windows.achievement);
    assert(!game_map_dimensions(&width, &height) && width == 0 && height == 0);
    assert(!game_current_view() && !game_current_map_attr());
    assert(strlen(game_current_map_name()) == 0);
    assert(!game_current_player() && !game_send_message(7, 1, 2, 3, 4));
    assert(messages == 1);
    puts("game context: absent mode/world/player/camera and message dispatch passed");
    return 0;
}
