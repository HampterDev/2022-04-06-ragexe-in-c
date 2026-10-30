#include "resources/attr.h"
#include "platform/error_message.h"
#include "std/string.h"
#include "resources/resource_ops.h"
#include "fileio/resource_api.h"
#include "fileio/res_mgr.h"
#include "resources/resource_factories.h"
#include "fileio/file.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct Block { unsigned char *data; size_t size; } blocks[8];
static unsigned allocations, fail_at, live, buffers, errors, destructs;
static void check_block(unsigned i)
{
    size_t j;
    for (j = 0; j < 16; ++j) {
        assert(blocks[i].data[j] == 0xa5);
        assert(blocks[i].data[16 + blocks[i].size + j] == 0x5a);
    }
}
static void *grid_malloc(size_t size)
{
    unsigned i;
    unsigned char *p;
    assert(size);
    if (++allocations == fail_at) return NULL;
    p = malloc(size + 32); assert(p);
    memset(p, 0xa5, 16); memset(p + 16, 0xcc, size); memset(p + 16 + size, 0x5a, 16);
    for (i = 0; i < 8 && blocks[i].data; ++i) {}
    assert(i < 8); blocks[i].data = p; blocks[i].size = size; ++live;
    return p + 16;
}
static void grid_free(void *p)
{
    unsigned i;
    if (!p) return;
    for (i = 0; i < 8 && blocks[i].data != (unsigned char *)p - 16; ++i) {}
    assert(i < 8); check_block(i); free(blocks[i].data); blocks[i].data = NULL; --live;
}
#define malloc grid_malloc
#define free grid_free
#include "../src/resources/attr.c"
#undef free
#undef malloc

static unsigned char input[14 + 4 * 20 + 8];
static size_t input_size;
static int missing;
struct GameFileMgr g_fileMgr;
BYTE *game_file_mgr_get_data(struct GameFileMgr *mgr, const char *name, int *size, BYTE disk)
{
    BYTE *p;
    (void)mgr; (void)disk; assert(strstr(name, ".gat"));
    if (missing) { *size = 0; return NULL; }
    p = malloc(input_size ? input_size : 1); assert(p);
    memcpy(p, input, input_size); *size = (int)input_size; ++buffers; return p;
}
void game_file_mgr_release_data(struct GameFileMgr *mgr, void *data)
{ (void)mgr; assert(data && buffers); --buffers; free(data); }
BYTE game_file_mgr_is_file_exist(struct GameFileMgr *mgr, const char *name, BYTE disk)
{ (void)mgr; (void)name; (void)disk; return !missing; }
void error_msg(const char *message, int flags) { assert(message && !flags); ++errors; }
void ErrorMsgNumber(int value) { (void)value; ++errors; }
struct Res *res_construct(struct Res *res) { memset(res, 0, sizeof *res); return res; }
void res_destruct(struct Res *res) { res->ops = NULL; ++destructs; }
static void dimensions(int width, int height)
{ memcpy(input + 6, &width, 4); memcpy(input + 10, &height, 4); }
static void fixture(unsigned char minor)
{
    struct AttrCell cell;
    unsigned i, j;
    memset(input, 0, sizeof input); memcpy(input, "GRAT", 4);
    input[4] = 1; input[5] = minor; dimensions(2, 2);
    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 4; ++j) cell.height[j] = (float)(10 * i + 2 * j);
        cell.type = (int)(0x80000000u | i);
        memcpy(input + 14 + i * sizeof cell, &cell, sizeof cell);
    }
    input_size = 94;
}
static void retained_failure(struct C3dAttr *grid)
{
    struct AttrCell *old = grid->cells;
    struct AttrCell contents[4];
    unsigned old_live = live;
    memcpy(contents, old, sizeof contents);
    assert(!c3d_attr_load(grid, "fixture.gat"));
    assert(grid->cells == old && grid->width == 2 && grid->height == 2 && grid->zoom == 7);
    assert(!memcmp(old, contents, sizeof contents));
    assert(live == old_live && !buffers);
}
static void test_loading(void)
{
    struct { unsigned char before[16]; struct C3dAttr grid; unsigned char after[16]; } guarded;
    struct C3dAttr *grid = &guarded.grid;
    unsigned i, minor, before;
    size_t length;
    memset(&guarded, 0xa7, sizeof guarded);
    assert(attribute_resource_init(grid) == grid);
    assert(grid->res.ops == &attribute_resource_ops && grid->zoom == 5 && !grid->cells);
    assert(!grid->width && !grid->height);
    grid->zoom = 7;
    for (minor = 0; minor <= 3; ++minor) {
        fixture((unsigned char)minor);
        assert(c3d_attr_load(grid, "fixture.gat"));
        assert(live == 1 && !buffers && grid->width == 2 && grid->height == 2 && grid->zoom == 7);
        for (i = 0; i < 4; ++i) {
            assert(grid->cells[i].type == (int)((minor == 3 ? 0 : 0x80000000u) | i));
            assert(grid->cells[i].height[3] == (float)(10 * i + 6));
        }
    }
    for (length = 0; length < 94; ++length) {
        fixture(3); input_size = length; retained_failure(grid);
    }
    fixture(3); missing = 1; retained_failure(grid); missing = 0;
    fixture(3); input[0] = 'X'; retained_failure(grid);
    fixture(3); input[4] = 2; retained_failure(grid);
    fixture(3); input[5] = 4; retained_failure(grid);
    fixture(3); dimensions(-1, 2); retained_failure(grid);
    fixture(3); dimensions(2, -1); retained_failure(grid);
    fixture(3); dimensions(INT_MAX, INT_MAX); retained_failure(grid);
    fixture(3); dimensions(0x40000000, 4); retained_failure(grid);
    fixture(3); dimensions(INT_MAX / 20 + 1, 1); retained_failure(grid);
    fixture(3); fail_at = allocations + 1; retained_failure(grid); fail_at = 0;
    fixture(3); input_size = sizeof input; assert(c3d_attr_load(grid, "fixture.gat"));
    fixture(3); dimensions(0, 0); input_size = 14; before = allocations;
    assert(c3d_attr_load(grid, "fixture.gat") && !grid->cells && !live && allocations == before);
    assert(!grid->width && !grid->height);
    fixture(3); assert(c3d_attr_load(grid, "fixture.gat"));
    attribute_resource_clear(grid); attribute_resource_clear(grid);
    assert(!grid->cells && !grid->width && !grid->height && grid->zoom == 7 && !live);
    attribute_resource_finish(grid); attribute_resource_finish(grid);
    assert(!live && !buffers);
    for (i = 0; i < 16; ++i) assert(guarded.before[i] == 0xa7 && guarded.after[i] == 0xa7);
}
static void test_factory(void)
{
    struct C3dAttr *grid;
    fail_at = allocations + 1; assert(!attribute_resource_create()); fail_at = 0;
    grid = (struct C3dAttr *)attribute_resource_create(); assert(grid && live == 1);
    assert(grid->zoom == 5 && !grid->cells && !grid->width && !grid->height);
    fixture(3); assert(c3d_attr_load(grid, "fixture.gat") && live == 2);
    attribute_resource_clear(grid); assert(live == 1);
    resource_destroy(&grid->res); assert(!live && !buffers);
}
static void test_queries(void)
{
    struct C3dAttr grid;
    struct AttrCell *cell;
    RECT rect = {0,0,1,1};
    float low, high;
    int x,y;
    attribute_resource_init(&grid); fixture(3); assert(c3d_attr_load(&grid, "fixture.gat"));
    for (y = -1; y < 3; ++y) for (x = -1; x < 3; ++x) {
        cell = c3d_attr_get_cell(&grid, x, y);
        if (x < 0 || y < 0 || x >= 2 || y >= 2) assert(!cell && !c3d_attr_cell_walkable(&grid, x, y));
        else assert(cell == grid.cells + y * 2 + x);
    }
    grid.cells[0].type = 0; grid.cells[1].type = 1; grid.cells[2].type = 5; grid.cells[3].type = 3;
    assert(c3d_attr_cell_walkable(&grid, 0, 0) && c3d_attr_cell_walkable(&grid, 1, 1));
    assert(!c3d_attr_cell_walkable(&grid, 1, 0) && !c3d_attr_cell_walkable(&grid, 0, 1));
    assert(c3d_attr_get_height(&grid, -2.5f, -2.5f) == 3);
    assert(c3d_attr_get_height(&grid, 2.5f, 2.5f) == 33);
    c3d_attr_get_height_min_max(&grid, &rect, &low, &high); assert(low == 0 && high == 36);
    assert(!c3d_attr_get_cell(NULL, 0, 0));
    attribute_resource_clear(&grid); assert(!c3d_attr_get_cell(&grid, 0, 0));
    assert(c3d_attr_get_height(&grid, 0, 0) == 0);
    attribute_resource_finish(&grid); assert(!live);
}
static void test_ray_distance(void)
{
    struct C3dAttr grid;
    struct Ray3 ray = {{-4, -10, -4}, {0, 1, 0}, 0};
    int i;
    attribute_resource_init(&grid); fixture(3); assert(c3d_attr_load(&grid, "fixture.gat"));
    for (i = 0; i < 4; ++i) grid.cells[0].height[i] = 3;
    assert(ground_ray_distance(&grid, &ray, 0, 0) == 13);
    ray.origin.x = ray.origin.z = -1;
    assert(ground_ray_distance(&grid, &ray, 0, 0) == 13);
    ray.origin.x = 1;
    assert(ground_ray_distance(&grid, &ray, 0, 0) == 999999.0f);
    assert(ground_ray_distance(&grid, &ray, -1, 0) == 0);
    attribute_resource_finish(&grid); assert(!live);
}

int main(void)
{
    test_loading(); test_factory(); test_queries(); test_ray_distance();
    puts("attribute grid: GAT versions, byte-exact cells, every-byte truncation, dimensions, allocation failure, retained reloads, queries and repeat cleanup passed");
    return 0;
}
