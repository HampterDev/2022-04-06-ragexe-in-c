#include "ui/worldmap/kafra_services.h"
#include "contents/client_lua.h"
#include <lauxlib.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

struct KafraPosition { char *map; float x, y; };
struct KafraService {
    char *map;
    char **destinations;
    size_t count, capacity;
};
struct KafraTables {
    struct KafraPosition *positions;
    size_t position_count, position_capacity;
    struct KafraService *services;
    size_t service_count, service_capacity;
};
static struct KafraTables tables;
static char load_key;

static void clear_positions(struct KafraTables *data)
{
    size_t i;
    for (i = 0; i < data->position_count; ++i)
        free(data->positions[i].map);
    free(data->positions);
    data->positions = NULL;
    data->position_count = data->position_capacity = 0;
}

static void clear_destinations(struct KafraTables *data)
{
    size_t i, j;
    for (i = 0; i < data->service_count; ++i) {
        struct KafraService *service = &data->services[i];
        for (j = 0; j < service->count; ++j)
            free(service->destinations[j]);
        free(service->destinations);
        free(service->map);
    }
    free(data->services);
    data->services = NULL;
    data->service_count = data->service_capacity = 0;
}

void kafra_positions_clear(void) { clear_positions(&tables); }
void kafra_destinations_clear(void) { clear_destinations(&tables); }

static struct KafraPosition *find_position(struct KafraTables *data, const char *map)
{
    size_t i;
    if (map)
        for (i = 0; i < data->position_count; ++i)
            if (strcmp(data->positions[i].map, map) == 0)
                return &data->positions[i];
    return NULL;
}

static struct KafraService *find_service(struct KafraTables *data, const char *map)
{
    size_t i;
    if (map)
        for (i = 0; i < data->service_count; ++i)
            if (strcmp(data->services[i].map, map) == 0)
                return &data->services[i];
    return NULL;
}

size_t kafra_destination_count(const char *map)
{
    const struct KafraService *service = find_service(&tables, map);
    return service ? service->count : 0;
}

const char *kafra_destination(const char *map, size_t index)
{
    const struct KafraService *service = find_service(&tables, map);
    return service && index < service->count ? service->destinations[index] : NULL;
}

static void *grow(void *data, size_t *capacity, size_t count, size_t size)
{
    size_t maximum = (size_t)-1 / size, next;
    void *replacement;
    if (count < *capacity) return data;
    if (count >= maximum) return NULL;
    next = *capacity <= (maximum - 8) / 2 ? *capacity * 2 + 8 : maximum;
    replacement = realloc(data, next * size);
    if (replacement) *capacity = next;
    return replacement;
}

static char *copy_text(const char *text)
{
    size_t size = strlen(text) + 1;
    char *copy = malloc(size);
    if (copy) memcpy(copy, text, size);
    return copy;
}

static struct KafraTables *callback_tables(lua_State *L)
{
    struct KafraTables *data;
    lua_pushlightuserdata(L, &load_key);
    lua_rawget(L, LUA_REGISTRYINDEX);
    data = lua_touserdata(L, -1);
    lua_pop(L, 1);
    return data ? data : &tables;
}

static const char *map_name(lua_State *L, int argument)
{
    size_t length;
    const char *name = luaL_checklstring(L, argument, &length);
    luaL_argcheck(L, strlen(name) == length, argument, "map name contains a zero byte");
    return name;
}

static int coordinate(lua_State *L, int argument)
{
    double value = luaL_checknumber(L, argument);
    luaL_argcheck(L, value >= INT_MIN && value <= INT_MAX, argument, "coordinate outside integer range");
    return (int)value;
}

static int add_minimap_pos(lua_State *L)
{
    const char *map = map_name(L, 1);
    int x = coordinate(L, 2), y = coordinate(L, 3);
    struct KafraTables *data = callback_tables(L);
    struct KafraPosition *position = find_position(data, map);
    if (!position) {
        struct KafraPosition *rows = grow(data->positions, &data->position_capacity,
                                          data->position_count, sizeof *rows);
        char *name;
        if (!rows) return luaL_error(L, "cannot allocate Kafra positions");
        data->positions = rows;
        name = copy_text(map);
        if (!name) return luaL_error(L, "cannot copy Kafra map name");
        position = &rows[data->position_count++];
        position->map = name;
    }
    position->x = (float)x;
    position->y = (float)y;
    return 0;
}

static int add_kafra_destination(lua_State *L)
{
    const char *map = map_name(L, 1), *destination = map_name(L, 2);
    struct KafraTables *data = callback_tables(L);
    struct KafraService *service = find_service(data, map);
    char **destinations, *name;
    if (!service) {
        struct KafraService *rows = grow(data->services, &data->service_capacity,
                                         data->service_count, sizeof *rows);
        if (!rows) return luaL_error(L, "cannot allocate Kafra services");
        data->services = rows;
        name = copy_text(map);
        if (!name) return luaL_error(L, "cannot copy Kafra map name");
        service = &rows[data->service_count++];
        memset(service, 0, sizeof *service);
        service->map = name;
    }
    destinations = grow(service->destinations, &service->capacity, service->count, sizeof *destinations);
    if (!destinations) return luaL_error(L, "cannot allocate Kafra destinations");
    service->destinations = destinations;
    name = copy_text(destination);
    if (!name) return luaL_error(L, "cannot copy Kafra destination");
    destinations[service->count++] = name;
    return 0;
}

void map_list_register_lua_functions(lua_State *L)
{
    lua_register(L, "AddItmeTokafraServiceMapList", add_kafra_destination);
    lua_register(L, "AddItmeToExceptionMiniMapPosList", add_minimap_pos);
}

static int read_tables(lua_State *L)
{
    static const char *const readers[] = { "ReadKafraMoveMapServiceInfo", "ReadKafraMiniMapPosInfo" };
    struct KafraTables *next = lua_touserdata(L, 1);
    size_t i;
    lua_pushlightuserdata(L, &load_key);
    lua_pushlightuserdata(L, next);
    lua_rawset(L, LUA_REGISTRYINDEX);
    for (i = 0; i < sizeof readers / sizeof *readers; ++i) {
        lua_getglobal(L, readers[i]);
        lua_call(L, 0, 0);
    }
    return 0;
}

int kafra_services_load(struct ClientLua *lua)
{
    struct KafraTables next = {0};
    lua_State *L;
    int base, result, busy;
    if (!lua || !(L = lua->L) || !lua_checkstack(L, 8)) return 0;
    base = lua_gettop(L);
    lua_pushlightuserdata(L, &load_key);
    lua_rawget(L, LUA_REGISTRYINDEX);
    busy = !lua_isnil(L, -1);
    lua_pop(L, 1);
    if (busy) return 0;
    result = lua_cpcall(L, read_tables, &next);
    lua_pushlightuserdata(L, &load_key);
    lua_pushnil(L);
    lua_rawset(L, LUA_REGISTRYINDEX);
    if (result) {
        const char *error = lua_type(L, -1) == LUA_TSTRING ? lua_tostring(L, -1) : "Lua reader failed";
        client_lua_error_box(lua, "Kafra tables", "%s\n", error);
        clear_positions(&next);
        clear_destinations(&next);
    } else {
        kafra_positions_clear();
        kafra_destinations_clear();
        tables = next;
    }
    lua_settop(L, base);
    return result == 0;
}
