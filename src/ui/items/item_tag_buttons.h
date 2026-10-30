#ifndef UI_ITEMS_ITEM_TAG_BUTTONS_H
#define UI_ITEMS_ITEM_TAG_BUTTONS_H

#include <stddef.h>
struct UIItemTagButton;

struct ItemTagButtons {
    struct UIItemTagButton **items;
    size_t size, capacity;
};

void item_tag_buttons_append(struct ItemTagButtons *buttons, struct UIItemTagButton *button);
void item_tag_buttons_clear(struct ItemTagButtons *buttons);

#endif
