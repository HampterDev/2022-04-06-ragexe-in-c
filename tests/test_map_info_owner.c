#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lauxlib.h>
#include <lualib.h>

static struct { void *pointer; size_t size; } blocks[20000];
static size_t live;
static int fail_after = -1;
static void *tracked_malloc(size_t size)
{
    void *p;
    if (fail_after == 0) return NULL;
    if (fail_after > 0) --fail_after;
    p = malloc(size); assert(p && live < 20000);
    blocks[live].pointer = p; blocks[live++].size = size;
    return p;
}
static void tracked_free(void *p)
{
    size_t i;
    if (!p) return;
    for (i=0; i<live && blocks[i].pointer!=p; ++i) {}
    assert(i<live);
    memset(p, 0xdd, blocks[i].size); free(p); blocks[i]=blocks[--live];
}
static void *tracked_calloc(size_t count, size_t size)
{
    void *p = tracked_malloc(count*size);
    if (p) memset(p, 0, count*size);
    return p;
}
static void *tracked_realloc(void *p, size_t size)
{
    size_t i;
    void *next=tracked_malloc(size);
    if (!next) return NULL;
    if (p) {
        for (i=0; i<live && blocks[i].pointer!=p; ++i) {}
        assert(i<live);
        memcpy(next,p,blocks[i].size<size?blocks[i].size:size);
        tracked_free(p);
    }
    return next;
}
#define malloc tracked_malloc
#define calloc tracked_calloc
#define realloc tracked_realloc
#define free tracked_free
#include "../src/contents/map_info.c"
#undef malloc
#undef calloc
#undef realloc
#undef free

static const char *lua_script = "";
static char default_name[64]="Unknown map";
char *msg_str(int id) { assert(id==0xbb); return default_name; }
int lua_data_reply(lua_State *L,int ok,const char *message)
{ lua_pushboolean(L,ok); lua_pushstring(L,message); return 2; }
char lua_data_run(const char *path,const char *owner,const struct LuaDataFunction *functions,
    int count,enum LuaDataCheck check)
{
    lua_State *L=luaL_newstate(); int status;
    assert(!strcmp(path,"system\\mapInfo_true.lub") && !strcmp(owner,"CMapInfoMgr"));
    assert(count==3 && check==LUA_DATA_IGNORE_RESULT); luaL_openlibs(L);
    for (int i=0;i<count;++i) lua_register(L,functions[i].name,functions[i].function);
    status=luaL_dostring(L,lua_script);
    if (status) fprintf(stderr,"Lua: %s\n",lua_tostring(L,-1));
    lua_close(L); return status==0;
}
static void reset(void)
{
    fail_after=-1; map_info_destroy(); map_info_destroy(); assert(!live && !g_mapInfo);
    lua_script="";
    assert(map_info_create()); assert(!map_info_create()); assert(map_info_load(g_mapInfo));
}
static void test_failures(void)
{
    int success;
    fail_after=0; assert(!map_info_create() && !g_mapInfo && !live); fail_after=-1;
    for (int existing=0;existing<2;++existing) {
        success=0;
        for (int n=0;n<6;++n) {
            size_t before;
            reset();
            if (existing)
                assert(map_info_set_display_name(g_mapInfo,"map","original"));
            before=live; fail_after=n;
            success=map_info_set_display_name(g_mapInfo,"map","changed");
            fail_after=-1;
            if (success) break;
            assert(live==before);
            assert(!strcmp(map_info_display_name(g_mapInfo,"map"),existing?"original":"Unknown map"));
        }
        assert(success);
    }
    reset(); fail_after=0; strcpy(default_name,"New default");
    assert(!map_info_load(g_mapInfo)); fail_after=-1;
    assert(!strcmp(map_info_display_name(g_mapInfo,"missing"),"Unknown map"));
    strcpy(default_name,"Unknown map");
}
static void test_storage(void)
{
    const char *borrowed;
    char key[64],value[64],input[32]="copied";
    reset(); assert(map_info_set_display_name(g_mapInfo,"stable",input));
    borrowed=map_info_display_name(g_mapInfo,"stable"); strcpy(input,"mutated");
    for (int i=2047;i>=0;--i) {
        sprintf(key,"map%04d.rsw",i); sprintf(value,"name-%04d",i);
        assert(map_info_set_display_name(g_mapInfo,key,value));
    }
    for (int i=0;i<2048;++i) {
        sprintf(key,"map%04d.rsw",i); sprintf(value,"name-%04d",i);
        assert(!strcmp(map_info_display_name(g_mapInfo,key),value));
    }
    assert(borrowed==map_info_display_name(g_mapInfo,"stable") && !strcmp(borrowed,"copied"));
    assert(map_info_set_display_name(g_mapInfo,"stable",borrowed));
    assert(map_info_set_display_name(g_mapInfo,"stable",""));
    assert(!*map_info_display_name(g_mapInfo,"stable"));
}
static void test_instances(void)
{
    const char *cases[]={"","@","a@","ab@","abc@","abcd@","abcde@map",
        "1234@map.rsw","prefix1234@map.rsw","1234@xx5678@map.rsw","a@b@c",
        "long_map_name_without_an_at.rsw","\xb0\xa1\xb0\xa2@map.rsw"};
    char normalized[1024], name[64];
    reset();
    for (size_t i=0;i<sizeof cases/sizeof cases[0];++i) {
        char *at;
        strcpy(normalized,cases[i]); at=strrchr(normalized,'@');
        if (at && at-normalized>=4) memmove(at-4,at-1,strlen(at-1)+1);
        sprintf(name,"value%u",(unsigned)i);
        assert(map_info_set_display_name(g_mapInfo,normalized,name));
        assert(!strcmp(map_info_display_name(g_mapInfo,cases[i]),name));
    }
    assert(map_info_set_display_name(g_mapInfo,"case","lower"));
    assert(map_info_set_display_name(g_mapInfo,"CASE","upper"));
    assert(!strcmp(map_info_display_name(g_mapInfo,"case"),"lower"));
    assert(!strcmp(map_info_display_name(g_mapInfo,"CASE"),"upper"));
}
static void test_lua(void)
{
    reset();
    lua_script=
        "local function check(f,msg,...) local ok,why=f(...); assert(not ok and why==msg) end "
        "check(AddMapDisplayName,'argument 1st is must string',{},'x') "
        "check(AddMapDisplayName,'argument 2nd is must string','x',{}) "
        "check(AddMapDisplayName,'argument 2nd is must string','x') "
        "assert(AddMapDisplayName('two','Two arguments')) "
        "assert(AddMapDisplayName('lua','Lua map',true)) "
        "assert(AddMapSignName('legacy-only','Lua first','Lua second')) "
        "assert(AddMapBackgroundBmp('legacy-only','Lua art')) "
        "assert(AddMapDisplayName('after','After old decoration calls',false)) "
        "assert(AddMapDisplayName(42,123,false)) "
        "assert(AddMapDisplayName('nul'..string.char(0)..'tail','head'..string.char(0)..'tail')) ";
    assert(map_info_load(g_mapInfo));
    assert(!strcmp(map_info_display_name(g_mapInfo,"two"),"Two arguments"));
    assert(!strcmp(map_info_display_name(g_mapInfo,"lua"),"Lua map"));
    assert(!strcmp(map_info_display_name(g_mapInfo,"after"),"After old decoration calls"));
    assert(!strcmp(map_info_display_name(g_mapInfo,"legacy-only"),"Unknown map"));
    assert(!strcmp(map_info_display_name(g_mapInfo,"42"),"123"));
    assert(!strcmp(map_info_display_name(g_mapInfo,"nul"),"head"));
    lua_script="local ok,why=AddMapDisplayName('lua','replacement'); assert(not ok and why=='out of memory')";
    fail_after=1; assert(map_info_load(g_mapInfo)); fail_after=-1;
    assert(!strcmp(map_info_display_name(g_mapInfo,"lua"),"Lua map"));
}
int main(void)
{
    test_failures(); test_storage(); test_instances(); test_lua();
    fail_after=-1; map_info_destroy(); map_info_destroy(); assert(!live);
    assert(!*map_info_display_name(NULL,"x"));
    assert(!map_info_load(NULL) && !map_info_set_display_name(NULL,"x","y"));
    puts("Map metadata: C ownership, moving storage, stable text, atomic failures, instance names, real Lua callbacks and legacy decoration compatibility passed.");
    return 0;
}
