#include <mbctype.h>
#include "core/fatal.h"
#include <mbstring.h>
#include <stdlib.h>
#include <string.h>
#include "ui/ui_std.h"
#include "game/rodex_mail.h"
#include "ui/items/styling_shop_std.h"

void std_list_styling_destroy(struct List *list)
{
    void **head = (void **)list->head, **node = head[0];

    head[0] = head;
    head[1] = head;
    list->size = 0;
    while (node != (void **)list->head) {
        void **next = node[0];

        mem_free(node);
        node = next;
    }
    mem_free(list->head);
}

void std_list_styling_push_back(struct List *list, int value)
{
    struct ListNode *head = list->head, *prev = head->prev, *node;

    node = mem_alloc(sizeof *node + sizeof value);
    node->next = head;
    node->prev = prev;
    *LIST_VALUE(node, int) = value;
    if (STYLE_LIST_MAX - list->size < 1)
        fatal_error("list<T> too long");
    list->size++;
    head->prev = node;
    prev->next = node;
}
