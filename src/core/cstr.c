#include "core/cstr.h"
#include <stdint.h>

static int ascii_lower(unsigned char c)
{
    return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
}

void cstr_copy_n(char *dst, size_t size, const char *src, size_t count)
{
    size_t n = 0;

    if (size == 0)
        return;
    while (n < count && n + 1 < size && src[n] != 0) {
        dst[n] = src[n];
        n++;
    }
    dst[n] = 0;
}

void cstr_copy(char *dst, size_t size, const char *src)
{
    cstr_copy_n(dst, size, src, SIZE_MAX);
}

int cstr_compare_nocase(const char *a, const char *b)
{
    int ca, cb;

    do {
        ca = ascii_lower((unsigned char)*a++);
        cb = ascii_lower((unsigned char)*b++);
    } while (ca == cb && ca != 0);
    return ca - cb;
}

void cstr_lower(char *text)
{
    for (; *text; text++)
        *text = (char)ascii_lower((unsigned char)*text);
}
