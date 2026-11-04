#include "fileio/resource_path.h"
#include "platform/code_page.h"
#include "core/utf8.h"
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int resource_normalize_name(char *out, size_t capacity, const char *name)
{
    size_t i, length;
    if (!capacity)
        return 0;
    out[0] = 0;
    if (!name)
        return 0;
    length = strlen(name);
    if (length >= capacity)
        return 0;
    for (i = 0; i < length; ++i) {
        unsigned char c = (unsigned char)name[i];
        out[i] = (char)(c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c);
    }
    out[length] = 0;
    return 1;
}

int resource_build_path(char *out, size_t capacity, const char *name, const char *directory)
{
    size_t length, prefix;
    if (!capacity)
        return 0;
    out[0] = 0;
    if (!name || !directory)
        return 0;
    length = strlen(name);
    prefix = strlen(directory);
    if (strncmp(name, directory, prefix) == 0)
        prefix = 0;
    if (length >= capacity || prefix >= capacity - length)
        return 0;
    memcpy(out, directory, prefix);
    memcpy(out + prefix, name, length + 1);
    return 1;
}

/* CP949 holds a character when it comes back from CP949 as itself: a character CP949 lacks becomes
   the default character or a best-fit substitute (Windows) or fails to convert (iconv). */
int resource_name_to_cp949(char *out, size_t capacity, const char *name)
{
    uint16_t local[2 * 260], *wide = local, *back;
    size_t length, i;
    int units, bytes, converted = 0;

    if (!capacity)
        return 0;
    out[0] = 0;
    if (!name)
        return 0;
    length = strlen(name);
    for (i = 0; i < length && (unsigned char)name[i] < 0x80; ++i)
        ;
    if (i == length || length > INT_MAX || !utf8_valid(name, length))
        return 0;
    units = platform_code_page_to_utf16(CODE_PAGE_UTF8, name, (int)length, NULL, 0);
    if (!units)
        return 0;
    if ((size_t)units > sizeof local / sizeof local[0] / 2
        && !(wide = malloc((size_t)units * 2 * sizeof *wide)))
        return 0;
    back = wide + units;
    if (platform_code_page_to_utf16(CODE_PAGE_UTF8, name, (int)length, wide, units) == units) {
        bytes = platform_code_page_from_utf16(RESOURCE_NAME_CODE_PAGE, wide, units, NULL, 0);
        if (bytes > 0 && (size_t)bytes < capacity
            && platform_code_page_from_utf16(RESOURCE_NAME_CODE_PAGE, wide, units, out, bytes) == bytes
            && platform_code_page_to_utf16(RESOURCE_NAME_CODE_PAGE, out, bytes, back, units) == units
            && !memcmp(back, wide, (size_t)units * sizeof *wide)) {
            out[bytes] = 0;
            converted = 1;
        } else {
            out[0] = 0;
        }
    }
    if (wide != local)
        free(wide);
    return converted;
}
