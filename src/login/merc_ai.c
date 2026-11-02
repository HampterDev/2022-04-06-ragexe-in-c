#include "session/state.h"
#include <stdlib.h>
#include "login/merc_ai.h"

#include <string.h>
#include <stdio.h>
#include <math.h>
#include "session/session_jobs.h"
#include "platform/window.h"

struct MercenaryAI *merc_ai_construct(struct MercenaryAI *self, int kind)
{
    self->kind = kind;
    self->L = NULL;
    self->startTick = 0;
    self->duration = 0;
    self->lastRun = 0;
    return self;
}

void merc_ai_close(struct MercenaryAI *self)
{
    if (self->L) {
        lua_close(self->L);
        self->L = NULL;
    }
}

static void lua_error_box(lua_State *L)
{
    char text[0x400];

    sprintf(text, "%s\n", lua_tolstring(L, -1, NULL));
    platform_message_box(NULL, text, "AI.lua error", 0);
}

static void register_functions(struct MercenaryAI *self)
{
    lua_pushcclosure(self->L, merc_ai_get_actors, 0);
    lua_setfield(self->L, LUA_GLOBALSINDEX, "GetActors");
    lua_pushcclosure(self->L, merc_ai_trace, 0);
    lua_setfield(self->L, LUA_GLOBALSINDEX, "Trace");
    lua_pushcclosure(self->L, merc_ai_get_tick, 0);
    lua_setfield(self->L, LUA_GLOBALSINDEX, "GetTick");
    lua_pushcclosure(self->L, merc_ai_skill_ground, 0);
    lua_setfield(self->L, LUA_GLOBALSINDEX, "SkillGround");
    lua_pushcclosure(self->L, merc_ai_is_monster, 0);
    lua_setfield(self->L, LUA_GLOBALSINDEX, "IsMonster");
    lua_pushcclosure(self->L, merc_ai_trace_ai, 0);
    lua_setfield(self->L, LUA_GLOBALSINDEX, "TraceAI");
    lua_pushcclosure(self->L, merc_ai_move_to_owner, 0);
    lua_setfield(self->L, LUA_GLOBALSINDEX, "MoveToOwner");
    lua_pushcclosure(self->L, merc_ai_move, 0);
    lua_setfield(self->L, LUA_GLOBALSINDEX, "Move");
    lua_pushcclosure(self->L, merc_ai_get_msg, 0);
    lua_setfield(self->L, LUA_GLOBALSINDEX, "GetMsg");
    lua_pushcclosure(self->L, merc_ai_get_res_msg, 0);
    lua_setfield(self->L, LUA_GLOBALSINDEX, "GetResMsg");
    lua_pushcclosure(self->L, merc_ai_skill_object, 0);
    lua_setfield(self->L, LUA_GLOBALSINDEX, "SkillObject");
    if (self->kind == 0) {
        lua_pushcclosure(self->L, merc_ai_homun_get_v, 0);
        lua_setfield(self->L, LUA_GLOBALSINDEX, "GetV");
        lua_pushcclosure(self->L, merc_ai_homun_attack, 0);
        lua_setfield(self->L, LUA_GLOBALSINDEX, "Attack");
    } else if (self->kind == 1) {
        lua_pushcclosure(self->L, merc_ai_merc_get_v, 0);
        lua_setfield(self->L, LUA_GLOBALSINDEX, "GetV");
        lua_pushcclosure(self->L, merc_ai_merc_attack, 0);
        lua_setfield(self->L, LUA_GLOBALSINDEX, "Attack");
    }
    lua_pushcclosure(self->L, merc_ai_trace_value, 0);
    lua_setfield(self->L, LUA_GLOBALSINDEX, "TraceValue");
}

int merc_ai_load(struct MercenaryAI *self, const char *file)
{
    if (!file)
        return 1;
    if (self->L) {
        lua_close(self->L);
        self->L = NULL;
    }
    self->L = luaL_newstate();
    if (!self->L)
        return 2;
    luaL_openlibs(self->L);
    register_functions(self);
    if (luaL_loadfile(self->L, file) == 0 && lua_pcall(self->L, 0, 0, 0) == 0)
        return 0;
    lua_error_box(self->L);
    return 3;
}

int merc_ai_run(struct MercenaryAI *self, DWORD aid)
{
    BYTE *actor;
    DWORD id = aid;
    DWORD now;
    int kind, order;

    actor = (BYTE *)game_find_actor(aid);

    if (!actor)
        return 0;

    if (ACTOR_STATE(actor) == 2 || ACTOR_STATE(actor) == 7) {
        if (ACTOR(actor)->motion_finished == 0)
            return 0;
        actor_set_state(actor, 0, 1);
    }

    if (self->startTick == 0) {
        kind = self->kind;
        if (kind == 0)
            order = g_session1.homun_ai_state[0];
        else if (kind == 1)
            order = g_session1.mercOrder;
        else
            goto run;
        if (order != 0 && kind == 0) {
            self->startTick = GetTickCount();
            if (order == 0x1d) {
                render_object_set_action((struct RenderObject *)actor,
                    (unsigned)(g_session1.homunJob - 0x17a0) <= 4 ? 0x30 : 0x38, 0);
                self->duration = 3000;
            }
            return 0;
        }
    } else if (GetTickCount() - self->startTick > self->duration) {
        actor_set_state(actor, 0, 1);
        if (self->kind == 0) {
            g_session1.homun_ai_state[0] = 0;
            self->startTick = 0;
            self->duration = 0;
        } else {
            g_session1.mercOrder = self->kind == 1 ? 0 : g_session1.mercOrder;
            self->startTick = 0;
            self->duration = 0;
        }
    }

run:
    now = GetTickCount();
    if (now - self->lastRun < 0x8c)
        return 0;
    self->lastRun = now;
    if (!self->L)
        return 0;
    lua_getfield(self->L, LUA_GLOBALSINDEX, "AI");
    lua_pushnumber(self->L, (double)(DWORD)(id));
    if (lua_pcall(self->L, 1, 0, 0) == 0)
        return 0;
    lua_error_box(self->L);
    if (self->L) {
        lua_close(self->L);
        self->L = NULL;
    }
    return 1;
}

void lua_table_set_int(lua_State *L, int key, int value)
{
    lua_pushnumber(L, (double)key);
    lua_pushnumber(L, (double)value);
    lua_settable(L, -3);
}

int merc_ai_owner_attack_range_at_least(DWORD aid, int value)
{
    BYTE *actor, *owner;

    actor = (BYTE *)game_find_actor(aid);
    if (!actor)
        return value;
    owner = (BYTE *)game_find_actor(ACTOR(actor)->requests[5]);
    if (!owner)
        return value;
    if (value >= game_actor_get_npc_location_radius((struct GameActor *)owner))
        return value;
    return game_actor_get_npc_location_radius((struct GameActor *)owner);
}

void merc_ai_apply_skill_attack_range(DWORD aid, void *actor, int *x, int *y, int a, int b)
{
    BYTE *target;
    int tx, ty, range, dx, dy;
    float len, angle, frange, fx;

    if (!actor)
        return;
    if (ACTOR_AID(actor) == (DWORD)g_session1.ownAid)
        return;
    if (session_is_own_unit_aid(&g_session1, ACTOR_AID(actor)))
        return;
    target = (BYTE *)game_find_actor(aid);
    if (!target)
        return;
    if (!game_world_to_cell(ACTOR(target)->pos.x, ACTOR(target)->pos.z, &tx, &ty))
        return;
    range = a ? skill_attack_range(a, b) : 0;
    range = merc_ai_owner_attack_range_at_least(g_session1.homunAid, range);
    if (range <= 0)
        return;
    dy = *y - ty;
    dx = tx - *x;
    len = float_sqrt((float)dy * (float)dy + (float)dx * (float)dx);
    if (!(len > 0.0f))
        return;
    angle = (float)acos((double)(1.0f / len * (float)dx));
    frange = (float)range;
    *x = (int)((float)*x + float_cos(angle) * frange);
    fx = (float)*y;
    *y = (int)(float)((double)fx - (double)float_sin(angle) * (double)frange);
    if (game_can_walk_to_cell(*x, *y))
        return;
    if (game_closest_walkable_cell(x, y))
        return;
    *x = tx;
    *y = ty;
}

int merc_ai_get_actors(lua_State *L)
{
    const struct List *actors = game_current_actor_list();
    struct ListNode *node;
    int i = 1;

    lua_createtable(L, 0, 0);
    if (!actors || !actors->head)
        return 1;
    for (node = actors->head->next; node != actors->head; node = node->next) {
        BYTE *actor = *LIST_VALUE(node, BYTE *);
        int id;

        if (!actor)
            continue;
        id = (int)ACTOR_AID(actor);

        lua_pushnumber(L, (double)i);
        lua_pushnumber(L, (double)id);
        lua_settable(L, -3);
        i++;
    }
    if (game_current_player()) {
        lua_pushnumber(L, (double)i);
        lua_pushnumber(L, (double)g_session1.ownAid);
        lua_settable(L, -3);
    }
    return 1;
}

int merc_ai_get_tick(lua_State *L)
{
    lua_pushnumber(L, (double)(DWORD)(GetTickCount()));
    return 1;
}
