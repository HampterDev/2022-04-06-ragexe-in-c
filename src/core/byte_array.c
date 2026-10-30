#include "core/byte_array.h"
#include <stdlib.h>
#include <string.h>

void byte_array_init(struct ByteArray *array)
{
    array->items = NULL;
    array->size = array->capacity = 0;
}

void byte_array_clear(struct ByteArray *array)
{
    free(array->items);
    byte_array_init(array);
}

int byte_array_resize(struct ByteArray *array, size_t size)
{
    if (size > array->capacity) {
        unsigned char *items = realloc(array->items, size);
        if (!items) return 0;
        array->items = items;
        array->capacity = size;
    }
    if (size > array->size)
        memset(array->items + array->size, 0, size - array->size);
    array->size = size;
    return 1;
}

int byte_array_append(struct ByteArray *array, unsigned char value)
{
    if (array->size == (size_t)-1) return 0;
    if (array->size == array->capacity) {
        size_t capacity = array->capacity > (size_t)-1 / 2 ? (size_t)-1 : array->capacity * 2;
        unsigned char *items;
        if (!capacity) capacity = 16;
        items = realloc(array->items, capacity);
        if (!items) return 0;
        array->items = items;
        array->capacity = capacity;
    }
    array->items[array->size++] = value;
    return 1;
}

unsigned char byte_array_at(const struct ByteArray *array, size_t index)
{
    return array && index < array->size ? array->items[index] : 0;
}
