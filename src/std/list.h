#ifndef STD_LIST_H
#define STD_LIST_H

#include <stddef.h>
#include "std/container_types.h"

struct ListNode {
    struct ListNode *next;
    struct ListNode *prev;
};

#define LIST_VALUE(node, type) ((type *)((struct ListNode *)(node) + 1))
#define LIST_NODE_OF(value) ((struct ListNode *)(value) - 1)

typedef void (*ListDestroyFn)(void *value);
typedef int (*ListLessFn)(const void *a, const void *b);

void list_init(struct List *list);
void *list_insert(struct List *list, struct ListNode *where, const void *value, size_t size);
struct ListNode *list_erase(struct List *list, struct ListNode *node, ListDestroyFn destroy);
void list_clear(struct List *list, ListDestroyFn destroy);
void list_free(struct List *list, ListDestroyFn destroy);
void list_sort(struct List *list, ListLessFn less);

static __inline void *list_push_back(struct List *list, const void *value, size_t size)
{
    return list_insert(list, list->head, value, size);
}

static __inline void *list_push_front(struct List *list, const void *value, size_t size)
{
    return list_insert(list, list->head->next, value, size);
}

#endif
