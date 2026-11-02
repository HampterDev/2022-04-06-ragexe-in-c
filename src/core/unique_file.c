#include "core/unique_file.h"
#include "platform/filesystem.h"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

FILE *file_open_unique(const char *path)
{
    const char *name, *extension, *p;
    char *candidate;
    size_t length, prefix;
    unsigned number = 0;
    FILE *file;
    int error = 0;

    if (!path || !*path) {
        errno = EINVAL;
        return NULL;
    }
    length = strlen(path);
    if (length > (size_t)-1 - 13) {
        errno = ENAMETOOLONG;
        return NULL;
    }
    name = path;
    for (p = path; *p; ++p)
        if (*p == '/' || *p == '\\' || *p == ':')
            name = p + 1;
    if (!*name) {
        errno = EINVAL;
        return NULL;
    }
    extension = strrchr(name, '.');
    if (!extension)
        extension = path + length;
    prefix = (size_t)(extension - path);
    candidate = malloc(length + 13);
    if (!candidate) {
        errno = ENOMEM;
        return NULL;
    }
    memcpy(candidate, path, length + 1);
    for (;;) {
        file = fopen(candidate, "wx");
        if (file)
            break;
        error = errno;
        if (error != EEXIST && !platform_path_is_directory(candidate))
            break;
        if (number == UINT_MAX) {
            error = EEXIST;
            break;
        }
        ++number;
        snprintf(candidate + prefix, length + 13 - prefix, "(%u)%s", number, extension);
    }
    free(candidate);
    if (!file)
        errno = error;
    return file;
}
