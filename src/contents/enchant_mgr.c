#include <windows.h>
#include <lua.h>
#include "contents/enchant_mgr.h"
#include "contents/item_db_to_id.h"
#include "contents/lua_script.h"
#include "std/std.h"
#include "std/tree.h"

int enchant_item_id(const struct String *item_name)
{
    return item_db_to_id_lookup(item_db_to_id_instance(), string_cstr(item_name));
}

static void item_counts_from_names(struct Tree *out, const struct Tree *names)
{
    struct TreeNode *node;
    int item_id;

    tree_init(out);
    for (node = tree_first(names); node != names->head; node = tree_next(node)) {
        const struct NameCount *name = TREE_VALUE(node, const struct NameCount);
        struct ItemCount *count;

        item_id = enchant_item_id(&name->name);
        if (item_id) {
            count = tree_get(out, &item_id, sizeof item_id, sizeof *count, tree_less_int);
            count->count = name->count;
        }
    }
}

static void item_counts_set_from_names(struct Tree *dst, const struct Tree *names)
{
    struct Tree counts;

    item_counts_from_names(&counts, names);
    tree_free(dst, NULL);
    *dst = counts;
}

static struct EnchantSlot *slot_of(struct EnchantMgr *self, unsigned __int64 id, unsigned slot)
{
    return enchant_slot_at(&enchant_list_at(&self->lists, id)->slots, slot);
}

void enchant_mgr_add_target_item(struct EnchantMgr *self, unsigned __int64 id, const struct String *item_name)
{
    int item_id = enchant_item_id(item_name);
    struct Vec *items;

    if (!self || !item_id)
        return;
    items = &enchant_list_at(&self->lists, id)->target_items;
    vec_push(items, &item_id, sizeof(int));
}

void enchant_mgr_set_condition(struct EnchantMgr *self, unsigned __int64 id, unsigned min_refine, unsigned min_grade)
{
    struct EnchantList *list;

    if (!self)
        return;
    list = enchant_list_at(&self->lists, id);
    list->min_refine = min_refine;
    list->min_grade = min_grade;
}

void enchant_mgr_set_approve_random_option(struct EnchantMgr *self, unsigned __int64 id, char approve)
{
    if (self)
        enchant_list_at(&self->lists, id)->approve_random_option = approve;
}

void enchant_mgr_set_slot_order(struct EnchantMgr *self, unsigned __int64 id, const struct Vec *order)
{
    if (self)
        vec_assign(&enchant_list_at(&self->lists, id)->slot_order, order->first, order->last, sizeof(unsigned));
}

void enchant_mgr_set_reset(struct EnchantMgr *self, unsigned __int64 id, char enabled,
                           unsigned __int64 chance, unsigned __int64 zeny, const struct Tree *materials)
{
    struct EnchantReset *reset;

    if (!self)
        return;
    reset = &enchant_list_at(&self->lists, id)->reset;
    reset->enabled = enabled;
    reset->chance = chance;
    reset->zeny = zeny;
    item_counts_set_from_names(&reset->materials, materials);
}

void enchant_mgr_set_caution(struct EnchantMgr *self, unsigned __int64 id, const struct String *text)
{
    struct String *caution;

    if (!self)
        return;
    caution = &enchant_list_at(&self->lists, id)->caution;
    string_assign(caution, string_cstr(text), text->size);
}

void enchant_mgr_set_require(struct EnchantMgr *self, unsigned __int64 id, unsigned slot,
                             unsigned __int64 zeny, const struct Tree *materials)
{
    struct EnchantSlot *s;

    if (!self)
        return;
    s = slot_of(self, id, slot);
    s->require_zeny = zeny;
    item_counts_set_from_names(&s->require_materials, materials);
}

void enchant_mgr_set_success_rate(struct EnchantMgr *self, unsigned __int64 id, unsigned slot, unsigned __int64 rate)
{
    if (self)
        slot_of(self, id, slot)->success_rate = rate;
}

void enchant_mgr_set_grade_bonus(struct EnchantMgr *self, unsigned __int64 id, unsigned slot, const struct Tree *bonus)
{
    struct Tree *grade_bonus;

    if (!self)
        return;
    grade_bonus = &slot_of(self, id, slot)->grade_bonus;
    tree_free(grade_bonus, NULL);
    tree_copy(grade_bonus, bonus, sizeof(struct GradeBonus), NULL);
}

void enchant_mgr_set_enchant(struct EnchantMgr *self, unsigned __int64 id, unsigned slot, unsigned grade,
                             const struct Tree *amount_by_name)
{
    struct Tree amounts, *set;
    struct TreeNode *node;
    int item_id;

    if (!self)
        return;
    tree_init(&amounts);
    for (node = tree_first(amount_by_name); node != amount_by_name->head; node = tree_next(node)) {
        const struct NameAmount *name = TREE_VALUE(node, const struct NameAmount);
        struct ItemAmount *amount;

        item_id = enchant_item_id(&name->name);
        if (item_id) {
            amount = tree_get(&amounts, &item_id, sizeof item_id, sizeof *amount, tree_less_int);
            amount->amount = name->amount;
        }
    }
    set = enchant_set_at(&slot_of(self, id, slot)->enchant_sets, grade);
    tree_free(set, NULL);
    tree_copy(set, &amounts, sizeof(struct ItemAmount), NULL);
    tree_free(&amounts, NULL);
}

void enchant_mgr_add_perfect_enchant(struct EnchantMgr *self, unsigned __int64 id, unsigned slot,
                                     const struct String *item_name, unsigned __int64 zeny,
                                     const struct Tree *materials)
{
    int item_id = enchant_item_id(item_name);
    struct PerfectEnchant *perfect;

    if (!self || !item_id)
        return;
    perfect = perfect_enchant_at(&slot_of(self, id, slot)->perfect_enchants, item_id);
    perfect->zeny = zeny;
    item_counts_set_from_names(&perfect->materials, materials);
}

void enchant_mgr_add_upgrade_enchant(struct EnchantMgr *self, unsigned __int64 id, unsigned slot,
                                     const struct String *item_name, const struct String *result_name,
                                     unsigned __int64 zeny, const struct Tree *materials)
{
    int item_id = enchant_item_id(item_name), result_id;
    struct UpgradeEnchant *upgrade;

    if (!self || !item_id)
        return;
    result_id = enchant_item_id(result_name);
    if (!result_id)
        return;
    upgrade = upgrade_enchant_at(&slot_of(self, id, slot)->upgrade_enchants, item_id);
    upgrade->result_item_id = result_id;
    upgrade->zeny = zeny;
    item_counts_set_from_names(&upgrade->materials, materials);
}

static struct EnchantMgr *g_enchantMgr;

struct EnchantMgr *enchant_mgr_instance(void)
{
    if (!g_enchantMgr) {
        struct EnchantMgr *manager = mem_try_alloc(sizeof *manager);
        g_enchantMgr = manager ? enchant_mgr_construct(manager) : NULL;
    }
    return g_enchantMgr;
}

void enchant_mgr_destroy(void)
{
    if (!g_enchantMgr)
        return;
    lua_script_destruct(&g_enchantMgr->lua);
    tree_free(&g_enchantMgr->lists, enchant_list_entry_destroy);
    mem_free(g_enchantMgr);
    g_enchantMgr = NULL;
}

struct EnchantMgr *enchant_mgr_construct(struct EnchantMgr *self)
{
    tree_init(&self->lists);
    lua_script_construct(&self->lua);
    return self;
}

struct EnchantList *enchant_mgr_find_list(struct EnchantMgr *self, unsigned __int64 id)
{
    /* A NULL manager (its allocation failed) has no lists, as one whose file did not load. */
    struct EnchantListEntry *entry = self ? tree_find(&self->lists, &id, tree_less_uint64) : NULL;

    return entry ? &entry->list : NULL;
}

static char call_lua(struct EnchantMgr *self, const char *name)
{
    lua_State *L = self->lua.L;
    int ok;

    if (!L)
        return 0;
    lua_getfield(L, LUA_GLOBALSINDEX, name);
    if (lua_pcall(L, 0, 2, 0) != 0) {
        MessageBoxA(NULL, lua_tolstring(L, -1, NULL), "CEnchantMgr", 0);
        return 0;
    }
    if (lua_type(L, -2) != LUA_TBOOLEAN) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "1th argument must be boolean");
        return 0;
    }
    if (!lua_isstring(L, -1)) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "2th argument must be string");
        return 0;
    }
    ok = lua_toboolean(L, -2);
    if (!ok) {
        MessageBoxA(NULL, lua_tolstring(L, -1, NULL), "CEnchantMgr", 0);
        return 0;
    }
    return 1;
}

static void set_number(struct EnchantMgr *self, const char *name, double value)
{
    lua_script_set_number(&self->lua, name, value);
}

static void set_boolean(struct EnchantMgr *self, const char *name, char value)
{
    lua_script_set_boolean(&self->lua, name, value);
}

static void register_function(struct EnchantMgr *self, const char *name, int (*fn)(struct lua_State *))
{
    lua_script_register(&self->lua, name, fn);
}

static char run_file(struct EnchantMgr *self, const char *name)
{
    return lua_script_run_file(&self->lua, name);
}

static char call_function(struct EnchantMgr *self, const char *name)
{
    return call_lua(self, name);
}

char enchant_mgr_load(struct EnchantMgr *self)
{
    set_number(self, "MAX_REFINE_LEVEL", 20.0);
    set_number(self, "MAX_GRADE_LEVEL", 7.0);
    set_number(self, "MAX_SLOT_NUM", 4.0);
    set_number(self, "MAX_MATERIAL_NUM", 8.0);
    set_boolean(self, "IS_CLIENT", 1);
    register_function(self, "C_SetSlotOrder", enchant_lua_set_slot_order);
    register_function(self, "C_AddTargetItem", enchant_lua_add_target_item);
    register_function(self, "C_SetCondition", enchant_lua_set_condition);
    register_function(self, "C_ApproveRandomOption", enchant_lua_approve_random_option);
    register_function(self, "C_SetReset", enchant_lua_set_reset);
    register_function(self, "C_SetCaution", enchant_lua_set_caution);
    register_function(self, "C_SetRequire", enchant_lua_set_require);
    register_function(self, "C_SetSuccessRate", enchant_lua_set_success_rate);
    register_function(self, "C_SetGradeBonus", enchant_lua_set_grade_bonus);
    register_function(self, "C_SetEnchant", enchant_lua_set_enchant);
    register_function(self, "C_AddPerfectEnchant", enchant_lua_add_perfect_enchant);
    register_function(self, "C_AddUpgradeEnchant", enchant_lua_add_upgrade_enchant);
    register_function(self, "C_GetSlotCount", enchant_lua_get_slot_count);
    if (!run_file(self, "LuaFiles514\\Lua Files\\Enchant\\EnchantList_f.lub"))
        return 0;
    if (!run_file(self, "LuaFiles514\\Lua Files\\Enchant\\EnchantList.lub"))
        return 0;
    if (!call_function(self, "CheckFile"))
        return 0;
    return call_function(self, "LoadAllData");
}
