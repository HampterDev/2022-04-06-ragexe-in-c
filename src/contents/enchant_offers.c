#include "session/state.h"
#include <stdlib.h>
#include "contents/enchant_mgr.h"
#include "contents/enchant_ui.h"
#include "contents/enchant_wnd.h"
#include "contents/game_session.h"
#include "iteminfo/item_api.h"
#include "std/vec.h"
#include "std/std.h"
#include "std/tree.h"
#include "contents/ui_window.h"
#include "session/session_items.h"

struct UIEnchantInfo **enchant_wnd_page_ref(struct UIEnchantWnd *self, int page)
{
    struct EnchantPage *entry = tree_get(&self->pages, &page, sizeof page, sizeof *entry, tree_less_int);

    return &entry->info;
}

static const struct EnchantSlot *slot_find(const struct EnchantList *table, unsigned slot)
{
    const struct EnchantSlotEntry *entry = tree_find(&table->slots, &slot, tree_less_uint);

    return entry ? &entry->slot : 0;
}

static void results_push(struct Vec *v, int item_id, char available)
{
    struct EnchantResult r;

    r.item_id = item_id;
    r.available = available;
    vec_push(v, &r, sizeof(struct EnchantResult));
}

static void perfect_results(struct Vec *into, const struct Tree *perfects)
{
    struct TreeNode *node, *need;

    for (node = tree_first(perfects); node != perfects->head; node = tree_next(node)) {
        const struct PerfectEnchantEntry *perfect = TREE_VALUE(node, const struct PerfectEnchantEntry);
        int item_id = perfect->item_id;
        unsigned __int64 zeny = perfect->perfect.zeny;
        struct Tree needs;
        char available = 0;

        tree_copy(&needs, &perfect->perfect.materials, sizeof(struct ItemCount), NULL);
        for (need = tree_first(&needs); need != needs.head; need = tree_next(need)) {
            const struct ItemCount *count = TREE_VALUE(need, const struct ItemCount);

            if ((unsigned)session_inventory_count(&g_session1, count->item_id) < (unsigned)count->count)
                break;
        }
        if (need == needs.head && (unsigned __int64)(__int64)g_session1.zeny >= zeny)
            available = 1;
        results_push(into, item_id, available);
        tree_free(&needs, NULL);
    }
}

static void enchant_set_results(struct Vec *out, const struct EnchantSlot *slot, int grade)
{
    unsigned key = (unsigned)grade;
    const struct EnchantSet *set = tree_find(&slot->enchant_sets, &key, tree_less_uint);
    struct TreeNode *node;

    out->first = out->last = out->end = 0;
    if (!set)
        return;
    for (node = tree_first(&set->amounts); node != set->amounts.head; node = tree_next(node)) {
        int item_id = TREE_VALUE(node, const struct ItemAmount)->item_id;
        struct EnchantResult *r;
        for (r = out->first; r != out->last; r++)
            if (r->item_id == item_id)
                break;
        if (r == out->last)
            results_push(out, item_id, 1);
    }
}

static void upgrade_results(struct Vec *out, const struct EnchantSlot *slot, int item_id)
{
    const struct UpgradeEnchantEntry *up = tree_find(&slot->upgrade_enchants, &item_id, tree_less_int);

    out->first = out->last = out->end = 0;
    if (up)
        results_push(out, up->upgrade.result_item_id, 1);
}

int enchant_wnd_free_slot(struct UIEnchantWnd *self, const struct ItemInfo *item)
{
    const int *slot;

    if (!self->table)
        return -1;
    for (slot = self->table->slot_order.first; slot != self->table->slot_order.last; slot++)
        if (*slot >= 4 || item->slot[*slot] == 0)
            return *slot;
    return -1;
}

char enchant_wnd_fill_enchant(struct UIEnchantWnd *self, const struct ItemInfo *item)
{
    const struct EnchantSlot *slot;
    struct Vec results;
    int free_slot;
    char offered;

    if (!item || !self->table)
        return 0;
    enchant_info_set_target(PAGE(self, PAGE_ENCHANT), item);
    if (self->table->slot_order.first == self->table->slot_order.last)
        return 0;
    free_slot = enchant_wnd_free_slot(self, item);
    if (free_slot == -1)
        return 1;
    PAGE(self, PAGE_ENCHANT)->enchant_slot = free_slot;
    ui_window_invalidate_base((&PAGE(self, PAGE_ENCHANT)->base));
    slot = slot_find(self->table, (unsigned)free_slot);
    if (!slot)
        return 0;
    enchant_set_results(&results, slot, item->grade);
    if (results.first == results.last) {
        offered = 0;
    } else {
        enchant_info_set_results(PAGE(self, PAGE_ENCHANT), &results, 1);
        enchant_info_set_materials(PAGE(self, PAGE_ENCHANT), &slot->require_materials, slot->require_zeny);
        enchant_info_update_button(PAGE(self, PAGE_ENCHANT));
        ui_window_invalidate_base(&self->frame.base);
        offered = 1;
    }
    vec_free(&results);
    return offered;
}

char enchant_wnd_fill_perfect(struct UIEnchantWnd *self, const struct ItemInfo *item)
{
    const struct EnchantSlot *slot;
    struct Vec results;
    int free_slot;

    if (!item || !self->table)
        return 0;
    enchant_info_set_target(PAGE(self, PAGE_PERFECT), item);
    if (self->table->slot_order.first == self->table->slot_order.last)
        return 0;
    free_slot = enchant_wnd_free_slot(self, item);
    if (free_slot == -1)
        return 1;
    PAGE(self, PAGE_PERFECT)->enchant_slot = free_slot;
    ui_window_invalidate_base((&PAGE(self, PAGE_PERFECT)->base));
    slot = slot_find(self->table, (unsigned)free_slot);
    if (!slot)
        return 0;
    results.first = results.last = results.end = 0;
    perfect_results(&results, &slot->perfect_enchants);
    enchant_info_set_results(PAGE(self, PAGE_PERFECT), &results, 0);
    vec_free(&results);
    ui_window_invalidate_base(&self->frame.base);
    return 1;
}

char enchant_wnd_fill_upgrade(struct UIEnchantWnd *self, const struct ItemInfo *item)
{
    if (!item || !self->table)
        return 0;
    enchant_info_set_target(PAGE(self, PAGE_UPGRADE), item);
    enchant_info_lock_sockets(PAGE(self, PAGE_UPGRADE), 0);
    ui_window_invalidate_base(&self->frame.base);
    return 1;
}

char enchant_wnd_fill_reset(struct UIEnchantWnd *self, const struct ItemInfo *item)
{
    const struct EnchantList *table;
    const int *slot;

    if (!item || !self->table)
        return 0;
    enchant_info_set_target(PAGE(self, PAGE_RESET), item);
    table = self->table;
    for (slot = table->slot_order.first; slot != table->slot_order.last; slot++) {
        if (*slot < 4 && item->slot[*slot] != 0) {
            if (table->reset.enabled == 1) {
                enchant_info_set_materials(PAGE(self, PAGE_RESET), &table->reset.materials, table->reset.zeny);
                enchant_info_update_button(PAGE(self, PAGE_RESET));
            }
            ui_window_invalidate_base(&self->frame.base);
            return 1;
        }
    }
    return 1;
}

void enchant_wnd_select_target(struct UIEnchantWnd *self, const struct ItemInfo *item)
{
    if (!enchant_wnd_fill_enchant(self, item) && enchant_wnd_shown_page(self) == PAGE_ENCHANT)
        enchant_group_select(self->tabs, PAGE_PERFECT);
    if (!item || !self->table)
        return;
    enchant_wnd_fill_perfect(self, item);
    if (!self->table)
        return;
    enchant_wnd_fill_upgrade(self, item);
    if (!self->table)
        return;
    enchant_wnd_fill_reset(self, item);
}

char enchant_wnd_pick_perfect(struct UIEnchantWnd *self, const struct ItemInfo *item)
{
    const struct EnchantSlot *slot;
    const struct PerfectEnchantEntry *perfect;
    int item_id;

    if (!item || !self->table)
        return 0;
    slot = slot_find(self->table, (unsigned)PAGE(self, PAGE_PERFECT)->enchant_slot);
    if (!slot)
        return 0;
    item_id = atoi(string_cstr(&item->id_text));
    perfect = tree_find(&slot->perfect_enchants, &item_id, tree_less_int);
    if (!perfect)
        return 0;
    enchant_info_set_materials(PAGE(self, PAGE_PERFECT), &perfect->perfect.materials, perfect->perfect.zeny);
    enchant_info_update_button(PAGE(self, PAGE_PERFECT));
    ui_window_invalidate_base(&self->frame.base);
    return 1;
}

char enchant_wnd_pick_upgrade(struct UIEnchantWnd *self, const struct ItemInfo *item)
{
    const struct EnchantSlot *slot;
    const struct UpgradeEnchantEntry *up;
    int item_id;

    if (!item || !self->table)
        return 0;
    slot = slot_find(self->table, (unsigned)PAGE(self, PAGE_UPGRADE)->enchant_slot);
    if (!slot)
        return 0;
    item_id = enchant_info_enchant_slot_item_id(PAGE(self, PAGE_UPGRADE));
    up = tree_find(&slot->upgrade_enchants, &item_id, tree_less_int);
    if (!up)
        return 0;
    if (atoi(string_cstr(&item->id_text)) != up->upgrade.result_item_id)
        return 0;
    enchant_info_set_materials(PAGE(self, PAGE_UPGRADE), &up->upgrade.materials, up->upgrade.zeny);
    enchant_info_update_button(PAGE(self, PAGE_UPGRADE));
    ui_window_invalidate_base(&self->frame.base);
    return 1;
}

char enchant_wnd_pick_socket(struct UIEnchantWnd *self, unsigned slot_no, int item_id)
{
    const struct EnchantSlot *slot;
    struct Vec results;
    struct UIEnchantInfo *page;

    if (!item_id || !self->table)
        return 0;
    slot = slot_find(self->table, slot_no);
    if (!slot)
        return 0;
    page = PAGE(self, PAGE_UPGRADE);
    page->enchant_slot = (int)slot_no;
    ui_window_invalidate_base(&page->base);
    enchant_info_clear_materials(PAGE(self, PAGE_UPGRADE));
    upgrade_results(&results, slot, item_id);
    enchant_info_set_results(PAGE(self, PAGE_UPGRADE), &results, 1);
    vec_free(&results);
    page = PAGE(self, PAGE_UPGRADE);
    enchant_listbox_select(page->results, 0);
    ui_window_invalidate_base(&page->base);
    ui_window_invalidate_base(&self->frame.base);
    return 1;
}
