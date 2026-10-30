#include "ui/items/item_tag_list.h"
#include <stdlib.h>

static void append_node(struct ItemTagList *list, struct ItemTagNode *node)
{
    struct ItemTagNode *last = list->head;

    while (last && last->next)
        last = last->next;
    node->prev = last;
    node->next = NULL;
    if (last)
        last->next = node;
    else
        list->head = node;
    ++list->size;
}

static void unlink_node(struct ItemTagList *list, struct ItemTagNode *node)
{
    if (node->prev)
        node->prev->next = node->next;
    else
        list->head = node->next;
    if (node->next)
        node->next->prev = node->prev;
    --list->size;
}

struct ItemTagNode *item_tag_list_append(struct ItemTagList *list, const struct ItemTagPayload *tag)
{
    struct ItemTagNode *node;

    if (list->size == (size_t)-1)
        return NULL;
    node = malloc(sizeof *node);
    if (!node)
        return NULL;
    node->tag = *tag;
    append_node(list, node);
    return node;
}

void item_tag_list_move(struct ItemTagList *to, struct ItemTagList *from, struct ItemTagNode *node)
{
    unlink_node(from, node);
    append_node(to, node);
}

struct ItemTagNode *item_tag_list_erase(struct ItemTagList *list, struct ItemTagNode *node)
{
    struct ItemTagNode *next = node->next;
    unlink_node(list, node);
    free(node);
    return next;
}
