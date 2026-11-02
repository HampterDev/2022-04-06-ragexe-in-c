#ifndef SESSION_SESSION_JOBS_H
#define SESSION_SESSION_JOBS_H

#include "session/csession.h"
#include "session/item_predicates.h"
#include "session/party_queries.h"

int session_get_drawn_job(struct Session *self);
float session_get_job_float(struct Session *self, int job, int b, int c, int mode);
void session_set_job(struct Session *self, int job);

void session_init_job_name_tables(struct Session *self);

void ui_window_mgr_render_menu(struct Session *self);

BYTE session_job_allows_level(struct Session *self, int job, int level);
BYTE session_job_accepts_value(struct Session *self, int job, unsigned __int64 value);
BYTE session_get_pet_acc_act_name(struct Session *self, int job);
BYTE session_job_accepts_weapon(struct Session *self, int job, int mode, unsigned __int64 item);
int session_is_third_job(struct Session *self, int job);
int session_is_pc_job(struct Session *self, int job);

int session_get_job_without_mount(struct Session *self, int job);

int session_is_abracadabra_skill(struct Session *self, int id);
int session_is_attack(struct Session *self, int state);
BYTE session_is_baby_job(struct Session *self, int id);
BYTE session_is_cash_item_type(struct Session *self, int id);
BYTE session_is_costume_or_shadow_location(struct Session *self, int flag);
BYTE session_is_equip_item_type(struct Session *self, int id);
int session_is_guardian(struct Session *self, int job);
int session_is_homunculus_job(struct Session *self, int id);
int session_is_homunculus_s_job(struct Session *self, int id);
int session_is_homunculus_skill(struct Session *self, int id);
int session_is_mercenary_or_homun_s_job(struct Session *self, int id);
int session_is_homun_or_merc_job(struct Session *self, int id);
int session_is_merc_or_elemental_skill(struct Session *self, int id);
BYTE session_is_space(struct Session *self, int id);
BYTE session_is_expanded_super_novice_job(struct Session *self, int id);
BYTE session_is_super_novice_job(struct Session *self, int id);
BYTE session_is_any_super_novice_job(struct Session *self, int id);
BYTE session_is_expanded_job(struct Session *self, int id);
int session_is_monster_actor(struct Session *self, struct GameActor *actor);
int session_is_player_job(struct Session *self, int id);
int session_has_trick_dead(struct Session *self);
int session_has_status(struct Session *self, int id);
int session_has_status_effect(struct Session *self, int id);
int session_is_homunculus_aid(struct Session *self, int aid);
int session_is_mercenary_aid(struct Session *self, int aid);
int session_is_own_aid(struct Session *self, int aid);
int session_is_own_unit_aid(struct Session *self, int aid);
int session_is_own_guild_id(struct Session *self, int id);
int session_is_siege_mode(struct Session *self);
BYTE session_is_tracked_achievement(struct Session *self, int id);
BYTE session_has_tracked_achievements(struct Session *self);
int session_map_is_gvg(struct Session *self);
int session_map_is_pvp(struct Session *self);
int session_map_is_battlefield(struct Session *self);
int session_map_counts_pk(struct Session *self);
int session_map_disables_lock_on(struct Session *self);
int session_map_locks_party(struct Session *self);
int session_map_is_siege(struct Session *self);
int session_map_uses_simple_effect(struct Session *self);
BYTE session_is_doram_job(struct Session *self, int job);
BYTE session_is_head_replacing_view(struct Session *self, int id);
BYTE session_is_npc(struct Session *self, int aid, int job);
BYTE session_can_request_adoption(struct Session *self, int aid);

void session_init_pc_name_table(struct Session *self);

#endif
