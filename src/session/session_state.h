#include "session/job_sounds.h"
#include "session/number_codec.h"
#include "session/character_name.h"
#include "session/skill_levels.h"
#ifndef SESSION_SESSION_STATE_H
#define SESSION_SESSION_STATE_H

#include "session/csession.h"

struct PlateRecord {
    int found;
    int id;
    int cell;
};

struct Session *session_construct(struct Session *self);
void session_destruct(struct Session *self);

DWORD session_get_server_time(struct Session *self);
void session_init_at_startup(struct Session *self);

void session_init_pet_emotions(struct Session *self);
void session_load_talk_tables(struct Session *self);
void session_load_map_info_table(struct Session *self);
#include "session/chat_macros.h"
void session_init_skill_effect_id_maps(struct Session *self);
void session_clear_transform_jobs(struct Session *self, BYTE a, BYTE b, BYTE c);
void session_reset_pet(struct Session *self);

void session_startup(struct Session *self);
void session_pre_load_always_needed_res(struct Session *self);

void session_reroll_values(struct Session *self);
void session_clear_shorten_item_name(struct Session *self);
void session_clear_plate_records(struct Session *self);
void session_clear_stat_bonuses(struct Session *self);
BYTE session_cash_shop_goods_empty(struct Session *self);
void scale_rect_to_screen(RECT *r);
struct PlateRecord *session_get_plate_record(struct Session *self, struct PlateRecord *out, int key);
int session_get_transparent_item_alpha(struct Session *self, int key);
void session_reset_store_search(struct Session *self);
int session_is_admin_option(struct Session *self, int what);
BYTE session_is_screen_flipped(struct Session *self);
void session_push_ping_sample(struct Session *self, unsigned value);
void session_erase_plate_record(struct Session *self, int value);
void session_save_mini_party_info(struct Session *self);
void session_save_emblem(struct Session *self, const void *data, size_t size, int guildId,
                                    int version);
void session_add_plate_record(struct Session *self, int key, int cell);
void session_set_plate_cell(struct Session *self, int key, int value);
void session_set_server_time(struct Session *self, DWORD serverTime);
void session_sync_server_time(struct Session *self, DWORD serverTime);
void session_set_player_pos_dir(struct Session *self, int x, int y, int dir);

int session_load_buying_store_items(struct Session *self);
int session_load_exception_minimap_names(struct Session *self);
BYTE session_is_not_minimap_exception(struct Session *self);
int session_load_indoor_rsws(struct Session *self);
int session_scale_rect_maps(struct Session *self);
void session_clear_homun_commands(struct Session *self);
int session_load_fog_parameters(struct Session *self);
int session_load_view_points(struct Session *self);
void named_strings_clear(struct Vec *v);
void named_strings_free(struct Vec *v);
void session_reload_texts(struct Session *self);
void session_clear_merc_commands(struct Session *self);
int session_load_mp3_names(struct Session *self);
void session_load_mini_party_info(struct Session *self);
void session_init_table(struct Session *self);

int session_init_map_pos_table(struct Session *self);

void session_set_combo_delay(struct Session *self, int key, int value);
int session_get_max_base_level(struct Session *self, int job);
int session_get_max_job_level(struct Session *self, int job);
BYTE session_pet_talk_due(struct Session *self, __int64 interval);
BYTE session_reset_for_replay(struct Session *self);
int session_count_worn_costumes(struct Session *self);
int session_count_worn_equips(struct Session *self);
const char *session_get_map_display_name(struct Session *self);

BYTE session_is_skill_delay_over(struct Session *self, int id);

void session_set_homun_auto_feed(struct Session *self, BYTE on);
void session_set_pet_auto_feed(struct Session *self, BYTE on);
void session_send_homun_auto_feed(struct Session *self, BYTE on);
void session_send_pet_auto_feed(struct Session *self, BYTE on);

void session_clear_items_and_equipment(struct Session *self);
void session_clear_character_lists(struct Session *self);

void session_init_variables_(struct Session *self);

struct Vec *session_get_stored_packet(struct Session *self, struct Vec *out,
                                                      short packetId);
struct Vec *session_get_stored_packet2(struct Session *self, struct Vec *out);

#endif
