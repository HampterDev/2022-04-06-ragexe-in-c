#include "world/path_info.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

void path_info_init(struct PathInfo *path)
{
    memset(path, 0, sizeof *path);
}

void path_info_destroy(struct PathInfo *path)
{
    free(path->cells);
    path_info_init(path);
}

void path_info_clear(struct PathInfo *path)
{
    path->count = 0;
}



int path_info_resize(struct PathInfo *path, size_t count)
{
    const size_t maximum = (size_t)-1 / sizeof *path->cells;
    if (count > INT_MAX || count > maximum) return 0;
    if (count > path->capacity) {
        size_t capacity = path->capacity + path->capacity / 2 + 8;
        struct PathCell *cells;
        if (capacity < count || capacity > maximum) capacity = count;
        cells = realloc(path->cells, capacity * sizeof *cells);
        if (!cells) return 0;
        path->cells = cells;
        path->capacity = capacity;
    }
    if (count > path->count)
        memset(path->cells + path->count, 0, (count - path->count) * sizeof *path->cells);
    path->count = count;
    return 1;
}

