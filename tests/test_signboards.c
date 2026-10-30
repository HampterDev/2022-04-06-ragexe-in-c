#include "contents/client_lua.h"
#include <lauxlib.h>
#include <lualib.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fail_after = -1, allocations, errors;
static char error_function[80];
static void *checked_malloc(size_t size)
{
    unsigned char *raw;
    if (fail_after == 0)
        return NULL;
    if (fail_after > 0)
        --fail_after;
    assert(size < 1000000);
    raw = malloc(size + 24); assert(raw);
    *(size_t *)raw = size;
    memset(raw + 8, 0x63, 8); memset(raw + 16 + size, 0x57, 8);
    ++allocations;
    return raw + 16;
}
static void checked_free(void *p)
{
    unsigned char *raw;
    size_t size, i;
    if (!p)
        return;
    raw = (unsigned char *)p - 16; size = *(size_t *)raw;
    for (i = 0; i < 8; ++i) { assert(raw[8 + i] == 0x63); assert(raw[16 + size + i] == 0x57); }
    memset(raw, 0xee, size + 24); free(raw); --allocations;
}
static void *checked_calloc(size_t count, size_t size)
{
    void *p = checked_malloc(count * size);
    if (p)
        memset(p, 0, count * size);
    return p;
}

#define malloc checked_malloc
#define calloc checked_calloc
#define free checked_free
#include "../src/ui/quests/signboard_data.c"
#undef malloc
#undef calloc
#undef free

void client_lua_loadfile_hook(const char *filename) { (void)filename; abort(); }
void client_lua_error_box(struct ClientLua *holder, const char *caption, const char *format, ...)
{
    va_list arguments;
    assert(holder && holder->L && strcmp(format, "%s\n") == 0);
    va_start(arguments, format); assert(va_arg(arguments, const char *) != NULL); va_end(arguments);
    strcpy_s(error_function, sizeof error_function, caption); ++errors;
}

static const char source[] =
    "function GetSignBoardListTable(i) "
    " collectgarbage('collect') "
    " if i==1 then return nil,0,0,0,0,string.rep('d',5000)..'.bmp',string.rep('t',4500)..'기본','0xffffff' end "
    " if i==2 then return 'prontera'..i,1,2,10,3,nil,nil,nil end "
    " if i==3 then return 'geffen'..i,-1,8,3,2,'icon.spr',nil,nil end "
    " return 'payon'..i,4,5,-4,99,'유저인터페이스\\\\picture.bmp',nil,nil "
    "end "
    "function GetTableSizeForC(name) assert(name=='SignBoardList');collectgarbage('collect');return 4 end ";

static void run(lua_State *L, const char *script)
{
    assert(luaL_dostring(L, script) == 0);
}
static void stack_ok(lua_State *L)
{
    assert(lua_gettop(L) == 2);
    assert(lua_tonumber(L, 1) == 1234 && strcmp(lua_tostring(L, 2), "keep me") == 0);
}
static void check_rows(void)
{
    const struct SignBoardEntry *row;
    assert(signboards_count() == 3 && !signboards_entry(3) && !signboards_entry((size_t)-1));
    row = signboards_entry(0);
    assert(row->type == SIGNBOARD_BOARD && row->x == 1 && row->y == 2 && row->height == 7);
    assert(strcmp(row->map, "prontera2") == 0 && strlen(row->image) == 5004);
    assert(strcmp(row->image + 5000, ".bmp") == 0 && strcmp(row->text + 4500, "기본") == 0);
    assert(strcmp(row->colour, "0xffffff") == 0);
    row = signboards_entry(1);
    assert(row->type == SIGNBOARD_ICON && row->height == 0 && row->x == -1 && row->y == 8);
    assert(strcmp(row->image, "icon.spr") == 0 && !row->text && !row->colour);
    row = signboards_entry(2);
    assert(row->type == SIGNBOARD_PICTURE && row->height == -7);
    assert(strcmp(row->map, "payon4") == 0 && strcmp(row->image, "유저인터페이스\\picture.bmp") == 0);
}

int main(void)
{
    struct ClientLua holder;
    lua_State *L = luaL_newstate();
    const struct SignBoardEntry *previous;
    int i;
    const char *bad_counts[] = { "-1", "1/0", "0/0", "2147483648", "2147483647", "2.5", "nil", "'wrong'" };
    char script[256];
    assert(L); holder.L = L; luaL_openlibs(L);
    lua_pushnumber(L, 1234); lua_pushliteral(L, "keep me");
    assert(!signboards_load(NULL)); holder.L = NULL; assert(!signboards_load(&holder)); holder.L = L;
    run(L, source); assert(signboards_load(&holder)); stack_ok(L); check_rows();
    assert(allocations == 4);
    previous = signboards_entry(0);
    for (i = 0; i < 5; ++i) {
        fail_after = i; assert(!signboards_load(&holder)); fail_after = -1;
        assert(signboards_entry(0) == previous && allocations == 4); check_rows(); stack_ok(L);
    }
    for (i = 0; i < sizeof bad_counts / sizeof bad_counts[0]; ++i) {
        sprintf_s(script, sizeof script, "function GetTableSizeForC() return %s end", bad_counts[i]);
        run(L, script); assert(!signboards_load(&holder));
        assert(signboards_entry(0) == previous && allocations == 4); stack_ok(L);
    }
    run(L, source);
    run(L, "old=GetSignBoardListTable;function GetSignBoardListTable(i) if i==3 then error('row failure') end return old(i) end");
    assert(!signboards_load(&holder)); assert(errors == 1 && strcmp(error_function, "GetSignBoardListTable") == 0);
    check_rows(); stack_ok(L); assert(allocations == 4);
    run(L, source); run(L, "GetTableSizeForC=nil"); assert(!signboards_load(&holder));
    assert(errors == 2 && strcmp(error_function, "GetTableSizeForC") == 0); check_rows(); stack_ok(L);
    run(L, source); run(L, "function GetTableSizeForC() error({}) end"); assert(!signboards_load(&holder));
    assert(errors == 3); check_rows(); stack_ok(L);
    run(L, source);
    run(L, "old=GetSignBoardListTable;function GetSignBoardListTable(i) if i==3 then return nil,0,0,0,1,'x' end return old(i) end");
    assert(!signboards_load(&holder)); check_rows(); stack_ok(L);
    run(L, source);
    run(L, "old=GetSignBoardListTable;function GetSignBoardListTable(i) if i==3 then return 'x',0,0,1/0,1,'x' end return old(i) end");
    assert(!signboards_load(&holder)); check_rows(); stack_ok(L);
    run(L, source);
    run(L, "function GetSignBoardListTable(i) if i==1 then return end return 'x',0,0,0,3 end");
    assert(!signboards_load(&holder)); check_rows(); stack_ok(L);
    run(L, source);
    run(L, "old=GetSignBoardListTable;function GetSignBoardListTable(i) if i==3 then return 'x',0,0,-2147483648,1,'x' end return old(i) end");
    assert(!signboards_load(&holder)); check_rows(); stack_ok(L);
    run(L, source); assert(signboards_load(&holder)); assert(allocations == 4);
    lua_close(L); check_rows();
    signboards_clear(); signboards_clear(); assert(!allocations && !signboards_count());
    L = luaL_newstate(); assert(L); holder.L = L; luaL_openlibs(L);
    run(L, source); run(L, "function GetTableSizeForC() return 201 end");
    assert(signboards_load(&holder)); assert(signboards_count() == 200 && allocations == 201);
    assert(strcmp(signboards_entry(199)->map, "payon201") == 0);
    for (i = 0; i < 20; ++i) { assert(signboards_load(&holder)); assert(allocations == 201); }
    run(L, "function GetTableSizeForC() return 1 end"); assert(signboards_load(&holder));
    assert(!signboards_count() && !allocations && lua_gettop(L) == 0);
    signboards_clear(); lua_close(L);
    puts("Signboards: real Lua queries, GC-safe strings, defaults, bounds, stack preservation, transactional reload and cleanup passed");
    return 0;
}
