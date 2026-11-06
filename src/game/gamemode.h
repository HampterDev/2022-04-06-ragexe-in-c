#include "web/name_card_api.h"
#include "world/ground_api.h"
#include "std/container_types.h"
#include "world/mode_manager.h"
#include "core/fatal.h"
#include "game/context.h"
#include "game/quest_marks.h"
#ifndef GAMEMODE_GAMEMODE_H
#define GAMEMODE_GAMEMODE_H

#include "base.h"
#include "world/mode_types.h"
#include "game/item_info.h"

#include "render/vec3.h"

#include "std/list.h"
#include "std/tree.h"

#include "resources/attr.h"

#include "actor/types.h"

#include "world/state.h"

struct Mode;
struct GameMode;
#include "world/mode_dispatch.h"
#include "game/mode_callbacks.h"
static __inline int mode_send_msg(struct GameMode *mode, int msg, int a, int b, int c, int d)
{
    return mode ? game_mode_send_msg((struct Mode *)mode, msg, a, b, c, d) : 0;
}
enum {
    MODE_MSG_ACTOR_REMOVED = 0xd,
};

struct ModeMgr;

struct MemberMark {
    unsigned id;
    int x, y;
    unsigned color;
};
struct PingMark {
    unsigned id;
    int x, y;
    int sticky;
    unsigned color;
    unsigned time;
};
struct QuestMark {
    unsigned id;
    struct QuestMarkValue value;
};

struct UnitCell {
    int aid;
    int x, y;
};

struct AwaitingName {
    unsigned aid;
    int request;
};

struct SkillUse {
    int type;
    int skill_id;
    int range;
    int level;
};

struct LoopSound {
    struct String name;
    unsigned npc_aid;
    unsigned interval;
    unsigned next_tick;
};

#include "game/drag_info.h"

#include "game/chat_room.h"

struct CameraPreset {
    short distance_min;
    short distance_range;
    short distance;
    short yaw_min;
    short yaw_max;
    short yaw;
    short pitch_max;
    short pitch_min;
    short pitch;
};
SIZE_ASSERT(CameraPreset, 0x12);

struct View;
struct UIWindow;
struct UITransBalloonText;

struct GameMode {
    struct Mode base;
    int map_bounds[4];
    char map_name[40];
    char minimap_texture[60];
    struct World *world;
    struct View *view;
    void *mouse_pointer;
    unsigned click_repeat_time;
    int last_mouse_x;
    int last_mouse_y;
    int right_press_x;
    int right_press_y;
    int pointed_player_aid;
    int balloon_follow_aid;
    unsigned attack_target_aid;
    int held_walk_allowed;
    int chat_room_pending;
    int exchange_add_pending;
    unsigned wear_ack_time;
    unsigned takeoff_ack_time;
    int skill_upgrade_pending;
    int trade_add_amount;
    int exchange_open;
    struct String whisper_setting_name;
    struct String exchange_partner;
    struct ActorNameCardHolder
        actor_names;
    struct Tree awaiting_name;
    struct Tree unit_cells;
    struct List pickup_requests;
    struct Tree chat_room_owners;
    struct Tree square_marks;
    struct Tree triangle_marks;
    struct Tree ping_marks;
    struct Vec popup_entries;
    struct List
        shown_unit_ids;
    unsigned ui_tick_time;
    struct Vec loop_sounds;
    struct Tree quest_marks;
    int tip_icon;
    struct String popup_target_name;
    struct String whisper_text;
    struct String whisper_target;
    int server_move_pending;
    unsigned server_move_time;
    int npc_dialog_open;
    int player_dead;
    int camera_free;
    int has_camera_preset;
    struct CameraPreset camera_preset;
    int server_time_answered;
    unsigned server_time_requested_at;
    unsigned initialized_at;
    unsigned emotion_time;
    unsigned chat_room_enter_time;
    unsigned
        store_browse_time;
    int emotion_pending;
    float yaw_centre;
    unsigned family_request_aid;
    unsigned family_request_char_id;
    char couple_name[24];
    struct UIWindow *name_balloon;
    struct UIWindow *name_balloon2;
    struct UIWindow
        *notice_wnd;
    struct UIWindow *status_bars;
    struct UIWindow
        *skill_name_wnd;
    struct UIWindow *skill_hint_wnd;
    unsigned skill_shout_time;
    unsigned notice_shown_at;
    int name_balloon_stale;
    int name_balloon2_stale;
    int use_item_pending;
    int throw_item_pending;
    int status_request_pending;
    unsigned picked_aid;
    unsigned npc_dialog_id;
    int popup_target_aid;
    int name_balloon_width;
    int name_balloon_height;
    struct DragInfo drag;
    struct ChatRoomInfo
        own_chat_room;
    struct SkillUse skill_use;
    char skill_use_unchecked;
    int illust_position;
    struct String illust_file;
    int server_time_syncs;
    int ray_tests_cells;
    int composition_card_index;
    int skill_name_id;
    int name_balloon_pos;
    unsigned timer_shown_since;
    int chat_history_count;
    struct String chat_history[11];
    unsigned chat_history_time[11];
    int flood_count;
    unsigned flood_time[3];
    unsigned sit_times[6];
    int attack_repeat;
    int whispers_logged;
    int boss_x;
    int boss_y;
    char boss_shown;
    char copy_name;
    struct UITransBalloonText *state_icon_tip;
    unsigned char
        item_move_ready;
    struct UIWindow *admin_whisper_balloon;
    unsigned admin_whisper_time;
    unsigned throttle_time;
    unsigned
        replay_paused_at;
    char replay_drop_taken;
    unsigned char stat_raises[6];
    int wheel_count;
    int wheel_count_max;
    struct UIWindow *achievement_notice;
    unsigned achievement_notice_time;
    struct List listed;
    struct Vec
        own_double_aids;
    int preferences_loaded;
};

void game_mode_update_balloons(struct GameMode *self);

void actor_list_push_back(struct List *list, void *actor);

#endif
