#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "session/state.h"
#include "contents/client_lua.h"
#include <lauxlib.h>
#include <lualib.h>

static int allocations, fail_after = -1, attempts, diagnostics, sent;
static unsigned char wire[64];
static int wire_size;
struct Session g_session1;
static void *checked_malloc(size_t size)
{
    unsigned char *p;
    ++attempts;
    if (!fail_after) return NULL;
    if (fail_after > 0) --fail_after;
    p = malloc(size + 24); assert(p);
    *(size_t *)p = size; memset(p + 8, 0x63, 8);
    memset(p + 16, 0xcc, size); memset(p + 16 + size, 0x57, 8);
    ++allocations; return p + 16;
}
static void checked_free(void *pointer)
{
    unsigned char *p; size_t size, i;
    if (!pointer) return;
    p = (unsigned char *)pointer - 16; size = *(size_t *)p;
    for (i = 0; i < 8; ++i) { assert(p[8+i] == 0x63); assert(p[16+size+i] == 0x57); }
    free(p); --allocations;
}
static void *checked_calloc(size_t count, size_t size)
{
    void *p = checked_malloc(count * size);
    if (p) memset(p, 0, count * size);
    return p;
}
static void *checked_realloc(void *pointer, size_t size)
{
    void *p = checked_malloc(size);
    if (p && pointer) {
        size_t old = *(size_t *)((unsigned char *)pointer - 16);
        memcpy(p, pointer, old < size ? old : size); checked_free(pointer);
    }
    return p;
}
#define malloc checked_malloc
#define calloc checked_calloc
#define realloc checked_realloc
#define free checked_free
#include "../src/ui/social/entry_queue_catalog.c"
#include "../src/ui/social/entry_queue_mgr.c"
#undef malloc
#undef calloc
#undef realloc
#undef free

struct RagConnection *rag_connection_instance(void) { return (void *)1; }
char rag_connection_send_packet(struct RagConnection *connection, int size, const void *packet)
{
    assert(connection == (void *)1 && size > 0 && size <= sizeof wire);
    memcpy(wire, packet, size); wire_size = size; ++sent; return 1;
}
void client_lua_loadfile_hook(const char *name) { (void)name; abort(); }
void client_lua_error_box(struct ClientLua *lua, const char *caption, const char *format, ...)
{
    char text[500]; va_list arguments; int size;
    assert(lua && lua->L && caption && format);
    va_start(arguments,format);size=vsnprintf(text,sizeof text,format,arguments);va_end(arguments);
    assert(size>=0 && size<sizeof text);++diagnostics;
}
static void script(lua_State *L, const char *text) { assert(luaL_dostring(L, text) == 0); }
static int recursive_load(lua_State *L)
{
    struct ClientLua lua = {L}; int top = lua_gettop(L);
    assert(!entry_queue_catalog_load(&lua) && lua_gettop(L) == top);
    return 0;
}
static int load(struct ClientLua *lua)
{
    int result, top = lua_gettop(lua->L);
    lua_pushliteral(lua->L, "stack sentinel"); result = entry_queue_catalog_load(lua);
    assert(lua_gettop(lua->L) == top + 1 && !strcmp(lua_tostring(lua->L, -1), "stack sentinel"));
    lua_pop(lua->L, 1); return result;
}
static const char *dataset =
    "function add(id,name) AddEntry(id,name,'title',3.9,-4.9,1,2,255,256,2,11,-12,'four','five','six','one','picture') end "
    "function ReadEntryQueueList() repeat_load() "
    "add(7,'queue') add(7,'duplicate') add(-1,'last') "
    "for i=70,1,-1 do add(i,'queue'..i) end collectgarbage('collect') end";
static void verify(void)
{
    struct EntryQueueInfo *p = entry_queue_find(7); unsigned i;
    assert(entry_queue_count() == 71 && p && !strcmp(p->name, "queue"));
    for (i = 0; i < 70; ++i) assert(entry_queue_at(i)->id == (int)i+1);
    assert(entry_queue_at(70)->id == -1 && !entry_queue_at(71) && !entry_queue_find(0));
    assert(p->member_limit == 3 && p->enemy_limit == -4 && p->option_flags == 0x00ff0201);
    assert(p->kind == 2 && p->kind_arg1 == 11 && p->kind_arg2 == -12);
    assert(!strcmp(p->text_row4,"four") && !strcmp(p->text_row5,"five"));
    assert(!strcmp(p->text_row6,"six") && !strcmp(p->text_row1,"one") && !strcmp(p->picture,"picture"));
}
struct LuaAllocator { lua_Alloc original; void *context; int remaining; };
static void *lua_allocator(void *context, void *pointer, size_t old, size_t size)
{
    struct LuaAllocator *state = context;
    if (size > old) { if (!state->remaining) return NULL; if (state->remaining > 0) --state->remaining; }
    return state->original(state->context, pointer, old, size);
}
static void packets(void)
{
    struct EntryQueueMgr *m = g_entryQueueMgr;
    static const unsigned char apply[] = {0xd7,8,2,0,'q','u','e','u','e',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    static const unsigned char cancel[] = {0xda,8,'q','u','e','u','e',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    entry_queue_mgr_apply(m, ENTRY_APPLY_PARTY, "queue");
    assert(wire_size == sizeof apply && !memcmp(wire,apply,sizeof apply));
    entry_queue_mgr_cancel(m); assert(wire_size == sizeof cancel && !memcmp(wire,cancel,sizeof cancel));
    entry_queue_mgr_request_ranking(m); assert(wire_size == 26 && wire[0] == 0x0a && wire[1] == 9 && !memcmp(wire+2,cancel+2,24));
    memset(m->lobby_name,'r',24); m->in_queue=1;
    entry_queue_mgr_answer(m,ENTRY_ANSWER_DECLINE);
    assert(wire_size==51 && wire[0]==0xe0 && wire[1]==8 && wire[2]==2 && !m->in_queue);
    assert(!memcmp(wire+3,cancel+2,24) && !memcmp(wire+27,m->lobby_name,24));
    entry_queue_mgr_apply(m,1,"abcdefghijklmnopqrstuvwxyz");
    assert(wire[27]==0 && !memcmp(wire+4,"abcdefghijklmnopqrstuvw",23));
    m->current=NULL; sent=0;
    entry_queue_mgr_cancel(m); entry_queue_mgr_request_ranking(m); entry_queue_mgr_answer(m,1);
    entry_queue_mgr_cancel(NULL); entry_queue_mgr_apply(NULL,1,"queue"); assert(!sent);
}
int main(void)
{
    struct ClientLua lua = {luaL_newstate()}; struct LuaAllocator allocator;
    struct EntryQueueInfo *selected; int i,total,live,succeeded=0,failures=0; char reader[512];
    const char *bad[]={"error('reader')","error(string.rep('x',2000))","add(0/0,'bad')","add(math.huge,'bad')",
        "add(2147483648,'bad')","add(-2147483649,'bad')","add({},'bad')","add(99,string.rep('x',260))"};
    assert(lua.L); luaL_openlibs(lua.L); lua_register(lua.L,"AddEntry",entry_queue_lua_add);
    lua_register(lua.L,"repeat_load",recursive_load); script(lua.L,dataset);
    g_session1.luaState=&lua;
    fail_after=0; assert(!entry_queue_mgr_create() && !g_entryQueueMgr && !allocations); fail_after=-1;
    assert(!entry_queue_catalog_load(NULL)); assert(entry_queue_mgr_create()); assert(!entry_queue_mgr_create()); verify();
    g_entryQueueMgr->current=entry_queue_find(7); attempts=0; assert(load(&lua)); total=attempts; live=allocations;
    assert(total>70 && g_entryQueueMgr->current==entry_queue_find(7)); selected=g_entryQueueMgr->current;
    for(i=0;i<total;++i) {
        fail_after=i; assert(!load(&lua)); fail_after=-1; verify();
        assert(allocations==live && g_entryQueueMgr->current==selected);
    }
    for(i=0;i<sizeof bad/sizeof *bad;++i) {
        sprintf_s(reader,sizeof reader,"function ReadEntryQueueList() add(99,'pending') %s end",bad[i]);
        script(lua.L,reader); assert(!load(&lua)); verify(); assert(allocations==live && g_entryQueueMgr->current==selected);
    }
    script(lua.L,"ReadEntryQueueList=nil"); assert(!load(&lua)); verify();
    script(lua.L,dataset); allocator.original=lua_getallocf(lua.L,&allocator.context);
    for(i=0;i<2000 && !succeeded;++i) {
        int top=lua_gettop(lua.L); allocator.remaining=i; lua_setallocf(lua.L,lua_allocator,&allocator);
        succeeded=entry_queue_catalog_load(&lua); lua_setallocf(lua.L,allocator.original,allocator.context);
        if(!succeeded) ++failures;
        assert(lua_gettop(lua.L)==top && allocations==live); verify();
    }
    assert(succeeded && failures>0); selected=g_entryQueueMgr->current;
    script(lua.L,"for i=71,150 do add(i,'new') end collectgarbage('collect')");
    assert(entry_queue_find(7)==selected && !strcmp(selected->name,"queue"));
    attempts=0; script(lua.L,"add(7,'duplicate')"); assert(!attempts);
    script(lua.L,"add(151,'prefix'..string.char(0)..'ignored')"); assert(!strcmp(entry_queue_find(151)->name,"prefix"));
    packets();
    script(lua.L,"function ReadEntryQueueList() end"); assert(load(&lua) && !entry_queue_count() && allocations==1);
    entry_queue_mgr_release(); entry_queue_mgr_release(); assert(!allocations && !g_entryQueueMgr);
    script(lua.L,"ReadEntryQueueList=nil"); assert(entry_queue_mgr_create() && !entry_queue_count());
    entry_queue_mgr_release(); assert(!allocations && diagnostics>total); lua_close(lua.L);
    puts("Entry queue: real Lua, ordered C catalog, stable rows, transactional reload, all allocation failures, packet bytes and repeat cleanup passed");
    return 0;
}
