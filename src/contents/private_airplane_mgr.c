#include "fileio/res_mgr.h"
#include "core/memory.h"
#include <string.h>
#include "contents/lua_data.h"
#include "std/std.h"
#include "contents/private_airplane.h"

struct PrivateAirplaneMgr *g_airshipTickets;

static int map_less(const void *a, const void *b)
{
    return string_compare(a, b) < 0;
}

static void destroy_map(void *value)
{
    string_free(value);
}

static struct PrivateAirplaneMgr *private_airplane_construct(struct PrivateAirplaneMgr *self)
{
    tree_init(&self->startable_maps);
    tree_init(&self->endable_maps);
    tree_init(&self->need_items);
    return self;
}

static void private_airplane_destruct(struct PrivateAirplaneMgr *self)
{
    tree_free(&self->need_items, NULL);
    tree_free(&self->endable_maps, destroy_map);
    tree_free(&self->startable_maps, destroy_map);
}

int private_airplane_need_item(struct PrivateAirplaneMgr *self, int index)
{
    struct TreeNode *head = self->need_items.head, *node = tree_first(&self->need_items);
    int i = 0;

    while (i < index && node != head) {
        node = tree_next(node);
        i++;
    }
    return *TREE_VALUE(node, int);
}

static char map_set_contains(struct Tree *set, const struct String *map)
{
    return tree_find(set, map, map_less) != NULL;
}

static void map_set_add(struct Tree *set, const struct String *map)
{
    struct String copy;
    int added;

    string_copy(&copy, map);
    tree_add(set, &copy, sizeof copy, map_less, &added);
    if (!added)
        string_free(&copy);
}

char private_airplane_can_start(struct PrivateAirplaneMgr *self, struct String map)
{
    char found = map_set_contains(&self->startable_maps, &map);

    string_free(&map);
    return found;
}

char private_airplane_can_end(struct PrivateAirplaneMgr *self, struct String map)
{
    char found = map_set_contains(&self->endable_maps, &map);

    string_free(&map);
    return found;
}

static int insert_map(lua_State *L, struct Tree *set)
{
    const char *name = lua_tolstring(L, -1, 0);
    struct String map;

    string_init(&map);
    string_assign_cstr(&map, name);
    string_append_cstr(&map, ".gat");
    map_set_add(set, &map);
    lua_data_reply(L, 1, "good");
    string_free(&map);
    return 2;
}

static int lua_insert_startable_map(lua_State *L)
{
    if (!lua_isstring(L, -1))
        return lua_data_reply(L, 0, "[InsertStartableMap] argument 1st is must string");
    return insert_map(L, &g_airshipTickets->startable_maps);
}

static int lua_insert_endable_map(lua_State *L)
{
    if (!lua_isstring(L, -1))
        return lua_data_reply(L, 0, "[InsertEndableMap] argument 1st is must string");
    return insert_map(L, &g_airshipTickets->endable_maps);
}

static int lua_insert_need_item(lua_State *L)
{
    int item_id;

    if (!lua_isstring(L, -1))
        return lua_data_reply(L, 0, "[InsertNeedItem] argument 1st is must string");
    item_id = (int)lua_tonumber(L, -2);
    tree_add(&g_airshipTickets->need_items, &item_id, sizeof item_id, tree_less_int, NULL);
    return lua_data_reply(L, 1, "good");
}

static const struct LuaDataFunction private_airplane_functions[] = {
    { "InsertStartableMap", lua_insert_startable_map },
    { "InsertEndableMap", lua_insert_endable_map },
    { "InsertNeedItem", lua_insert_need_item },
};

static char load_file(void)
{
    return lua_data_run("System\\PrivateAirplane_True.lub", "CPrivateAirplaneMgr",
                        private_airplane_functions,
                        sizeof private_airplane_functions / sizeof private_airplane_functions[0],
                        LUA_DATA_ERROR_STRING);
}

char private_airplane_create(void)
{
    struct PrivateAirplaneMgr *mgr;

    if (g_airshipTickets)
        return 0;
    mgr = (struct PrivateAirplaneMgr *)mem_try_alloc(sizeof *mgr);
    if (mgr)
        private_airplane_construct(mgr);
    g_airshipTickets = mgr;
    if (!mgr)
        return 0;
    if (load_file())
        return 1;
    private_airplane_destroy();
    return 0;
}

void private_airplane_destroy(void)
{
    if (!g_airshipTickets)
        return;
    private_airplane_destruct(g_airshipTickets);
    mem_free(g_airshipTickets);
    g_airshipTickets = 0;
}
