#ifndef WORLD_PATH_INFO_H
#define WORLD_PATH_INFO_H
#include <stddef.h>
#include <stdint.h>

struct PathCell { int x, y, dir; uint32_t time; };
struct PathInfo {
    struct PathCell *cells;
    size_t count, capacity;
    size_t current;
    float startX, startY;
};

void path_info_init(struct PathInfo *path);
void path_info_destroy(struct PathInfo *path);
void path_info_clear(struct PathInfo *path);
void path_info_rewind(struct PathInfo *path);
int path_info_resize(struct PathInfo *path, size_t count);
int path_info_get_cell(const struct PathInfo *path, int index, int *x, int *y);
uint32_t path_info_end_time(const struct PathInfo *path);
int path_info_end_direction(const struct PathInfo *path);
int path_info_next_cell(const struct PathInfo *path, uint32_t time,
                        uint32_t *deadline, float *x, float *y);
int path_info_position_at(struct PathInfo *path, uint32_t time,
                         float *x, float *y, int *direction);
#endif
