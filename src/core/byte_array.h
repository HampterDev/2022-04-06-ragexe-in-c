#ifndef CORE_BYTE_ARRAY_H
#define CORE_BYTE_ARRAY_H

#include <stddef.h>

struct ByteArray {
    unsigned char *items;
    size_t size, capacity;
};

void byte_array_init(struct ByteArray *array);
void byte_array_clear(struct ByteArray *array);
int byte_array_resize(struct ByteArray *array, size_t size);
int byte_array_append(struct ByteArray *array, unsigned char value);
unsigned char byte_array_at(const struct ByteArray *array, size_t index);

#endif
