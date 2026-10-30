#include "core/memory.h"
#include <string.h>
#include "contents/lua_data.h"
#include "std/std.h"
#include "contents/town_info.h"

struct TownInfoMgr {
    struct Tree towns;
};

struct TownFacilities {
    struct String map;
    struct Vec facilities;
};

struct TownInfoMgr *g_townInfo;

static void destroy_town(void *value);

#define UI "유저인터페이스\\"

static const char *const town_icon_files[] = {
    UI "Information\\Store.bmp",
    UI "Information\\Weaponshop.bmp",
    UI "Information\\ArmorShops.bmp",
    UI "Information\\Smithy.bmp",
    UI "Information\\Guide.bmp",
    UI "Information\\Inn.bmp",
    UI "Information\\kafra.bmp",
    UI "Information\\style.bmp",
    UI "Information\\warp.bmp",
    UI "minimap\\Quest.bmp",
};

static void town_info_copy(struct TownInfo *dst, const struct TownInfo *src)
{
    dst->kind = src->kind;
    string_copy(&dst->name, &src->name);
    dst->x = src->x;
    dst->y = src->y;
    string_copy(&dst->icon, &src->icon);
}

static void town_info_free(struct TownInfo *info)
{
    string_free(&info->icon);
    string_free(&info->name);
}

static struct TownInfoMgr *town_info_construct(struct TownInfoMgr *self)
{
    tree_init(&self->towns);
    return self;
}

static void town_info_destruct(struct TownInfoMgr *self)
{
    tree_free(&self->towns, destroy_town);
}

static int map_less(const void *a, const void *b)
{
    return string_compare(a, b) < 0;
}

static struct TownFacilities *find_town(struct TownInfoMgr *mgr, const struct String *map)
{
    return tree_find(&mgr->towns, map, map_less);
}

static void push_facility(struct Vec *facilities, const struct TownInfo *info)
{
    struct TownInfo copy;

    town_info_copy(&copy, info);
    vec_push(facilities, &copy, sizeof copy);
}

void town_info_add(struct TownInfoMgr *self, struct String map, struct String name,
                              int x, int y, int kind)
{
    struct TownFacilities *node;
    struct TownInfo info;

    node = find_town(self, &map);
    info.kind = kind;
    string_copy(&info.name, &name);
    info.x = x;
    info.y = y;
    string_init(&info.icon);
    string_assign_cstr(&info.icon, town_icon_files[kind]);
    if (node) {
        push_facility(&node->facilities, &info);
    } else {
        struct TownFacilities town;

        string_copy(&town.map, &map);
        vec_init(&town.facilities);
        push_facility(&town.facilities, &info);
        tree_add(&self->towns, &town, sizeof town, map_less, NULL);
    }
    town_info_free(&info);
    string_free(&map);
    string_free(&name);
}

static void town_info_none(struct TownInfo *out)
{
    out->kind = -1;
    string_init(&out->name);
    out->x = -1;
    out->y = -1;
    string_init(&out->icon);
}

struct TownInfo *town_info_get(struct TownInfoMgr *self, struct TownInfo *out,
                                          struct String map, unsigned index)
{
    struct String key;
    struct TownFacilities *node;
    const char *text, *dot;

    string_copy(&key, &map);
    text = string_cstr(&key);
    dot = key.size ? memchr(text, '.', key.size) : 0;
    if (dot) {
        struct String stem;

        string_init(&stem);
        string_assign(&stem, text, (size_t)(dot - text));
        string_free(&key);
        key = stem;
    }
    node = find_town(self, &key);
    if (node && index < VEC_COUNT(&node->facilities, struct TownInfo))
        town_info_copy(out, (struct TownInfo *)node->facilities.first + index);
    else
        town_info_none(out);
    string_free(&key);
    string_free(&map);
    return out;
}

static int lua_add_town_info(lua_State *L)
{
    const char *map_text, *name_text;
    struct String map, name;
    int x, y, kind;

    if (!lua_isnumber(L, -1))
        return lua_data_reply(L, 0, "5th argment is must number");
    if (!lua_isnumber(L, -2))
        return lua_data_reply(L, 0, "4th argment is must number");
    if (!lua_isnumber(L, -3))
        return lua_data_reply(L, 0, "3th argment is must number");
    if (!lua_isstring(L, -4))
        return lua_data_reply(L, 0, "2nd argment is must string");
    if (!lua_isstring(L, -5))
        return lua_data_reply(L, 0, "1st argment is must string");
    map_text = lua_tolstring(L, -5, 0);
    name_text = lua_tolstring(L, -4, 0);
    x = (int)lua_tonumber(L, -3);
    y = (int)lua_tonumber(L, -2);
    kind = (int)lua_tonumber(L, -1);
    string_init(&name);
    string_assign_cstr(&name, name_text);
    string_init(&map);
    string_assign_cstr(&map, map_text);
    town_info_add(g_townInfo, map, name, x, y, kind);
    return lua_data_reply(L, 1, "good");
}

static const struct LuaDataFunction town_info_functions[] = {
    { "AddTownInfo", lua_add_town_info },
};

char town_info_load(void)
{
    return lua_data_run("System\\Towninfo.lub", "CTownInfoMgr", town_info_functions,
                        sizeof town_info_functions / sizeof town_info_functions[0],
                        LUA_DATA_ERROR_PREFIX);
}

char town_info_create(void)
{
    struct TownInfoMgr *mgr;

    if (g_townInfo)
        return 0;
    mgr = (struct TownInfoMgr *)mem_try_alloc(sizeof *mgr);
    if (mgr)
        town_info_construct(mgr);
    g_townInfo = mgr;
    if (!mgr)
        return 0;
    return 1;
}

void town_info_destroy(void)
{
    if (!g_townInfo)
        return;
    town_info_destruct(g_townInfo);
    mem_free(g_townInfo);
    g_townInfo = 0;
}

static void facilities_free(struct Vec *facilities)
{
    struct TownInfo *info;

    for (info = facilities->first; info != facilities->last; info++)
        town_info_free(info);
    vec_free(facilities);
}

static void destroy_town(void *value)
{
    struct TownFacilities *town = value;

    facilities_free(&town->facilities);
    string_free(&town->map);
}
