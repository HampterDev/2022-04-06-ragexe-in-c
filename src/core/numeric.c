#include "core/numeric.h"

uint32_t number_truncate_low32(double value)
{
    if (!(value >= -9223372036854775808.0 && value < 9223372036854775808.0))
        return 0;
    return (uint32_t)(int64_t)value;
}
