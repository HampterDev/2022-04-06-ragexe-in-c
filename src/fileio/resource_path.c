#include "fileio/resource_path.h"
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
