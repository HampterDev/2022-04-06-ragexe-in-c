#ifndef CONTENTS_ENCHANT_MGR_H
#define CONTENTS_ENCHANT_MGR_H

#include "base.h"
#include "contents/lua_script.h"
#include "std/std.h"
#include "std/tree.h"

struct ItemCount {
    int item_id;
    int count;
};

struct ItemAmount {
    int item_id;
    unsigned __int64 amount;
};

struct GradeBonus {
    unsigned grade;
    unsigned __int64 bonus;
};

struct NameCount {
    struct String name;
    int count;
};

struct NameAmount {
    struct String name;
    unsigned __int64 amount;
};

struct EnchantSet {
    unsigned grade;
    struct Tree amounts;
};

struct PerfectEnchant {
    unsigned __int64 zeny;
    struct Tree materials;
};
SIZE_ASSERT(PerfectEnchant, 0x10);

struct PerfectEnchantEntry {
    int item_id;
    struct PerfectEnchant perfect;
};

struct UpgradeEnchant {
    int result_item_id;
    unsigned __int64 zeny;
    struct Tree materials;
};
OFFSET_ASSERT(UpgradeEnchant, zeny, 0x08);
OFFSET_ASSERT(UpgradeEnchant, materials, 0x10);
SIZE_ASSERT(UpgradeEnchant, 0x18);

struct UpgradeEnchantEntry {
    int item_id;
    struct UpgradeEnchant upgrade;
};

struct EnchantSlot {
    unsigned __int64 require_zeny;
    struct Tree require_materials;
    unsigned __int64 success_rate;
    struct Tree grade_bonus;
    struct Tree enchant_sets;
    struct Tree perfect_enchants;
    struct Tree upgrade_enchants;
};
OFFSET_ASSERT(EnchantSlot, require_materials, 0x08);
OFFSET_ASSERT(EnchantSlot, success_rate, 0x10);
OFFSET_ASSERT(EnchantSlot, grade_bonus, 0x18);
OFFSET_ASSERT(EnchantSlot, enchant_sets, 0x20);
OFFSET_ASSERT(EnchantSlot, perfect_enchants, 0x28);
OFFSET_ASSERT(EnchantSlot, upgrade_enchants, 0x30);
SIZE_ASSERT(EnchantSlot, 0x38);

struct EnchantSlotEntry {
    unsigned index;
    struct EnchantSlot slot;
};

struct EnchantReset {
    char enabled;
    unsigned __int64 chance;
    unsigned __int64 zeny;
    struct Tree materials;
};
OFFSET_ASSERT(EnchantReset, zeny, 0x10);
OFFSET_ASSERT(EnchantReset, materials, 0x18);
SIZE_ASSERT(EnchantReset, 0x20);

struct EnchantList {
    struct Vec target_items;
    unsigned min_refine;
    unsigned min_grade;
    char approve_random_option;
    struct Vec slot_order;
    struct EnchantReset reset;
    struct String caution;
    struct Tree slots;
};
OFFSET_ASSERT(EnchantList, min_refine, 0x0c);
OFFSET_ASSERT(EnchantList, approve_random_option, 0x14);
OFFSET_ASSERT(EnchantList, slot_order, 0x18);
OFFSET_ASSERT(EnchantList, reset, 0x28);
OFFSET_ASSERT(EnchantList, caution, 0x48);

struct EnchantListEntry {
    unsigned __int64 id;
    struct EnchantList list;
};

struct EnchantMgr {
    struct Tree lists;
    struct LuaScript lua;
};

struct EnchantMgr *enchant_mgr_construct(struct EnchantMgr *self);
char enchant_mgr_load(struct EnchantMgr *self);
struct EnchantList *enchant_mgr_find_list(struct EnchantMgr *self, unsigned __int64 id);

void enchant_mgr_add_target_item(struct EnchantMgr *self, unsigned __int64 id, const struct String *item_name);
void enchant_mgr_set_condition(struct EnchantMgr *self, unsigned __int64 id, unsigned min_refine, unsigned min_grade);
void enchant_mgr_set_approve_random_option(struct EnchantMgr *self, unsigned __int64 id, char approve);
void enchant_mgr_set_slot_order(struct EnchantMgr *self, unsigned __int64 id, const struct Vec *order);
void enchant_mgr_set_reset(struct EnchantMgr *self, unsigned __int64 id, char enabled,
                           unsigned __int64 chance, unsigned __int64 zeny, const struct Tree *materials);
void enchant_mgr_set_caution(struct EnchantMgr *self, unsigned __int64 id, const struct String *text);
void enchant_mgr_set_require(struct EnchantMgr *self, unsigned __int64 id, unsigned slot,
                             unsigned __int64 zeny, const struct Tree *materials);
void enchant_mgr_set_success_rate(struct EnchantMgr *self, unsigned __int64 id, unsigned slot, unsigned __int64 rate);
void enchant_mgr_set_grade_bonus(struct EnchantMgr *self, unsigned __int64 id, unsigned slot, const struct Tree *bonus);
void enchant_mgr_set_enchant(struct EnchantMgr *self, unsigned __int64 id, unsigned slot, unsigned grade,
                             const struct Tree *amount_by_name);
void enchant_mgr_add_perfect_enchant(struct EnchantMgr *self, unsigned __int64 id, unsigned slot,
                                     const struct String *item_name, unsigned __int64 zeny,
                                     const struct Tree *materials);
void enchant_mgr_add_upgrade_enchant(struct EnchantMgr *self, unsigned __int64 id, unsigned slot,
                                     const struct String *item_name, const struct String *result_name,
                                     unsigned __int64 zeny, const struct Tree *materials);
int enchant_item_id(const struct String *item_name);

int enchant_lua_set_slot_order(struct lua_State *L);
int enchant_lua_add_target_item(struct lua_State *L);
int enchant_lua_set_condition(struct lua_State *L);
int enchant_lua_approve_random_option(struct lua_State *L);
int enchant_lua_set_reset(struct lua_State *L);
int enchant_lua_set_caution(struct lua_State *L);
int enchant_lua_set_require(struct lua_State *L);
int enchant_lua_set_success_rate(struct lua_State *L);
int enchant_lua_set_grade_bonus(struct lua_State *L);
int enchant_lua_set_enchant(struct lua_State *L);
int enchant_lua_add_perfect_enchant(struct lua_State *L);
int enchant_lua_add_upgrade_enchant(struct lua_State *L);
int enchant_lua_get_slot_count(struct lua_State *L);

struct EnchantList *enchant_list_at(struct Tree *lists, unsigned __int64 id);
struct EnchantSlot *enchant_slot_at(struct Tree *slots, unsigned index);
struct PerfectEnchant *perfect_enchant_at(struct Tree *perfects, int item_id);
struct UpgradeEnchant *upgrade_enchant_at(struct Tree *upgrades, int item_id);
struct Tree *enchant_set_at(struct Tree *sets, unsigned grade);

struct EnchantMgr *enchant_mgr_instance(void);
void enchant_mgr_destroy(void);

void enchant_list_entry_destroy(void *value);

#endif
