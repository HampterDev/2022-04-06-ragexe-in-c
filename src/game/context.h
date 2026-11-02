#ifndef GAMEMODE_CONTEXT_H
#define GAMEMODE_CONTEXT_H

#include <stdint.h>

struct GameMode;
struct Mode;
struct World;
struct Player;
struct View;
struct C3dAttr;
struct GameActor;
struct List;
struct UIWindow;
struct DragInfo;
struct ChatRoomInfo;
struct String;
struct SceneGraphNode;

struct GameNoticeWindows {
    struct UIWindow *notice;
    struct UIWindow *admin_whisper;
    struct UIWindow *achievement;
};

struct GameMode *game_current_mode(void);
struct Mode *game_current_base_mode(void);
struct World *game_current_world(void);
struct Player *game_current_player(void);
struct View *game_current_view(void);
struct C3dAttr *game_current_map_attr(void);
const char *game_current_map_name(void);
void game_get_notice_windows(struct GameNoticeWindows *windows);
int game_player_is_dead(void);
struct DragInfo *game_current_drag(void);
int game_take_item_move(void);
struct ChatRoomInfo *game_current_chat_room(void);

void game_mark_npc_dialog_closed(void);
void game_set_map_bounds(struct GameMode *mode, const int bounds[4]);
int game_inventory_drag_index(int *index);
void game_complete_item_use(void);
int game_camera_is_free(void);
float game_camera_pitch(void);
float game_camera_heading(void);
float game_camera_target_heading(void);
void game_camera_limit_distance(float maximum);
int game_map_dimensions(int *width, int *height);
int game_copy_view_matrix(float matrix[12]);
int game_copy_view_matrix_at(float matrix[12], float heading);
int game_copy_camera_position(float position[3]);
struct C3dGround15;
struct C3dGround15 *game_current_ground(void);
struct List *game_current_object_list(void);
struct SceneGraphNode *game_current_scene_root(void);
const struct List *game_current_actor_list(void);
struct GameActor *game_find_actor(int aid);
int game_world_to_cell(float x, float z, int *cell_x, int *cell_y);
int game_cell_to_world(int x, int y, float *world_x, float *world_z);
int game_can_walk_to_cell(int x, int y);
int game_closest_walkable_cell(int *x, int *y);
void game_remove_object(void *object);
int game_battleground_state(void);
int game_siege_state(void);
int game_map_has_party_marks(void);
int game_map_preserves_party_effects(void);
float game_camera_scale(void);
intptr_t game_send_message(int message, intptr_t a, intptr_t b, intptr_t c, intptr_t d);

void skill_cast_play_cast_look(struct GameActor *caster, int x, int y, int skill_id, unsigned cast_time,
                                    int cast_effect);

#endif
