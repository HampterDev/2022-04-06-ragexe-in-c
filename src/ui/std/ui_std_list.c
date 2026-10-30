#include <string.h>
#include "std/list.h"
#include "ui/ui_std.h"
#include "ui/window_list.h"

void ui_window_list_copy(struct List *dst, const struct List *src)
{
    struct ListNode *node;

    list_init(dst);
    for (node = src->head->next; node != src->head; node = node->next)
        list_push_back(dst, LIST_VALUE(node, struct UIWindow *), sizeof(struct UIWindow *));
}
