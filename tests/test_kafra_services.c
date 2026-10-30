#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contents/client_lua.h"
#include <lauxlib.h>
#include <lualib.h>

static int allocations, fail_after = -1, allocation_calls, diagnostics;
static void *checked_malloc(size_t size)
{
    unsigned char *raw;
    ++allocation_calls;
    if (fail_after == 0) return NULL;
    if (fail_after > 0) --fail_after;
    raw = malloc(size + 24); assert(raw);
    *(size_t *)raw = size;
    memset(raw + 8, 0x63, 8); memset(raw + 16 + size, 0x57, 8);
    memset(raw + 16, 0xcc, size); ++allocations;
    return raw + 16;
}
static void checked_free(void *pointer)
{
    unsigned char *raw; size_t size, i;
    if (!pointer) return;
    raw = (unsigned char *)pointer - 16; size = *(size_t *)raw;
    for (i = 0; i < 8; ++i) { assert(raw[8+i] == 0x63); assert(raw[16+size+i] == 0x57); }
    free(raw); --allocations;
}
static void *checked_realloc(void *pointer, size_t size)
{
    void *next = checked_malloc(size);
    if (next && pointer) {
        size_t old = *(size_t *)((unsigned char *)pointer - 16);
        memcpy(next, pointer, old < size ? old : size); checked_free(pointer);
    }
    return next;
}
#define malloc checked_malloc
#define realloc checked_realloc
#define free checked_free
#include "../src/ui/worldmap/kafra_lua.c"
#undef malloc
#undef realloc
#undef free

void client_lua_loadfile_hook(const char *name) { (void)name; abort(); }
void client_lua_error_box(struct ClientLua *lua, const char *caption, const char *format, ...)
{ assert(lua && lua->L && caption && format); ++diagnostics; }
static int recursive_load(lua_State *L)
{
    struct ClientLua lua = {L};
    int top = lua_gettop(L);
    assert(!kafra_services_load(&lua) && lua_gettop(L) == top);
    return 0;
}
static void script(lua_State *L, const char *text)
{ assert(luaL_dostring(L, text) == 0); }
static int load(struct ClientLua *lua)
{
    int result, top = lua_gettop(lua->L);
    lua_pushliteral(lua->L, "stack sentinel");
    result = kafra_services_load(lua);
    assert(lua_gettop(lua->L) == top + 1 && !strcmp(lua_tostring(lua->L, -1), "stack sentinel"));
    lua_pop(lua->L, 1); return result;
}
static const char *dataset =
    "function ReadKafraMoveMapServiceInfo() "
    " repeat_load() "
    " AddItmeTokafraServiceMapList('프론테라.rsw','이즈루드.rsw') "
    " AddItmeTokafraServiceMapList('프론테라.rsw',string.rep('x',300)) "
    " AddItmeTokafraServiceMapList('프론테라.rsw','이즈루드.rsw') "
    " for i=1,70 do AddItmeTokafraServiceMapList('map'..i,'stop'..i) end "
    " collectgarbage('collect') end "
    "function ReadKafraMiniMapPosInfo() "
    " AddItmeToExceptionMiniMapPosList('프론테라.rsw',1.9,-2.9) "
    " AddItmeToExceptionMiniMapPosList('프론테라.rsw',8.9,-9.9) "
    " AddItmeToExceptionMiniMapPosList('limits',-2147483648,2147483647) "
    " for i=1,70 do AddItmeToExceptionMiniMapPosList('map'..i,i,-i) end end";
static void verify(void)
{
    assert(kafra_destination_count("프론테라.rsw") == 3);
    assert(!strcmp(kafra_destination("프론테라.rsw", 0), "이즈루드.rsw"));
    assert(strlen(kafra_destination("프론테라.rsw", 1)) == 300);
    assert(!strcmp(kafra_destination("프론테라.rsw", 2), "이즈루드.rsw"));
    assert(!kafra_destination("프론테라.rsw", 3));
    assert(!kafra_destination(NULL, 0));
    assert(kafra_destination_count("map70") == 1 && !strcmp(kafra_destination("map70", 0), "stop70"));
}
struct LuaAllocator { lua_Alloc original; void *context; int remaining; };
static void *lua_allocator(void *context, void *pointer, size_t old, size_t size)
{
    struct LuaAllocator *state = context;
    if (size > old) {
        if (state->remaining == 0) return NULL;
        if (state->remaining > 0) --state->remaining;
    }
    return state->original(state->context, pointer, old, size);
}
int main(void)
{
    struct ClientLua lua = {luaL_newstate()};
    struct LuaAllocator allocator;
    int i, total, live, succeeded = 0, lua_failures = 0;
    const char *bad[] = {
        "error('reader failure')", "AddItmeToExceptionMiniMapPosList('bad',0/0,1)",
        "AddItmeToExceptionMiniMapPosList('bad',2147483648,1)",
        "AddItmeToExceptionMiniMapPosList('bad',-2147483649,1)",
        "AddItmeToExceptionMiniMapPosList('bad',1,math.huge)",
        "AddItmeToExceptionMiniMapPosList('bad',{},1)",
        "AddItmeTokafraServiceMapList('bad'..string.char(0),'stop')"
    };
    char reader[512];
    assert(lua.L); luaL_openlibs(lua.L); map_list_register_lua_functions(lua.L);
    lua_register(lua.L, "repeat_load", recursive_load);
    assert(!kafra_services_load(NULL)); assert(!kafra_destination_count("missing"));
    script(lua.L, dataset); allocation_calls = 0; assert(load(&lua)); total = allocation_calls; verify();
    assert(total > 280); live = allocations;
    assert(load(&lua)); verify(); assert(allocations == live);
    for (i = 0; i < total; ++i) {
        fail_after = i; assert(!load(&lua)); fail_after = -1;
        verify(); assert(allocations == live);
    }
    for (i = 0; i < (int)(sizeof bad / sizeof *bad); ++i) {
        sprintf_s(reader, sizeof reader, "function ReadKafraMiniMapPosInfo() %s end", bad[i]);
        script(lua.L, reader); assert(!load(&lua)); verify(); assert(allocations == live);
    }
    script(lua.L, "ReadKafraMiniMapPosInfo=nil"); assert(!load(&lua)); verify();
    script(lua.L, dataset);
    allocator.original = lua_getallocf(lua.L, &allocator.context);
    for (i = 0; i < 2000 && !succeeded; ++i) {
        int top = lua_gettop(lua.L);
        allocator.remaining = i; lua_setallocf(lua.L, lua_allocator, &allocator);
        succeeded = kafra_services_load(&lua);
        lua_setallocf(lua.L, allocator.original, allocator.context);
        if (!succeeded) ++lua_failures;
        assert(lua_gettop(lua.L) == top); verify(); assert(allocations == live);
    }
    assert(succeeded && lua_failures > 0);
    script(lua.L, "AddItmeTokafraServiceMapList('direct','new')");
    assert(!strcmp(kafra_destination("direct", 0), "new"));
    script(lua.L, "function ReadKafraMoveMapServiceInfo() end function ReadKafraMiniMapPosInfo() end");
    assert(load(&lua) && !kafra_destination_count("프론테라.rsw") && allocations == 0);
    kafra_positions_clear(); kafra_destinations_clear(); kafra_positions_clear(); kafra_destinations_clear();
    assert(allocations == 0 && diagnostics > total);
    lua_close(lua.L);
    puts("Kafra: real Lua, owned strings, ordered destinations, numeric bounds, transactional reload, allocation rollback and cleanup passed");
    return 0;
}
