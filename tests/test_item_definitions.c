#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

static int fail_after = -1, allocations, errors, loads, fail_state;
static const char *script;
static char last_error[1024];
static void *checked_malloc(size_t size)
{
    unsigned char *raw;
    if (fail_after == 0) return NULL;
    if (fail_after > 0) --fail_after;
    assert(size < 8000000);
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
    if (!p) return;
    raw = (unsigned char *)p - 16; size = *(size_t *)raw;
    for (i = 0; i < 8; ++i) { assert(raw[8 + i] == 0x63); assert(raw[16 + size + i] == 0x57); }
    memset(raw, 0xee, size + 24); free(raw); --allocations;
}
static void *checked_realloc(void *p, size_t size)
{
    void *next = checked_malloc(size);
    if (next && p) {
        size_t old = *(size_t *)((unsigned char *)p - 16);
        memcpy(next, p, old < size ? old : size);
        checked_free(p);
    }
    return next;
}
static void *checked_calloc(size_t count, size_t size)
{
    void *p = checked_malloc(count * size);
    if (p) memset(p, 0, count * size);
    return p;
}
static int WINAPI error_box(HWND window, LPCSTR text, LPCSTR caption, UINT type)
{
    assert(!window && text && strcmp(caption, "CItemInfoMgr") == 0 && !type);
    strncpy_s(last_error, sizeof last_error, text, _TRUNCATE); ++errors;
    return 0;
}
static lua_State *new_state(void) { return fail_state ? NULL : luaL_newstate(); }
static int load_file(lua_State *L, const char *name)
{
    ++loads; assert(strcmp(name, "System\\iteminfo_true.lub") == 0);
    if (!script) { lua_pushstring(L, "missing test script"); return LUA_ERRFILE; }
    return luaL_loadbuffer(L, script, strlen(script), name);
}
#define malloc checked_malloc
#define calloc checked_calloc
#define realloc checked_realloc
#define free checked_free
#define MessageBoxA error_box
#define luaL_loadfile load_file
#define luaL_newstate new_state
#include "../src/contents/item_info_mgr.c"
#include "../src/iteminfo/item_table.c"
#include "../src/iteminfo/item_tables.c"
#undef malloc
#undef calloc
#undef realloc
#undef free
#undef MessageBoxA
#undef luaL_loadfile
#undef luaL_newstate

#include "../src/iteminfo/item_loader.c"
void client_lua_loadfile_hook(const char *filename) { (void)filename; abort(); }
void *mem_alloc(size_t bytes) { void *p = checked_malloc(bytes); assert(p); return p; }
void mem_free(void *p)
{
    checked_free(p);
}
void *std_allocate_bytes(size_t bytes) { return bytes ? mem_alloc(bytes) : NULL; }

__declspec(noreturn) void fatal_error(const char *message) { puts(message); abort(); }

static const char normal[] =
    "function main() "
    "AddItemEffectInfo(3,99); AddItemIdentifiedDesc(3,'early'); "
    "AddItem(42,'old','old','old','old',9,9); AddItemIsCostume(42,true); "
    "AddItemUnidentifiedDesc(42,'미확인'); AddItemIdentifiedDesc(42,'literal\\\\nline'); "
    "AddItem(42,'미확인 사과','apple','사과','id_apple',2,500); "
    "AddItemIdentifiedDesc(42,string.rep('x',5000)..'끝'); "
    "for i=10,1,-1 do AddItem(i+100,'u'..i,'r'..i,'n'..i,'s'..i,0,0) end "
    "AddItem(-2147483648,'min','','min','',0,0); AddItem(2147483647,'max','','max','',0,0); "
    "collectgarbage('collect'); return true,'good' end";

static const struct ItemDefinition *entry_by_id(const struct ItemInfoData *data, int id)
{
    size_t i;
    for (i = 0; i < item_info_count(data); ++i) {
        const struct ItemDefinition *entry = item_info_entry(data, i);
        if (entry->id == id) return entry;
    }
    return NULL;
}
static void check_normal(struct ItemInfoData *data)
{
    const struct ItemDefinition *entry = entry_by_id(data, 42);
    assert(item_info_count(data) == 14 && !item_info_entry(data, 14) && !item_info_entry(NULL, 0));
    assert(item_info_entry(data,0)->id == INT_MIN && item_info_entry(data,13)->id == INT_MAX);
    assert(strcmp(entry->identified.name,"사과") == 0 && strcmp(entry->unidentified.name,"미확인 사과") == 0);
    assert(strcmp(entry->identified.resource,"id_apple") == 0 && strcmp(entry->unidentified.resource,"apple") == 0);
    assert(entry->slot_count == 2 && entry->class_num == 500 && entry->is_costume == 1 && entry->effect_id == 0);
    assert(entry->identified.line_count == 2 && strcmp(entry->identified.lines[0],"literal\\nline") == 0);
    assert(strlen(entry->identified.lines[1]) == 5002 && strcmp(entry->identified.lines[1]+5000,"끝") == 0);
    assert(entry->unidentified.line_count == 1 && strcmp(entry->unidentified.lines[0],"미확인") == 0);
    entry = entry_by_id(data, 3);
    assert(!strcmp(entry->identified.name,"") && !strcmp(entry->unidentified.resource,"") && entry->effect_id == 99 && !entry->is_costume);
}

static void test_companion_tables(void)
{
    static const int ids[] = { INT_MAX, -1, INT_MIN, 0 };
    int flags[8] = { 0, -1, 1, 2, 0, INT_MIN, INT_MAX, 0 };
    int changed[8] = {1, 0, 0, 0, 1, 0, 0, 1};
    struct CardAffix *card;
    const struct ItemMoveInfo *move;
    const struct ItemTextLines *lines;
    int i, n;
    card_affix_table_init(); book_item_table_init(); metal_process_table_init(); item_move_info_table_init();
    assert(!card_affix_has(42) && !book_item_has(42) && !card_affix_get(42)->illustName);
    assert(!metal_process_get(42)->count && !item_move_info_get(42)->item_id);
    for (i = 511; i >= 1; --i) {
        card = card_affix_at(i); card->illustName = "card"; card->prefix = "prefix";
        book_item_add(i); metal_process_append(i, "first"); item_move_info_add(i, flags);
    }
    for (i = 0; i < 4; ++i) {
        card_affix_at(ids[i]); book_item_add(ids[i]); metal_process_append(ids[i], "limit"); item_move_info_add(ids[i], flags);
    }
    for (i = 1; i <= 511; ++i) {
        assert(card_affix_has(i) && !strcmp(card_affix_get(i)->prefix, "prefix"));
        assert(book_item_has(i) && metal_process_get(i)->count == 1);
        move = item_move_info_get(i); assert(move->item_id == i);
        for (n = 0; n < 8; ++n) assert(move->flags[n] == (flags[n] != 0));
        book_item_add(i); item_move_info_add(i, changed);
        assert(item_move_info_get(i)->flags[0] == 0);
    }
    for (i = 0; i < 4; ++i) assert(card_affix_has(ids[i]) && book_item_has(ids[i]) && item_move_info_get(ids[i])->item_id == ids[i]);
    for (i = 0; i < 20; ++i) metal_process_append(42, i % 2 ? "odd" : "even");
    lines = metal_process_get(42); assert(lines->count == 21 && !strcmp(lines->data[0], "first"));
    for (i = 0; i < 20; ++i) assert(!strcmp(lines->data[i+1], i % 2 ? "odd" : "even"));
    card = card_affix_at(42); card->postfix = card->prefix; card->prefix = "";
    assert(!strcmp(card_affix_get(42)->postfix, "prefix"));
    card_affix_set_unknown();
    assert(!strcmp(card_affix_get(0)->illustName, "sorry") && !strcmp(card_affix_get(-1)->illustName, "sorry"));
    assert(!card_affix_get(-999)->illustName);
    card_affix_clear(); book_items_clear(); metal_process_clear();
    assert(!card_affix_has(42) && !book_item_has(42) && !metal_process_get(42)->count);
    item_move_info_add(42, changed); assert(item_move_info_get(42)->flags[0] == 0);
    card_affix_set_unknown(); assert(!strcmp(card_affix_get(-999)->illustName, "sorry"));
    card_affix_clear(); assert(!strcmp(card_affix_get(-999)->illustName, "sorry"));
    card = card_affix_at(0); card_affix_set_unknown(); assert(!strcmp(card_affix_get(-999)->illustName, "sorry"));
    card_affix_clear(); card_affix_at(-1); card_affix_set_unknown(); assert(!strcmp(card_affix_get(-1)->illustName, "sorry"));
    item_move_info_clear(); item_move_info_add(42, changed); assert(item_move_info_get(42)->flags[0] == 1);
    card_affix_clear(); book_items_clear(); metal_process_clear(); item_move_info_clear();
    card_affix_clear(); book_items_clear(); metal_process_clear(); item_move_info_clear();
    assert(!allocations && !item_move_info_get(42)->item_id);
}

static void test_item_storage(void)
{
    static const int special[] = { INT_MAX, -1, INT_MIN, 0 };
    const struct ItemTableEntry *entry, *copy;
    struct ItemTableEntry *write;
    int i, id, last;
    item_table_init();
    assert(!item_table_count() && !item_table_get(1)->name);
    id = 73; assert(!item_table_entry_at(0, &id) && id == 73);
    for (i = 256; i >= 1; --i) {
        write = item_table_at(i);
        item_table_append_params(write, i, ~i);
    }
    for (i = 0; i < 4; ++i) item_table_at(special[i]);
    assert(item_table_count() == 260);
    last = INT_MIN;
    for (i = 0; i < 260; ++i) {
        entry = item_table_entry_at(i, &id);
        assert(entry == item_table_get(id) && (!i || id > last)); last = id;
        if (id >= 1 && id <= 256) assert(entry->params.count == 2 && entry->params.data[0] == id && entry->params.data[1] == ~id);
    }
    write = item_table_at(0); assert(item_table_count() == 260);
    write->name = "original"; write->slotCount = 3; write->class_num = 987;
    item_table_append_params(write, 11, 22);
    for (i = 0; i < 19; ++i) item_table_append_description(write, 1, "line");
    item_table_append_description(write, 0, "replace this");
    item_table_use_identified_description(write);
    assert(write->unidName == write->name && write->unidDesc.count == 19);
    assert(write->unidDesc.data != write->desc.data && write->unidDesc.data[18] == write->desc.data[18]);
    item_table_set_unknown();
    entry = item_table_get(0); copy = item_table_get(-1);
    assert(!strcmp(entry->name, "Unknown Item") && !strcmp(copy->resName, "사과"));
    assert(entry->desc.count == 20 && copy->desc.count == 20 && copy->desc.data != entry->desc.data);
    assert(copy->unidDesc.data != entry->unidDesc.data && copy->params.data != entry->params.data);
    assert(copy->slotCount == 3 && copy->class_num == 987 && copy->params.data[1] == 22);
    assert(!item_table_get(-999)->name);
    item_table_clear(); item_table_clear(); assert(!allocations && !item_table_count());
    item_table_set_unknown();
    assert(!strcmp(item_table_get(99)->name, "Unknown Item") && item_table_get(99)->desc.count == 1);
    item_table_clear();
    write = item_table_at(0); item_table_append_params(write, 12, 34);
    item_table_set_unknown();
    assert(item_table_get(99)->params.data[1] == 34 && item_table_get(99)->params.data != item_table_get(0)->params.data);
    item_table_clear();
    write = item_table_at(-1); item_table_append_params(write, 56, 78);
    item_table_set_unknown();
    assert(!item_table_get(-1)->params.count && !strcmp(item_table_get(-1)->name, "Unknown Item"));
    item_table_clear(); assert(!allocations);
}

int main(void)
{
    struct ItemInfoData *held, *next;
    struct ItemTableEntry *entry;
    const char *name;
    int i, fail, before, last;
    size_t count;
    const char *event_line = "event description";
    const char *bad[] = {
        "AddItemEffectInfo(9,9);error('chunk failed')",
        "function main() AddItemEffectInfo(9,9);error({}) end",
        "function main() AddItemEffectInfo(9,9);return false,'bad data' end",
        "function main() return true,{} end", "AddItemEffectInfo(9,9)", "syntax !"
    };
    test_companion_tables();
    test_item_storage();
    assert(!item_info_acquire() && !item_info_count(NULL) && !item_info_is_costume(1) && item_info_effect_id(1) == -1);
    script = normal; assert(item_info_load() && loads == 1);
    held = item_info_acquire(); check_normal(held);
    assert(item_info_is_costume(42) && item_info_effect_id(3) == 99);
    assert(!item_info_is_costume(999) && item_info_effect_id(999) == -1 && item_info_count(held) == 14);
    before = allocations;
    for (i = 0; i < sizeof bad / sizeof bad[0]; ++i) {
        script = bad[i]; assert(!item_info_load() && allocations == before);
        next = item_info_acquire(); assert(next == held); item_info_release(next);
    }
    script = NULL; loads = 0; assert(!item_info_load() && loads == 1 && allocations == before);
    fail_state = 1; assert(!item_info_load() && allocations == before); fail_state = 0;
    script = "function main() local p=newproxy(true);getmetatable(p).__gc=function() AddItemEffectInfo(5,7) end;keep=p;return true,'good' end";
    fail_after = 1; assert(!item_info_load() && allocations == before); fail_after = -1;
    script = normal;
    for (fail = 0; fail < 200; ++fail) {
        fail_after = fail;
        if (item_info_load()) break;
        assert(allocations == before);
        next = item_info_acquire(); assert(next == held); item_info_release(next);
    }
    assert(fail > 60 && fail < 200); fail_after = -1;
    check_normal(held); item_info_mgr_clear(); check_normal(held); assert(!item_info_acquire()); item_info_release(held); assert(allocations == 0);

    script = normal; assert(item_info_load());
    entry = item_table_at(42);
    assert(!entry->name && !entry->desc.data && entry->slotCount == 0);
    assert(item_table_at(42) == entry && item_table_count() == 1);
    item_table_append_description(entry, 1, event_line);
    entry->slotTableValue = 123; item_table_append_params(entry, 77, 88);
    item_table_apply_lua(); entry = item_table_at(42); name = entry->name;
    assert(item_table_count() == 14 && strcmp(entry->name,"사과") == 0 && entry->slotCount == 2 && entry->class_num == 500);
    assert(entry->slotTableValue == 123 && entry->params.data[0] == 77);
    assert(entry->desc.count == 3 && !strcmp(entry->desc.data[0],"event description"));
    assert(item_table_get(42) == entry);
    assert(!strcmp(item_table_resource_name(entry, 1), "id_apple"));
    assert(!strcmp(item_table_resource_name(entry, 0), "apple"));
    assert(!item_table_resource_name(item_table_get(-999), 1));
    script = "function main() AddItem(42,'new','','new','',1,1);return 0,123 end";
    assert(item_info_load() && strcmp(name,"사과") == 0);
    held = item_info_acquire(); assert(strcmp(entry_by_id(held,42)->identified.name,"new") == 0);
    item_info_mgr_clear(); assert(strcmp(name,"사과") == 0);
    item_table_clear(); assert(!item_table_count() && !item_table_get(-999)->name);
    assert(strcmp(entry_by_id(held,42)->identified.name,"new") == 0); item_info_release(held);
    assert(allocations == 0);

    script = "function main() "
        "assert(not AddItem(0/0,'','','','',0,0)); assert(not AddItem(1,'','','','',1/0,0)); "
        "assert(not AddItemEffectInfo(2147483648,0)); assert(not AddItemIsCostume(1,1)); "
        "assert(not AddItemIdentifiedDesc(1,{})); "
        "for i=1,20000 do local id=(i*4051)%32768; AddItem(id,'u'..id,'r','n'..id,'s',id%4,id);AddItemEffectInfo(id,id) end "
        "return true,'good' end";
    for (i = 0; i < 3; ++i) {
        assert(item_info_load()); held = item_info_acquire(); count = item_info_count(held); assert(count == 20000);
        last = -1;
        for (count = 0; count < item_info_count(held); ++count) {
            const struct ItemDefinition *value = item_info_entry(held,count);
            assert(value->id > last && item_info_effect_id(value->id) == value->id); last = value->id;
        }
        item_table_apply_lua(); assert(item_table_count() == 20000); last = -1;
        for (count = 0; count < item_table_count(); ++count) {
            int id;
            const struct ItemTableEntry *value = item_table_entry_at(count, &id);
            assert(id > last && value == item_table_get(id) && value->class_num == id); last = id;
        }
        item_info_mgr_clear(); item_info_release(held); item_table_clear(); assert(allocations == 0);
    }
    script = "function main() return true,'empty' end"; assert(item_info_load()); held = item_info_acquire(); assert(!item_info_count(held));
    item_info_mgr_clear(); item_info_mgr_clear(); item_info_release(held); item_table_clear();
    item_table_clear(); assert(allocations == 0);
    puts("Item info: real Lua, owned readers, guarded rollback, 20000 items, C item/companion arrays, source lifetime and cleanup passed");
    return 0;
}
