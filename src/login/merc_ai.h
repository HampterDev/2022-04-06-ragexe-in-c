#include "session/party_queries.h"
#include "actor/actor_ops.h"
#include "session/skill_queries.h"
#include "skills/skill_info.h"
#include "ui/window_messages.h"
#include "skills/attack_range.h"
#ifndef LOGINMODE_MERC_AI_H
#define LOGINMODE_MERC_AI_H

#include "render/math3d.h"

#include "game/context.h"
#include "login/loginmode.h"

typedef struct lua_State lua_State;
typedef double lua_Number;
typedef int (*lua_CFunction)(lua_State *L);
#define LUA_GLOBALSINDEX        (-10002)

void lua_close(lua_State *L);
lua_State *luaL_newstate(void);
void luaL_openlibs(lua_State *L);
int luaL_loadfile(lua_State *L, const char *name);
int lua_pcall(lua_State *L, int nargs, int nresults, int errfunc);
const char *lua_tolstring(lua_State *L, int idx, size_t *len);
void lua_pushcclosure(lua_State *L, lua_CFunction fn, int n);
void lua_setfield(lua_State *L, int idx, const char *k);
void lua_getfield(lua_State *L, int idx, const char *k);
void lua_pushnumber(lua_State *L, lua_Number n);
void lua_createtable(lua_State *L, int narr, int nrec);
void lua_settable(lua_State *L, int idx);
lua_Number luaL_checknumber(lua_State *L, int narg);
const char *luaL_checklstring(lua_State *L, int narg, size_t *len);

#include "login/merc_ai_types.h"
#include "actor/types.h"

extern struct String g_traceAiLast;
extern struct Once g_traceAiLastGuard;

struct MercenaryAI *merc_ai_construct(struct MercenaryAI *self, int kind);
void merc_ai_close(struct MercenaryAI *self);
int merc_ai_load(struct MercenaryAI *self, const char *file);
int merc_ai_run(struct MercenaryAI *self, DWORD aid);
void merc_ai_trace_last_atexit(void);

#define ACTOR(a)                ((struct GameActor *)(a))
#define ACTOR_STATE(a)          (ACTOR(a)->state)
#define ACTOR_AID(a)            ((DWORD)ACTOR(a)->aid)

#define SKILL_INFO_FOUND(i)     ((i)->found)
#define SKILL_INFO_RANGE(i)     ((i)->range)

int merc_ai_get_actors(lua_State *L);
int merc_ai_get_tick(lua_State *L);
int merc_ai_trace(lua_State *L);
int merc_ai_trace_value(lua_State *L);
int merc_ai_trace_ai(lua_State *L);
void lua_table_set_int(lua_State *L, int key, int value);
int merc_ai_owner_attack_range_at_least(DWORD aid, int value);
void merc_ai_apply_skill_attack_range(DWORD aid, void *actor, int *x, int *y, int a, int b);

int merc_ai_homun_attack(lua_State *L);
int merc_ai_merc_attack(lua_State *L);
int merc_ai_get_msg(lua_State *L);
int merc_ai_get_res_msg(lua_State *L);
int merc_ai_homun_get_v(lua_State *L);
int merc_ai_merc_get_v(lua_State *L);
int merc_ai_is_monster(lua_State *L);
int merc_ai_move(lua_State *L);
int merc_ai_move_to_owner(lua_State *L);
int merc_ai_skill_ground(lua_State *L);
int merc_ai_skill_object(lua_State *L);

#endif
