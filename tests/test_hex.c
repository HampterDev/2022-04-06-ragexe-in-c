#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "core/hex.h"

int main(void)
{
    const char *lower = "0123456789abcdef";
    const char *upper = "0123456789ABCDEF";
    unsigned c;

    for (c = 0; c <= 255; ++c) {
        const char *found = c ? strchr(lower, (int)c) : NULL;
        unsigned expected = 0;
        if (found)
            expected = (unsigned)(found - lower);
        else if (c && (found = strchr(upper, (int)c)) != NULL)
            expected = (unsigned)(found - upper);
        assert(hex_digit_value((unsigned char)c) == expected);
        assert(hex_digit_value((unsigned char)(signed char)c) == expected);
    }
    puts("hex decoding: all 256 byte values and signed-char inputs passed");
    return 0;
}
