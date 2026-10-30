#include "contents/client_lua.h"
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum QueryKind { QUERY_INTEGER, QUERY_STRING, QUERY_TABLE };
struct GlobalQuery {
    const char *name;
    enum QueryKind kind;
    int number;
    char *text;
};

static int read_global(lua_State *state)
{
    struct GlobalQuery *query = lua_touserdata(state, 1);
    lua_getfield(state, LUA_GLOBALSINDEX, query->name);
    if (query->kind == QUERY_TABLE) {
        query->number = lua_istable(state, -1);
    } else if (query->kind == QUERY_INTEGER && lua_isnumber(state, -1)) {
        double value = lua_tonumber(state, -1);
        if (value > (double)INT_MIN - 1.0 && value < (double)INT_MAX + 1.0)
            query->number = (int)value;
    } else if (query->kind == QUERY_STRING && lua_isstring(state, -1)) {
        size_t length;
        const char *text = lua_tolstring(state, -1, &length);
        query->number = 1;
        if (length < SIZE_MAX) {
            query->text = malloc(length + 1);
            if (query->text) memcpy(query->text, text, length + 1);
        }
    }
    return 0;
}

static int query_global(struct ClientLua *self, struct GlobalQuery *query)
{
    int base, ok;
    if (!self || !self->L || !query->name) return 0;
    base = lua_gettop(self->L);
    ok = lua_cpcall(self->L, read_global, query) == 0;
    lua_settop(self->L, base);
    return ok;
}

int client_lua_get_global_int(struct ClientLua *self, const char *name)
{
    struct GlobalQuery query = {name, QUERY_INTEGER, -1, NULL};
    if (!self || !self->L) return 0;
    return query_global(self, &query) ? query.number : -1;
}

int client_lua_copy_global_string_checked(struct ClientLua *self, const char *name, char **out)
{
    struct GlobalQuery query = {name, QUERY_STRING, 0, NULL};
    *out = NULL;
    if (query_global(self, &query) && (!query.number || query.text)) {
        *out = query.text;
        return 1;
    }
    free(query.text);
    return 0;
}

int client_lua_has_global_table(struct ClientLua *self, const char *name)
{
    struct GlobalQuery query = {name, QUERY_TABLE, 0, NULL};
    return query_global(self, &query) && query.number;
}
