#include "fileio/resource_path.h"
#include "platform/code_page.h"
#include <windows.h>
#include <limits.h>
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

int resource_name_to_cp949(char *out, size_t capacity, const char *name)
{
    wchar_t local[260], *wide = local;
    size_t length, i;
    int units, bytes, converted = 0;
    BOOL lossy = FALSE;

    if (!capacity)
        return 0;
    out[0] = 0;
    if (!name)
        return 0;
    length = strlen(name);
    for (i = 0; i < length && (unsigned char)name[i] < 0x80; ++i)
        ;
    if (i == length || length > INT_MAX)
        return 0;
    units = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name, (int)length, NULL, 0);
    if (!units)
        return 0;
    if ((size_t)units > sizeof local / sizeof local[0] && !(wide = malloc((size_t)units * sizeof *wide)))
        return 0;
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name, (int)length, wide, units) == units) {
        bytes = WideCharToMultiByte(RESOURCE_NAME_CODE_PAGE, WC_NO_BEST_FIT_CHARS, wide, units, NULL, 0, NULL,
                                    &lossy);
        if (bytes > 0 && !lossy && (size_t)bytes < capacity
            && WideCharToMultiByte(RESOURCE_NAME_CODE_PAGE, WC_NO_BEST_FIT_CHARS, wide, units, out, bytes, NULL,
                                   &lossy) == bytes && !lossy) {
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
