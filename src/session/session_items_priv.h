#ifndef SESSION_ITEMS_PRIV_H
#define SESSION_ITEMS_PRIV_H

#include <stdlib.h>
#include "session/csession.h"
#include "iteminfo/iteminfo.h"

static __inline struct UIWindow *const *store_candidate_window_slot(int which)
{
    if (which == 1)
        return &g_windowMgr.buying_store_mirror_item_wnd;
    return which == 2 ? &g_windowMgr.buying_store_mirror_sell_list_wnd : &g_windowMgr.merchant_mirror_item_wnd;
}

static __inline struct ListNode *find_node_by_index(struct List *list, int index)
{
    struct ListNode *node;
    for (node = list->head->next; node != list->head; node = node->next)
        if (LIST_VALUE(node, struct ItemInfo)->index == index)
            return node;
    return NULL;
}

static __inline struct ItemInfo *find_by_index(struct List *list, int index)
{
    struct ListNode *node = find_node_by_index(list, index);
    return node ? LIST_VALUE(node, struct ItemInfo) : NULL;
}

static __inline struct ListNode *find_node_by_name(struct List *list, const struct ItemInfo *item)
{
    struct ListNode *node;
    for (node = list->head->next; node != list->head; node = node->next)
        if (string_equal(&LIST_VALUE(node, struct ItemInfo)->id_text, &item->id_text))
            return node;
    return NULL;
}

static __inline struct ItemInfo *find_by_name(struct List *list, const struct ItemInfo *item)
{
    struct ListNode *node = find_node_by_name(list, item);
    return node ? LIST_VALUE(node, struct ItemInfo) : NULL;
}

static __inline struct ItemInfo *copy_nth(struct ItemInfo *out, struct List *list, int n)
{
    struct ListNode *node;
    int i = 0;

    if (list->size != 0)
        for (node = list->head->next; node != list->head; node = node->next, i++)
            if (i == n)
                return item_info_copy(out, LIST_VALUE(node, struct ItemInfo));
    return item_info_construct_with_arg(out);
}

#endif
