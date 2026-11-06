#include "contents/item_info_mgr.h"
#include <windows.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

struct OwnedItem {
    struct ItemDefinition value;
    size_t unidentified_capacity, identified_capacity;
};

struct ItemInfoData {
    struct OwnedItem *items;
    size_t count, capacity, readers;
};

struct ItemBuilder {
    struct ItemInfoData *data;
    size_t *index, index_capacity;
    int failed;
};

static struct ItemInfoData *current;

static void free_text(struct ItemText *text)
{
    size_t i;
    free((void *)text->name);
    free((void *)text->resource);
    for (i = 0; i < text->line_count; ++i) free((void *)text->lines[i]);
    free((void *)text->lines);
}

void item_info_release(struct ItemInfoData *data)
{
    size_t i;
    if (!data || --data->readers) return;
    for (i = 0; i < data->count; ++i) {
        free_text(&data->items[i].value.identified);
        free_text(&data->items[i].value.unidentified);
    }
    free(data->items);
    free(data);
}

void item_info_mgr_clear(void)
{
    struct ItemInfoData *old = current;
    current = NULL;
    item_info_release(old);
}

struct ItemInfoData *item_info_acquire(void)
{
    if (current) {
        if (current->readers == (size_t)-1) return NULL;
        ++current->readers;
    }
    return current;
}

size_t item_info_count(const struct ItemInfoData *data)
{
    return data ? data->count : 0;
}

const struct ItemDefinition *item_info_entry(const struct ItemInfoData *data, size_t index)
{
    return data && index < data->count ? &data->items[index].value : NULL;
}

static const struct ItemDefinition *find_item(int id)
{
    size_t lo = 0, hi = item_info_count(current);
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        const struct ItemDefinition *item = &current->items[mid].value;
        if (item->id == id) return item;
        if (item->id < id) lo = mid + 1;
        else hi = mid;
    }
    return NULL;
}

int item_info_is_costume(int id)
{
    const struct ItemDefinition *item = find_item(id);
    return item ? item->is_costume : 0;
}

int item_info_effect_id(int id)
{
    const struct ItemDefinition *item = find_item(id);
    return item ? item->effect_id : -1;
}

static void *grow(void *data, size_t *capacity, size_t count, size_t size)
{
    size_t next;
    void *result;
    if (count <= *capacity) return data;
    if (count > (size_t)-1 / size) return NULL;
    next = *capacity > (size_t)-1 / 2 ? count : *capacity * 2;
    if (next < 8) next = 8;
    if (next < count || next > (size_t)-1 / size) next = count;
    result = realloc(data, next * size);
    if (result) *capacity = next;
    return result;
}

static char *copy_text(const char *text)
{
    size_t size = strlen(text) + 1;
    char *copy = malloc(size);
    if (copy) memcpy(copy, text, size);
    return copy;
}

static size_t item_hash(int id)
{
    unsigned value = (unsigned)id;
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    return value;
}

static int grow_index(struct ItemBuilder *builder)
{
    size_t capacity = builder->index_capacity ? builder->index_capacity * 2 : 16;
    size_t *index, i;
    if (builder->index_capacity > (size_t)-1 / 2 || capacity > (size_t)-1 / sizeof *index)
        return 0;
    index = calloc(capacity, sizeof *index);
    if (!index) return 0;
    for (i = 0; i < builder->data->count; ++i) {
        size_t at = item_hash(builder->data->items[i].value.id) & (capacity - 1);
        while (index[at]) at = (at + 1) & (capacity - 1);
        index[at] = i + 1;
    }
    free(builder->index);
    builder->index = index;
    builder->index_capacity = capacity;
    return 1;
}

static struct OwnedItem *get_item(struct ItemBuilder *builder, int id)
{
    struct ItemInfoData *data = builder->data;
    struct OwnedItem *items;
    size_t at;
    if (data->count >= builder->index_capacity / 2 && !grow_index(builder)) return NULL;
    at = item_hash(id) & (builder->index_capacity - 1);
    while (builder->index[at]) {
        struct OwnedItem *item = &data->items[builder->index[at] - 1];
        if (item->value.id == id) return item;
        at = (at + 1) & (builder->index_capacity - 1);
    }
    items = grow(data->items, &data->capacity, data->count + 1, sizeof *items);
    if (!items) return NULL;
    data->items = items;
    memset(items + data->count, 0, sizeof *items);
    items[data->count].value.id = id;
    builder->index[at] = data->count + 1;
    return items + data->count++;
}

static int push_result(lua_State *L, int ok, const char *format, ...)
{
    char text[256];
    va_list args;
    va_start(args, format);
    vsnprintf(text, sizeof text, format, args);
    va_end(args);
    lua_pushboolean(L, ok);
    lua_pushstring(L, text);
    return 2;
}

static struct ItemBuilder *builder_for(lua_State *L)
{
    return lua_touserdata(L, lua_upvalueindex(1));
}

static int allocation_failed(lua_State *L)
{
    builder_for(L)->failed = 1;
    return push_result(L, 0, "not enough memory for item definitions");
}

static int integer(lua_State *L, int index, int *out)
{
    double n;
    if (!lua_isnumber(L, index)) return 0;
    n = lua_tonumber(L, index);
    if (!(n >= INT_MIN && n <= INT_MAX)) return 0;
    *out = (int)n;
    return 1;
}

static int lua_add_item(lua_State *L)
{
    struct OwnedItem *item;
    const char *source[4];
    char *text[4];
    int id = 0, valid_id, slots, class_num, i;
    if (!integer(L, -1, &class_num)) return push_result(L, 0, "7th argment is must number");
    valid_id = integer(L, -7, &id);
    if (!integer(L, -2, &slots)) return push_result(L, 0, "6th argment is must number [itemID:%d]", id);
    if (!lua_isstring(L, -3)) return push_result(L, 0, "5th argment is must string [itemID:%d]", id);
    if (!lua_isstring(L, -4)) return push_result(L, 0, "4th argment is must string [itemID:%d]", id);
    if (!lua_isstring(L, -5)) return push_result(L, 0, "3th argment is must string [itemID:%d]", id);
    if (!lua_isstring(L, -6)) return push_result(L, 0, "2nd argment is must string [itemID:%d]", id);
    if (!valid_id) return push_result(L, 0, "1st argment is must number [itemID:%d]", id);
    for (i = 0; i < 4; ++i) source[i] = lua_tostring(L, -6 + i);
    item = get_item(builder_for(L), id);
    if (!item) return allocation_failed(L);
    for (i = 0; i < 4; ++i) {
        text[i] = copy_text(source[i]);
        if (!text[i]) {
            while (i) free(text[--i]);
            return allocation_failed(L);
        }
    }
    free((void *)item->value.unidentified.name);
    free((void *)item->value.unidentified.resource);
    free((void *)item->value.identified.name);
    free((void *)item->value.identified.resource);
    item->value.unidentified.name = text[0]; item->value.unidentified.resource = text[1];
    item->value.identified.name = text[2]; item->value.identified.resource = text[3];
    item->value.slot_count = slots; item->value.class_num = class_num;
    return push_result(L, 1, "good");
}

static int lua_add_item_effect_info(lua_State *L)
{
    struct OwnedItem *item;
    int id, effect;
    if (!integer(L, -1, &effect)) return push_result(L, 0, "2nd argment is must number");
    if (!integer(L, -2, &id)) return push_result(L, 0, "1st argment is must number");
    item = get_item(builder_for(L), id);
    if (!item) return allocation_failed(L);
    item->value.effect_id = effect;
    return push_result(L, 1, "good");
}

static int add_item_desc(lua_State *L, int identified)
{
    struct OwnedItem *item;
    struct ItemText *text;
    const char *source;
    char **lines, *line;
    size_t *capacity;
    int id;
    if (!lua_isstring(L, -1)) return push_result(L, 0, "2nd argment is must string");
    if (!integer(L, -2, &id)) return push_result(L, 0, "1st argment is must number");
    source = lua_tostring(L, -1);
    item = get_item(builder_for(L), id);
    if (!item) return allocation_failed(L);
    text = identified ? &item->value.identified : &item->value.unidentified;
    capacity = identified ? &item->identified_capacity : &item->unidentified_capacity;
    lines = grow((void *)text->lines, capacity, text->line_count + 1, sizeof *lines);
    if (!lines) return allocation_failed(L);
    text->lines = (const char *const *)lines;
    line = copy_text(source);
    if (!line) return allocation_failed(L);
    lines[text->line_count++] = line;
    return push_result(L, 1, "good");
}

static int lua_add_item_identified_desc(lua_State *L)
{
    return add_item_desc(L, 1);
}

static int lua_add_item_unidentified_desc(lua_State *L)
{
    return add_item_desc(L, 0);
}

static int lua_add_item_is_costume(lua_State *L)
{
    struct OwnedItem *item;
    int id;
    if (lua_type(L, -1) != LUA_TBOOLEAN) return push_result(L, 0, "2nd argment is must boolean");
    if (!integer(L, -2, &id)) return push_result(L, 0, "1st argment is must number");
    item = get_item(builder_for(L), id);
    if (!item) return allocation_failed(L);
    item->value.is_costume = lua_toboolean(L, -1) != 0;
    return push_result(L, 1, "good");
}

static int load_script(lua_State *L)
{
    static const struct { const char *name; lua_CFunction callback; } callbacks[] = {
        {"AddItem", lua_add_item}, {"AddItemEffectInfo", lua_add_item_effect_info},
        {"AddItemIdentifiedDesc", lua_add_item_identified_desc},
        {"AddItemUnidentifiedDesc", lua_add_item_unidentified_desc},
        {"AddItemIsCostume", lua_add_item_is_costume}
    };
    struct ItemBuilder *builder = lua_touserdata(L, 1);
    size_t i;
    luaL_openlibs(L);
    for (i = 0; i < sizeof callbacks / sizeof callbacks[0]; ++i) {
        lua_pushlightuserdata(L, builder);
        lua_pushcclosure(L, callbacks[i].callback, 1);
        lua_setglobal(L, callbacks[i].name);
    }
    if (luaL_loadfile(L, "System\\iteminfo.lua") != 0) return lua_error(L);
    lua_call(L, 0, 0);
    lua_getglobal(L, "main");
    lua_call(L, 0, 2);
    if (!lua_isstring(L, -1)) return luaL_error(L, "second return value is not string...");
    if (!lua_toboolean(L, -2)) return luaL_error(L, "error found :%s", lua_tostring(L, -1));
    if (builder->failed) return luaL_error(L, "not enough memory for item definitions");
    return 0;
}

static int compare_items(const void *a, const void *b)
{
    int left = ((const struct OwnedItem *)a)->value.id;
    int right = ((const struct OwnedItem *)b)->value.id;
    return (left > right) - (left < right);
}

int item_info_load(void)
{
    struct ItemBuilder builder = {0};
    struct ItemInfoData *old;
    lua_State *L;
    size_t i;
    int ok;
    builder.data = calloc(1, sizeof *builder.data);
    if (!builder.data) return 0;
    builder.data->readers = 1;
    L = luaL_newstate();
    if (!L) { item_info_release(builder.data); return 0; }
    ok = lua_cpcall(L, load_script, &builder) == 0;
    if (!ok) {
        const char *error = lua_type(L, -1) == LUA_TSTRING ? lua_tostring(L, -1) : NULL;
        MessageBoxA(NULL, error ? error : "Item info script failed", "CItemInfoMgr", 0);
    }
    lua_close(L);
    free(builder.index);
    if (!ok || builder.failed) { item_info_release(builder.data); return 0; }
    for (i = 0; i < builder.data->count; ++i) {
        struct ItemDefinition *item = &builder.data->items[i].value;
        if (!item->identified.name) item->identified.name = copy_text("");
        if (!item->identified.resource) item->identified.resource = copy_text("");
        if (!item->unidentified.name) item->unidentified.name = copy_text("");
        if (!item->unidentified.resource) item->unidentified.resource = copy_text("");
        if (!item->identified.name || !item->identified.resource || !item->unidentified.name || !item->unidentified.resource) {
            item_info_release(builder.data);
            return 0;
        }
    }
    if (builder.data->count > 1)
        qsort(builder.data->items, builder.data->count, sizeof *builder.data->items, compare_items);
    old = current;
    current = builder.data;
    item_info_release(old);
    return 1;
}
