#include "std/list.h"
#include <string.h>
#include "core/memory.h"

void list_init(struct List *list)
{
    struct ListNode *head = mem_alloc(sizeof *head);

    head->next = head->prev = head;
    list->head = head;
    list->size = 0;
}

void *list_insert(struct List *list, struct ListNode *where, const void *value, size_t size)
{
    struct ListNode *node = mem_alloc(sizeof *node + size);

    if (value)
        memcpy(node + 1, value, size);
    else
        memset(node + 1, 0, size);
    node->next = where;
    node->prev = where->prev;
    where->prev->next = node;
    where->prev = node;
    list->size++;
    return node + 1;
}

struct ListNode *list_erase(struct List *list, struct ListNode *node, ListDestroyFn destroy)
{
    struct ListNode *next = node->next;

    node->prev->next = next;
    next->prev = node->prev;
    list->size--;
    if (destroy)
        destroy(node + 1);
    mem_free(node);
    return next;
}

void list_clear(struct List *list, ListDestroyFn destroy)
{
    struct ListNode *head = list->head, *node;

    if (!head)
        return;
    node = head->next;
    head->next = head->prev = head;
    list->size = 0;
    while (node != head) {
        struct ListNode *next = node->next;

        if (destroy)
            destroy(node + 1);
        mem_free(node);
        node = next;
    }
}

void list_free(struct List *list, ListDestroyFn destroy)
{
    if (!list->head)
        return;
    list_clear(list, destroy);
    mem_free(list->head);
    list->head = NULL;
}

static struct ListNode *merge_sort(struct ListNode *first, unsigned count, ListLessFn less)
{
    struct ListNode *a, *b, *out = NULL, **tail = &out;
    unsigned half = count / 2, i;

    if (count == 0)
        return NULL;
    if (count == 1) {
        first->next = NULL;
        return first;
    }
    b = first;
    for (i = 0; i < half; i++)
        b = b->next;
    a = merge_sort(first, half, less);
    b = merge_sort(b, count - half, less);
    while (a && b) {
        if (less(b + 1, a + 1)) {
            *tail = b;
            b = b->next;
        } else {
            *tail = a;
            a = a->next;
        }
        tail = &(*tail)->next;
    }
    *tail = a ? a : b;
    return out;
}

void list_sort(struct List *list, ListLessFn less)
{
    struct ListNode *head = list->head, *node, *prev = head;

    node = merge_sort(head->next, list->size, less);
    for (; node; prev = node, node = node->next) {
        node->prev = prev;
        prev->next = node;
    }
    prev->next = head;
    head->prev = prev;
}
