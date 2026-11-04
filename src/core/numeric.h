#ifndef CORE_NUMERIC_H
#define CORE_NUMERIC_H

#include <stdint.h>

uint32_t number_truncate_low32(double value);

static inline int int_min(int a, int b)
{
    return a < b ? a : b;
}

static inline int int_max(int a, int b)
{
    return a > b ? a : b;
}

#endif
