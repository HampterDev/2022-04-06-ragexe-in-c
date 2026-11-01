#include "core/fatal.h"
#include "core/abstract_method.h"
#include "core/memory.h"
#include "std/container_memory.h"
#ifndef LIFT_H
#define LIFT_H

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <intrin.h>

#ifndef STATIC_ASSERT
#define STATIC_ASSERT(name, cond) typedef char static_assert_##name[(cond) ? 1 : -1]
#endif
#define OFFSET_ASSERT(type, field, off) \
    STATIC_ASSERT(type##_##field, offsetof(struct type, field) == (off))
#define SIZE_ASSERT(type, size) STATIC_ASSERT(type##_size, sizeof(struct type) == (size))

#include "std/string.h"

#include "std/container_types.h"

#include "fileio/file_api.h"

#include "core/once.h"

#endif
