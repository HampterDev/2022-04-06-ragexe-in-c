#include "effects/effect_mgr.h"
#include "actor/map_type_table.h"
struct EffectMgr *g_effectMgr;
struct MapPropertyMgr *g_mapFlags;
void client_lua_loadfile_hook(const char *name) { (void)name; abort(); }
struct Effect;
struct EffectMgr;
struct GameActor;
struct GameMode;
struct Player;
struct RagEffect;
struct Session;
struct ItemInfo;
struct OwnStateEntry;
struct PartyMember;
struct String;
struct Tree;
struct TreeNode;
struct Vec;
unsigned char able_to_make_effect_is_effect_exist(void *actor, int effect) { abort(); }
struct Effect *effect_mgr_find_effect(struct EffectMgr *self, int aid, int type) { abort(); }
struct Effect *effect_mgr_spawn(struct EffectMgr *self, int aid, int type, Vec3 pos, int target_aid,
                                             int level, unsigned char reuse, int instance_key) { abort(); }
struct RagEffect *game_actor_launch_effect(struct GameActor *self, int effect_id,
                                         Vec3 offset, float rotation) { abort(); }
int game_actor_get_job(struct GameActor *self) { abort(); }
int game_actor_get_guild_id(struct GameActor *self) { abort(); }
struct String *game_mode_get_actor_name(struct GameMode *gameMode, struct String *out, int aid) { abort(); }
int map_property_mgr_get(struct MapPropertyMgr *table, int row, int column) { abort(); }
void *session_get_actor_by_aid(void *self, int aid) { abort(); }
int session_get_drawn_job(void *self) { abort(); }
struct PartyMember *session_get_party_member_by_name(struct Session *self, struct PartyMember *out,
                                                                 const char *name) { abort(); }
BYTE session_get_pet_acc_act_name(struct Session *self, int job) { abort(); }
int session_is_pc_job(void *self, int job) { abort(); }
unsigned char is_effect_state_cloaking(int state) { abort(); }
struct ItemInfo *item_info_copy(struct ItemInfo *self, const struct ItemInfo *src) { abort(); }
void store_search_item_copy(void *self, const void *src) { abort(); }
void store_search_items_destroy(void *first, void *last) { abort(); }
struct GameMode *game_current_mode(void) { abort(); }
struct Player *game_current_player(void) { abort(); }
int game_siege_state(void) { abort(); }
const struct OwnStateEntry *own_states_find(int index) { abort(); }
char *std_copy_memmove(const char *first, const char *last,
                                  char *dest) { abort(); }
