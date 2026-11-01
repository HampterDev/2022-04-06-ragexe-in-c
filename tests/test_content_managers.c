#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contents/client_lua.h"
#include "core/map_sky.h"
#include "fileio/file.h"
#include <lauxlib.h>
#include <lualib.h>
#undef malloc
#undef free
#undef luaL_newstate

static struct Block { unsigned char *raw; size_t size; } blocks[8192];
static unsigned live_blocks, allocation_count, close_count;
static size_t lua_bytes;
static int fail_manager, fail_state, missing_file;
static unsigned trace_hash = 2166136261u;
static FILE *trace;
unsigned int g_codePage = 949;
struct GameFileMgr g_fileMgr;

void *test_heap_alloc(size_t size)
{
    unsigned i;
    for (i = 0; i < 8192 && blocks[i].raw; ++i) {}
    assert(i < 8192);
    blocks[i].raw = malloc(size + 32); assert(blocks[i].raw);
    blocks[i].size = size;
    memset(blocks[i].raw, 0xa5, size + 32);
    ++live_blocks; ++allocation_count;
    return blocks[i].raw + 16;
}
void test_heap_free(void *memory)
{
    unsigned i, j;
    if (!memory) return;
    for (i = 0; i < 8192 && (!blocks[i].raw || blocks[i].raw + 16 != memory); ++i) {}
    assert(i < 8192 && live_blocks);
    for (j = 0; j < 16; ++j) {
        assert(blocks[i].raw[j] == 0xa5);
        assert(blocks[i].raw[16 + blocks[i].size + j] == 0xa5);
    }
    free(blocks[i].raw); blocks[i].raw = NULL; --live_blocks;
}
void *mem_alloc(size_t size) { return test_heap_alloc(size); }
void *mem_try_alloc(size_t size) { return fail_manager ? NULL : test_heap_alloc(size); }
void mem_free(void *memory) { test_heap_free(memory); }
void fatal_error(const char *message) { (void)message; abort(); }
void error_msg(const char *message, int flags) { (void)flags; fprintf(stderr, "%s\n", message); abort(); }

static void *lua_memory(void *context, void *memory, size_t old_size, size_t size)
{
    void *result;
    (void)context;
    if (!memory) old_size = 0;
    assert(lua_bytes >= old_size);
    if (!size) { free(memory); lua_bytes -= old_size; return NULL; }
    result = realloc(memory, size);
    assert(result); lua_bytes = lua_bytes - old_size + size;
    return result;
}
lua_State *test_lua_newstate(void) { return fail_state ? NULL : lua_newstate(lua_memory, NULL); }
void client_lua_register_functions(struct ClientLua *self) { assert(self && self->L); }

struct File *file_construct(struct File *file) { memset(file,0,sizeof *file); return file; }
unsigned char file_open(struct File *file, const char *name, int mode, unsigned char resolve, unsigned char disk)
{
    const char *text;
    assert(mode == 0 && resolve == 1 && disk == 0);
    if (missing_file) return 0;
    if (strstr(name,"MapSkyData")) text="sky_loaded=true";
    else { assert(!"unexpected resource file"); return 0; }
    file->buffer=(unsigned char *)text; file->size=(int)strlen(text);
    return 1;
}
const unsigned char *file_consume(struct File *file, size_t count, size_t size)
{
    assert(size == 1 && count == (size_t)file->size && file->position == 0);
    file->position=file->size; return file->buffer;
}
void file_destruct(struct File *file) { (void)file; }
unsigned char file_make_file_name(const char *name,int disk) { assert(name && !disk); return 1; }

#ifdef CONTENT_MANAGERS_REFERENCE
struct MapSkyMgr *g_mapSkyMgrInstance;
void *map_sky_mgr_instance(void);
static void map_sky_mgr_destroy(void)
{ if(g_mapSkyMgrInstance) CMapSkyMgr_Delete(g_mapSkyMgrInstance,1); g_mapSkyMgrInstance=NULL; }

#endif

static void record(unsigned value)
{
    unsigned shift;
    for(shift=0;shift<32;shift+=8) {
        unsigned char byte=(unsigned char)(value>>shift);
        trace_hash=(trace_hash^byte)*16777619u;
        if(trace) assert(fwrite(&byte,1,1,trace)==1);
    }
}
static int closing(lua_State *state)
{
    unsigned owner=*(unsigned *)lua_touserdata(state,1);
    assert(owner == 0); ++close_count; record(0x100+owner);
    return 0;
}
static void watch_close(lua_State *state,unsigned owner)
{
    unsigned *value=lua_newuserdata(state,sizeof *value); *value=owner;
    lua_newtable(state); lua_pushcfunction(state,closing); lua_setfield(state,-2,"__gc");
    lua_setmetatable(state,-2); lua_setglobal(state,"fixture_owner");
}
static void destroy_all(void)
{
    map_sky_mgr_destroy();
    assert(!live_blocks && !lua_bytes);
}

static void populated_case(void)
{
    struct MapSkyMgr *sky=map_sky_mgr_instance();
    unsigned before=allocation_count;
    assert(sky);
    assert(map_sky_mgr_instance()==sky && allocation_count==before);
    watch_close(sky->lua->L,0);
    assert(map_sky_mgr_load(sky));lua_getglobal(sky->lua->L,"sky_loaded");assert(lua_toboolean(sky->lua->L,-1));lua_pop(sky->lua->L,1);
    missing_file=1;
    assert(!map_sky_mgr_load(sky));
    missing_file=0;
    destroy_all();before=allocation_count;destroy_all();assert(allocation_count==before);
}
int main(int argc,char **argv)
{
    unsigned before,i;
    if(argc>1) {trace=fopen(argv[1],"wb");assert(trace);}
    destroy_all();assert(!allocation_count);
    fail_manager=1;
    assert(!map_sky_mgr_instance());assert(!live_blocks && !lua_bytes);
    fail_manager=0;fail_state=1;
    assert(map_sky_mgr_instance());
    assert(!((struct MapSkyMgr *)map_sky_mgr_instance())->lua->L);
    destroy_all();fail_state=0;
    for(i=0;i<3;++i) populated_case();
    assert(close_count==3);
    assert(trace_hash==0xf0fa1abau);
    before=allocation_count;destroy_all();assert(allocation_count==before);
    if(trace) assert(fclose(trace)==0);
    printf("Content managers: real Lua, populated containers, failed loads, allocation failures and ordered cleanup passed; digest %08x.\n",trace_hash);
    return 0;
}

#include "content_managers_unreached.h"
