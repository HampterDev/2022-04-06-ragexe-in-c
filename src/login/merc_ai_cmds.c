#include "skills/skill_info.h"
#include "session/state.h"
#include "login/merc_ai.h"
#include "core/numeric.h"

#include <string.h>
#include <stdlib.h>
#include "actor/job.h"
#include "session/session_jobs.h"
#include "session/session_skills.h"

static DWORD arg_id(lua_State *L, int n)
{
    return number_truncate_low32(luaL_checknumber(L, n));
}

static int arg_int(lua_State *L, int n)
{
    return (int)luaL_checknumber(L, n);
}

DWORD g_homunLastAttackTick;
DWORD g_mercLastAttackTick;

static int actor_hidden(void *actor)
{
    if (effect_state_bit6(actor_get_effect_state(actor)))
        return 1;
    if (is_effect_state_burrow(actor_get_effect_state(actor)))
        return 1;
    return is_effect_state_star2(actor_get_effect_state(actor)) != 0;
}

#pragma pack(push, 1)
struct LoginPktAttack {
    WORD id;
    DWORD aid;
    DWORD target;
    BYTE action;
};
#pragma pack(pop)

static void send_attack(DWORD aid, DWORD target)
{
    struct LoginPktAttack p;

    p.target = target;
    p.id = 0x233;
    p.aid = aid;
    p.action = 0;
    login_send_packet(0x233, &p);
}

int merc_ai_homun_attack(lua_State *L)
{
    BYTE *actor, *target;
    DWORD aid, tid, now;
    int value = session_get_instant_var(&g_session1, 0x11);

    if (value > 0)
        return 0;

    aid = arg_id(L, 1);
    tid = arg_id(L, 2);
    if (!session_is_homunculus_aid(&g_session1, aid))
        return 0;
    if (g_session1.mercAid != 0 && session_is_mercenary_aid(&g_session1, tid))
        return 0;
    actor = (BYTE *)game_find_actor(aid);
    target = (BYTE *)game_find_actor(tid);
    if (!actor || !target)
        return 0;
    if (actor_hidden(target))
        return 0;
    now = timeGetTime();
    if ((DWORD)g_session1.homunStats[7] + g_homunLastAttackTick >= now)
        return 0;
    send_attack(aid, tid);
    g_homunLastAttackTick = timeGetTime();
    return 0;
}

int merc_ai_merc_attack(lua_State *L)
{
    BYTE *actor, *target;
    DWORD aid, tid, now;

    aid = arg_id(L, 1);
    tid = arg_id(L, 2);
    if (!session_is_mercenary_aid(&g_session1, aid))
        return 0;
    if (g_session1.homunAid != 0 && session_is_homunculus_aid(&g_session1, tid))
        return 0;
    actor = (BYTE *)game_find_actor(aid);
    target = (BYTE *)game_find_actor(tid);
    if (!actor || !target)
        return 0;
    if (actor_hidden(target))
        return 0;
    now = timeGetTime();
    if ((DWORD)g_session1.mercStats[7] + g_mercLastAttackTick >= now)
        return 0;
    send_attack(aid, tid);
    g_mercLastAttackTick = timeGetTime();
    return 0;
}

static int get_msg(lua_State *L, int *(*get_request)(struct GameActor *, int),
                   void (*set_request)(struct GameActor *, const int *, int))
{
    int msg[6];
    DWORD id;
    int kind;
    struct GameActor *actor;

    id = arg_id(L, 1);
    if (session_is_homunculus_aid(&g_session1, id))
        kind = 0;
    else if (session_is_mercenary_aid(&g_session1, id))
        kind = 1;
    else
        return 0;
    actor = game_find_actor(id);
    if (!actor)
        return 0;
    memcpy(msg, get_request(actor, kind), sizeof msg);
    lua_createtable(L, 0, 0);
    lua_pushnumber(L, 1.0);
    lua_pushnumber(L, (double)msg[0]);
    lua_settable(L, -3);
    switch (msg[0]) {
    case 1:
    case 4:
    case 5:
        lua_table_set_int(L, 2, msg[1]);
        lua_table_set_int(L, 3, msg[2]);
        break;
    case 3:
        lua_table_set_int(L, 2, msg[5]);
        break;
    case 7:
        lua_table_set_int(L, 2, msg[3]);
        lua_table_set_int(L, 3, msg[4]);
        lua_table_set_int(L, 4, msg[5]);
        break;
    case 8:
        lua_table_set_int(L, 2, msg[3]);
        lua_table_set_int(L, 3, msg[4]);
        lua_table_set_int(L, 4, msg[1]);
        lua_table_set_int(L, 5, msg[2]);
        break;
    }
    msg[0] = 0;
    set_request(actor, msg, kind);
    return 1;
}

int merc_ai_get_msg(lua_State *L)
{
    return get_msg(L, game_actor_get_request, game_actor_set_request);
}

int merc_ai_get_res_msg(lua_State *L)
{
    return get_msg(L, game_actor_get_request2, game_actor_set_request2);
}

static BYTE *getv_actor(DWORD id)
{
    if (id == (DWORD)g_session1.ownAid)
        return (BYTE *)game_current_player();
    return (BYTE *)game_find_actor(id);
}

static int getv_position(lua_State *L, BYTE *actor, DWORD id, int what, DWORD self)
{
    int x, y, a = 0, b = 0;

    if (!actor || (id != (DWORD)g_session1.ownAid && actor_hidden(actor))
        || !game_world_to_cell(ACTOR(actor)->pos.x, ACTOR(actor)->pos.z, &x, &y)) {
        lua_pushnumber(L, -1.0);
        lua_pushnumber(L, -1.0);
        return 2;
    }
    if (what == 13) {
        a = arg_int(L, 3);
        b = arg_int(L, 4);
    }
    merc_ai_apply_skill_attack_range(self, actor, &x, &y, a, b);
    lua_pushnumber(L, (double)x);
    lua_pushnumber(L, (double)y);
    return 2;
}

static int getv_skill_range(lua_State *L, DWORD self, int what,
    struct SkillInfo *(*get)(struct Session *, struct SkillInfo *, int))
{
    struct SkillInfo info;
    DWORD skill = arg_id(L, 3);
    int range;

    get(&g_session1, &info, skill);
    if (SKILL_INFO_FOUND(&info)) {
        range = SKILL_INFO_RANGE(&info);
        if (what == 14) {
            int level = arg_int(L, 4);
            range = skill_attack_range(skill, level);
        }
        lua_pushnumber(L, (double)merc_ai_owner_attack_range_at_least(self, range));
    } else {
        lua_pushnumber(L, 1.0);
    }
    skill_info_destruct(&info);
    return 1;
}

static double getv_pair(DWORD id, int mine, int isOurs, int theirs)
{
    if (id == (DWORD)g_session1.ownAid)
        return (double)mine;
    if (isOurs)
        return (double)theirs;
    return -1.0;
}

int merc_ai_homun_get_v(lua_State *L)
{
    double v = luaL_checknumber(L, 1);
    DWORD id = arg_id(L, 2);
    BYTE *actor = getv_actor(id);
    int what = (int)v;
    double r;

    switch (what) {
    case 0:
        r = (double)(DWORD)g_session1.ownAid;
        break;
    case 1:
    case 13:
        return getv_position(L, actor, id, what, g_session1.homunAid);
    case 2:
        return 1;
    case 3:
        r = actor ? (double)ACTOR_STATE(actor) : -1.0;
        break;
    case 4:
        r = (actor && g_session1.homun_ai_state[2]) ? (double)g_session1.homun_ai_state[2] : -1.0;
        break;
    case 5:
        r = actor ? (double)ACTOR(actor)->attack_target_aid : -1.0;
        break;
    case 6:
    case 14:
        return getv_skill_range(L, g_session1.homunAid, what, session_get_homun_skill_by_id);
    case 7:
        r = actor ? (double)(game_actor_get_job(ACTOR(actor)) % 6000) : -1.0;
        break;
    case 8:
        r = getv_pair(id, g_session1.hp, id != (DWORD)g_session1.ownAid && session_is_homunculus_aid(&g_session1, id), g_session1.homunHp);
        break;
    case 9:
        r = getv_pair(id, g_session1.sp, id != (DWORD)g_session1.ownAid && session_is_homunculus_aid(&g_session1, id), g_session1.homunSp);
        break;
    case 10:
        r = getv_pair(id, g_session1.maxHp, id != (DWORD)g_session1.ownAid && session_is_homunculus_aid(&g_session1, id), g_session1.homunMaxHp);
        break;
    case 11:
        r = getv_pair(id, g_session1.maxSp, id != (DWORD)g_session1.ownAid && session_is_homunculus_aid(&g_session1, id), g_session1.homunMaxSp);
        break;
    default:
        return 0;
    }
    lua_pushnumber(L, r);
    return 1;
}

int merc_ai_merc_get_v(lua_State *L)
{
    double v = luaL_checknumber(L, 1);
    DWORD id = arg_id(L, 2);
    BYTE *actor = getv_actor(id);
    int what = (int)v;
    double r;

    switch (what) {
    case 0:
        r = (double)(DWORD)g_session1.ownAid;
        break;
    case 1:
    case 13:
        return getv_position(L, actor, id, what, g_session1.mercAid);
    case 2:
        return 1;
    case 3:
        r = actor ? (double)ACTOR_STATE(actor) : -1.0;
        break;
    case 4:
        r = (actor && g_session1.merc_contract_stats[3]) ? (double)g_session1.merc_contract_stats[3] : -1.0;
        break;
    case 5:
        r = (actor && ACTOR_STATE(actor) != 3) ? (double)ACTOR(actor)->attack_target_aid : -1.0;
        break;
    case 6:
    case 14:
        return getv_skill_range(L, g_session1.mercAid, what, session_get_mer_skill_by_id);
    case 8:
        r = getv_pair(id, g_session1.hp, id != (DWORD)g_session1.ownAid && session_is_mercenary_aid(&g_session1, id), g_session1.mercHp);
        break;
    case 9:
        r = getv_pair(id, g_session1.sp, id != (DWORD)g_session1.ownAid && session_is_mercenary_aid(&g_session1, id), g_session1.mercSp);
        break;
    case 10:
        r = getv_pair(id, g_session1.maxHp, id != (DWORD)g_session1.ownAid && session_is_mercenary_aid(&g_session1, id), g_session1.mercMaxHp);
        break;
    case 11:
        r = getv_pair(id, g_session1.maxSp, id != (DWORD)g_session1.ownAid && session_is_mercenary_aid(&g_session1, id), g_session1.mercMaxSp);
        break;
    case 12:
        if (!actor)
            r = -1.0;
        else if (session_is_mercenary_job(&g_session1, game_actor_get_job(ACTOR(actor))))
            r = (double)(game_actor_get_job(ACTOR(actor)) % 0x1781) + 1.0;
        else
            r = 1.0;
        break;
    default:
        return 0;
    }
    lua_pushnumber(L, r);
    return 1;
}

int merc_ai_is_monster(lua_State *L)
{
    DWORD id = arg_id(L, 1);
    BYTE *actor = (BYTE *)game_find_actor(id);
    double r = 0.0;
    int job, test;
    BYTE c;

    if (!actor)
        goto push;
    job = game_actor_get_job(ACTOR(actor));
    if ((job > 1000 && job < 0xf9f) || (unsigned)(job - 0x4e35) <= 0x2eca) {
        c = ACTOR(actor)->object_type;
        if (c != 7) {
            switch (c) {
            case 9:
                r = 0.0;
                goto push;
            }
            goto states;
        }
    }
    if (!session_is_player_job(&g_session1, job))
        goto push;
    if (game_map_has_party_marks()) {
        test = session_is_party_member_aid(&g_session1, ACTOR_AID(actor), 1);
    } else {
        if (!game_siege_state())
            goto push;
        test = session_is_own_guild_id(&g_session1, game_actor_get_guild_id((struct GameActor *)actor));
    }
    if (test != 0)
        goto push;
states:
    r = actor_hidden(actor) ? 0.0 : 1.0;
push:
    lua_pushnumber(L, r);
    return 1;
}

#pragma pack(push, 1)
struct LoginPktMove {
    WORD id;
    DWORD aid;
    BYTE pos[3];
};
struct LoginPktToOwner {
    WORD id;
    DWORD aid;
};
#pragma pack(pop)

int merc_ai_move(lua_State *L)
{
    struct LoginPktMove p;
    DWORD id = arg_id(L, 1);
    int x = arg_int(L, 2);
    int y = arg_int(L, 3);
    BYTE *actor;
    int ax, ay;

    if (!session_is_homunculus_aid(&g_session1, id) && !session_is_mercenary_aid(&g_session1, id))
        return 0;
    actor = (BYTE *)game_find_actor(id);
    if (!actor)
        return 0;
    if (!game_can_walk_to_cell(x, y) && !game_closest_walkable_cell(&x, &y))
        return 0;
    if (!game_world_to_cell(ACTOR(actor)->pos.x, ACTOR(actor)->pos.z, &ax, &ay))
        return 0;
    if (abs(x - ax) + abs(y - ay) > 24)
        return 0;
    p.id = 0x232;
    p.pos[0] = (BYTE)(x >> 2);
    p.pos[1] = (BYTE)(((y >> 4) & 0x3f) | (x << 6));
    p.aid = id;
    p.pos[2] = (BYTE)(y << 4);
    login_send_packet(0x232, &p);
    return 0;
}

int merc_ai_move_to_owner(lua_State *L)
{
    struct LoginPktToOwner p;
    DWORD id = arg_id(L, 1);

    if (!session_is_homunculus_aid(&g_session1, id) && !session_is_mercenary_aid(&g_session1, id))
        return 0;
    if (!game_find_actor(id))
        return 0;
    p.aid = id;
    p.id = 0x234;
    login_send_packet(0x234, &p);
    return 0;
}

int merc_ai_skill_ground(lua_State *L)
{
    struct GroundSkillUse use;
    DWORD id = arg_id(L, 1);
    BYTE *actor;
    int st;

    use.skill_id = arg_int(L, 3);
    use.x = arg_int(L, 4);
    use.y = arg_int(L, 5);
    use.level = arg_int(L, 2);
    if (!session_is_homunculus_aid(&g_session1, id) && !session_is_mercenary_aid(&g_session1, id))
        return 0;
    if (!session_is_homunculus_skill(&g_session1, use.skill_id)
            && !session_is_mercenary_skill(&g_session1, use.skill_id))
        return 0;
    actor = (BYTE *)game_find_actor(id);
    if (!actor)
        return 0;
    st = ACTOR_STATE(actor);
    if (st == 1 || st == 0 || st == 4)
        game_send_message(0x46, (int)&use, 0, 0, 0);
    game_send_message(0x1a, 0, 0, 0, 0);
    return 0;
}

int merc_ai_skill_object(lua_State *L)
{
    struct SkillInfo info;
    DWORD id = arg_id(L, 1);
    DWORD level = arg_id(L, 2);
    DWORD skill = arg_id(L, 3);
    DWORD tid = arg_id(L, 4);
    BYTE *target, *actor;
    int st, r = 0;

    session_get_skill_info(&info, skill);
    if (!SKILL_INFO_FOUND(&info))
        goto done;
    if (!session_is_homunculus_aid(&g_session1, id) && !session_is_mercenary_aid(&g_session1, id))
        goto done;
    if (!session_is_homunculus_skill(&g_session1, skill) && !session_is_mercenary_skill(&g_session1, skill))
        goto done;
    if (tid == (DWORD)g_session1.ownAid) {
        target = (BYTE *)game_current_player();
    } else {
        target = (BYTE *)game_find_actor(tid);
        if (target && actor_hidden(target))
            goto done;
    }
    actor = (BYTE *)game_find_actor(id);
    if (!actor)
        goto done;
    if (target) {
        st = ACTOR_STATE(actor);
        if (st == 1 || st == 0 || st == 4)
            game_send_message(0x45, skill, tid, level, 0);
    }
    game_send_message(0x1a, 0, 0, 0, 0);
    if (!skill_info_is_good_skill_for_actor(skill))
        goto done;
    lua_pushnumber(L, 1.0);
    r = 1;
done:
    skill_info_destruct(&info);
    return r;
}
