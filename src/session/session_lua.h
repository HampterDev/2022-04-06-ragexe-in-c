#include "session/service_address.h"
#ifndef SESSION_SESSION_LUA_H
#define SESSION_SESSION_LUA_H

#include "session/csession.h"

int session_get_real_weapon_id(int weapon);
float session_get_npc_location_radius(struct Session *self, int job, BYTE mapJob);
const char *session_get_weapon_name_by_class(int itemId);
void session_load_transparent_items(struct Session *self);
void session_init_lua_files(struct Session *self);
void session_load_shadow_factors(struct Session *self);
void session_load_weapon_hit_waves(struct Session *self);
void session_load_external_settings(struct Session *self);

BYTE session_is_pet_accessory(struct Session *self, int accessory);
BYTE session_is_top_layer_robe(struct Session *self, int robe);
int session_get_inherited_job(struct Session *self, int job);
struct String *session_get_title_string(struct Session *self, struct String *out, int id);
void session_init_skill_tree_view(struct Session *self);

#endif
