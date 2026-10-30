#ifndef UI_ITEMS_ITEM_TAG_LIST_H
#define UI_ITEMS_ITEM_TAG_LIST_H

#include <stddef.h>

struct UIWindow;

struct ItemTagPayload {
    struct UIWindow *button;
    char code[64];
    char name[128];
    char link[256];
    int order;
};

struct ItemTagNode {
    struct ItemTagNode *next, *prev;
    struct ItemTagPayload tag;
};

struct ItemTagList {
    struct ItemTagNode *head;
    size_t size;
};

struct ItemTagNode *item_tag_list_append(struct ItemTagList *list, const struct ItemTagPayload *tag);
void item_tag_list_move(struct ItemTagList *to, struct ItemTagList *from, struct ItemTagNode *node);
struct ItemTagNode *item_tag_list_erase(struct ItemTagList *list, struct ItemTagNode *node);

#endif
