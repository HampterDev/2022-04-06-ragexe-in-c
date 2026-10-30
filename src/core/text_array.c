#include "core/text_array.h"
#include <stdlib.h>
#include <string.h>

void text_array_init(struct TextArray *array)
{
    array->items = NULL;
    array->size = array->capacity = 0;
}

void text_array_clear(struct TextArray *array)
{
    size_t i;
    for (i = 0; i < array->size; ++i) owned_text_clear(&array->items[i]);
    free(array->items);
    text_array_init(array);
}

int text_array_reserve(struct TextArray *array, size_t capacity)
{
    struct OwnedText *items;
    if (capacity > (size_t)-1 / sizeof *items) return 0;
    if (capacity <= array->capacity) return 1;
    items = realloc(array->items, capacity * sizeof *items);
    if (!items) return 0;
    array->items = items;
    array->capacity = capacity;
    return 1;
}

int text_array_resize(struct TextArray *array, size_t size)
{
    size_t i;
    if (!text_array_reserve(array, size)) return 0;
    for (i = size; i < array->size; ++i) owned_text_clear(&array->items[i]);
    for (i = array->size; i < size; ++i) owned_text_init(&array->items[i]);
    array->size = size;
    return 1;
}

int text_array_append(struct TextArray *array, struct TextView value)
{
    struct OwnedText copy = {0};
    size_t limit = (size_t)-1 / sizeof *array->items;
    size_t capacity;

    if (array->size == limit) return 0;
    if (!owned_text_assign(&copy, value)) return 0;
    if (array->size == array->capacity) {
        capacity = array->capacity > limit / 2 ? limit : array->capacity * 2;
        if (!capacity) capacity = 8;
        if (!text_array_reserve(array, capacity)) {
            owned_text_clear(&copy);
            return 0;
        }
    }
    array->items[array->size++] = copy;
    return 1;
}

void text_array_erase(struct TextArray *array, size_t index)
{
    if (index >= array->size) return;
    owned_text_clear(&array->items[index]);
    --array->size;
    memmove(array->items + index, array->items + index + 1,
            (array->size - index) * sizeof *array->items);
    owned_text_init(&array->items[array->size]);
}

struct TextView text_array_at(const struct TextArray *array, size_t index)
{
    return array && index < array->size ? owned_text_view(&array->items[index]) : text_view("", 0);
}
