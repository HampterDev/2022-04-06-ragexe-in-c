#ifndef EFFECTS_EFFECT_MGR_H
#define EFFECTS_EFFECT_MGR_H

#include "effects/effect.h"
#include "std/list.h"

struct EffectFactory {
    int type;
    struct Effect *(*create)(void);
};

struct OwnerEffects {
    int aid;
    struct List effects;
};

struct TrackedEffects {
    int type;
    struct Vec effects;
};

struct EffectMgr {
    unsigned char ready;
    struct Tree factories;
    struct Tree by_owner;
    struct List removed;
    struct Tree tracked;
};

#define EFFECT_TRACKED_TYPE(type) \
    ((type) == 0x3ce || (type) == 0x401 || (type) == 0x40f || (type) == 0x431 || \
     (type) == 0x584 || (type) == 0x927)

unsigned char effect_mgr_create(void);
void effect_mgr_destroy_instance(void);
struct EffectMgr *effect_mgr_construct(struct EffectMgr *self);
void effect_mgr_destruct(struct EffectMgr *self);

unsigned char effect_mgr_register_effect_types(struct EffectMgr *self);
unsigned char effect_mgr_register_factory(struct EffectMgr *self, int type, struct Effect *(*create)(void));
unsigned char effect_mgr_is_registered(struct EffectMgr *self, int type);
struct Effect *effect_mgr_create_of_type(struct EffectMgr *self, int type);
unsigned char effect_type_is_listed(int type);

struct Effect *effect_mgr_spawn(struct EffectMgr *self, int aid, int type, Vec3 pos, int target_aid,
                                             int level, unsigned char reuse, int instance_key);
void effect_mgr_add(struct EffectMgr *self, int aid, struct Effect *effect);
void effect_mgr_track(struct EffectMgr *self, struct Effect *effect);
void effect_mgr_spawn_level99_aura(struct EffectMgr *self, void *actor, unsigned char spawn, int val1);

struct Effect *effect_mgr_find_effect(struct EffectMgr *self, int aid, int type);
struct Effect *effect_mgr_find_keyed(struct EffectMgr *self, int aid, int key, int type);
unsigned char effect_mgr_has_effect(struct EffectMgr *self, int aid, int type);
struct Effect *effect_mgr_tracked_at(struct EffectMgr *self, int type, unsigned index);

void effect_mgr_remove(struct EffectMgr *self, struct Effect *effect, unsigned char unlink);
void effect_mgr_remove_type(struct EffectMgr *self, int aid, int type);
void effect_mgr_remove_keyed(struct EffectMgr *self, int aid, int key, int type);
void effect_mgr_remove_owner(struct EffectMgr *self, int aid);
void effect_mgr_destroy_removed(struct EffectMgr *self);
void effect_mgr_reset(struct EffectMgr *self);
void effect_mgr_clear(struct EffectMgr *self);

void effect_mgr_process(struct EffectMgr *self);
void effect_mgr_render(struct EffectMgr *self);

void effect_mgr_load_effect_tool(struct EffectMgr *self);


struct lua_State;
int effect_lua_set_animated_emitter_info(struct lua_State *L);
int effect_lua_set_emitter_info(struct lua_State *L);
int effect_lua_set_evils_paw_info(struct lua_State *L);
int effect_lua_set_ez2str_info(struct lua_State *L);
int effect_lua_set_magic_floor_info(struct lua_State *L);
int effect_lua_set_other_effect_info(struct lua_State *L);
int effect_lua_set_wind_effect_info(struct lua_State *L);

#endif
