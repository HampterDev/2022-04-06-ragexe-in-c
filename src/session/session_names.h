#include "contents/map_info_api.h"
#include "session/job_sounds.h"
#include "session/character_name.h"
#include "session/map_names.h"
#ifndef SESSION_SESSION_NAMES_H
#define SESSION_SESSION_NAMES_H

#include "session/csession.h"

const char *session_get_rsw_name(const char *mapName);
const char *session_get_attr_wave_name(int attr);
struct String *session_get_res_name0(struct String *out, int a, int b,
                                                  int c, int d);
struct String *session_get_res_name1(struct String *out, int a, int b,
                                                  int c, int d);
struct String *session_get_body_pal_name(struct String *out, int a, int b,
                                                     int c, int d, int e);
struct String *session_get_body_spr_name(struct String *out, int a, int b,
                                                     int c, int d, int e);
struct String *session_get_head_spr_name0(struct String *out, int job,
                                                      int sex, int *head);
struct String *session_get_head_spr_name1(struct String *out, int job,
                                                      int sex, int *head);
struct String *session_get_own_head_spr_name0(struct Session *self, struct String *out);
struct String *session_get_own_head_spr_name1(struct Session *self, struct String *out);
struct String *session_get_head_pal_name(struct String *out, int a, int b,
                                                     int c, int d);
struct String *session_get_own_head_pal_name(struct Session *self, struct String *out);
struct String *session_get_imf_name(struct String *out, int a, int b);
struct String *session_get_job_imf_name(struct Session *self, struct String *out, int i);
int session_get_item_bitmap_file_name(const char *itemIdText, char *buf,
                                              int identified);
const char *session_get_item_display_name(const char *itemIdText,
                                                  int identified);
const char *session_get_item_act_name(const char *itemName);
struct NamedStrings {
    struct String name;
    struct Vec values;
};
struct Vec *session_find_fog_parameters(struct Session *self, const char *name, int *found);
struct Vec *session_get_map_view_preset(struct Session *self, const char *name, int *found);
struct String *session_get_body_spr_name_variant0(struct String *out,
                                                             int a, int b, int c, int d);
struct String *session_get_body_spr_name_variant1(struct String *out,
                                                             int a, int b, int c, int d);
const char *session_get_job_name(struct Session *self, unsigned job, int sex);

struct NamedText {
    const char *name;
    const char *text;
};
/* Borrowed configured path, or NULL when the map has no music entry. */
const char *session_get_mp3_name(struct Session *self, const char *mapName);
int session_lookup_talk_type(struct Session *self, const char *name);
BYTE session_is_fixed_view_map(struct Session *self, const char *mapName);

int session_get_skill_effect_id(struct Session *self, int key);
int session_get_skill_hit_effect_id(struct Session *self, int key);
struct String *session_get_shield_name0(struct String *out, int a, int b,
                                                     int c);
struct String *session_get_shield_name1(struct String *out, int a, int b,
                                                     int c);
const char *session_get_shorten_item_name(struct Session *self, int i);
void session_set_shorten_item_name(struct Session *self, const char *name, int slot);
int session_format_item_bitmap_path(const char *name, char *buf);
char *session_remove_underbar(const char *src, char *dst);
void session_rsw_name_atexit(void);
void session_item_display_name_atexit(void);
void session_item_act_name_atexit(void);
void session_fog_none_atexit(void);
void session_view_none_atexit(void);

BYTE session_is_bard_or_dancer(struct Session *self, int job, int female);
float session_get_baby_job_scale(struct Session *self, int job);
int session_remap_job(struct Session *self, int job);
int session_count_hidden_hotkeys_below(struct Session *self, int list, int index);
#include "session/abr.h"
int session_find_combo_delay(struct Session *self, int key);
void *session_get_actor_by_aid(struct Session *self, DWORD aid);
int session_get_default_option_int(struct Session *self, const char *name, int def);
struct String *tokens_at(struct Vec *tokens, int i);
BYTE session_is_emotion_text(struct Session *self, const char *text);
struct String *session_get_monster_act_name(struct Session *self, struct String *out, int job);

struct String *session_get_accessory_name(struct Session *self, struct String *out,
                                                      int accessory);
struct String *session_get_job_sprite_name(struct Session *self, struct String *out, int job);
struct String *session_get_sex_sprite_name(struct Session *self, struct String *out, int sex);
struct String *session_get_weapon_sprite_name(struct Session *self, struct String *out,
                                                         int weapon);
struct ItemInfo *session_get_viewed_costume(struct Session *self, struct ItemInfo *out, int slot);
int session_get_transform_job(struct Session *self);
void session_set_transform_job(struct Session *self, int value, int index);
struct String *session_get_shield_sprite_name(struct Session *self, struct String *out,
                                                         int shield);
struct String *session_get_robe_act_name(struct String *out, int a, int b,
                                                    int c, int d);
struct String *session_get_robe_spr_name(struct String *out, int a, int b,
                                                    int c, int d);
struct String *session_get_weapon_act_name(struct String *out, int a,
                                                      int b, int c, int d);
struct String *session_get_weapon_spr_name(struct String *out, int a,
                                                      int b, int c, int d);
struct String *session_get_glow_weapon_act_name(struct String *out, int a,
                                                          int b, int c, int d, int e);
struct String *session_get_glow_weapon_spr_name(struct String *out, int a,
                                                          int b, int c, int d, int e);
struct MotionStepping {
    int ratio, delay, block;
    DWORD start_time;
};
int session_get_motion_stepping(void *master, void *slave, int action, int block, DWORD start_time,
                                struct MotionStepping *out);
int motion_stepping_frame(const struct MotionStepping *stepping);
void *session_get_stepped_motion(void *master, void *slave, int action,
                                          int block, DWORD startTime, int *frame);
int session_get_mounted_job(int job);
int session_get_item_view_id(int id);
int session_get_weapon_view_id(int id);

int session_get_battlefield_state(struct Session *self);
int session_has_berserk(struct Session *self);

#endif
