#include "fileio/resource_errors.h"
#include "ui/window_mgr.h"
#include "platform/thread.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static struct {
    struct PlatformMutex *lock;
    char **paths;
    size_t count, capacity;
} errors;

void resource_errors_init(void)
{
    memset(&errors, 0, sizeof errors);
    errors.lock = platform_mutex_create();
}

void resource_errors_destroy(void)
{
    size_t i;
    for (i = 0; i < errors.count; ++i)
        free(errors.paths[i]);
    free(errors.paths);
    platform_mutex_destroy(errors.lock);
    memset(&errors, 0, sizeof errors);
}

static int remember_path(const char *path)
{
    size_t first = 0, last = errors.count, length;
    char *copy;
    while (first < last) {
        size_t middle = first + (last - first) / 2;
        if (strcmp(errors.paths[middle], path) < 0)
            first = middle + 1;
        else
            last = middle;
    }
    if (first < errors.count && strcmp(errors.paths[first], path) == 0)
        return 0;
    if (errors.count == errors.capacity) {
        size_t capacity;
        char **paths;
        if (errors.capacity > SIZE_MAX / 2 / sizeof *paths)
            abort();
        capacity = errors.capacity ? errors.capacity * 2 : 16;
        paths = realloc(errors.paths, capacity * sizeof *paths);
        if (!paths)
            abort();
        errors.paths = paths;
        errors.capacity = capacity;
    }
    length = strlen(path);
    if (length == SIZE_MAX)
        abort();
    copy = malloc(length + 1);
    if (!copy)
        abort();
    memcpy(copy, path, length + 1);
    memmove(errors.paths + first + 1, errors.paths + first,
            (errors.count - first) * sizeof *errors.paths);
    errors.paths[first] = copy;
    ++errors.count;
    return 1;
}

void resource_error_report(const char *name, const char *path)
{
    const char *extension;
    int first;
    if (!name || !path)
        return;
    extension = strrchr(path, '.');
    if (extension && _stricmp(extension, ".ebm") == 0)
        return;
    platform_mutex_lock(errors.lock);
    first = remember_path(path);
    platform_mutex_unlock(errors.lock);
    if (first) {
        ui_chat_line("Resource File Loading fail", 0xff);
        ui_chat_line(name, 0xff);
        ui_chat_line(path, 0xff);
    }
}
