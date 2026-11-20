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
static __inline intptr_t mode_send_msg(struct GameMode *mode, int msg, intptr_t a, intptr_t b, intptr_t c, intptr_t d)
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

#include "std/clock_sync.h"
#include "std/way_up.h"
#include "std/render_delay.h"
#include "world/act_track.h"
#include "world/record_wait.h"
#include "world/view_floors.h"
#include "world/own_swing.h"

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
    int trade_add_amount;
    int exchange_open;
    struct String whisper_setting_name;
    struct String exchange_partner;
    struct ActorNameCardHolder
        actor_names;
    struct Tree awaiting_name;
    struct Tree unit_cells;
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
    /* The swings that began (a swing record of ZC_TRACK_ACT) and have not yet landed or been
       cancelled, oldest first. */
    struct OpenSwing {
        unsigned aid;
        unsigned target;
    } open_swings[64];
    int open_swing_count;
    /* The least time the server wants a request to wait after its press (ZC_HOLD_BACK,
       plans/netcode.md), its clock as estimated from the timed exchanges and the way up they
       measure, the numbers of the last input stamp and of the last stamped move, and the tick
       the last stamp named. */
    unsigned hold_back_ms;
    struct ClockSync clock_sync;
    struct WayUp way_up;
    /* How far behind that clock everyone else is drawn while the server sends tracks
       (plans/core-loop.md). */
    struct RenderDelay render_delay;
    /* The acts, hits and HP the server sent, waiting for their tick (game_mode_play_tracks). */
    struct TimedQueue tracks;
    /* Records whose unit was out of view or had no actor when they came (plans/delivery.md),
       waiting for it. */
    struct RecordWait record_wait;
    /* What the client has of each unit by its id, whichever actor shows it: its tracks' floors
       and its last coming and going (plans/presence.md). */
    struct ViewFloors view_floors;
    /* A ZC_VIEW_STAMP that waits for the very next packet (game_mode_view_stamped). */
    struct ViewStamp view_stamp;
    unsigned char view_stamp_waits;
    /* The book of the player's own swing that is shown, which follows the predicted world's
       word of it (game_mode_own_swing). */
    struct OwnSwing own_swing;
    /* The swings and casts of other actors at the player's own character that wait for their
       actor to be drawn standing (game_mode_play_tracks), the one that came first first. */
    struct ActRecord aimed_waiting[TIMED_AIMED_ACTORS];
    int aimed_waiting_count;
    unsigned short input_seq;
    unsigned short move_seq;
    unsigned stamp_tick;
    /* The number of the last intent sent (CZ_INTENT, plans/intents.md). */
    unsigned short intent_seq;
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
    struct String
        last_chat_line;
    int chat_repeats;
    int server_time_syncs;
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

#endif
