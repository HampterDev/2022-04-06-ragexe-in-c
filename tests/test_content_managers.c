#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contents/enchant_mgr.h"
#include "contents/item_reform.h"
#include "contents/item_db_to_id.h"
#include "contents/repute_mgr.h"
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

static const char item_ids[] =
    "function ItemDB_To_ItemID(name) local ids={Base=501,Result=502,Material=503,Reformer=504} return ids[name] or 0 end";
static const char enchant_script[] =
    "function CheckFile() return true,'' end "
    "function LoadAllData() "
    "for id=1,3 do "
    "C_AddTargetItem(id,'Base'); C_SetCondition(id,2,1); C_ApproveRandomOption(id,true); "
    "C_SetSlotOrder(id,{1,2}); C_SetReset(id,true,95,700,{Material=3}); "
    "C_SetCaution(id,'한국어 안내 문구 with heap storage'); "
    "for slot=1,2 do "
    "C_SetRequire(id,slot,900,{Material=2}); C_SetSuccessRate(id,slot,80); "
    "C_SetGradeBonus(id,slot,{[1]=4,[2]=8}); C_SetEnchant(id,slot,1,{Base=10,Result=20}); "
    "C_AddPerfectEnchant(id,slot,'Base',1000,{Material=5}); "
    "C_AddUpgradeEnchant(id,slot,'Base','Result',1100,{Material=6}); end end "
    "return true,'' end";
static const char reform_script[] =
    "ReformInfo={} for id=1,3 do ReformInfo[id]={BaseItem='Base',ResultItem='Result',"
    "Material={Material=id},NeedRefineMin=1,NeedRefineMax=10,NeedOptionNumMin=0,"
    "IsEmptySocket=false,ChangeRefineValue=2,RandomOptionCode='long_random_option_code',"
    "PreserveSocketItem=true,PreserveGrade=true,InformationString={'한국어 안내 문구','second line'}} end "
    "ReformItemList={Reformer={1,2,3}}";

struct File *file_construct(struct File *file) { memset(file,0,sizeof *file); return file; }
unsigned char file_open(struct File *file, const char *name, int mode, unsigned char resolve, unsigned char disk)
{
    const char *text;
    assert(mode == 0 && resolve == 1 && disk == 0);
    if (missing_file) return 0;
    if (strstr(name,"ItemDBNameTbl")) text=item_ids;
    else if (strstr(name,"EnchantList_f")) text=enchant_script;
    else if (strstr(name,"EnchantList")) text="loaded=true";
    else if (strstr(name,"ItemReformSystem")) text=reform_script;
    else if (strstr(name,"MapSkyData")) text="sky_loaded=true";
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
struct EnchantMgr *g_enchantMgr;
struct ItemReformMgr *g_itemReformMgr;
struct MapSkyMgr *g_mapSkyMgrInstance;
const void *const g_enchant_mgr_ops[1] = {enchant_mgr_delete};
void *const vtbl_item_reform_mgr[1] = {item_reform_mgr_delete};
void *item_db_to_id_instance(void);
void *map_sky_mgr_instance(void);
static void map_sky_mgr_destroy(void)
{ if(g_mapSkyMgrInstance) CMapSkyMgr_Delete(g_mapSkyMgrInstance,1); g_mapSkyMgrInstance=NULL; }
static void enchant_mgr_destroy(void)
{ if(g_enchantMgr) enchant_mgr_delete(g_enchantMgr,1); g_enchantMgr=NULL; }
static void item_reform_mgr_destroy(void)
{ if(g_itemReformMgr) item_reform_mgr_delete(g_itemReformMgr,1); g_itemReformMgr=NULL; }
static void item_db_to_id_destroy(void)
{ if(g_item_db) item_db_to_id_delete(g_item_db,1); g_item_db=NULL; }
static void repute_mgr_destroy(void)
{ if(g_reputeMgr) repute_mgr_delete(g_reputeMgr,1); g_reputeMgr=NULL; }
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
    assert(owner == close_count%5); ++close_count; record(0x100+owner);
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
    map_sky_mgr_destroy(); enchant_mgr_destroy(); item_reform_mgr_destroy();
    item_db_to_id_destroy(); repute_mgr_destroy();
    assert(!live_blocks && !lua_bytes);
}
static void populate_repute(struct ReputeMgr *mgr)
{
    unsigned long long id;
    for(id=1;id<=3;++id) {
        struct ReputeInfo *info=&((struct ReputeInfoEntry *)tree_get(&mgr->infos,&id,sizeof id,sizeof(struct ReputeInfoEntry),tree_less_uint64))->info;
        struct ReputeGroup *group=&((struct ReputeGroupEntry *)tree_get(&mgr->groups,&id,sizeof id,sizeof(struct ReputeGroupEntry),tree_less_uint64))->group;
        string_assign_cstr(&info->name, "한국어 명성 이름 with heap storage");
        info->max_point_positive=100;info->max_point_negative=-100;info->visibility=REPUTE_SHOWN;
        info->highlighted=0;
        string_assign_cstr(&group->text, "한국어 명성 그룹 with heap storage");
        vec_push(&group->ids,&id,sizeof id);
        repute_mgr_set_points(mgr,id,30+(int)id);
        assert(info->points==30+(int)id);
        record((unsigned)info->points);
    }
    record(mgr->infos.size);record(mgr->groups.size);
}
static void populated_case(void)
{
    struct ItemDbToIdMgr *db=item_db_to_id_instance();
    struct EnchantMgr *enchant=enchant_mgr_instance();
    struct ItemReformMgr *reform=item_reform_mgr();
    struct ReputeMgr *repute=repute_mgr_instance();
    struct MapSkyMgr *sky=map_sky_mgr_instance();
    struct EnchantList *list;
    struct ReformInfo *info;
    struct Vec bases;
    unsigned before=allocation_count;
    assert(db && enchant && reform && repute && sky);
    assert(item_db_to_id_instance()==db && enchant_mgr_instance()==enchant && item_reform_mgr()==reform);
    assert(repute_mgr_instance()==repute && map_sky_mgr_instance()==sky && allocation_count==before);
    watch_close(sky->lua->L,0);watch_close(enchant->lua.L,1);watch_close(reform->lua.L,2);
    watch_close(db->script.L,3);watch_close(repute->lua.L,4);
    assert(item_db_to_id_load(db));assert(item_db_to_id_lookup(db,"Base")==501);
    assert(enchant_mgr_load(enchant));record(enchant->lists.size);
    list=enchant_mgr_find_list(enchant,2);assert(list && list->min_refine==2 && list->min_grade==1);
    assert(list->target_items.first && *(int *)list->target_items.first==501);
    assert(list->slots.size==2 && list->reset.materials.size==1 && list->caution.capacity>=16);
    record(list->slots.size);record(list->reset.zeny==700);
    assert(item_reform_mgr_load(reform));assert(item_reform_mgr_load(reform));
    assert(item_reform_is_reform_item(reform,504));
    item_reform_base_items(reform,&bases,504);
    assert(VEC_COUNT(&bases,int)==3);record((unsigned)VEC_COUNT(&bases,int));
    vec_free(&bases);
    info=item_reform_find(reform,504,501);assert(info && info->result_item==502 && info->materials.size==1);
    assert(info->random_option_code.capacity>=16 && info->information.capacity>=16);
    record(info->result_item);record(reform->reforms.size);record(reform->reform_items.size);
    populate_repute(repute);
    assert(map_sky_mgr_load(sky));lua_getglobal(sky->lua->L,"sky_loaded");assert(lua_toboolean(sky->lua->L,-1));lua_pop(sky->lua->L,1);
    missing_file=1;
    assert(!item_db_to_id_load(db));assert(!item_reform_mgr_load(reform));assert(!enchant_mgr_load(enchant));
    assert(!map_sky_mgr_load(sky));assert(reform->reforms.size==3 && enchant->lists.size==3);
    missing_file=0;
    destroy_all();before=allocation_count;destroy_all();assert(allocation_count==before);
}
int main(int argc,char **argv)
{
    unsigned before,i;
    if(argc>1) {trace=fopen(argv[1],"wb");assert(trace);}
    destroy_all();assert(!allocation_count);
    fail_manager=1;
    assert(!item_db_to_id_instance());assert(!enchant_mgr_instance());assert(!item_reform_mgr());
    assert(!repute_mgr_instance());assert(!map_sky_mgr_instance());assert(!live_blocks && !lua_bytes);
    fail_manager=0;fail_state=1;
    assert(item_db_to_id_instance() && enchant_mgr_instance() && item_reform_mgr() && repute_mgr_instance() && map_sky_mgr_instance());
    assert(!((struct ItemDbToIdMgr *)item_db_to_id_instance())->script.L);
    assert(!enchant_mgr_instance()->lua.L && !item_reform_mgr()->lua.L && !repute_mgr_instance()->lua.L);
    assert(!((struct MapSkyMgr *)map_sky_mgr_instance())->lua->L);
    destroy_all();fail_state=0;
    for(i=0;i<3;++i) populated_case();
    assert(close_count==15);
    assert(trace_hash==0x2b78921cu);
    before=allocation_count;destroy_all();assert(allocation_count==before);
    if(trace) assert(fclose(trace)==0);
    printf("Content managers: real Lua, populated containers, failed loads, allocation failures and ordered cleanup passed; digest %08x.\n",trace_hash);
    return 0;
}

#include "content_managers_unreached.h"
