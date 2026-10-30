#ifndef CORE_TEXT_ARRAY_H
#define CORE_TEXT_ARRAY_H

#include "core/owned_text.h"

struct TextArray {
    struct OwnedText *items;
    size_t size;
    size_t capacity;
};

void text_array_init(struct TextArray *array);
void text_array_clear(struct TextArray *array);
int text_array_reserve(struct TextArray *array, size_t capacity);
int text_array_resize(struct TextArray *array, size_t size);
int text_array_append(struct TextArray *array, struct TextView value);
void text_array_erase(struct TextArray *array, size_t index);
struct TextView text_array_at(const struct TextArray *array, size_t index);

#endif
