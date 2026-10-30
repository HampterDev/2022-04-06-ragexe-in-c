#include "contents/enchant_mgr.h"
#include "std/std.h"
#include "std/tree.h"

static void destroy_perfect(void *value)
{
    tree_free(&((struct PerfectEnchantEntry *)value)->perfect.materials, NULL);
}

static void destroy_upgrade(void *value)
{
    tree_free(&((struct UpgradeEnchantEntry *)value)->upgrade.materials, NULL);
}

static void destroy_enchant_set(void *value)
{
    tree_free(&((struct EnchantSet *)value)->amounts, NULL);
}

static void destroy_slot(void *value)
{
    struct EnchantSlot *slot = &((struct EnchantSlotEntry *)value)->slot;

    tree_free(&slot->upgrade_enchants, destroy_upgrade);
    tree_free(&slot->perfect_enchants, destroy_perfect);
    tree_free(&slot->enchant_sets, destroy_enchant_set);
    tree_free(&slot->grade_bonus, NULL);
    tree_free(&slot->require_materials, NULL);
}

void enchant_list_entry_destroy(void *value)
{
    struct EnchantList *list = &((struct EnchantListEntry *)value)->list;

    tree_free(&list->slots, destroy_slot);
    string_free(&list->caution);
    tree_free(&list->reset.materials, NULL);
    vec_free(&list->slot_order);
    vec_free(&list->target_items);
}
