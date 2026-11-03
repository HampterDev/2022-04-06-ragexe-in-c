#include "contents/client_lua.h"
#include "session/session_talk.h"
#include <lauxlib.h>
#include <lualib.h>
#include <assert.h>

/* Real Lua and production lookup/call logic; replace resource/actor boundaries. */
struct Session { struct ClientLua *luaState; int aid, petAccessory; };
struct GameActor { int job, guild; const char *act; };
struct RenderObject;
static struct Session g_session1;
static int normal_loads, accessory_loads, errors, resource_checks;
static char last_act[260];

static int message_box(HWND window, const char *text, const char *caption, UINT flags)
{
    assert(!window && !flags && text && caption);
    ++errors;
    return 0;
}
#include "platform/win_utf8.h"
#define win_utf8_message_box message_box
#include "../src/contents/lua_calls.c"
#undef win_utf8_message_box

static void *g_resMgr(void) { return &resource_checks; }
static int res_mgr_is_available(void *mgr, const char *name)
{
    assert(mgr == &resource_checks && name && name[0]);
    ++resource_checks;
    return strcmp(name, "Monster\\missing.act") != 0;
}
static int game_actor_get_job(struct GameActor *actor) { return actor->job; }
static int game_actor_get_guild_id(struct GameActor *actor) { return actor->guild; }
static int session_is_own_aid(struct Session *session, int aid) { return session->aid == aid; }
static void actor_set_spr_job(struct GameActor *actor, int job)
{
    assert(job == actor->job);
    actor->act = "normal.act";
    ++normal_loads;
}
static void render_object_load_act_res(struct RenderObject *object, const char *name)
{
    struct GameActor *actor = (struct GameActor *)object;
    assert(name && strlen(name) < sizeof last_act);
    strcpy(last_act, name);
    actor->act = last_act;
    ++accessory_loads;
}

#define ENTRY_OBJ_PET 7
#define PET_STATE_ACCESSORY 3
#include "pet_accessory.inc"

static const char script[] =
    "calls=0; PetAccActNameTable={[10013]='poring.act',[10002]='chocho_gasmask.act',"
    "[10014]='missing.act',[10030]=false,[10031]=42,[10032]=''}; "
    "function GetPetAccActName(id) calls=calls+1; "
    "return 'Monster\\\\' .. (PetAccActNameTable[id] or 'chocho_gasmask.act') end";

static void run(lua_State *state, const char *text)
{
    int base = lua_gettop(state);
    assert(luaL_dostring(state, text) == 0);
    assert(lua_gettop(state) == base);
}

static void expect_normal(struct GameActor *actor, int id)
{
    int before = normal_loads, loads = accessory_loads;
    int base = lua_gettop(g_session1.luaState->L);
    actor->act = "stale-accessory.act";
    game_actor_set_pet_accessory(actor, id);
    assert(normal_loads == before + 1 && accessory_loads == loads);
    assert(!strcmp(actor->act, "normal.act"));
    assert(g_session1.petAccessory == 12345);
    assert(lua_gettop(g_session1.luaState->L) == base);
}

int main(void)
{
    struct ClientLua lua = {luaL_newstate()}, closed = {NULL};
    struct GameActor actor = {1002, 0, "normal.act"};
    void (*routes[])(struct GameActor *, unsigned, int) = {
        entry_note_pet, entry_note_pet_existing, receive_hp, receive_accessory
    };
    size_t i;
    int base, checks;
    assert(lua.L);
    luaL_openlibs(lua.L);
    g_session1.luaState = &lua;
    g_session1.aid = 4321;
    run(lua.L, script);
    lua_pushliteral(lua.L, "stack sentinel");
    base = lua_gettop(lua.L);
    for (i = 0; i < sizeof routes / sizeof *routes; ++i) {
        routes[i](&actor, 4321, 10013);
        assert(!strcmp(actor.act, "Monster\\poring.act") && g_session1.petAccessory == 10013);
        routes[i](&actor, 4321, 99999);
        assert(!strcmp(actor.act, "normal.act") && g_session1.petAccessory == 99999);
        routes[i](&actor, 4321, 10002);
        assert(!strcmp(actor.act, "Monster\\chocho_gasmask.act") && g_session1.petAccessory == 10002);
        routes[i](&actor, 4321, 10014);
        assert(!strcmp(actor.act, "normal.act") && g_session1.petAccessory == 10014);
        assert(lua_gettop(lua.L) == base);
    }
    receive_accessory(&actor, 4321, 0);
    assert(!strcmp(actor.act, "normal.act") && g_session1.petAccessory == 0);
    g_session1.petAccessory = 12345;
    for (i = 0; i < 3; ++i) {
        routes[i](&actor, 9999, 10013);
        assert(g_session1.petAccessory == 12345);
    }
    actor.guild = 1;
    actor.act = "special.act";
    entry_note_pet_existing(&actor, 4321, 10014);
    receive_hp(&actor, 4321, 10014);
    assert(!strcmp(actor.act, "special.act"));
    actor.guild = 0;
    g_session1.petAccessory = 12345;
    checks = resource_checks;
    expect_normal(&actor, 10030);
    expect_normal(&actor, 10031);
    expect_normal(&actor, 10032);
    expect_normal(&actor, 0);
    assert(resource_checks == checks);
    run(lua.L, "calls=0; setmetatable(PetAccActNameTable,{__index=function() error('unexpected fallback') end})");
    expect_normal(&actor, 99999);
    lua_getglobal(lua.L, "calls");
    assert(lua_tointeger(lua.L, -1) == 0);
    lua_pop(lua.L, 1);
    run(lua.L, "PetAccActNameTable=nil");
    expect_normal(&actor, 10013);
    run(lua.L, "PetAccActNameTable=false");
    expect_normal(&actor, 10013);
    run(lua.L, script);
    run(lua.L, "GetPetAccActName=function() return nil end");
    expect_normal(&actor, 10013);
    run(lua.L, "GetPetAccActName=function() return '' end");
    expect_normal(&actor, 10013);
    run(lua.L, "GetPetAccActName=function() error('lookup failure') end");
    expect_normal(&actor, 10013);
    assert(errors == 1);
    g_session1.luaState = &closed;
    assert(session_get_pet_accessory_act(&g_session1, 10013) == NULL);
    g_session1.luaState = NULL;
    assert(session_get_pet_accessory_act(&g_session1, 10013) == NULL);
    assert(lua_gettop(lua.L) == base);
    assert(!strcmp(lua_tostring(lua.L, -1), "stack sentinel"));
    lua_close(lua.L);
    puts("Pet accessories: four packet paths, valid/unknown/missing mappings, explicit gas-mask entry, stale appearance reset, equipment state and Lua boundaries passed.");
    return 0;
}
