#include "contents/enchant_mgr.h"
#include "std/std.h"
#include "std/tree.h"

struct EnchantList *enchant_list_at(struct Tree *lists, unsigned __int64 id)
{
    struct EnchantListEntry *entry = tree_find(lists, &id, tree_less_uint64);

    if (!entry) {
        entry = tree_get(lists, &id, sizeof id, sizeof *entry, tree_less_uint64);
        tree_init(&entry->list.reset.materials);
        string_init(&entry->list.caution);
        tree_init(&entry->list.slots);
    }
    return &entry->list;
}

struct EnchantSlot *enchant_slot_at(struct Tree *slots, unsigned index)
{
    struct EnchantSlotEntry *entry = tree_find(slots, &index, tree_less_uint);

    if (!entry) {
        entry = tree_get(slots, &index, sizeof index, sizeof *entry, tree_less_uint);
        tree_init(&entry->slot.require_materials);
        tree_init(&entry->slot.grade_bonus);
        tree_init(&entry->slot.enchant_sets);
        tree_init(&entry->slot.perfect_enchants);
        tree_init(&entry->slot.upgrade_enchants);
    }
    return &entry->slot;
}

struct PerfectEnchant *perfect_enchant_at(struct Tree *perfects, int item_id)
{
    struct PerfectEnchantEntry *entry = tree_find(perfects, &item_id, tree_less_int);

    if (!entry) {
        entry = tree_get(perfects, &item_id, sizeof item_id, sizeof *entry, tree_less_int);
        tree_init(&entry->perfect.materials);
    }
    return &entry->perfect;
}

struct UpgradeEnchant *upgrade_enchant_at(struct Tree *upgrades, int item_id)
{
    struct UpgradeEnchantEntry *entry = tree_find(upgrades, &item_id, tree_less_int);

    if (!entry) {
        entry = tree_get(upgrades, &item_id, sizeof item_id, sizeof *entry, tree_less_int);
        tree_init(&entry->upgrade.materials);
    }
    return &entry->upgrade;
}

struct Tree *enchant_set_at(struct Tree *sets, unsigned grade)
{
    struct EnchantSet *set = tree_find(sets, &grade, tree_less_uint);

    if (!set) {
        set = tree_get(sets, &grade, sizeof grade, sizeof *set, tree_less_uint);
        tree_init(&set->amounts);
    }
    return &set->amounts;
}
