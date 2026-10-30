#include <string.h>
#include <limits.h>
#include "contents/item_db_to_id.h"
#include "contents/lua_script.h"
#include <lua.h>

static struct ItemDbToIdMgr *g_item_db;

struct ItemDbToIdMgr *item_db_to_id_instance(void)
{
    if (!g_item_db) {
        struct ItemDbToIdMgr *manager = mem_try_alloc(sizeof *manager);
        g_item_db = manager ? item_db_to_id_construct(manager) : NULL;
    }
    return g_item_db;
}

void item_db_to_id_destroy(void)
{
    if (!g_item_db)
        return;
    item_db_to_id_destruct(g_item_db);
    mem_free(g_item_db);
    g_item_db = NULL;
}

struct ItemDbToIdMgr *item_db_to_id_construct(struct ItemDbToIdMgr *self)
{
    lua_script_construct(&self->script);
    return self;
}

void item_db_to_id_destruct(struct ItemDbToIdMgr *self)
{
    lua_script_destruct(&self->script);
}

char item_db_to_id_load(struct ItemDbToIdMgr *self)
{
    char ok;

    ok = lua_script_run_file(&self->script, "LuaFiles514\\Lua Files\\ItemDBNameTbl.lub");
    return ok;
}

struct ItemIdQuery {
    const char *name;
    int value;
};

static int lookup_item_id(lua_State *state)
{
    struct ItemIdQuery *query = lua_touserdata(state, 1);
    double value;
    lua_getfield(state, LUA_GLOBALSINDEX, "ItemDB_To_ItemID");
    lua_pushstring(state, query->name);
    lua_call(state, 1, 1);
    value = lua_tonumber(state, -1);
    if (value > (double)INT_MIN - 1.0 && value < (double)INT_MAX + 1.0)
        query->value = (int)value;
    return 0;
}

int item_db_to_id_lookup(struct ItemDbToIdMgr *self, const char *name)
{
    struct ItemIdQuery query = {name, 0};
    int base, ok;
    lua_State *state;
    if (!self || !self->script.L || !name) return 0;
    state = self->script.L;
    base = lua_gettop(state);
    ok = lua_cpcall(state, lookup_item_id, &query) == 0;
    lua_settop(state, base);
    return ok ? query.value : 0;
}
