#include "core/memory.h"
#include "contents/champion_monster.h"
#include "contents/game_session.h"
#include "contents/lua_data.h"
#include "std/std.h"
#include "std/tree.h"
#include "actor/render_ops.h"
#include "actor/actor_common.h"
#include "session/session_skills.h"

static void destroy_champion(void *value);

struct ChampionMonsterMgr *g_championMonster;

#define ACTOR_MSG_REMOVE_EFFECT 0x6d

static struct ChampionMonsterMgr *champion_monster_construct(struct ChampionMonsterMgr *self)
{
    tree_init(&self->jobs);
    return self;
}

static void champion_monster_destruct(struct ChampionMonsterMgr *self)
{
    tree_free(&self->jobs, destroy_champion);
}

static struct ChampionMonster *find_job(struct ChampionMonsterMgr *mgr, int job)
{
    if (!mgr->jobs.size)
        return 0;
    return tree_find(&mgr->jobs, &job, tree_less_int);
}

static void add_job(struct ChampionMonsterMgr *mgr, int job, float size)
{
    struct ChampionMonster champion;

    champion.job = job;
    champion.data.size = size;
    champion.data.effects.first = champion.data.effects.last = champion.data.effects.end = 0;
    tree_add(&mgr->jobs, &champion, sizeof champion, tree_less_int, NULL);
}

static void push_effect(struct ChampionMonster *node, int effect)
{
    struct Vec *effects = &node->data.effects;

    vec_push(effects, &effect, sizeof(int));
}

float champion_monster_size(struct ChampionMonsterMgr *self, int job)
{
    struct ChampionMonster *node = find_job(self, job);

    return node ? node->data.size : 0.0f;
}

char champion_monster_is_champion(struct ChampionMonsterMgr *self, int job)
{
    return find_job(self, job) != 0;
}

int champion_monster_apply_effects(struct ChampionMonsterMgr *self, int job,
                                              struct GameActor *actor, int actor_id)
{
    struct ChampionMonster *node = find_job(self, job);
    struct Vec *effects;
    unsigned i;

    if (!node || !actor)
        return -1;
    effects = &node->data.effects;
    if (actor->visible && actor->revealed
        && !is_effect_state_cloaking(render_object_ops_of((struct RenderObject *)actor)->get_effect_state((struct RenderObject *)actor))) {
        for (i = 0; i < VEC_COUNT(effects, int); i++) {
            int effect = ((int *)effects->first)[i];
            Vec3 at;

            if (effect_mgr_find_effect(g_effectMgr, actor_id, effect))
                continue;
            at.x = actor->pos.x;
            at.y = actor->pos.y;
            at.z = actor->pos.z;
            if (effect_mgr_spawn(g_effectMgr, actor_id, effect, at, 0, -1, 0, 0))
                continue;
            if (able_to_make_effect_is_effect_exist(actor, ((int *)effects->first)[i]))
                continue;
            game_actor_launch_effect((struct GameActor *)actor, ((int *)effects->first)[i],
                       (Vec3){ actor->pos.x, actor->pos.y, actor->pos.z }, 0.0f);
        }
        return -1;
    }
    for (i = 0; i < VEC_COUNT(effects, int); i++)
        render_object_send(actor, actor, ACTOR_MSG_REMOVE_EFFECT, ((int *)effects->first)[i], 0, 0, 0);
    return -1;
}

static int lua_add_item(lua_State *L)
{
    int job;
    float size;

    if (!lua_isnumber(L, -1))
        return lua_data_reply(L, 0, "2nd argment is must number");
    if (!lua_isnumber(L, -2))
        return lua_data_reply(L, 0, "1st argment is must number");
    job = (int)lua_tonumber(L, -2);
    size = (float)lua_tonumber(L, -1);
    add_job(g_championMonster, job, size);
    return lua_data_reply(L, 1, "good");
}

static int lua_add_effect(lua_State *L)
{
    struct ChampionMonster *node;
    int job, effect;

    if (!lua_isnumber(L, -1))
        return lua_data_reply(L, 0, "2nd argment is must number");
    if (!lua_isnumber(L, -2))
        return lua_data_reply(L, 0, "1st argment is must number");
    job = (int)lua_tonumber(L, -2);
    effect = (int)lua_tonumber(L, -1);
    node = find_job(g_championMonster, job);
    if (node)
        push_effect(node, effect);
    return lua_data_reply(L, 1, "good");
}

static const struct LuaDataFunction champion_monster_functions[] = {
    { "AddItem", lua_add_item },
    { "AddEffect", lua_add_effect },
};

char champion_monster_load(void)
{
    return lua_data_run("System\\monster_size_effect_new.lub", "ChampionMonsterMgr",
                        champion_monster_functions,
                        sizeof champion_monster_functions / sizeof champion_monster_functions[0],
                        LUA_DATA_ERROR_PREFIX);
}

char champion_monster_create(void)
{
    struct ChampionMonsterMgr *mgr;

    if (g_championMonster)
        return 0;
    mgr = (struct ChampionMonsterMgr *)mem_try_alloc(sizeof *mgr);
    if (mgr)
        champion_monster_construct(mgr);
    g_championMonster = mgr;
    if (!mgr)
        return 0;
    return 1;
}

void champion_monster_destroy(void)
{
    if (!g_championMonster)
        return;
    champion_monster_destruct(g_championMonster);
    mem_free(g_championMonster);
    g_championMonster = 0;
}

static void champion_monster_data_destruct(struct ChampionMonsterData *self)
{
    vec_free(&self->effects);
}

static void destroy_champion(void *value)
{
    champion_monster_data_destruct(&((struct ChampionMonster *)value)->data);
}
