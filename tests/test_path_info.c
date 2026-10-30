#include "world/pathfinder_state.h"
#include "resources/attr.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>

static unsigned char *path_allocation;
static size_t path_bytes;
static int fail_path_allocation, allocation_calls;

static void check_path_guard(void)
{
    size_t i;
    if (path_allocation)
        for (i = 0; i < 16; ++i) assert(path_allocation[path_bytes + i] == 0xdb);
}

static void *path_reallocate(void *old, size_t bytes)
{
    unsigned char *p;
    assert(old == path_allocation);
    check_path_guard();
    ++allocation_calls;
    if (fail_path_allocation) return NULL;
    p = malloc(bytes + 16);
    assert(p);
    memset(p, 0xdb, bytes + 16);
    if (old) memcpy(p, old, bytes < path_bytes ? bytes : path_bytes);
    free(old);
    path_allocation = p;
    path_bytes = bytes;
    return p;
}

static void path_free(void *p)
{
    assert(p == path_allocation);
    check_path_guard();
    free(p);
    path_allocation = NULL;
    path_bytes = 0;
}

#define realloc path_reallocate
#define free path_free
#include "../src/world/pathinfo.c"
#undef realloc
#undef free

struct AttrCell *c3d_attr_get_cell(const struct C3dAttr *attr, int x, int y)
{
    if (x < 0 || y < 0 || x >= attr->width || y >= attr->height) return NULL;
    return attr->cells + y * attr->width + x;
}

struct Res *attribute_resource_create(void) { abort(); }
unsigned char c3d_attr_load(struct C3dAttr *self, const char *name) { abort(); }
void c3d_attr_destroy(struct C3dAttr *self) { abort(); }
unsigned char c3d_attr_cell_walkable(struct C3dAttr *self, int x, int y) { abort(); }
static void close_to(float actual, float expected)
{
    assert(fabsf(actual - expected) < 0.0001f);
}

static void test_storage(void)
{
    struct PathInfo path, saved;
    struct PathCell cell;
    int calls, x = 17, y = 19, dir = 23;
    uint32_t deadline = 29;
    float px = 31, py = 37;
    size_t i;
    memset(&path, 0xa5, sizeof path);
    path_info_init(&path);
    assert(!path.cells && !path.count && !path.capacity && !path.current);
    assert(!path.startX && !path.startY);
    assert(path_info_end_time(&path) == 0 && path_info_end_direction(&path) == -1);
    assert(!path_info_get_cell(&path, 0, &x, &y) && x == 17 && y == 19);
    assert(path_info_next_cell(&path, 100, &deadline, &px, &py) == -1);
    assert(deadline == 29 && px == 31 && py == 37);
    assert(path_info_position_at(&path, 100, &px, &py, &dir) == -1);
    assert(px == 31 && py == 37 && dir == 23);
    fail_path_allocation = 1;
    assert(!path_info_resize(&path, 1) && !path.cells && !path.count);
    fail_path_allocation = 0;
    assert(path_info_resize(&path, 3));
    for (i = 0; i < 3; ++i) {
        assert(!path.cells[i].x && !path.cells[i].y && !path.cells[i].dir && !path.cells[i].time);
        path.cells[i].x = (int)i + 10;
    }
    path.cells[0].y = 40; path.cells[0].dir = 6; path.cells[0].time = 100;
    cell = path.cells[0];
    assert(path_info_resize(&path, 300));
    assert(!memcmp(&cell, path.cells, sizeof cell));
    for (i = 3; i < path.count; ++i)
        assert(!path.cells[i].x && !path.cells[i].y && !path.cells[i].dir && !path.cells[i].time);
    path.current = 27; path.startX = 1.5f; path.startY = 2.25f;
    saved = path;
    fail_path_allocation = 1;
    assert(!path_info_resize(&path, path.capacity + 1));
    assert(!memcmp(&saved, &path, sizeof path) && !memcmp(&cell, path.cells, sizeof cell));
    fail_path_allocation = 0;
    calls = allocation_calls;
    assert(!path_info_resize(&path, SIZE_MAX));
    assert(!path_info_resize(&path, (size_t)INT_MAX + 1));
    assert(allocation_calls == calls && !memcmp(&saved, &path, sizeof path));
    assert(path_info_resize(&path, 1) && !path.current && path.capacity == saved.capacity);
    assert(path_info_resize(&path, 3) && path.cells[1].x == 0 && path.cells[2].x == 0);
    assert(path_info_get_cell(&path, 0, &x, &y) && x == 10 && y == 40);
    assert(!path_info_get_cell(&path, -1, &x, &y));
    assert(!path_info_get_cell(&path, 3, &x, &y));
    assert(x == 10 && y == 40);
    path_info_clear(&path);
    assert(path.cells == saved.cells && path.capacity == saved.capacity && !path.count && !path.current);
    assert(path_info_resize(&path, 3) && allocation_calls == calls);
    for (i = 0; i < path.count; ++i)
        assert(!path.cells[i].x && !path.cells[i].y && !path.cells[i].dir && !path.cells[i].time);
    check_path_guard();
    path_info_destroy(&path); path_info_destroy(&path);
    assert(!path.cells && !path.count && !path.capacity && !path_allocation);
}

static void test_queries(void)
{
    static const struct PathCell cells[] = {{1,2,2,100}, {3,4,6,200}, {4,4,6,300}, {4,5,0,400}};
    struct PathInfo path;
    float x, y;
    uint32_t deadline, time;
    int dir;
    path_info_init(&path);
    assert(path_info_resize(&path, 1));
    path.cells[0] = cells[0];
    for (time = 0; time <= 200; time += 100) {
        assert(path_info_next_cell(&path, time, &deadline, &x, &y) == -1);
        assert(deadline == 100 && x == 1 && y == 2);
        assert(path_info_position_at(&path, time, &x, &y, &dir) == -1);
        assert(x == 1 && y == 2 && dir == 2);
    }
    assert(path_info_resize(&path, 4));
    memcpy(path.cells, cells, sizeof cells);
    path.startX = 1.25f; path.startY = 2.5f;
    assert(path_info_next_cell(&path, 99, &deadline, &x, &y) == 0);
    assert(deadline == 200 && x == 3 && y == 4);
    assert(path_info_next_cell(&path, 250, &deadline, &x, &y) == 1);
    assert(deadline == 300 && x == 4 && y == 4);
    assert(path_info_next_cell(&path, 400, &deadline, &x, &y) == -1);
    assert(deadline == 400 && x == 4 && y == 5);
    assert(path_info_position_at(&path, 99, &x, &y, &dir) == 0 && x == 1 && y == 2 && dir == 6);
    assert(path_info_position_at(&path, 100, &x, &y, &dir) == 0);
    assert(x == 1.25f && y == 2.5f && dir == 2);
    assert(path_info_position_at(&path, 149, &x, &y, &dir) == 0 && dir == 2);
    assert(path_info_position_at(&path, 150, &x, &y, &dir) == 0 && dir == 6);
    close_to(x, 2.125f); close_to(y, 3.25f);
    assert(path_info_position_at(&path, 250, &x, &y, &dir) == 1);
    assert(x == 3.5f && y == 4 && dir == 6);
    assert(path_info_position_at(&path, 350, &x, &y, &dir) == 2);
    assert(x == 4 && y == 4.5f && dir == 0);
    assert(path_info_position_at(&path, 450, &x, &y, &dir) == -1);
    assert(x == 4 && y == 5 && dir == 0);
    assert(path_info_position_at(&path, 150, &x, &y, &dir) == 0);
    close_to(x, 2.125f); close_to(y, 3.25f);
    path.current = SIZE_MAX;
    assert(path_info_position_at(&path, 250, &x, &y, &dir) == 1 && x == 3.5f);
    path_info_rewind(&path); assert(!path.current);
    assert(path_info_end_time(&path) == 400 && path_info_end_direction(&path) == 0);
    path.cells[0].time = 0;
    assert(path_info_next_cell(&path, 250, &deadline, &x, &y) == -1);
    assert(deadline == 400 && x == 4 && y == 5);
    path.cells[0].time = 100; path.cells[1].time = 100;
    assert(path_info_position_at(&path, 100, &x, &y, &dir) == 1 && x == 3 && y == 4);
    path.cells[2].time = 50;
    assert(path_info_position_at(&path, 100, &x, &y, &dir) == 2);
    assert(isfinite(x) && isfinite(y));
    path_info_destroy(&path);
}

static void test_finder(void)
{
    struct PathFinder finder;
    struct C3dAttr attr = {0};
    struct AttrCell grid[25] = {0};
    struct PathInfo path, saved;
    struct PathCell saved_cell;
    size_t i;
    memset(&finder, 0xa5, sizeof finder);
    path_finder_construct(&finder);
    attr.width = attr.height = 5;
    attr.cells = grid;
    path_finder_set_attr(&finder, &attr);
    path_info_init(&path);
    assert(path_finder_find_path(&finder, 1000, 1, 2, 8, 8, 4, 2, 100, &path, 0));
    assert(path.count == 4 && path.startX == 1 && path.startY == 2);
    for (i = 0; i < path.count; ++i) {
        assert(path.cells[i].x == 1 + (int)i && path.cells[i].y == 2);
        assert(path.cells[i].time == 1000 + 100 * i);
        assert(path.cells[i].dir == (i ? 6 : 0));
    }
    assert(path_finder_find_path(&finder, 2000, 1, 1, 8, 8, 3, 3, 100, &path, 0));
    assert(path.count == 3 && path.cells[1].x == 2 && path.cells[1].y == 2);
    assert(path.cells[1].time == 2141 && path.cells[2].time == 2282);
    for (i = 0; i < 5; ++i) grid[2 * 5 + i].type = 1;
    saved = path;
    assert(!path_finder_find_path(&finder, 3000, 1, 1, 8, 8, 3, 3, 100, &path, 0));
    assert(!memcmp(&saved, &path, sizeof path));
    memset(grid, 0, sizeof grid);
    assert(!path_finder_find_path(&finder, 3000, 1, 1, 8, 8, 1, 1, 100, &path, 0));
    assert(!memcmp(&saved, &path, sizeof path));
    path_info_destroy(&path);
    assert(path_info_resize(&path, 1));
    path.cells[0].x = 77; path.cells[0].time = 123;
    for (i = 0; i < 5; ++i) grid[5 + i].type = grid[15 + i].type = 1;
    grid[9].type = grid[15].type = 0;
    path.startX = 1.5f; path.startY = 2.5f;
    saved = path; saved_cell = path.cells[0];
    fail_path_allocation = 1;
    assert(!path_finder_find_path(&finder, 4000, 0, 0, 0, 0, 4, 4, 100, &path, 0));
    assert(!memcmp(&saved, &path, sizeof path) && !memcmp(&saved_cell, path.cells, sizeof saved_cell));
    fail_path_allocation = 0;
    assert(path_finder_find_path(&finder, 4000, 0, 0, 0, 0, 4, 4, 100, &path, 0));
    assert(path.count > 8 && path.cells[path.count - 1].x == 4 && path.cells[path.count - 1].y == 4);
    for (i = 1; i < path.count; ++i) {
        assert(abs(path.cells[i].x - path.cells[i-1].x) <= 1);
        assert(abs(path.cells[i].y - path.cells[i-1].y) <= 1);
        assert(path.cells[i].time > path.cells[i-1].time);
        assert(c3d_attr_get_cell(&attr, path.cells[i].x, path.cells[i].y)->type == 0);
    }
    path_info_destroy(&path);
    path_finder_destruct(&finder);
    assert(!path_allocation);
}

int main(void)
{
    test_storage(); test_queries(); test_finder();
    puts("Path info: owned storage, failure rollback, short paths, replay time and real grid searches passed");
    return 0;
}
