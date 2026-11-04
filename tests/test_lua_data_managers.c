#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "contents/champion_monster.h"
#include "contents/check_attendance.h"
#include "contents/pet_auto_feed.h"
#include "contents/private_airplane.h"
#include "session/state.h"
#include <lauxlib.h>
#undef malloc
#undef free
#undef luaL_newstate
#undef luaL_loadfile
#undef MessageBoxA
#undef platform_message_box

static struct Block { unsigned char *raw; size_t size; } blocks[8192];
static unsigned live_blocks, allocation_count, message_count;
static size_t lua_bytes;
static int fail_manager, fail_state, script_mode;
static unsigned trace_hash = 2166136261u;
static FILE *trace;
unsigned int g_codePage = 949;
struct Session g_session1;

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

const char *thread_text_format(const char *format, ...)
{
    static char buffer[2048];
    va_list args;
    int length;
    va_start(args,format); length=vsnprintf(buffer,sizeof buffer,format,args); va_end(args);
    assert(length>=0 && (size_t)length<sizeof buffer); return buffer;
}

#ifdef LUA_DATA_MANAGERS_REFERENCE
struct CheckAttendanceMgr *g_attendanceTable;
struct PrivateAirplaneMgr *g_airshipTickets;
const void *const champion_monster_ops[1] = {champion_monster_delete};
const void *const check_attendance_ops[1] = {check_attendance_delete};
const void *const private_airplane_ops[1] = {private_airplane_delete};
#endif

enum { SCRIPT_VALID, SCRIPT_MISSING, SCRIPT_FALSE_RESULT, SCRIPT_BAD_CHUNK, SCRIPT_NO_MAIN, SCRIPT_DUPLICATE };
static const char champion_script[] =
    "function main() "
    "assert(not AddItem('bad',1)); assert(not AddEffect(1001,'bad')); "
    "assert(AddEffect(9999,12)); "
    "for job=1001,1003 do assert(AddItem(job,job-1000.5)); "
    "assert(AddItem(job,99)); for effect=-2,2 do assert(AddEffect(job,effect)) end end "
    "return true,'good' end";
static const char attendance_script[] =
    "function main() "
    "assert(not InsertCheckAttendanceConfig('bad',1)); "
    "assert(InsertCheckAttendanceConfig(20261001,20261031)); "
    "assert(not InsertCheckAttendanceReward(1,0,1)); "
    "assert(not InsertCheckAttendanceReward(1,501,0)); "
    "for day=1,3 do assert(InsertCheckAttendanceReward(day,500+day,day*2)) end "
    "return true,'good' end";
static const char pet_script[] =
    "function main() "
    "assert(InsertEvolutionRecipeLGU(1,9002,501,1)); "
    "assert(InsertEvolutionRecipeLGU(9001,2,501,1)); "
    "assert(InsertEvolutionRecipeLGU(9001,9002,501,0)); "
    "for target=9002,9004 do for material=501,503 do "
    "assert(InsertEvolutionRecipeLGU(9001,target,material,material-500)) end end "
    "assert(InsertEvolutionRecipeLGU(9001,9002,501,5)); "
    "assert(not InsertPetAutoFeeding(false)); assert(not InsertPetAutoFeeding(9000)); "
    "assert(InsertPetAutoFeeding(9001)); assert(InsertPetAutoFeeding(9001)); "
    "assert(InsertPetAutoFeeding(9499)); return true,'good' end";
static const char airplane_script[] =
    "function main() "
    "assert(not InsertStartableMap(false)); assert(not InsertEndableMap({})); "
    "assert(InsertStartableMap('prontera')); assert(InsertStartableMap('prontera')); "
    "assert(InsertStartableMap('한국어_긴_이름_map_123456')); "
    "assert(InsertEndableMap('izlude')); assert(InsertEndableMap('한국어_긴_이름_map_123456')); "
    "assert(InsertNeedItem(503,'한국어 이름 long item name')); "
    "assert(InsertNeedItem(501,'short')); assert(InsertNeedItem(502,'second')); "
    "assert(InsertNeedItem(501,'duplicate')); return true,'good' end";
static const char duplicate_script[] =
    "function main() assert(InsertCheckAttendanceReward(1,501,2)); "
    "local ok,message=InsertCheckAttendanceReward(1,502,3); "
    "assert(not ok); assert(message=='[InsertCheckAttendanceReward] Check RewardInfo : 1 [502] 3'); "
    "return true,'good' end";

int test_lua_loadfile(lua_State *L, const char *filename)
{
    const char *source;
    char failed_script[4096];
    int length;
    if (script_mode == SCRIPT_MISSING) { lua_pushliteral(L,"fixture: missing file"); return LUA_ERRFILE; }
    if (script_mode == SCRIPT_BAD_CHUNK) return luaL_loadstring(L,"this is not Lua!");
    if (script_mode == SCRIPT_NO_MAIN) return luaL_loadstring(L,"loaded=true");
    if (script_mode == SCRIPT_DUPLICATE) {
        assert(strstr(filename,"CheckAttendance")); source=duplicate_script;
    } else if (strstr(filename,"monster_size")) source=champion_script;
    else if (strstr(filename,"CheckAttendance")) source=attendance_script;
    else if (strstr(filename,"PetEvolution")) source=pet_script;
    else if (strstr(filename,"PrivateAirplane")) source=airplane_script;
    else { assert(!"unexpected resource"); return LUA_ERRFILE; }
    if (script_mode == SCRIPT_FALSE_RESULT) {
        length=snprintf(failed_script,sizeof failed_script,
            "%s local populate=main function main() populate() return false,'fixture failure' end",source);
        assert(length>=0 && (size_t)length<sizeof failed_script);
        source=failed_script;
    }
    return luaL_loadstring(L,source);
}
void test_message_box(HWND window, const char *message, const char *caption, unsigned flags)
{
    assert(!window && !flags && message && caption); ++message_count;
}
static void emit(unsigned value)
{
    unsigned i;
    for(i=0;i<4;++i) { trace_hash^=(value>>(i*8))&255; trace_hash*=16777619u; }
    if(trace) assert(fwrite(&value,sizeof value,1,trace)==1);
}
static void empty(void)
{
    assert(!g_championMonster && !g_attendanceTable && !g_petAutoFeed && !g_airshipTickets);
    assert(!live_blocks && !lua_bytes);
}
static void destroy_all(void)
{
    pet_auto_feed_destroy(); private_airplane_destroy(); check_attendance_destroy(); champion_monster_destroy();
    empty();
}
static struct String text(const char *value)
{
    struct String s; string_init(&s); string_assign_cstr(&s, value); return s;
}
static void populated(void)
{
    struct AttendanceItem reward;
    struct ChampionMonster *node;
    unsigned before;
    int i,job;
    script_mode=SCRIPT_VALID;
    assert(champion_monster_create()); assert(champion_monster_load()); assert(!lua_bytes);
    assert(check_attendance_create()); assert(pet_auto_feed_create()); assert(private_airplane_create());
    assert(!lua_bytes && live_blocks && !message_count);
    before=allocation_count;
    assert(!champion_monster_create() && !check_attendance_create() && !pet_auto_feed_create() && !private_airplane_create());
    assert(allocation_count==before);
    for(job=1001;job<=1003;++job) {
        assert(champion_monster_is_champion(g_championMonster,job));
        assert(champion_monster_size(g_championMonster,job)==job-1000.5f);
        emit((unsigned)(champion_monster_size(g_championMonster,job)*2));
        node=tree_find(&g_championMonster->jobs,&job,tree_less_int);
        assert(VEC_COUNT(&node->data.effects,int)==5);
        for(i=0;i<5;++i) { assert(((int *)node->data.effects.first)[i]==i-2); emit((unsigned)(i-2)); }
    }
    assert(!champion_monster_is_champion(g_championMonster,9999));
    assert(champion_monster_size(g_championMonster,9999)==0);
    assert(g_attendanceTable->rewards.size==3); emit(g_attendanceTable->rewards.size);
    for(i=0;i<=4;++i) {
        assert(check_attendance_reward(g_attendanceTable,&reward,i)==&reward);
        assert(reward.item_id==(i>=1 && i<=3 ? 500+i : 0));
        assert(reward.count==(i>=1 && i<=3 ? i*2 : 0));
        emit(reward.item_id);emit(reward.count);
    }
    assert(pet_auto_feed_is_allowed(g_petAutoFeed,9001) && pet_auto_feed_is_allowed(g_petAutoFeed,9499));
    assert(!pet_auto_feed_is_allowed(g_petAutoFeed,9002));
    assert(VEC_COUNT(&g_petAutoFeed->auto_feeding,int)==2);
    assert(((int *)g_petAutoFeed->auto_feeding.first)[0]==9001);
    assert(((int *)g_petAutoFeed->auto_feeding.first)[1]==9499);
    emit(9001);emit(9499);
    assert(g_airshipTickets->need_items.size==3);
    for(i=0;i<3;++i) { assert(private_airplane_need_item(g_airshipTickets,i)==501+i); emit(private_airplane_need_item(g_airshipTickets,i)); }
    assert(private_airplane_can_start(g_airshipTickets,text("prontera.gat")));
    assert(!private_airplane_can_start(g_airshipTickets,text("izlude.gat")));
    assert(private_airplane_can_end(g_airshipTickets,text("izlude.gat")));
    assert(private_airplane_can_end(g_airshipTickets,text("한국어_긴_이름_map_123456.gat")));
    assert(private_airplane_can_start(g_airshipTickets,text("한국어_긴_이름_map_123456.gat")));
    script_mode=SCRIPT_MISSING;before=message_count;
    assert(!champion_monster_load());
    assert(message_count==before+1 && !lua_bytes);
    assert(g_championMonster->jobs.size==3 && g_attendanceTable->rewards.size==3 && g_airshipTickets->need_items.size==3);
    emit(3); destroy_all(); destroy_all(); message_count=0;
}
static void failed_loads(void)
{
    int mode;
    for(mode=SCRIPT_MISSING;mode<=SCRIPT_NO_MAIN;++mode) {
        script_mode=mode;message_count=0;
        assert(champion_monster_create());assert(!champion_monster_load());champion_monster_destroy();empty();
        assert(!check_attendance_create());empty();assert(!pet_auto_feed_create());empty();assert(!private_airplane_create());empty();
        assert(message_count==4);emit(message_count);
    }
    message_count=0;script_mode=SCRIPT_VALID;fail_state=1;
    assert(champion_monster_create());assert(!champion_monster_load());champion_monster_destroy();empty();
    assert(!check_attendance_create() && !pet_auto_feed_create() && !private_airplane_create());empty();
    assert(!message_count);fail_state=0;fail_manager=1;
    assert(!champion_monster_create() && !check_attendance_create() && !pet_auto_feed_create() && !private_airplane_create());empty();
    fail_manager=0;emit(0);
}
static void duplicate_reward(void)
{
    struct AttendanceItem reward;
    script_mode=SCRIPT_DUPLICATE;message_count=0;
    assert(check_attendance_create());
    assert(g_attendanceTable->rewards.size==1 && !message_count);
    check_attendance_reward(g_attendanceTable,&reward,1);assert(reward.item_id==501 && reward.count==2);
    check_attendance_destroy();empty();
}
int main(int argc,char **argv)
{
    int i;
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    if(argc==2 && !strcmp(argv[1],"--duplicate-only")) { duplicate_reward();puts("Duplicate attendance reward: numeric diagnostic and original reward preserved.");return 0; }
    if(argc==2) { trace=fopen(argv[1],"wb");assert(trace); }
    empty();destroy_all();
    for(i=0;i<3;++i) populated();
    failed_loads();populated();
#ifndef LUA_DATA_MANAGERS_REFERENCE
    duplicate_reward();
#endif
    if(trace) assert(fclose(trace)==0);
    assert(trace_hash==0x756fb6edu);
    printf("Lua data managers: real loaders, populated queries, owned results, failures and repeated cleanup passed; digest %08x.\n",trace_hash);
    return 0;
}
#include "lua_data_managers_unreached.h"
