#ifndef CORE_LAYOUT_H
#define CORE_LAYOUT_H

#include <stddef.h>

#ifndef STATIC_ASSERT
#define STATIC_ASSERT(name, cond) typedef char static_assert_##name[(cond) ? 1 : -1]
#endif
#define OFFSET_ASSERT(type, field, off) \
    STATIC_ASSERT(type##_##field, offsetof(struct type, field) == (off))
#define SIZE_ASSERT(type, size) STATIC_ASSERT(type##_size, sizeof(struct type) == (size))

#endif
