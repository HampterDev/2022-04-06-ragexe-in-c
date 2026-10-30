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
    path->count = path->current = 0;
}

void path_info_rewind(struct PathInfo *path)
{
    path->current = 0;
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
    path->current = 0;
    return 1;
}

int path_info_get_cell(const struct PathInfo *path, int index, int *x, int *y)
{
    if (index < 0 || (size_t)index >= path->count) return 0;
    *x = path->cells[index].x;
    *y = path->cells[index].y;
    return 1;
}

uint32_t path_info_end_time(const struct PathInfo *path)
{
    return path->count ? path->cells[path->count - 1].time : 0;
}

int path_info_end_direction(const struct PathInfo *path)
{
    return path->count ? path->cells[path->count - 1].dir : -1;
}

int path_info_next_cell(const struct PathInfo *path, uint32_t time,
                        uint32_t *deadline, float *x, float *y)
{
    const struct PathCell *cells = path->cells;
    size_t i, target;
    int segment = -1;
    if (!path->count) return -1;
    target = path->count - 1;
    if (path->count > 1 && cells[0].time) {
        if (time < cells[0].time) {
            target = 1;
            segment = 0;
        } else {
            for (i = 0; i + 1 < path->count; ++i) {
                if (cells[i].time <= time && time < cells[i + 1].time) {
                    target = i + 1;
                    segment = (int)i;
                    break;
                }
            }
        }
    }
    *deadline = cells[target].time;
    *x = (float)cells[target].x;
    *y = (float)cells[target].y;
    return segment;
}

int path_info_position_at(struct PathInfo *path, uint32_t time,
                         float *x, float *y, int *direction)
{
    const struct PathCell *cells = path->cells;
    size_t i, last;
    float dx, dy, elapsed, duration;
    if (!path->count) return -1;
    last = path->count - 1;
    if (!last) {
        *x = (float)cells[0].x; *y = (float)cells[0].y; *direction = cells[0].dir;
        path->current = 0;
        return -1;
    }
    if (time < cells[0].time) {
        *x = (float)cells[0].x; *y = (float)cells[0].y; *direction = cells[1].dir;
        path->current = 0;
        return 0;
    }
    i = path->current;
    if (i > last || time < cells[i].time) i = 0;
    for (; i < last; ++i)
        if (cells[i].time <= time && time < cells[i + 1].time)
            break;
    path->current = i;
    if (i == last) {
        *x = (float)cells[i].x; *y = (float)cells[i].y; *direction = cells[i].dir;
        return -1;
    }
    dx = (float)cells[i + 1].x - (i == 0 ? path->startX : (float)cells[i].x);
    dy = (float)cells[i + 1].y - (i == 0 ? path->startY : (float)cells[i].y);
    elapsed = (float)(time - cells[i].time);
    duration = (float)(cells[i + 1].time - cells[i].time);
    *x = (i == 0 ? path->startX : (float)cells[i].x) + elapsed * dx / duration;
    *y = (i == 0 ? path->startY : (float)cells[i].y) + elapsed * dy / duration;
    *direction = time < cells[i].time + ((cells[i + 1].time - cells[i].time) >> 1)
                     ? cells[i].dir : cells[i + 1].dir;
    return (int)i;
}
