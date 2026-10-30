#include "ui/social/entry_queue_state.h"
#include "contents/client_lua.h"
#include <lauxlib.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

struct QueueCatalog {
    struct EntryQueueInfo **items;
    size_t count, capacity;
};
static struct QueueCatalog catalog;
static char load_key;

static void clear_catalog(struct QueueCatalog *data)
{
    size_t i;
    for (i = 0; i < data->count; ++i) free(data->items[i]);
    free(data->items);
    memset(data, 0, sizeof *data);
}
void entry_queue_catalog_clear(void)
{
    if (g_entryQueueMgr) g_entryQueueMgr->current = NULL;
    clear_catalog(&catalog);
}
size_t entry_queue_count(void) { return catalog.count; }
struct EntryQueueInfo *entry_queue_at(size_t index)
{ return index < catalog.count ? catalog.items[index] : NULL; }

static size_t lower_bound(const struct QueueCatalog *data, unsigned id)
{
    size_t first = 0, last = data->count;
    while (first < last) {
        size_t middle = first + (last - first) / 2;
        if ((unsigned)data->items[middle]->id < id) first = middle + 1;
        else last = middle;
    }
    return first;
}
struct EntryQueueInfo *entry_queue_find(unsigned id)
{
    size_t at = lower_bound(&catalog, id);
    return at < catalog.count && (unsigned)catalog.items[at]->id == id ? catalog.items[at] : NULL;
}
static int add_queue(struct QueueCatalog *data, const struct EntryQueueInfo *value)
{
    size_t at = lower_bound(data, (unsigned)value->id);
    struct EntryQueueInfo *copy;
    if (at < data->count && data->items[at]->id == value->id) return 1;
    copy = malloc(sizeof *copy);
    if (!copy) return 0;
    if (data->count == data->capacity) {
        size_t limit = (size_t)-1 / sizeof *data->items;
        size_t capacity = data->capacity ? data->capacity : 8;
        struct EntryQueueInfo **items;
        if (data->count == limit) { free(copy); return 0; }
        if (data->capacity) capacity = capacity <= limit / 2 ? capacity * 2 : limit;
        items = realloc(data->items, capacity * sizeof *items);
        if (!items) { free(copy); return 0; }
        data->items = items; data->capacity = capacity;
    }
    *copy = *value;
    memmove(data->items + at + 1, data->items + at, (data->count - at) * sizeof *data->items);
    data->items[at] = copy; ++data->count;
    return 1;
}

static int read_integer(lua_State *L, int index)
{
    double value = luaL_checknumber(L, index);
    if (!(value >= INT_MIN && value <= INT_MAX))
        luaL_argerror(L, index, "entry queue number is outside the integer range");
    return (int)value;
}
static void read_text(lua_State *L, int index, char *out, size_t capacity)
{
    size_t size;
    const char *text = luaL_checklstring(L, index, &size);
    const char *zero = memchr(text, 0, size);
    if (zero) size = (size_t)(zero - text);
    if (size >= capacity) luaL_argerror(L, index, "entry queue text is too long");
    memcpy(out, text, size); out[size] = 0;
}
int entry_queue_lua_add(lua_State *L)
{
    struct EntryQueueInfo info = {0};
    struct QueueCatalog *target;
    unsigned char flags[4];
    unsigned i;
    info.id = read_integer(L, 1);
    read_text(L, 2, info.name, sizeof info.name);
    read_text(L, 3, info.title, sizeof info.title);
    info.member_limit = read_integer(L, 4); info.enemy_limit = read_integer(L, 5);
    for (i = 0; i < 4; ++i) flags[i] = (unsigned char)read_integer(L, 6 + (int)i);
    memcpy(&info.option_flags, flags, sizeof flags);
    info.kind = (unsigned char)read_integer(L, 10);
    info.kind_arg1 = read_integer(L, 11); info.kind_arg2 = read_integer(L, 12);
    read_text(L, 13, info.text_row4, sizeof info.text_row4);
    read_text(L, 14, info.text_row5, sizeof info.text_row5);
    read_text(L, 15, info.text_row6, sizeof info.text_row6);
    read_text(L, 16, info.text_row1, sizeof info.text_row1);
    read_text(L, 17, info.picture, sizeof info.picture);
    lua_pushlightuserdata(L, &load_key); lua_rawget(L, LUA_REGISTRYINDEX);
    target = lua_touserdata(L, -1); lua_pop(L, 1);
    if (!target) target = &catalog;
    if (!add_queue(target, &info)) return luaL_error(L, "entry queue allocation failed");
    return 0;
}

static int read_catalog(lua_State *L)
{
    struct QueueCatalog *pending = lua_touserdata(L, 1);
    lua_pushlightuserdata(L, &load_key);
    lua_pushlightuserdata(L, pending);
    lua_rawset(L, LUA_REGISTRYINDEX);
    lua_getglobal(L, "ReadEntryQueueList");
    lua_call(L, 0, 0);
    return 0;
}

int entry_queue_catalog_load(struct ClientLua *lua)
{
    struct QueueCatalog pending = {0};
    lua_State *L;
    int top, status, busy;
    unsigned selected = 0;
    int had_selection = g_entryQueueMgr && g_entryQueueMgr->current;
    if (!lua || !(L = lua->L) || !lua_checkstack(L, 8)) return 0;
    if (had_selection) selected = (unsigned)g_entryQueueMgr->current->id;
    top = lua_gettop(L);
    lua_pushlightuserdata(L, &load_key); lua_rawget(L, LUA_REGISTRYINDEX);
    busy = !lua_isnil(L, -1); lua_pop(L, 1);
    if (busy) return 0;
    status = lua_cpcall(L, read_catalog, &pending);
    lua_pushlightuserdata(L, &load_key); lua_rawget(L, LUA_REGISTRYINDEX);
    if (!lua_isnil(L, -1)) {
        lua_pushlightuserdata(L, &load_key); lua_pushnil(L); lua_rawset(L, LUA_REGISTRYINDEX);
    }
    lua_pop(L, 1);
    if (status) {
        const char *error = lua_type(L, -1) == LUA_TSTRING ? lua_tostring(L, -1) : "Lua reader failed";
        client_lua_error_box(lua, "Entry queues", "%.400s\n", error);
    }
    lua_settop(L, top);
    if (status) { clear_catalog(&pending); return 0; }
    clear_catalog(&catalog); catalog = pending;
    if (g_entryQueueMgr) g_entryQueueMgr->current = had_selection ? entry_queue_find(selected) : NULL;
    return 1;
}
