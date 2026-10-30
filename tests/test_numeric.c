#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "core/numeric.h"

static uint32_t previous_conversion(double x)
{
    int high, mid, low;
    double remainder;

    if (!(x > -9223372036854775808.0 && x < 9223372036854775808.0))
        return 0;
    high = (int)(x / 4294967296.0);
    remainder = x - (double)high * 4294967296.0;
    mid = (int)(remainder / 65536.0);
    low = (int)(remainder - (double)mid * 65536.0);
    return (uint32_t)mid * 65536u + (uint32_t)low;
}

int main(void)
{
    static const struct {
        double input;
        uint32_t expected;
    } cases[] = {
        { 0.0, 0 }, { -0.0, 0 }, { 0.9, 0 }, { -0.9, 0 },
        { 1.9, 1 }, { -1.9, UINT32_MAX },
        { 2147483648.0, UINT32_C(0x80000000) },
        { 4294967295.0, UINT32_MAX }, { 4294967296.0, 0 },
        { 4294967297.0, 1 }, { -4294967297.0, UINT32_MAX },
        { 9223372036854775808.0, 0 }, { -9223372036854775808.0, 0 },
        { INFINITY, 0 }, { -INFINITY, 0 }, { NAN, 0 }
    };
    uint64_t bits = UINT64_C(0x123456789abcdef);
    size_t i;

    for (i = 0; i < sizeof cases / sizeof cases[0]; ++i)
        assert(number_truncate_low32(cases[i].input) == cases[i].expected);

    for (i = 0; i < 200000; ++i) {
        double value;
        bits ^= bits << 13;
        bits ^= bits >> 7;
        bits ^= bits << 17;
        memcpy(&value, &bits, sizeof value);
        assert(number_truncate_low32(value) == previous_conversion(value));
    }
    puts("numeric conversion: boundary cases and 200000 differential inputs passed");
    return 0;
}
