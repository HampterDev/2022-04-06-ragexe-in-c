#include "game/gamemode.h"
#include "game/context.h"
#include "game/camera.h"

struct World *game_current_world(void)
{
    struct GameMode *mode = game_current_mode();
    return mode ? mode->world : 0;
}

struct Player *game_current_player(void)
{
    struct World *world = game_current_world();
    return world ? world->player : 0;
}

const char *game_current_map_name(void)
{
    struct GameMode *mode = game_current_mode();
    return mode ? mode->map_name : "";
}

void game_get_notice_windows(struct GameNoticeWindows *windows)
{
    struct GameMode *mode = game_current_mode();
    windows->notice = mode ? mode->notice_wnd : 0;
    windows->admin_whisper = mode ? mode->admin_whisper_balloon : 0;
    windows->achievement = mode ? mode->achievement_notice : 0;
}

struct View *game_current_view(void)
{
    struct GameMode *mode = game_current_mode();
    return mode ? mode->view : 0;
}

struct C3dAttr *game_current_map_attr(void)
{
    struct World *world = game_current_world();
    return world ? world->attr : 0;
}

int game_player_is_dead(void)
{
    struct GameMode *mode = game_current_mode();
    return mode && mode->player_dead != 0;
}

struct DragInfo *game_current_drag(void)
{
    struct GameMode *mode = game_current_mode();
    return mode ? &mode->drag : 0;
}

int game_take_item_move(void)
{
    struct GameMode *mode = game_current_mode();
    if (!mode || !mode->item_move_ready)
        return 0;
    mode->item_move_ready = 0;
    return 1;
}

int game_inventory_drag_index(int *index)
{
    const struct DragInfo *drag = game_current_drag();
    if (!drag || drag->source != 1)
        return 0;
    *index = drag->index;
    return 1;
}

struct ChatRoomInfo *game_current_chat_room(void)
{
    struct GameMode *mode = game_current_mode();
    return mode ? &mode->own_chat_room : 0;
}

void game_mark_npc_dialog_closed(void)
{
    struct GameMode *mode = game_current_mode();
    if (mode)
        mode->npc_dialog_open = 0;
}

void game_complete_item_use(void)
{
    struct GameMode *mode = game_current_mode();
    if (mode)
        mode->use_item_pending = 0;
}

int game_camera_is_free(void)
{
    struct GameMode *mode = game_current_mode();
    return mode && mode->camera_free != 0;
}

float game_camera_pitch(void)
{
    struct View *view = game_current_view();
    return view ? view->latitude : 0.0f;
}

void game_camera_limit_distance(float maximum)
{
    struct View *view = game_current_view();
    if (view && view->destDistance > maximum)
        view->destDistance = maximum;
}

float game_camera_heading(void)
{
    struct View *view = game_current_view();
    return view ? view->longitude : 0.0f;
}

float game_camera_target_heading(void)
{
    struct View *view = game_current_view();
    return view ? view->destLongitude : 0.0f;
}

int game_map_dimensions(int *width, int *height)
{
    struct C3dAttr *attr = game_current_map_attr();
    *width = attr ? attr->width * attr->zoom : 0;
    *height = attr ? attr->height * attr->zoom : 0;
    return attr != 0;
}

int game_battleground_state(void)
{
    struct World *world = game_current_world();
    return world ? world->is_battleground : 0;
}

int game_siege_state(void)
{
    struct World *world = game_current_world();
    return world ? world->is_siege : 0;
}

int game_map_has_party_marks(void)
{
    struct World *world = game_current_world();
    return world && (world->is_pvp || world->is_event_pvp);
}

int game_map_preserves_party_effects(void)
{
    struct World *world = game_current_world();
    return world && (world->is_pvp == 1 || world->is_event_pvp == 1);
}

float game_camera_scale(void)
{
    struct View *view = game_current_view();
    if (!view || !(view->destDistance > 0.0f))
        return 0.0f;
    return 400.0f / view->destDistance;
}

int game_send_message(int message, int a, int b, int c, int d)
{
    struct GameMode *mode = game_current_mode();
    return mode ? mode_send_msg(mode, message, a, b, c, d) : 0;
}

void game_set_map_bounds(struct GameMode *mode, const int bounds[4])
{
    mode->map_bounds[0] = bounds[2];
    mode->map_bounds[1] = bounds[3];
    mode->map_bounds[2] = bounds[0];
    mode->map_bounds[3] = bounds[1];
}
