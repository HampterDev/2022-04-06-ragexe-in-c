#include <string.h>
#include <lua.h>
#include "contents/enchant_mgr.h"
#include "iteminfo/item_api.h"
#include "std/vec.h"
#include "std/std.h"
#include "std/tree.h"

static int argument_error(lua_State *L, const char *message)
{
    lua_pushboolean(L, 0);
    lua_pushstring(L, message);
    return 2;
}

static unsigned __int64 to_u64(lua_State *L, int index)
{
    return (unsigned __int64)lua_tonumber(L, index);
}

static unsigned to_unsigned(lua_State *L, int index)
{
    return (unsigned)lua_tonumber(L, index);
}

static void read_string(lua_State *L, int index, struct String *s)
{
    string_init(s);
    string_assign_cstr(s, lua_tolstring(L, index, NULL));
}

static int less_name(const void *a, const void *b)
{
    return string_compare(a, b) < 0;
}

static void destroy_name_count(void *value)
{
    string_free(&((struct NameCount *)value)->name);
}

static void destroy_name_amount(void *value)
{
    string_free(&((struct NameAmount *)value)->name);
}

static void read_name_counts(lua_State *L, struct Tree *names)
{
    struct NameCount entry, *stored;
    int added;

    tree_init(names);
    lua_pushnil(L);
    while (lua_next(L, -2)) {
        read_string(L, -2, &entry.name);
        entry.count = (int)(__int64)lua_tonumber(L, -1);
        stored = tree_add(names, &entry, sizeof entry, less_name, &added);
        if (!added) {
            stored->count = entry.count;
            string_free(&entry.name);
        }
        lua_settop(L, -2);
    }
}

int enchant_lua_set_slot_order(lua_State *L)
{
    struct Vec order = { 0, 0, 0 };
    unsigned __int64 id;
    unsigned slot;

    if (!lua_isnumber(L, -2))
        return argument_error(L, "L_SetSlotOrder : 1th argument must be number");
    if (lua_type(L, -1) != LUA_TTABLE)
        return argument_error(L, "L_SetSlotOrder : 2th argument must be table");
    id = to_u64(L, -2);
    lua_pushnil(L);
    while (lua_next(L, -2)) {
        slot = to_unsigned(L, -1);
        vec_push(&order, &slot, sizeof(unsigned));
        lua_settop(L, -2);
    }
    enchant_mgr_set_slot_order(enchant_mgr_instance(), id, &order);
    vec_free(&order);
    return 2;
}

int enchant_lua_add_target_item(lua_State *L)
{
    struct String name;
    struct EnchantMgr *mgr;
    unsigned __int64 id;

    if (!lua_isnumber(L, -2))
        return argument_error(L, "L_AddTargetItem : 1th argument must be number");
    if (!lua_isstring(L, -1))
        return argument_error(L, "L_AddTargetItem : 2th argument must be string");
    id = to_u64(L, -2);
    read_string(L, -1, &name);
    mgr = enchant_mgr_instance();
    enchant_mgr_add_target_item(mgr, id, &name);
    string_free(&name);
    return 2;
}

int enchant_lua_set_condition(lua_State *L)
{
    unsigned __int64 id;
    unsigned min_refine, min_grade;

    if (!lua_isnumber(L, -3))
        return argument_error(L, "L_SetCondition : 1th argument must be number");
    if (!lua_isnumber(L, -2))
        return argument_error(L, "L_SetCondition : 2th argument must be number");
    if (!lua_isnumber(L, -1))
        return argument_error(L, "L_SetCondition : 3th argument must be number");
    id = to_u64(L, -3);
    min_refine = to_unsigned(L, -2);
    min_grade = to_unsigned(L, -1);
    enchant_mgr_set_condition(enchant_mgr_instance(), id, min_refine, min_grade);
    return 2;
}

int enchant_lua_approve_random_option(lua_State *L)
{
    unsigned __int64 id;
    char approve;

    if (!lua_isnumber(L, -2))
        return argument_error(L, "L_ApproveRandomOption : 1th argument must be number");
    if (lua_type(L, -1) != LUA_TBOOLEAN)
        return argument_error(L, "L_ApproveRandomOption : 2th argument must be boolean");
    id = to_u64(L, -2);
    approve = lua_toboolean(L, -1) != 0;
    enchant_mgr_set_approve_random_option(enchant_mgr_instance(), id, approve);
    return 2;
}

int enchant_lua_set_reset(lua_State *L)
{
    struct Tree materials;
    unsigned __int64 id, chance, zeny;
    char enabled;

    if (!lua_isnumber(L, -5))
        return argument_error(L, "L_SetReset : 1th argument must be number");
    if (lua_type(L, -4) != LUA_TBOOLEAN)
        return argument_error(L, "L_SetReset : 2th argument must be boolean");
    if (!lua_isnumber(L, -3))
        return argument_error(L, "L_SetReset : 3th argument must be number");
    if (!lua_isnumber(L, -2))
        return argument_error(L, "L_SetReset : 4th argument must be number");
    if (lua_type(L, -1) != LUA_TTABLE)
        return argument_error(L, "L_SetReset : 5th argument must be table");
    id = to_u64(L, -5);
    enabled = lua_toboolean(L, -4) != 0;
    chance = to_u64(L, -3);
    zeny = to_u64(L, -2);
    read_name_counts(L, &materials);
    enchant_mgr_set_reset(enchant_mgr_instance(), id, enabled, chance, zeny, &materials);
    tree_free(&materials, destroy_name_count);
    return 2;
}

int enchant_lua_set_caution(lua_State *L)
{
    struct String text;
    unsigned __int64 id;

    if (!lua_isnumber(L, -2))
        return argument_error(L, "L_SetCaution : 1th argument must be number");
    if (!lua_isstring(L, -1))
        return argument_error(L, "L_SetCaution : 2th argument must be string");
    id = to_u64(L, -2);
    read_string(L, -1, &text);
    enchant_mgr_set_caution(enchant_mgr_instance(), id, &text);
    string_free(&text);
    return 2;
}

int enchant_lua_set_require(lua_State *L)
{
    struct Tree materials;
    unsigned __int64 id, zeny;
    unsigned slot;

    if (!lua_isnumber(L, -4))
        return argument_error(L, "L_SetRequire : 1th argument must be number");
    if (!lua_isnumber(L, -3))
        return argument_error(L, "L_SetRequire : 2th argument must be number");
    if (!lua_isnumber(L, -2))
        return argument_error(L, "L_SetRequire : 3th argument must be number");
    if (lua_type(L, -1) != LUA_TTABLE)
        return argument_error(L, "L_SetRequire : 4th argument must be table");
    id = to_u64(L, -4);
    slot = to_unsigned(L, -3);
    zeny = to_u64(L, -2);
    read_name_counts(L, &materials);
    enchant_mgr_set_require(enchant_mgr_instance(), id, slot, zeny, &materials);
    tree_free(&materials, destroy_name_count);
    return 2;
}

int enchant_lua_set_success_rate(lua_State *L)
{
    unsigned __int64 id, rate;
    unsigned slot;

    if (!lua_isnumber(L, -3))
        return argument_error(L, "L_SetSuccessRate : 1th argument must be number");
    if (!lua_isnumber(L, -2))
        return argument_error(L, "L_SetSuccessRate : 2th argument must be number");
    if (!lua_isnumber(L, -1))
        return argument_error(L, "L_SetSuccessRate : 3th argument must be number");
    id = to_u64(L, -3);
    slot = to_unsigned(L, -2);
    rate = to_u64(L, -1);
    enchant_mgr_set_success_rate(enchant_mgr_instance(), id, slot, rate);
    return 2;
}

int enchant_lua_set_grade_bonus(lua_State *L)
{
    struct Tree bonus;
    struct GradeBonus *entry;
    unsigned __int64 id, value;
    unsigned slot, grade;

    if (!lua_isnumber(L, -3))
        return argument_error(L, "L_SetGradeBonus : 1th argument must be number");
    if (!lua_isnumber(L, -2))
        return argument_error(L, "L_SetGradeBonus : 2th argument must be number");
    if (lua_type(L, -1) != LUA_TTABLE)
        return argument_error(L, "L_SetGradeBonus : 3th argument must be table");
    id = to_u64(L, -3);
    slot = to_unsigned(L, -2);
    tree_init(&bonus);
    lua_pushnil(L);
    while (lua_next(L, -2)) {
        grade = to_unsigned(L, -2);
        value = to_u64(L, -1);
        entry = tree_get(&bonus, &grade, sizeof grade, sizeof *entry, tree_less_uint);
        entry->bonus = value;
        lua_settop(L, -2);
    }
    enchant_mgr_set_grade_bonus(enchant_mgr_instance(), id, slot, &bonus);
    tree_free(&bonus, NULL);
    return 2;
}

int enchant_lua_set_enchant(lua_State *L)
{
    struct Tree amounts;
    struct NameAmount entry, *stored;
    unsigned __int64 id;
    unsigned slot, grade;
    int added;

    if (!lua_isnumber(L, -4))
        return argument_error(L, "L_SetEnchant : 1th argument must be number");
    if (!lua_isnumber(L, -3))
        return argument_error(L, "L_SetEnchant : 2th argument must be number");
    if (!lua_isnumber(L, -2))
        return argument_error(L, "L_SetEnchant : 3th argument must be number");
    if (lua_type(L, -1) != LUA_TTABLE)
        return argument_error(L, "L_SetEnchant : 4th argument must be table");
    id = to_u64(L, -4);
    slot = to_unsigned(L, -3);
    grade = to_unsigned(L, -2);
    tree_init(&amounts);
    lua_pushnil(L);
    while (lua_next(L, -2)) {
        read_string(L, -2, &entry.name);
        entry.amount = to_u64(L, -1);
        stored = tree_add(&amounts, &entry, sizeof entry, less_name, &added);
        if (!added) {
            stored->amount = entry.amount;
            string_free(&entry.name);
        }
        lua_settop(L, -2);
    }
    enchant_mgr_set_enchant(enchant_mgr_instance(), id, slot, grade, &amounts);
    tree_free(&amounts, destroy_name_amount);
    return 2;
}

int enchant_lua_add_perfect_enchant(lua_State *L)
{
    struct String item;
    struct Tree materials;
    unsigned __int64 id, zeny;
    unsigned slot;

    if (!lua_isnumber(L, -5))
        return argument_error(L, "L_AddPerfectEnchant : 1th argument must be number");
    if (!lua_isnumber(L, -4))
        return argument_error(L, "L_AddPerfectEnchant : 2th argument must be number");
    if (!lua_isstring(L, -3))
        return argument_error(L, "L_AddPerfectEnchant : 3th argument must be string");
    if (!lua_isnumber(L, -2))
        return argument_error(L, "L_AddPerfectEnchant : 4th argument must be number");
    if (lua_type(L, -1) != LUA_TTABLE)
        return argument_error(L, "L_AddPerfectEnchant : 5th argument must be table");
    id = to_u64(L, -5);
    slot = to_unsigned(L, -4);
    read_string(L, -3, &item);
    zeny = to_u64(L, -2);
    read_name_counts(L, &materials);
    enchant_mgr_add_perfect_enchant(enchant_mgr_instance(), id, slot, &item, zeny, &materials);
    tree_free(&materials, destroy_name_count);
    string_free(&item);
    return 2;
}

int enchant_lua_add_upgrade_enchant(lua_State *L)
{
    struct String item, result;
    struct Tree materials;
    unsigned __int64 id, zeny;
    unsigned slot;

    if (!lua_isnumber(L, -6))
        return argument_error(L, "L_AddUpgradeEnchant : 1th argument must be number");
    if (!lua_isnumber(L, -5))
        return argument_error(L, "L_AddUpgradeEnchant : 2th argument must be number");
    if (!lua_isstring(L, -4))
        return argument_error(L, "L_AddUpgradeEnchant : 3th argument must be string");
    if (!lua_isstring(L, -3))
        return argument_error(L, "L_AddUpgradeEnchant : 4th argument must be string");
    if (!lua_isnumber(L, -2))
        return argument_error(L, "L_AddUpgradeEnchant : 5th argument must be number");
    if (lua_type(L, -1) != LUA_TTABLE)
        return argument_error(L, "L_AddUpgradeEnchant : 6th argument must be table");
    id = to_u64(L, -6);
    slot = to_unsigned(L, -5);
    read_string(L, -4, &item);
    read_string(L, -3, &result);
    zeny = to_u64(L, -2);
    read_name_counts(L, &materials);
    enchant_mgr_add_upgrade_enchant(enchant_mgr_instance(), id, slot, &item, &result, zeny, &materials);
    tree_free(&materials, destroy_name_count);
    string_free(&result);
    string_free(&item);
    return 2;
}

int enchant_lua_get_slot_count(lua_State *L)
{
    struct String name;
    struct ItemInfo info;
    int item_id;

    if (lua_tolstring(L, -1, NULL)) {
        read_string(L, -1, &name);
        item_id = enchant_item_id(&name);
        string_free(&name);
        if (item_id) {
            item_info_construct(&info);
            item_info_set_item_id(&info, item_id);
            lua_pushnumber(L, (double)item_info_get_slot_count(&info));
            string_free(&info.name);
            string_free(&info.id_text);
            return 1;
        }
    }
    lua_pushnumber(L, -1.0);
    return 1;
}
