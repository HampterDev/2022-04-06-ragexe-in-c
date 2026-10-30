#include "render/texture_cache.h"
#include "world/ground.h"
#include "resources/attr.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct Allocation { BYTE *data; size_t size; } allocations[64];
static unsigned live_allocations, allocation_calls, fail_at;
static void *checked_calloc(size_t count, size_t size)
{
    size_t bytes = count * size;
    unsigned i;
    BYTE *data;
    if (++allocation_calls == fail_at) return NULL;
    assert(count && size && bytes / size == count && bytes <= (size_t)-1 - 32);
    data = malloc(bytes + 32);
    assert(data);
    memset(data, 0xa5, 16);
    memset(data + 16, 0, bytes);
    memset(data + 16 + bytes, 0x5a, 16);
    for (i = 0; i < 64 && allocations[i].data; ++i) {}
    assert(i < 64);
    allocations[i].data = data; allocations[i].size = bytes;
    ++live_allocations;
    return data + 16;
}
static void checked_free(void *memory)
{
    unsigned i, j;
    BYTE *data;
    if (!memory) return;
    data = (BYTE *)memory - 16;
    for (i = 0; i < 64 && allocations[i].data != data; ++i) {}
    assert(i < 64 && live_allocations);
    for (j = 0; j < 16; ++j) {
        assert(data[j] == 0xa5);
        assert(data[16 + allocations[i].size + j] == 0x5a);
    }
    free(data); allocations[i].data = NULL; --live_allocations;
}
#define calloc checked_calloc
#define free checked_free
#include "../src/world/ground_lifecycle.c"
#include "../src/world/ground.c"
#include "../src/world/ground_load.c"
#include "../src/world/ground_water.c"
#undef free
#undef calloc

struct TexCache g_textureCache;
static struct Texture pages[3], borrowed_texture;
static unsigned live_pages, page_calls, fail_page_at, released_pages;
static int fail_texture;
void surface_destroy(struct Surface *page)
{
    assert(live_pages && page == &pages[live_pages - 1].base);
    --live_pages; ++released_pages;
}
struct Texture *texture_cache_get(struct TexCache *cache, const char *name, int a, int b)
{
    assert(cache == &g_textureCache && !strcmp(name, "ground.bmp") && !a && !b);
    return fail_texture ? NULL : &borrowed_texture;
}
struct Texture *texture_from_pixels(int width, int height,
    const DWORD *pixels, int a, int b, int c)
{
    assert(width == 256 && height == 256 && a == 1 && !b && !c);
    assert(pixels[0] == 0x80182028u);
    if (++page_calls == fail_page_at) return NULL;
    assert(live_pages < 3);
    return &pages[live_pages++];
}
void gnd_res_get_lightmap(const struct GndRes *resource, int index, BYTE *out)
{
    int i;
    assert(index >= 0 && index < resource->lightmapCount);
    memset(out, 128, 64);
    for (i = 0; i < 64; ++i) {
        out[64 + i * 3] = 24; out[65 + i * 3] = 32; out[66 + i * 3] = 40;
    }
}
static void empty_map(const struct C3dGround15 *ground, const struct C3dAttr *attr)
{
    assert(ground->attr == attr && !ground->width && !ground->height);
    assert(!ground->cells && !ground->cellCount && !ground->colours && !ground->colourCount);
    assert(!ground->surfaces && !ground->surfaceCount && !ground->waters && !ground->waterCount);
    assert(!ground->lightmaps.lightmaps && !ground->lightmaps.count);
    assert(!ground->lightmaps.pages && !ground->lightmaps.pageCount && !live_pages);
    assert(ground->blocksX == 1 && ground->blocksY == 1 && !ground->waterPerBlock);
    assert(live_allocations == 1);
}
static void check_cells(struct C3dGround15 *ground, int width, int height)
{
    size_t i, count = (size_t)width * height;
    assert(ground->width == width && ground->height == height);
    assert(ground->cellCount == count && ground->colourCount == count * 4);
    for (i = 0; i < count; ++i) {
        static const float empty_water[4][3] = {{0}};
        const struct GndCell *cell = &ground->cells[i];
        assert(!memcmp(cell->water, empty_water, sizeof empty_water));
        assert(!cell->top && !cell->frontWall && !cell->rightWall && cell->lastFrame == -1);
        assert(cell->colour[0] == 255 && cell->colour[1] == 255 && cell->colour[2] == 255);
    }
    for (i = 0; i < count * 4; ++i)
        assert(ground->colours[i].r == 1 && ground->colours[i].g == 1 && ground->colours[i].b == 1);
}
static void test_surface_picking(void)
{
    struct GndSurface surface = {0};
    struct GndCell cell = {0};
    struct C3dGround15 ground = {0};
    struct Ray3 ray = {{0.25f, -5, 0.25f}, {0, 1, 0}, 0};
    int triangle;
    ground.width = ground.height = 1;
    ground.cells = &cell;
    cell.top = &surface;
    surface.v[1].pos[0] = surface.v[3].pos[0] = 1;
    surface.v[2].pos[2] = surface.v[3].pos[2] = 1;
    for (triangle = 0; triangle < 2; ++triangle) {
        ray.origin.x = ray.origin.z = triangle ? 0.75f : 0.25f;
        surface.halfAlpha = 0;
        c3d_ground15_check_alpha(&ground, &ray, 0, 0, 5);
        assert(!surface.halfAlpha);
        c3d_ground15_check_alpha(&ground, &ray, 0, 0, 6);
        assert(surface.halfAlpha);
    }
    ray.origin.x = 2;
    surface.halfAlpha = 0;
    c3d_ground15_check_alpha(&ground, &ray, 0, 0, 6);
    assert(!surface.halfAlpha);
}

int main(void)
{
    struct C3dAttr attr = {0};
    struct C3dGround15 *ground;
    struct GndRes resource = {0};
    struct GndCellRec cells[4] = {0};
    struct GndSurfaceRec surfaces[4] = {0};
    struct GndWaterRec water = {7, 2, 3, 4, 5, 6};
    const char *names[] = {"ground.bmp"};
    const float direction[3] = {0, 1, 0}, diffuse[3] = {0.5f, 0.5f, 0.5f};
    const float ambient[3] = {0.25f, 0.25f, 0.25f};
    unsigned pass, i, j;
    test_surface_picking();
    ground_free(NULL);
    fail_at = 1;
    assert(!ground_new() && !live_allocations);
    fail_at = 0;
    ground = ground_new();
    assert(ground && !ground->attr && ground->zoom == 10);
    assert(!ground->waterType);
    ground_set_attribute_map(ground, &attr);
    empty_map(ground, &attr);
    assert(!c3d_ground15_create(ground, 0, 2));
    assert(!c3d_ground15_create(ground, -1, 2));
    assert(!c3d_ground15_create(ground, INT_MAX, INT_MAX));
    empty_map(ground, &attr);
    for (pass = 0; pass < 3; ++pass) {
        assert(c3d_ground15_create(ground, 3 + pass, 2));
        check_cells(ground, 3 + pass, 2);
        c3d_ground15_clear(ground);
        empty_map(ground, &attr);
    }
    resource.width = resource.height = 2;
    resource.zoom = 10; resource.cubes = cells; resource.surfaces = surfaces;
    resource.surfaceCount = 4;
    resource.lightmapCount = resource.textureCount = 1; resource.textureNames = names;
    borrowed_texture.base.width = borrowed_texture.base.height = 128;
    borrowed_texture.reqWidth = borrowed_texture.reqHeight = 64;
    for (i = 0; i < 4; ++i) {
        cells[i].top = i; cells[i].front = cells[i].right = -1;
        surfaces[i].colour = 0xffffffff;
        for (j = 0; j < 4; ++j) {
            cells[i].height[j] = 3;
            surfaces[i].u[j] = (float)(j & 1); surfaces[i].v[j] = (float)(j / 2);
        }
    }
    for (i = 1; i <= 6; ++i) {
        allocation_calls = 0; fail_at = i;
        assert(!c3d_ground15_load(ground, &resource, direction, diffuse, ambient));
        empty_map(ground, &attr);
    }
    fail_at = 0; resource.lightmapCount = 1025;
    page_calls = released_pages = 0; fail_page_at = 2;
    assert(!c3d_ground15_load(ground, &resource, direction, diffuse, ambient));
    assert(released_pages == 1); empty_map(ground, &attr);
    fail_page_at = 0;
    assert(c3d_ground15_load(ground, &resource, direction, diffuse, ambient));
    assert(live_pages == 2 && ground->lightmaps.lightmaps[1024].page == &pages[1]);
    c3d_ground15_clear(ground);
    empty_map(ground, &attr);
    resource.lightmapCount = 1; fail_texture = 1;
    assert(!c3d_ground15_load(ground, &resource, direction, diffuse, ambient));
    empty_map(ground, &attr); fail_texture = 0;
    for (pass = 0; pass < 3; ++pass) {
        float min, max;
        assert(c3d_ground15_load(ground, &resource, direction, diffuse, ambient));
        assert(live_allocations == 6 && live_pages == 1 && ground->attr == &attr);
        assert(ground->cells[0].top == &ground->surfaces[0]);
        assert(ground->surfaces[0].texture == &borrowed_texture);
        assert(ground->surfaces[0].lightmap == ground->lightmaps.lightmaps);
        assert(ground->surfaces[0].lightmapPage == &pages[0]);
        assert(ground->surfaces[0].v[0].pos[0] == -10 && ground->surfaces[0].v[0].pos[1] == 3);
        assert(ground->surfaces[0].v[3].u == 0.5f && ground->surfaces[0].v[3].v == 0.5f);
        assert(ground->cells[0].colour[0] == 48 && ground->cells[0].colour[1] == 64);
        assert(ground->cells[0].lastFrame == -1);
        c3d_ground15_get_cell_height_min_max(ground, 0, 0, &min, &max);
        assert(min == 3 && max == 3);
    }
    resource.major = 1; resource.minor = 9;
    resource.waterSplitW = resource.waterSplitH = 1;
    resource.water = water; resource.waters = &water; resource.waterCount = 1;
    assert(c3d_ground15_load_water(ground, &resource, NULL));
    {
        struct GndWater *previous = ground->waters;
        previous->step = 12; allocation_calls = 0; fail_at = 1; water.level = 99;
        assert(!c3d_ground15_load_water(ground, &resource, NULL));
        assert(ground->waters == previous && previous->step == 12);
        assert(ground->waters[0].level == 7);
        fail_at = 0;
        assert(c3d_ground15_load_water(ground, &resource, NULL));
        assert(ground->waters[0].level == 99 && !ground->waters[0].step);
    }
    {
        struct GndWaterRec records[2] = {water, water};
        struct GndWater *previous = ground->waters;
        unsigned count = live_allocations;
        records[1].animSpeed = 0;
        resource.waters = records; resource.waterCount = 2;
        assert(!c3d_ground15_load_water(ground, &resource, NULL));
        assert(ground->waters == previous && live_allocations == count);
        resource.waters = &water; resource.waterCount = 1;
    }
    resource.zoom = NAN;
    assert(!c3d_ground15_load(ground, &resource, direction, diffuse, ambient));
    empty_map(ground, &attr);
    resource.zoom = 0;
    assert(!c3d_ground15_load(ground, &resource, direction, diffuse, ambient));
    resource.zoom = 10; resource.cubes = NULL;
    assert(!c3d_ground15_load(ground, &resource, direction, diffuse, ambient));
    resource.cubes = cells; resource.surfaces = NULL;
    assert(!c3d_ground15_load(ground, &resource, direction, diffuse, ambient));
    assert(!c3d_ground15_load(ground, NULL, direction, diffuse, ambient));
    empty_map(ground, &attr);
    ground_free(ground);
    assert(!live_allocations && !live_pages);
    puts("ground storage: typed arrays, mesh loading, allocation failures, page cleanup, water replacement and borrowed ownership passed");
    return 0;
}
