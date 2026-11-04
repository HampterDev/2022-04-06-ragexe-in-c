/* The EZ2STR effect manager and the map-sky manager are allocated on first use (mem_try_alloc).
   When that fails at startup, the client goes on without them, as when their files do not load:
   their loads are not called with NULL (the map-sky load reads self->lua, the EZ2STR load its rows),
   and their later users take the missing manager as one whose file did not load: no EZ2STR rows,
   and a map without sky data (a black background). The startup loads, the two instances, the map-sky
   load and apply and the EZ2STR lookups are the production code; the allocator fails on request. */
#include "core/map_sky.h"
#include "effects/ez2str_effect_mgr.h"
#include "contents/client_lua.h"
#include "contents/client_lua_calls.h"
#include "render/renderer.h"
#include "std/string.h"
#include "std/tree.h"
#include "std/vec.h"
#include "core/memory.h"
#include "core/fatal.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fail_allocations, allocations;
static struct Renderer renderer;
struct Renderer *g_renderer = &renderer;

void *mem_try_alloc(size_t bytes)
{
    allocations++;
    return fail_allocations ? NULL : calloc(1, bytes);
}
void *mem_alloc(size_t bytes)
{
    void *p = calloc(1, bytes);
    assert(p);
    return p;
}
void mem_free(void *p) { free(p); }
void fatal_error(const char *message)
{
    puts(message);
    abort();
}

/* Lua: the map-sky manager's state; nothing is loaded, so every call finds no function. */
static struct ClientLua lua_state;
static int lua_loads, lua_calls;
struct ClientLua *client_lua_construct(struct ClientLua *self) { return self; }
void client_lua_open(struct ClientLua *self) { (void)self; }
char client_lua_load_file(struct ClientLua *self, const char *name, char via_cfile, char disk_only)
{
    (void)name; (void)via_cfile; (void)disk_only;
    assert(self);
    lua_loads++;
    return 1;
}
int client_lua_call(struct ClientLua *lua, const char *function, const char *format, ...)
{
    (void)function; (void)format;
    assert(lua);
    lua_calls++;
    return 0;
}
char *game_mode_short_name(struct String name)
{
    static char short_name[64];
    snprintf(short_name, sizeof short_name, "%s", string_cstr(&name));
    string_free(&name);
    return short_name;
}
/* Reached only when a map has sky data. */
void map_sky_mgr_set_background(struct MapSkyMgr *self, const struct String *map) { (void)self; (void)map; abort(); }
void map_sky_mgr_stars(struct MapSkyMgr *self, const struct String *map) { (void)self; (void)map; abort(); }
void map_sky_mgr_make_clouds(struct MapSkyMgr *self, const struct String *map) { (void)self; (void)map; abort(); }
void map_sky_mgr_old_clouds(struct MapSkyMgr *self, const struct String *map) { (void)self; (void)map; abort(); }

/* The EZ2STR load reads its file into self->rows; here it only records what it was given. */
static int ez2str_loads;
unsigned char ez2_str_effect_mgr_load(struct Ez2StrEffectMgr *self)
{
    if (!self) {
        puts("manager allocation failure: ez2_str_effect_mgr_load was called with NULL");
        exit(1);
    }
    ez2str_loads++;
    return 1;
}

#include "manager_alloc_failure.inc"

static void apply_sky(const char *map_name)
{
    struct String map;

    string_init(&map);
    string_assign_cstr(&map, map_name);
    /* As game mode initialisation does at every map. */
    map_sky_mgr_apply(map_sky_mgr_instance(), &map);
    string_free(&map);
}

int main(void)
{
    struct Vec ids;
    struct String name;

    /* Startup with no memory for the two managers. */
    fail_allocations = 1;
    startup_loads();
    assert(allocations == 2 && !g_ez2strMgr && !g_mapSkyMgrInstance);
    assert(ez2str_loads == 0 && lua_loads == 0);

    /* Their later users, still without memory: no rows, and no sky (black). */
    assert(ez2_str_effect_mgr_get_effect_ids(ez2str_effect_mgr_instance(), &ids) == &ids && ids.first == ids.last);
    assert(ez2_str_effect_mgr_find_by_effect_id(ez2str_effect_mgr_instance(), 10000) == NULL);
    string_init(&name);
    string_assign_cstr(&name, "EF_X");
#if HAVE_GET_EFFECT_ID
    assert(ez2_str_effect_mgr_get_effect_id(ez2str_effect_mgr_instance(), &name) == -1);
#endif
    renderer.clearColor = 0x12345678;
    apply_sky("prontera.rsw");
    assert(renderer.clearColor == 0xff000000 && lua_calls == 0);

    /* With memory, the managers are made on first use and their loads run. */
    fail_allocations = 0;
    startup_loads();
    assert(g_ez2strMgr && g_mapSkyMgrInstance && ez2str_loads == 1 && lua_loads == 1);
    renderer.clearColor = 0x12345678;
    apply_sky("prontera.rsw");
    assert(renderer.clearColor == 0xff000000 && lua_calls == 1);
    assert(ez2_str_effect_mgr_find_by_effect_id(ez2str_effect_mgr_instance(), 10000) == NULL);
#if HAVE_GET_EFFECT_ID
    assert(ez2_str_effect_mgr_get_effect_id(ez2str_effect_mgr_instance(), &name) == -1);
#endif
    string_free(&name);
    puts("Manager allocation failure: the EZ2STR and map-sky loads are skipped, their users see empty managers.");
    return 0;
}
