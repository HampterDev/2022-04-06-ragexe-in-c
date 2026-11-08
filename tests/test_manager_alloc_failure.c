#include "core/map_sky.h"
#include "effects/ez2str_effect_mgr.h"
#include "contents/client_lua.h"
#include "contents/client_lua_calls.h"
#include "contents/repute_api.h"
#include "contents/repute_mgr.h"
#include "contents/item_db_to_id.h"
#include "contents/item_reform.h"
#include "contents/enchant_mgr.h"
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

static struct { const char *at; size_t size; } given[64];
static int given_count;

int alloc_failure_owns(const void *p)
{
    int i;

    for (i = 0; i < given_count; i++)
        if ((const char *)p >= given[i].at && (const char *)p < given[i].at + given[i].size)
            return 1;
    return 0;
}

void *mem_try_alloc(size_t bytes)
{
    void *p;

    allocations++;
    if (fail_allocations)
        return NULL;
    p = calloc(1, bytes);
    assert(p && given_count < 64);
    given[given_count].at = p;
    given[given_count++].size = bytes;
    return p;
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
void map_sky_mgr_set_background(struct MapSkyMgr *self, const struct String *map) { (void)self; (void)map; abort(); }
void map_sky_mgr_stars(struct MapSkyMgr *self, const struct String *map) { (void)self; (void)map; abort(); }
void map_sky_mgr_make_clouds(struct MapSkyMgr *self, const struct String *map) { (void)self; (void)map; abort(); }
void map_sky_mgr_old_clouds(struct MapSkyMgr *self, const struct String *map) { (void)self; (void)map; abort(); }

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

extern int alloc_failure_window_sends, alloc_failure_window_opens, alloc_failure_script_runs, alloc_failure_repute_loads;

#include "manager_alloc_failure.inc"

static void apply_sky(const char *map_name)
{
    struct String map;

    string_init(&map);
    string_assign_cstr(&map, map_name);
    map_sky_mgr_apply(map_sky_mgr_instance(), &map);
    string_free(&map);
}

static void check_users(int missing)
{
    struct Vec list;
    struct String text;
    struct Tree none;
    int opens, sends;

    assert(ez2_str_effect_mgr_get_effect_ids(ez2str_effect_mgr_instance(), &list) == &list && list.first == list.last);
    assert(ez2_str_effect_mgr_find_by_effect_id(ez2str_effect_mgr_instance(), 10000) == NULL);
    string_init(&text);
    string_assign_cstr(&text, "EF_X");
#if HAVE_GET_EFFECT_ID
    assert(ez2_str_effect_mgr_get_effect_id(ez2str_effect_mgr_instance(), &text) == -1);
#endif
    renderer.clearColor = 0x12345678;
    apply_sky("prontera.rsw");
    assert(renderer.clearColor == 0xff000000);

    assert(repute_mgr_group_names(repute_mgr_instance(), &list) == &list && list.first == list.last);
    assert(repute_mgr_all_infos(repute_mgr_instance(), &list) == &list && list.first == list.last);
    assert(repute_mgr_group_infos(repute_mgr_instance(), &list, &text) == &list && list.first == list.last);
    assert(repute_mgr_search_infos(repute_mgr_instance(), &list, &text) == &list && list.first == list.last);
    repute_mgr_set_points(repute_mgr_instance(), 5, 10);
    opens = alloc_failure_window_opens;
    sends = alloc_failure_window_sends;
    repute_mgr_set_info_result(repute_mgr_instance(), 1);
    repute_mgr_on_changed(repute_mgr_instance(), 0, 5);
    repute_mgr_on_changed(repute_mgr_instance(), 7, 5);
    assert(alloc_failure_window_opens == opens + 2 && alloc_failure_window_sends == sends + 2);

    assert(item_db_to_id_lookup(item_db_to_id_instance(), "Red_Potion") == 0);
    assert(item_reform_base_items(item_reform_mgr(), &list, 1001) == &list && list.first == list.last);
    assert(item_reform_find(item_reform_mgr(), 1001, 1002) == NULL);
    assert(item_reform_is_reform_item(item_reform_mgr(), 1001) == 0);
    assert(item_reform_max_materials(item_reform_mgr()) == 6);
    assert(enchant_mgr_find_list(enchant_mgr_instance(), 1) == NULL);
    if (missing) {
        tree_init(&none);
        list.first = list.last = list.end = NULL;
        enchant_mgr_add_target_item(enchant_mgr_instance(), 1, &text);
        enchant_mgr_set_condition(enchant_mgr_instance(), 1, 2, 3);
        enchant_mgr_set_approve_random_option(enchant_mgr_instance(), 1, 1);
        enchant_mgr_set_slot_order(enchant_mgr_instance(), 1, &list);
        enchant_mgr_set_reset(enchant_mgr_instance(), 1, 1, 10, 20, &none);
        enchant_mgr_set_caution(enchant_mgr_instance(), 1, &text);
        enchant_mgr_set_require(enchant_mgr_instance(), 1, 0, 20, &none);
        enchant_mgr_set_success_rate(enchant_mgr_instance(), 1, 0, 50);
        enchant_mgr_set_grade_bonus(enchant_mgr_instance(), 1, 0, &none);
        enchant_mgr_set_enchant(enchant_mgr_instance(), 1, 0, 1, &none);
        enchant_mgr_add_perfect_enchant(enchant_mgr_instance(), 1, 0, &text, 20, &none);
        enchant_mgr_add_upgrade_enchant(enchant_mgr_instance(), 1, 0, &text, &text, 20, &none);
        assert(enchant_mgr_find_list(enchant_mgr_instance(), 1) == NULL);
        tree_free(&none, NULL);
    }
    string_free(&text);
}

int main(void)
{
    fail_allocations = 1;
    startup_loads();
    assert(allocations == 6 && !g_ez2strMgr && !g_mapSkyMgrInstance);
    assert(ez2str_loads == 0 && lua_loads == 0 && alloc_failure_repute_loads == 0 && alloc_failure_script_runs == 0);
    assert(!repute_mgr_instance() && !item_db_to_id_instance() && !item_reform_mgr() && !enchant_mgr_instance());

    check_users(1);
    assert(lua_calls == 0 && alloc_failure_script_runs == 0);

    fail_allocations = 0;
    startup_loads();
    assert(g_ez2strMgr && g_mapSkyMgrInstance && ez2str_loads == 1 && lua_loads == 1);
    assert(alloc_failure_repute_loads == 1 && alloc_failure_script_runs == 3);
    assert(repute_mgr_instance() && item_db_to_id_instance() && item_reform_mgr() && enchant_mgr_instance());
    check_users(0);
    assert(lua_calls == 1);
    puts("Manager allocation failure: the six managers' loads are skipped, their users answer as for unloaded managers.");
    return 0;
}
