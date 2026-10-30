#include "ui/items/item_tag_buttons.h"
#include <stdlib.h>

void item_tag_buttons_append(struct ItemTagButtons *buttons, struct UIItemTagButton *button)
{
    size_t limit = (size_t)-1 / sizeof *buttons->items;
    if (buttons->size == limit) abort();
    if (buttons->size == buttons->capacity) {
        size_t capacity = buttons->capacity > limit / 2 ? limit : buttons->capacity * 2;
        struct UIItemTagButton **items;
        if (!capacity) capacity = 8;
        items = realloc(buttons->items, capacity * sizeof *items);
        if (!items) abort();
        buttons->items = items;
        buttons->capacity = capacity;
    }
    buttons->items[buttons->size++] = button;
}

void item_tag_buttons_clear(struct ItemTagButtons *buttons)
{
    free(buttons->items);
    buttons->items = NULL;
    buttons->size = buttons->capacity = 0;
}
