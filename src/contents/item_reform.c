#include "core/memory.h"
#include <string.h>
#include <lua.h>
#include "contents/item_db_to_id.h"
#include "contents/item_reform.h"
#include "contents/lua_script.h"
#include "std/std.h"
#include "std/tree.h"
#include "contents/ui_window_mgr.h"
#include "core/text_util.h"

#define REFORM_MAX_MATERIALS 6

static void destroy_reform_info(void *value)
{
    reform_info_destroy(&((struct ReformInfoEntry *)value)->info);
}

static void destroy_reform_item(void *value)
{
    vec_free(&((struct ReformItem *)value)->ids);
}

static struct ReformInfo *reform_info_slot(struct Tree *reforms, unsigned long long id)
{
    struct ReformInfoEntry *entry = tree_find(reforms, &id, tree_less_uint64);

    if (!entry) {
        entry = tree_get(reforms, &id, sizeof id, sizeof *entry, tree_less_uint64);
        reform_info_init(&entry->info);
    }
    return &entry->info;
}

static int item_id_of(const char *name)
{
    return item_db_to_id_lookup(item_db_to_id_instance(), name);
}

static void report_entry(const char *fmt, unsigned long long id)
{
    struct String text;

    string_init(&text);
    string_format(&text, fmt, id);
    error_msg(string_cstr(&text), 0);
    string_free(&text);
}

static void push_field(lua_State *L, const char *name)
{
    lua_pushstring(L, name);
    lua_gettable(L, -2);
}

static void read_materials(struct ItemReformMgr *self, lua_State *L, struct ReformInfo *entry,
                           unsigned long long id)
{
    const char *name;
    int item;
    unsigned count;
    struct String text;
    struct ReformMaterial *material;

    lua_pushnil(L);
    while (lua_next(L, -2)) {
        if (self->max_materials <= entry->materials.size) {
            string_init(&text);
            string_format(&text, "Num of ReformInfo[ %I64u ].Material can't be more than %u",
                              id, self->max_materials);
            error_msg(string_cstr(&text), 0);
            string_free(&text);
        }
        name = lua_tolstring(L, -2, 0);
        item = item_id_of(name);
        count = (unsigned)lua_tointeger(L, -1);
        material = tree_get(&entry->materials, &item, sizeof item, sizeof *material, tree_less_int);
        material->count = count;
        lua_settop(L, -2);
    }
}

static void read_information(lua_State *L, struct ReformInfo *entry)
{
    lua_pushnil(L);
    while (lua_next(L, -2)) {
        string_append_cstr(&entry->information, lua_tolstring(L, -1, 0));
        string_append_char(&entry->information, '\n');
        lua_settop(L, -2);
    }
    if (entry->information.size && entry->information.data[entry->information.size - 1] == '\n')
        string_truncate(&entry->information, entry->information.size - 1);
}

static void read_reform_info(struct ItemReformMgr *self)
{
    lua_State *L;
    struct ReformInfo entry;
    struct ReformInfo *slot;
    unsigned long long id;
    long long value;
    const char *text;

    tree_clear(&self->reforms, destroy_reform_info);
    L = self->lua.L;
    if (!L)
        return;
    lua_getfield(L, LUA_GLOBALSINDEX, "ReformInfo");
    if (lua_type(L, -1) != LUA_TTABLE) {
        error_msg("Can't find global table \"ReformInfo\"", 0);
        lua_settop(L, -2);
        return;
    }
    memset(&entry, 0, sizeof(entry));
    lua_pushnil(L);
    while (lua_next(L, -2)) {
        id = (unsigned long long)(long long)lua_tointeger(L, -2);
        reform_info_init(&entry);

        push_field(L, "BaseItem");
        if (lua_isstring(L, -1)) {
            text = lua_tolstring(L, -1, 0);
            entry.base_item = item_id_of(text);
        } else {
            report_entry("ReformInfo[ %I64u ] : Can't find BaseItem", id);
        }
        lua_settop(L, -2);

        push_field(L, "ResultItem");
        if (lua_isstring(L, -1)) {
            text = lua_tolstring(L, -1, 0);
            entry.result_item = item_id_of(text);
        } else {
            report_entry("ReformInfo[ %I64u ] : Can't find ResultItem", id);
        }
        lua_settop(L, -2);

        push_field(L, "Material");
        if (lua_type(L, -1) == LUA_TTABLE)
            read_materials(self, L, &entry, id);
        else
            report_entry("ReformInfo[ %I64u ] : Can't find Material", id);
        lua_settop(L, -2);

        push_field(L, "NeedRefineMin");
        if (lua_isnumber(L, -1)) {
            value = lua_tointeger(L, -1);
            if (value < 0)
                report_entry("ReformInfo[ %I64u ].NeedRefineMin can't be less than 0", id);
            entry.need_refine_min = value;
        } else {
            report_entry("ReformInfo[ %I64u ] : Can't find NeedRefineMin", id);
        }
        lua_settop(L, -2);

        push_field(L, "NeedRefineMax");
        if (lua_isnumber(L, -1)) {
            value = lua_tointeger(L, -1);
            if ((unsigned long long)entry.need_refine_min > (unsigned long long)value || value > 20)
                report_entry("ReformInfo[ %I64u ].needRefineMax must be between needRefineMin and 20", id);
            entry.need_refine_max = value;
        } else {
            report_entry("ReformInfo[ %I64u ] : Can't find NeedRefineMax", id);
        }
        lua_settop(L, -2);

        push_field(L, "NeedOptionNumMin");
        if (lua_isnumber(L, -1)) {
            value = lua_tointeger(L, -1);
            if (value < 0)
                report_entry("ReformInfo[ %I64u ].needRanOptNum can't be less than 0", id);
            entry.need_option_num_min = value;
        } else {
            report_entry("ReformInfo[ %I64u ] : Can't find NeedOptionNumMin", id);
        }
        lua_settop(L, -2);

        push_field(L, "IsEmptySocket");
        if (lua_type(L, -1) == LUA_TBOOLEAN)
            entry.is_empty_socket = lua_toboolean(L, -1) > 0;
        else
            report_entry("ReformInfo[ %I64u ] : Can't find IsEmptySocket", id);
        lua_settop(L, -2);

        push_field(L, "ChangeRefineValue");
        if (lua_isnumber(L, -1)) {
            value = lua_tointeger(L, -1);
            if (value < -20 || value > 20)
                report_entry("ReformInfo[ %I64u ].ChangeRefineValue can't be less than 0 and more than 20", id);
            entry.change_refine_value = value;
        } else {
            report_entry("ReformInfo[ %I64u ] : Can't find ChangeRefineValue", id);
        }
        lua_settop(L, -2);

        push_field(L, "RandomOptionCode");
        text = lua_tolstring(L, -1, 0);
        string_assign_cstr(&entry.random_option_code, text);
        lua_settop(L, -2);

        push_field(L, "PreserveSocketItem");
        if (lua_type(L, -1) == LUA_TBOOLEAN)
            entry.preserve_socket_item = lua_toboolean(L, -1) > 0;
        else
            report_entry("ReformInfo[ %I64u ] : Can't find PreserveSocketItem", id);
        lua_settop(L, -2);

        push_field(L, "PreserveGrade");
        if (lua_type(L, -1) == LUA_TBOOLEAN)
            entry.preserve_grade = lua_toboolean(L, -1) > 0;
        else
            report_entry("ReformInfo[ %I64u ] : Can't find PreserveGrade", id);
        lua_settop(L, -2);

        push_field(L, "InformationString");
        if (lua_type(L, -1) == LUA_TTABLE)
            read_information(L, &entry);
        lua_settop(L, -2);

        slot = reform_info_slot(&self->reforms, id);
        slot->base_item = entry.base_item;
        slot->result_item = entry.result_item;
        tree_free(&slot->materials, NULL);
        tree_copy(&slot->materials, &entry.materials, sizeof(struct ReformMaterial), NULL);
        slot->need_refine_min = entry.need_refine_min;
        slot->need_refine_max = entry.need_refine_max;
        slot->need_option_num_min = entry.need_option_num_min;
        slot->is_empty_socket = entry.is_empty_socket;
        slot->change_refine_value = entry.change_refine_value;
        string_assign(&slot->random_option_code, string_cstr(&entry.random_option_code), entry.random_option_code.size);
        slot->preserve_socket_item = entry.preserve_socket_item;
        slot->preserve_grade = entry.preserve_grade;
        string_assign(&slot->information, string_cstr(&entry.information), entry.information.size);

        reform_info_destroy(&entry);
        lua_settop(L, -2);
    }
    lua_settop(L, -2);
}

static void read_reform_item_list(struct ItemReformMgr *self)
{
    lua_State *L;
    struct String name, text;
    struct ReformItem *reform_item;
    const char *key;
    unsigned long long id;
    int item;

    tree_clear(&self->reform_items, destroy_reform_item);
    L = self->lua.L;
    if (!L)
        return;
    lua_getfield(L, LUA_GLOBALSINDEX, "ReformItemList");
    if (lua_type(L, -1) != LUA_TTABLE)
        error_msg("Can't find global table \"ReformItemList\"", 0);
    lua_pushnil(L);
    while (lua_next(L, -2)) {
        key = lua_tolstring(L, -2, 0);
        string_init(&name);
        string_assign_cstr(&name, key);
        item = item_id_of(string_cstr(&name));
        lua_pushnil(L);
        while (lua_next(L, -2)) {
            id = (unsigned)lua_tointeger(L, -1);
            if (!tree_find(&self->reforms, &id, tree_less_uint64)) {
                string_init(&text);
                string_format(&text, "ReformItemList[ %s ] : Can't find [ %I64u ] in ReformInfo",
                                  string_cstr(&name), id);
                error_msg(string_cstr(&text), 0);
                string_free(&text);
            } else {
                reform_item = tree_get(&self->reform_items, &item, sizeof item, sizeof *reform_item, tree_less_int);
                vec_push(&reform_item->ids, &id, sizeof(unsigned long long));
            }
            lua_settop(L, -2);
        }
        string_free(&name);
        lua_settop(L, -2);
    }
    lua_settop(L, -2);
}

static struct ItemReformMgr *g_itemReformMgr;

struct ItemReformMgr *item_reform_mgr(void)
{
    if (!g_itemReformMgr) {
        struct ItemReformMgr *manager = mem_try_alloc(sizeof *manager);
        g_itemReformMgr = manager ? item_reform_mgr_construct(manager) : NULL;
    }
    return g_itemReformMgr;
}

void item_reform_mgr_destroy(void)
{
    if (!g_itemReformMgr)
        return;
    item_reform_mgr_destruct(g_itemReformMgr);
    mem_free(g_itemReformMgr);
    g_itemReformMgr = NULL;
}

struct ItemReformMgr *item_reform_mgr_construct(struct ItemReformMgr *self)
{
    tree_init(&self->reforms);
    tree_init(&self->reform_items);
    lua_script_construct(&self->lua);
    self->max_materials = REFORM_MAX_MATERIALS;
    return self;
}

void item_reform_mgr_destruct(struct ItemReformMgr *self)
{
    lua_script_destruct(&self->lua);
    tree_free(&self->reform_items, destroy_reform_item);
    tree_free(&self->reforms, destroy_reform_info);
}

char item_reform_mgr_load(struct ItemReformMgr *self)
{
    char loaded;

    loaded = lua_script_run_file(&self->lua, "LuaFiles514\\Lua Files\\ItemReform\\ItemReformSystem.lub");
    if (!loaded)
        return 0;
    read_reform_info(self);
    read_reform_item_list(self);
    return 1;
}

struct Vec *item_reform_base_items(struct ItemReformMgr *self, struct Vec *out,
                                                    int reform_item)
{
    struct Vec bases = { 0, 0, 0 };
    const struct ReformItem *node;
    const unsigned long long *id;
    struct ReformInfoEntry *info;

    node = tree_find(&self->reform_items, &reform_item, tree_less_int);
    if (!node) {
        out->first = out->last = out->end = 0;
    } else {
        for (id = node->ids.first; id != node->ids.last; id++) {
            info = tree_find(&self->reforms, id, tree_less_uint64);
            if (!info)
                continue;
            vec_push(&bases, &info->info.base_item, sizeof(int));
        }
        *out = bases;
        bases.first = bases.last = bases.end = 0;
    }
    vec_free(&bases);
    return out;
}

struct ReformInfo *item_reform_find(struct ItemReformMgr *self, int reform_item, int base_item)
{
    const struct ReformItem *node;
    const unsigned long long *id;
    struct ReformInfoEntry *info;

    node = tree_find(&self->reform_items, &reform_item, tree_less_int);
    if (!node)
        return 0;
    for (id = node->ids.first; id != node->ids.last; id++) {
        info = tree_find(&self->reforms, id, tree_less_uint64);
        if (info && info->info.base_item == base_item)
            return &info->info;
    }
    return 0;
}

char item_reform_is_reform_item(struct ItemReformMgr *self, int item)
{
    return tree_find(&self->reform_items, &item, tree_less_int) != 0;
}
