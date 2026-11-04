#ifndef ACTOR_TYPES_H
#define ACTOR_TYPES_H

#include <stdint.h>
#include "base.h"
#include "std/container_types.h"
#include "render/vec3.h"
#include "world/path_info.h"

enum { ACTOR_EFFECT_SLOT_COUNT = 5 };

struct GameObjectOps;

#define CGAMEOBJECT_MEMBERS                    \
    const struct GameObjectOps *ops;           \
    uint64_t serial;
struct GameObject { CGAMEOBJECT_MEMBERS };

#define CRENDEROBJECT_MEMBERS                  \
    CGAMEOBJECT_MEMBERS                        \
    Vec3 pos;                                  \
    struct String body_pal_name;            \
    int base_action;                           \
    int action;                                \
    int motion;                                \
    int sound_action;                          \
    int sound_motion;                          \
    int body_palette;                          \
    float roty;                                \
    float zoom;                                \
    float shadow_scale;                        \
    float motion_delay;                        \
    float scale;                               \
    float plays;                               \
    float delay_factor;                        \
    float attack_delay_factor;                 \
    int play_mode;                             \
    int state;                                 \
    int resume_state;                          \
    int shift_x;                               \
    int spr_angle;                             \
    int depth_layer;                           \
    unsigned hit_argb;                         \
    unsigned hit_tick;                         \
    unsigned start_tick;                       \
    unsigned char saved_tint[4];               \
    unsigned char tint_alpha;                  \
    unsigned char tint_rgb[3];                 \
    unsigned argb;                             \
    unsigned char slant_depth;                 \
    unsigned char motion_finished;             \
    unsigned char motion_held;                 \
    unsigned char hit_flash;                   \
    unsigned char visible;                     \
    unsigned char revealed;                    \
    unsigned char fixed_depth;                 \
    unsigned char argb_locked;                 \
    unsigned char has_shadow;                  \
    unsigned char pickable;                    \
    int is_pc;                                 \
    int screen_x;                              \
    int screen_y;                              \
    unsigned char cell_lit;                    \
    unsigned char script_action;               \
    unsigned char script_motion;               \
    unsigned char script_actions[5];           \
    unsigned char script_motions[5];           \
    unsigned char script_last;                 \
    unsigned char script_step_frames;          \
    unsigned char script_close_action;         \
    unsigned char script_close_motion;         \
    unsigned char script_step;                 \
    int script_range_on;                       \
    int script_loop;                           \
    int script_timed;                          \
    int script_closing;                        \
    int script_steps_on;                       \
    int script_pingpong_on;                    \
    unsigned script_frame;                     \
    unsigned script_end_tick;                  \
    int overlay_flags;                         \
    unsigned char drawn;                       \
    unsigned char layer5_toggle;               \
    unsigned char pulse_phase;                 \
    unsigned char ring_phase;                  \
    unsigned char swell_step;                  \
    unsigned char shrink_step;                 \
    unsigned char outline_alpha;               \
    unsigned short overlay_step;               \
    unsigned char tint_fade_step;              \
    unsigned char footprint_effect_frame;      \
    unsigned short hover_angle;                \
    unsigned char hover_rise;                  \
    unsigned char camp;                        \
    short balloon_font;                        \
    unsigned char script_phase;                \
    unsigned char costume_body;                \
    unsigned char costume_frames;              \
    void *spr;                                 \
    void *act;                                 \
    int monster_job;                           \
    int aid;                                   \
    unsigned char pose_locked;                 \
    struct Tree vars;                          \
    int prim_list;                             \
    int64_t hover_height;                      \
    unsigned char half_alpha;
struct RenderObject { CRENDEROBJECT_MEMBERS };

struct MousePointer {
    CRENDEROBJECT_MEMBERS
    int countdown;
};

#define CABLETOMAKEEFFECT_MEMBERS              \
    CRENDEROBJECT_MEMBERS                      \
    int effect_id;                             \
    int effect_skill_level;                    \
    struct List effects;                       \
    struct RagEffect *held_effect;             \
    struct RagEffect *held_effect2;
struct AbleToMakeEffect { CABLETOMAKEEFFECT_MEMBERS };

#define CGAMEACTOR_MEMBERS                     \
    CABLETOMAKEEFFECT_MEMBERS                  \
    int effect_slots[ACTOR_EFFECT_SLOT_COUNT]; \
    int cell_x;                                \
    int cell_y;                                \
    int walk_speed;                            \
    int casting;                               \
    int state_locked;                          \
    int wave_pending;                          \
    int body_relocation;                       \
    struct UIWindow *siege_wnd;                \
    struct UIWindow *word_display_wnd;         \
    unsigned char wave_name[128];              \
    int wave_extra_flag;                       \
    unsigned short level;                      \
    Vec3 facing_vector;                        \
    struct PathInfo path;                      \
    unsigned char glide;                       \
    int path_cell;                             \
    float walked;                              \
    unsigned process_tick;                     \
    unsigned balloon_tick;                     \
    int attack_target_aid;                     \
    int attack_target_extra;                   \
    unsigned path_start;                       \
    unsigned process_server_time;              \
    int job;                                   \
    int sex;                                   \
    struct UIWindow *balloon_wnd;              \
    struct UIWindow *chatroom_title_wnd;       \
    struct UIWindow *shop_title_wnd;           \
    struct UIWindow *recharge_gage_wnd;        \
    struct UIWindow *buy_shop_title_wnd;       \
    unsigned motion_hold_until;                \
    unsigned pet_emotion_tick;                 \
    unsigned cast_end_tick;                    \
    unsigned cast_start_tick;                  \
    int balloon_width;                         \
    int balloon_height;                        \
    int name_width;                            \
    int xsize;                                 \
    int ysize;                                 \
    int entry_head;                            \
    struct List extra_hit_queue;               \
    struct List hit_queue;                     \
    int dies_after_hits;                       \
    int aura_on;                               \
    unsigned char aura_level;                  \
    int body_state;                            \
    int effect_state;                          \
    int health_state;                          \
    int pk_mode;                               \
    unsigned char mounted;                     \
    float slide_x;                             \
    float slide_z;                             \
    int efst_effect_frames;                    \
    unsigned fade_until;                       \
    int model_kind;                            \
    int mark_listed;                           \
    int guild_id;                              \
    int owner_aid;                             \
    int miss_count;                            \
    int own_hit_damage;                        \
    int name_masked;                           \
    int mark_kind;                             \
    struct UIWindow *monster_gage_wnd;         \
    unsigned char is_boss;                     \
    struct MercenaryAI *homun_ai;              \
    struct MercenaryAI *merc_ai;               \
    unsigned char object_type;                 \
    int requests[24];                          \
    float attack_motion_count;                 \
    struct GameObject *falcon;                 \
    struct GameObject *cart;                   \
    struct GameObject *emotion_mark;           \
    struct GameObject *frozen_mark;            \
    struct GameObject *curse_mark;             \
    struct GameObject *sleep_mark;             \
    struct GameObject *stun_mark;              \
    struct GameObject *emotion_mark_a;         \
    struct GameObject *msg_mark_1;             \
    struct GameObject *msg_mark_2;             \
    struct GameObject *emotion_mute;             \
    struct GameObject *cursor_mark;            \
    struct List attached;                      \
    int transform_jobs[4];                     \
    int sprite_height;                         \
    int orb_kind;                              \
    int orb_count;                             \
    int mount_look;                            \
    unsigned char is_store_assistant;          \
    unsigned char not_pickable;                \
    int64_t sphere_count;
struct GameActor { CGAMEACTOR_MEMBERS };

struct ActorHit {
    int target_aid;
    int kind;
    int damage;
    unsigned tick;
    int attacker_aid;
    int cell_x, cell_y;
    int effect;
    int hit_effect;
    int value;
    float motion;
    unsigned char is_sp_damage;
    int skill_id;
};
OFFSET_ASSERT(ActorHit, attacker_aid, 0x10);
OFFSET_ASSERT(ActorHit, motion, 0x28);
SIZE_ASSERT(ActorHit, 0x34);

struct ActorAttack {
    int damage;
    int left_damage;
    int action;
    int target_aid;
    int start_time;
    int count;
    int cell_x, cell_y;
    int attack_mt;
    int attacked_mt;
    unsigned char is_sp_damage;
};

struct ActorSkillUse {
    int skill_id;
    int damage;
    int action;
    int target_aid;
    int start_time;
    int level;
    int count;
    int cell_x, cell_y;
    int attack_mt;
    int attacked_mt;
};

struct ActorMove {
    unsigned start_tick;
    int src_x, src_y;
    int sub_x, sub_y;
    int dst_x, dst_y;
};

struct GroundSkillInfo {
    int skill_id;
    int start_time;
    int x, y;
    int level;
};

struct GroundSkillUse {
    int skill_id;
    int x, y;
    int level;
};

struct GroundSkillTalkbox {
    struct GroundSkillUse use;
    const char *text;
};

struct ActorStates {
    int body_state;
    int effect_state;
    int health_state;
    int pk_mode;
};

#define CPC_MEMBERS                            \
    CGAMEACTOR_MEMBERS                         \
    struct GameObject *costume_body_obj;       \
    struct String imf_name;                 \
    int honor;                                 \
    unsigned virtue;                           \
    unsigned hat_flags;                        \
    int head_dir;                              \
    int head;                                  \
    int head_palette;                          \
    int64_t weapon;                            \
    int accessory[3];                          \
    int shield;                                \
    int shoes;                                 \
    int footprint_step;                        \
    Vec3 footprint_last;                       \
    int single_sprite;                         \
    struct String head_pal_name;            \
    struct UIWindow *hp_gage_wnd;              \
    int pvp_rank;                              \
    int pvp_total;                             \
    int robe;                                  \
    int body_flags;                            \
    int body_clip_x;                           \
    int body_clip_y;                           \
    struct Vec layer_sprs;                     \
    struct Vec layer_acts;                     \
    int char_id;                               \
    int body_style;                            \
    int look_job;                              \
    unsigned char helm_robe;                   \
    int madogear_type;                         \
    unsigned char skip_arrow;                  \
    float head_clip[4];
struct Pc { CPC_MEMBERS };

struct Player {
    CPC_MEMBERS
    int requested_cell_x;
    int requested_cell_y;
    unsigned attack_tick;
    unsigned move_request_tick;
    int pending;
    int move_request_pending;
    int attack_kind;
    int attack_sent;
    int owed_walk;
    int target_aid;
    int attacks;
    unsigned move_tick;
    unsigned held_walk_tick;
    int skill_id;
    int skill_level;
    int skill_cell_x;
    int skill_cell_y;
    int owed_walk_x;
    int owed_walk_y;
    struct RagEffect *status_icons;
};

struct C3dGrannyGameActor {
    CGAMEACTOR_MEMBERS
    struct Texture *emblem_tex;
    struct GrannyModelState *model;
    unsigned char clock_now[16];
    unsigned char clock_last[16];
    float anim_time;
    float facing;
    unsigned char alpha;
    unsigned color;
    int anim;
    char name[260];
    int hidden_state;
    int hidden_since;
    unsigned char first_frame;
    struct Texture *shadow_tex;
    struct Vec shadow_points;
};

struct GrannyCorpse3d {
    CGAMEOBJECT_MEMBERS
    Vec3 pos;
    struct GrannyModelState *model;
    unsigned char clock_now[16];
    unsigned char clock_last[16];
    float anim_time;
    int frames;
    float facing;
    unsigned char alpha;
    unsigned color;
    char name[264];
};

struct SoundMaker {
    CRENDEROBJECT_MEMBERS
    struct String wave;
    float volume;
    int area_w;
    int area_h;
    float range;
    float area_reach;
    unsigned last_play;
    unsigned interval;
};

#define CNPC_MEMBERS                           \
    CGAMEACTOR_MEMBERS                         \
    float area[4];                             \
    int player_in_area;                        \
    unsigned last_effect_tick;                 \
    int area_effect_launched;                  \
    struct UIWindow *player_gage;              \
    struct GameObject *mark;                   \
    short mark_effect;                         \
    short mark_icon;
struct Npc { CNPC_MEMBERS };

struct Corpse {
    CRENDEROBJECT_MEMBERS
    float extra_scale;
};

struct BlurPC {
    CPC_MEMBERS
    int fade_param;
    unsigned fade_step;
    struct GameActor *owner;
};

struct HalluPC {
    CPC_MEMBERS
    int fade_float;
    unsigned fade_step;
    unsigned frames;
    struct GameActor *owner;
};

struct HalluPC2 {
    CPC_MEMBERS
    int fade_float;
    unsigned fade_step;
    unsigned frames;
    struct GameActor *owner;
};

struct HalluNPC {
    CNPC_MEMBERS
    int fade_level;
    unsigned fade_step;
    unsigned frames;
};

struct HalluPlant {
    CNPC_MEMBERS
    unsigned frames;
};

#endif
