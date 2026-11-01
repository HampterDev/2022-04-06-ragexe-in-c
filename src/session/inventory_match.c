#include "session/state.h"
#include "contents/game_session.h"
#include "iteminfo/item_api.h"
#include "std/std.h"

struct InventoryMatch *session_find_inventory_item(struct Session *self,
                                                   struct InventoryMatch *out, const struct ItemInfo *item)
{
    struct List *list = &self->storeCandidateItems;
    struct ListNode *node;

    for (node = list->head->next; node != list->head; node = node->next) {
        const struct ItemInfo *have = LIST_VALUE(node, struct ItemInfo);
        int count, i;

        if (!string_equal(&have->id_text, &item->id_text))
            continue;
        if (have->slot[0] != item->slot[0] || have->slot[1] != item->slot[1]
            || have->slot[2] != item->slot[2] || have->slot[3] != item->slot[3])
            continue;
        if (have->identified != item->identified || have->refine != item->refine)
            continue;
        count = have->option_count;
        if (count != item->option_count || count < 0 || count > 5)
            continue;
        if (count == 0)
            break;
        for (i = 0; i < count; i++) {
            const struct ItemOption *a = &have->options[i], *b = &item->options[i];
            if (a->id != b->id || a->value != b->value || a->param != b->param)
                break;
        }
        if (i == count)
            break;
    }
    if (node != list->head) {
        out->found = 1;
        item_info_copy(&out->item, LIST_VALUE(node, struct ItemInfo));
    } else {
        struct ItemInfo empty;
        item_info_construct(&empty);
        out->found = 0;
        item_info_copy(&out->item, &empty);
        string_free(&empty.name);
        string_free(&empty.id_text);
    }
    return out;
}
