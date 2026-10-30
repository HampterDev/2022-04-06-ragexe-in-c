#include "world/pathfinder_state.h"
#include "resources/attr.h"
#include "platform/error_message.h"
#include "std/string.h"
#include "resources/resource_ops.h"
#include "fileio/resource_api.h"
#include "fileio/res_mgr.h"
#include "resources/resource_factories.h"
#include "fileio/file.h"
#include "world_state_memory.h"
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

static struct OwnerBlock { unsigned char *data; size_t size; } owner_blocks[128];
static unsigned owner_live, allocation_calls, fail_at, buffers, loads;
static void owner_guard(unsigned i)
{
    size_t j;
    for (j = 0; j < 16; ++j) {
        assert(owner_blocks[i].data[j] == 0xa9);
        assert(owner_blocks[i].data[16 + owner_blocks[i].size + j] == 0x9a);
    }
}
static unsigned owner_index(void *p)
{
    unsigned i;
    for (i = 0; i < 128 && owner_blocks[i].data != (unsigned char *)p - 16; ++i) {}
    assert(i < 128); owner_guard(i); return i;
}
static void *owner_allocate(size_t size)
{
    unsigned i;
    unsigned char *p;
    assert(size && size < 10000000);
    p = malloc(size + 32); assert(p);
    memset(p, 0xa9, 16); memset(p + 16, 0xcc, size); memset(p + 16 + size, 0x9a, 16);
    for (i = 0; i < 128 && owner_blocks[i].data; ++i) {}
    assert(i < 128); owner_blocks[i].data = p; owner_blocks[i].size = size; ++owner_live;
    return p + 16;
}
static void *owner_malloc(size_t size)
{ if (++allocation_calls == fail_at) return NULL; return owner_allocate(size); }
static void *owner_calloc(size_t count, size_t size)
{
    void *p;
    assert(count && size && count <= SIZE_MAX / size);
    p = owner_malloc(count * size); if (p) memset(p, 0, count * size); return p;
}
static void owner_free(void *p)
{
    unsigned i;
    if (!p) return;
    i = owner_index(p); free(owner_blocks[i].data); owner_blocks[i].data = NULL; --owner_live;
}
static void *owner_realloc(void *p, size_t size)
{
    void *next;
    size_t old_size = p ? owner_blocks[owner_index(p)].size : 0;
    if (++allocation_calls == fail_at) return NULL;
    next = owner_allocate(size);
    if (p) { memcpy(next, p, size < old_size ? size : old_size); owner_free(p); }
    return next;
}
#define malloc owner_malloc
#define calloc owner_calloc
#define realloc owner_realloc
#define free owner_free
#include "../src/resources/attr.c"
#include "../src/world/pathfinder.c"
#include "../src/world/pathinfo.c"
#undef free
#undef realloc
#undef calloc
#undef malloc

struct GameFileMgr g_fileMgr;
static int fail_load;
static char last_file[128];
BYTE *game_file_mgr_get_data(struct GameFileMgr *mgr, const char *name, int *size, BYTE disk)
{
    const int width = 2, height = 2;
    BYTE *data;
    (void)mgr; (void)disk;
    ++loads; strncpy(last_file, name, sizeof last_file - 1);
    if (fail_load) { *size = 0; return NULL; }
    *size = 94; data = calloc(1, *size); assert(data); ++buffers;
    memcpy(data, "GRAT", 4); data[4] = 1; data[5] = 3;
    memcpy(data + 6, &width, 4); memcpy(data + 10, &height, 4);
    return data;
}
void game_file_mgr_release_data(struct GameFileMgr *mgr, void *data)
{ (void)mgr; assert(buffers && data); --buffers; free(data); }
BYTE game_file_mgr_is_file_exist(struct GameFileMgr *mgr, const char *name, BYTE disk)
{ (void)mgr; (void)name; (void)disk; return !fail_load; }
void error_msg(const char *text, int flags) { (void)text; (void)flags; }
void ErrorMsgNumber(int value) { (void)value; }
struct Res *res_construct(struct Res *res) { memset(res, 0, sizeof *res); return res; }
void res_destruct(struct Res *res) { res->ops = NULL; }
unsigned char pathfinder_reference(struct C3dAttr *attr, int heap, unsigned long time,
    int sx, int sy, int subx, int suby, int tx, int ty, int speed, struct PathInfo *out, int kind);

static uint32_t random_state = UINT32_C(0x57ac0173);
static unsigned random_value(void)
{ random_state = random_state * UINT32_C(1664525) + UINT32_C(1013904223); return random_state; }

static void compare_search(struct PathFinder *finder, struct C3dAttr *grid, int sx, int sy,
                           int tx, int ty, int kind, int heap)
{
    struct PathInfo actual, expected;
    unsigned char a,b;
    unsigned long time = random_value();
    int subx = (int)(random_value() % 16), suby = (int)(random_value() % 16);
    int speed = (int)(random_value() % 300) + 1;
    path_info_init(&actual); path_info_init(&expected);
    finder->useHeapNodes = (unsigned char)heap;
    path_finder_set_attr(finder, grid);
    a = path_finder_find_path(finder, time, sx, sy, subx, suby, tx, ty, speed, &actual, kind);
    b = pathfinder_reference(grid, heap, time, sx, sy, subx, suby, tx, ty, speed, &expected, kind);
    assert(a == b && actual.count == expected.count);
    if (a) {
        assert(actual.startX == expected.startX && actual.startY == expected.startY);
        assert(!memcmp(actual.cells, expected.cells, actual.count * sizeof *actual.cells));
    }
    path_info_destroy(&actual); path_info_destroy(&expected);
    assert(!live_blocks && !buffers);
}

static void test_reference(void)
{
    struct PathFinder finder;
    struct C3dAttr grid = {0};
    struct AttrCell cells[256];
    unsigned i, trial;
    path_finder_construct(&finder); grid.width = grid.height = 16; grid.cells = cells;
    for (trial = 0; trial < 240; ++trial) {
        memset(cells, 0, sizeof cells);
        if (trial >= 16)
            for (i = 0; i < 256; ++i) {
                unsigned r = random_value() % 10;
                cells[i].type = r < 2 ? 1 : r == 2 ? 5 : 0;
            }
        compare_search(&finder, &grid, (int)(trial % 16), 0, 15, 15, 0, trial & 1);
        compare_search(&finder, &grid, 0, 0, 15, (int)(trial % 16), 12, trial & 1);
        compare_search(&finder, &grid, 15, 15, 0, 0, 15, trial & 1);
    }
    path_finder_destruct(&finder); path_finder_destruct(&finder);
    assert(!owner_live && !live_blocks);
}

static void test_storage(void)
{
    struct C3dAttr grid = {0};
    struct AttrCell cells[200] = {0};
    struct PathFinder finder;
    struct PathInfo path, saved;
    struct PathCell saved_cell;
    unsigned failure, before;
    grid.width = 200; grid.height = 1; grid.cells = cells;
    for (failure = 1; failure <= 5; ++failure) {
        memset(&finder, 0xa5, sizeof finder); path_finder_construct(&finder);
        assert(!finder.attr && !finder.search && !finder.maps && !finder.map_count && !finder.map_capacity);
        finder.useHeapNodes = 1; path_finder_set_attr(&finder, &grid);
        path_info_init(&path); assert(path_info_resize(&path, 1));
        path.cells[0].x = 123; path.startX = 7; saved = path; saved_cell = path.cells[0];
        fail_at = allocation_calls + failure;
        assert(!path_finder_find_path(&finder, 123, 0, 0, 8, 8, 199, 0, 100, &path, 0));
        assert(!memcmp(&path, &saved, sizeof path) && !memcmp(path.cells, &saved_cell, sizeof saved_cell));
        fail_at = 0;
        assert(path_finder_find_path(&finder, 123, 0, 0, 8, 8, 199, 0, 100, &path, 0));
        assert(path.count == 200 && path.cells[199].time == 19993);
        before = allocation_calls;
        assert(path_finder_find_path(&finder, 100, 199, 0, 8, 8, 0, 0, 100, &path, 0));
        assert(allocation_calls == before);
        finder.useHeapNodes = 0; saved = path;
        assert(!path_finder_find_path(&finder, 100, 0, 0, 8, 8, 199, 0, 100, &path, 0));
        assert(!memcmp(&saved, &path, sizeof path));
        path_finder_clear_search(&finder); path_finder_clear_search(&finder);
        assert(!finder.search->open_count && !finder.search->node_count);
        assert(!path_finder_find_path(&finder, 100, -1, 0, 8, 8, 1, 0, 100, &path, 0));
        assert(!path_finder_find_path(&finder, 100, 0, 0, 8, 8, 200, 0, 100, &path, 0));
        path_finder_set_attr(&finder, NULL);
        assert(!path_finder_find_path(&finder, 100, 0, 0, 8, 8, 1, 0, 100, &path, 0));
        path_info_destroy(&path); path_finder_destruct(&finder); path_finder_destruct(&finder);
        assert(!owner_live && !live_blocks);
    }
    path_finder_construct(&finder); finder.useHeapNodes = 1;
    grid.width = grid.height = INT_MAX; path_finder_set_attr(&finder, &grid);
    path_info_init(&path); before = allocation_calls;
    assert(!path_finder_find_path(&finder, 100, 0, 0, 8, 8, 1, 0, 100, &path, 0));
    assert(allocation_calls == before);
    path_finder_destruct(&finder);
}

static void test_cache(void)
{
    struct PathFinder finder;
    struct C3dAttr borrowed = {0}, *first;
    unsigned failure, before, i;
    char name[32];
    for (failure = 1; failure <= 4; ++failure) {
        path_finder_construct(&finder); path_finder_set_attr(&finder, &borrowed);
        fail_at = allocation_calls + failure;
        assert(!path_finder_use_map_attr(&finder, "map", 3));
        assert(!finder.map_count && finder.attr == &borrowed && !owner_live && !buffers);
        fail_at = 0;
        assert(path_finder_use_map_attr(&finder, "map", 3));
        assert(finder.map_count == 1 && finder.attr->width == 2);
        path_finder_destruct(&finder); assert(!owner_live);
    }
    path_finder_construct(&finder); path_finder_set_attr(&finder, &borrowed);
    fail_load = 1; assert(!path_finder_use_map_attr(&finder, "map", 3)); fail_load = 0;
    assert(!finder.map_count && finder.attr == &borrowed && !owner_live);
    strcpy(name, "map_junk"); assert(path_finder_use_map_attr(&finder, name, 3));
    first = finder.attr; memset(name, 'X', 3); before = loads;
    assert(path_finder_use_map_attr(&finder, "map", 3) && finder.attr == first && loads == before);
    assert(!strcmp(last_file, "data\\map.gat"));
    for (i = 0; i < 30; ++i) {
        sprintf(name, "map_%02u", 29 - i);
        assert(path_finder_use_map_attr(&finder, name, strlen(name)));
    }
    assert(finder.map_count == 31);
    assert(path_finder_use_map_attr(&finder, "map", 3) && finder.attr == first);
    for (i = 1; i < finder.map_count; ++i)
        assert(map_name_compare(finder.maps[i-1].file, finder.maps[i-1].name_length, &finder.maps[i]) < 0);
    before = allocation_calls;
    assert(!path_finder_use_map_attr(&finder, NULL, 0));
    assert(!path_finder_use_map_attr(&finder, "map", SIZE_MAX));
    assert(allocation_calls == before && finder.attr == first);
    fail_load = 1; assert(!path_finder_use_map_attr(&finder, "missing", 7)); fail_load = 0;
    assert(finder.map_count == 31 && finder.attr == first);
    assert(path_finder_use_map_attr(&finder, "last_slot", 9));
    assert(finder.map_count == finder.map_capacity && finder.map_count == 32);
    first = finder.attr; before = owner_live;
    fail_at = allocation_calls + 4;
    assert(!path_finder_use_map_attr(&finder, "growth", 6)); fail_at = 0;
    assert(finder.map_count == 32 && finder.attr == first && owner_live == before);
    assert(path_finder_use_map_attr(&finder, "growth", 6) && finder.map_count == 33);
    path_finder_destruct(&finder); path_finder_destruct(&finder);
    assert(!owner_live && !live_blocks && !buffers);
}

static void test_workspace_growth(void)
{
    struct C3dAttr grid = {0};
    struct AttrCell cells[200] = {0};
    struct PathFinder finder;
    struct PathInfo path, saved;
    struct PathNode *nodes;
    struct PathNode **open, **index;
    unsigned failure, before;
    for (failure = 1; failure <= 3; ++failure) {
        grid.width = 4; grid.height = 1; grid.cells = cells;
        path_finder_construct(&finder); finder.useHeapNodes = 1;
        path_finder_set_attr(&finder, &grid); path_info_init(&path);
        assert(path_finder_find_path(&finder, 100, 0, 0, 8, 8, 3, 0, 100, &path, 0));
        nodes = finder.search->nodes; open = finder.search->open; index = finder.search->index;
        saved = path; before = owner_live; grid.width = 200;
        fail_at = allocation_calls + failure;
        assert(!path_finder_find_path(&finder, 100, 0, 0, 8, 8, 199, 0, 100, &path, 0));
        fail_at = 0;
        assert(finder.search->nodes == nodes && finder.search->open == open && finder.search->index == index);
        assert(owner_live == before && !memcmp(&path, &saved, sizeof path));
        grid.width = 4; before = allocation_calls;
        assert(path_finder_find_path(&finder, 100, 3, 0, 8, 8, 0, 0, 100, &path, 0));
        assert(allocation_calls == before);
        grid.width = 200;
        assert(path_finder_find_path(&finder, 100, 0, 0, 8, 8, 199, 0, 100, &path, 0));
        assert(path.count == 200);
        path_info_destroy(&path); path_finder_destruct(&finder);
        assert(!owner_live && !live_blocks && !buffers);
    }
}

int main(void)
{
    test_reference(); test_storage(); test_cache(); test_workspace_growth();
    puts("path finder: 720 frozen-reference searches, stable storage, local cap, every allocation failure, output rollback, map cache failures and repeat cleanup passed");
    return 0;
}
