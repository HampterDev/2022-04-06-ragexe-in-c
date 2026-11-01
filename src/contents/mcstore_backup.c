#include "json/service_json.h"
#include "session/state.h"
#include "session/session_items.h"
#include <stdlib.h>
#include <string.h>
#include "contents/game_session.h"
#include "iteminfo/item_api.h"
#include "contents/mcstore_backup.h"

void mcstore_init(struct McStoreBackup *self)
{
    memset(self, 0, sizeof *self);
}

void mcstore_clear_items(struct McStoreBackup *self)
{
    free(self->items);
    self->items = NULL;
    self->item_count = 0;
}

void mcstore_destroy(struct McStoreBackup *self)
{
    mcstore_clear_items(self);
    owned_text_clear(&self->title);
    mcstore_init(self);
}

static void inventory_item(struct ItemInfo *out, const struct McStoreItem *saved)
{
    int i;
    item_info_construct(out);
    item_info_set_item_id(out, saved->id);
    out->count = saved->count;
    out->price = saved->price;
    out->identified = (unsigned char)saved->identified;
    out->refine = saved->refine;
    memcpy(out->slot, saved->slot, sizeof out->slot);
    out->option_count = saved->option_count;
    for (i = 0; i < MCSTORE_OPTION_COUNT; ++i) {
        out->options[i].id = saved->options[i].id;
        out->options[i].value = saved->options[i].value;
        out->options[i].param = saved->options[i].param;
    }
}

void mcstore_restore_shop(struct McStoreBackup *self, char mode, int count)
{
    size_t at, kept = 0;
    if (count <= 0) return;
    for (at = 0; at < self->item_count && kept < (size_t)count; ++at) {
        struct ItemInfo item;
        struct InventoryMatch match;
        inventory_item(&item, &self->items[at]);
        session_find_inventory_item(&g_session1, &match, &item);
        item_info_destruct(&item);
        if (match.found && match.item.index != 0 && match.item.count > 0 &&
            !session_get_store_setup_count(&g_session1, match.item.index)) {
            if (!mode && match.item.count > self->items[at].count)
                match.item.count = self->items[at].count;
            if (session_sell_list_add(&g_session1, &match.item, (unsigned char)mode, 0)) {
                session_mirror_remove(&g_session1, &match.item, (unsigned char)mode, 0);
                self->items[kept] = self->items[at];
                if (!mode) self->items[kept].count = match.item.count;
                ++kept;
            }
        }
        item_info_destruct(&match.item);
    }
    self->item_count = kept;
}

void mcstore_reset(struct McStoreBackup *self)
{
    owned_text_clear(&self->title);
    mcstore_clear_items(self);
}

int mcstore_read_items(struct McStoreBackup *self, const struct JsonTree *data)
{
    const struct JsonTree *items, *title;
    struct McStoreBackup next = {0};
    size_t i;
    if (!data || data->type != JSON_TREE_OBJECT) return 0;
    items = json_tree_find(data, "Item", 4);
    title = json_tree_find(data, "Title", 5);
    if (!items || items->type != JSON_TREE_ARRAY || items->count > 13 ||
        !title || title->type != JSON_TREE_STRING || title->count > 1024 ||
        memchr(title->u.text, 0, title->count)) return 0;
    if (!owned_text_assign(&next.title, json_value_text(title))) return 0;
    if (items->count) {
        next.items = malloc(items->count * sizeof *next.items);
        if (!next.items) goto failed;
        for (i = 0; i < items->count; ++i)
            if (!mcstore_read_item(&next.items[i], &items->u.items[i])) goto failed;
    }
    next.item_count = items->count;
    mcstore_destroy(self);
    *self = next;
    return 1;
failed:
    mcstore_destroy(&next);
    return 0;
}
