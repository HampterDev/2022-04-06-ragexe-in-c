#include "render/projection.h"
#include "render/renderer.h"
#include "render/render_api.h"
#include "render/primitive_types.h"
#include "resources/attr.h"
#include "fixtures/projection_reference.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

DWORD g_fogColor;
struct Renderer *g_renderer;
struct C3dDevice g_3dDevice;
static DWORD cleared_color;
static unsigned color_clears, depth_clears, frame_restores;

int c3d_device_clear(struct C3dDevice *device, DWORD color)
{
    assert(device == &g_3dDevice);
    cleared_color = color;
    ++color_clears;
    return 0;
}

int c3d_device_clear_z(struct C3dDevice *device)
{
    assert(device == &g_3dDevice);
    ++depth_clears;
    return 0;
}

void c3d_device_restore_frame(struct C3dDevice *device)
{
    assert(device == &g_3dDevice);
    ++frame_restores;
}

static void test_screen(void)
{
    struct Renderer renderer = {0};
    renderer_set_projection(&renderer, 1280, 960);
    assert(renderer.width == 1280 && renderer.height == 960);
    assert(renderer.centreX == 640 && renderer.centreY == 480);
    assert(renderer.halfW == 640 && renderer.halfH == 480);
    assert(renderer.scaleX == 2 && renderer.scaleY == 2);
    renderer.clearColor = 0xab102030;
    g_fogColor = 0xffc09060;
    renderer_clear_scene(&renderer);
    assert(cleared_color == 0xab102030 && color_clears == 1);
    renderer.fogEnable = 1;
    renderer_clear_scene(&renderer);
    assert(cleared_color == 0xabc09060 && color_clears == 2);
    g_fogColor = 0x55a0b0c0;
    renderer_clear_scene(&renderer);
    assert(cleared_color == 0xab405060 && color_clears == 3);
    renderer_clear_frame(&renderer, 1);
    assert(cleared_color == 0xff000000 && color_clears == 4);
    renderer_clear_frame(&renderer, 0);
    assert(depth_clears == 1 && color_clears == 4);
    renderer_restore_frame(&renderer);
    assert(frame_restores == 1);
    assert(renderer.clearColor == 0xab102030);
}

static void same_float(float a, float b)
{
    assert(!memcmp(&a, &b, sizeof a));
}

static void test_vertices(struct Renderer *renderer)
{
    static const float matrices[][12] = {
        {1,0,0, 0,1,0, 0,0,1, 0,0,0},
        {0,0,1, 0,1,0, -1,0,0, 10,-3,100},
        {0.5f,0.125f,0.25f, -0.25f,2,0.125f, 0.5f,-0.25f,1, -10,3,5}
    };
    unsigned matrix, sample;
    for (matrix = 0; matrix < sizeof matrices / sizeof matrices[0]; ++matrix) {
        for (sample = 0; sample < 200; ++sample) {
            Vec3 point = {(float)(int)(sample % 23) - 11.25f,
                          (float)(int)(sample % 31) - 15.75f,
                          20.5f + (float)sample * 0.375f};
            float raw[3] = {point.x, point.y, point.z};
            float expected[4], actual[4];
            struct { unsigned before; struct TexDrawVertex value; unsigned after; } single;
            struct { unsigned before; struct RPTwoTexVertex value; unsigned after; } multi;
            struct TexDrawVertex old_single;
            struct RPTwoTexVertex old_multi;
            const float *view = matrices[matrix];
            memset(&single, 0xa5, sizeof single);
            memset(&multi, 0x5a, sizeof multi);
            old_single = single.value; old_multi = multi.value;

            renderer_project_point(renderer, &point, view, &actual[0], &actual[1], &actual[2]);
            reference_CRenderer_ProjectPoint(renderer, raw, view, &expected[0], &expected[1], &expected[2]);
            assert(!memcmp(actual, expected, 3 * sizeof(float)));
            same_float(renderer_screen_scale(renderer, actual[2]),
                reference_CRenderer_ScreenScaleAtRhw(renderer, expected[2]));
            same_float(renderer_project_depth(actual[2]), reference_Renderer_ProjectZ(expected[2]));

            renderer_project_centred(renderer, &point, view, &actual[0], &actual[1], &actual[2], 327.5f, 255.25f);
            reference_Renderer_ProjectCentred(renderer, raw, view, &expected[0], &expected[1], &expected[2], 327.5f, 255.25f);
            assert(!memcmp(actual, expected, 3 * sizeof(float)));

            renderer_project_vertex(renderer, &point, view, &single.value);
            reference_CRenderer_ProjectVertex(renderer, raw, view, &old_single);
            assert(!memcmp(&single.value, &old_single, sizeof old_single));
            assert(single.before == 0xa5a5a5a5 && single.after == 0xa5a5a5a5);

            renderer_project_multitex_vertex(renderer, &point, view, &multi.value);
            reference_CRenderer_ProjectMultiTexVertex(renderer, raw, view, expected);
            actual[0] = multi.value.x; actual[1] = multi.value.y;
            actual[2] = multi.value.z; actual[3] = multi.value.rhw;
            assert(!memcmp(actual, expected, sizeof actual));
            old_multi.x = expected[0]; old_multi.y = expected[1];
            old_multi.z = expected[2]; old_multi.rhw = expected[3];
            assert(!memcmp(&multi.value, &old_multi, sizeof old_multi));
            assert(multi.before == 0x5a5a5a5a && multi.after == 0x5a5a5a5a);
        }
    }
}

static void test_cursor(struct Renderer *renderer)
{
    static const float inverse[][12] = {
        {1,0,0, 0,1,0, 0,0,1, 0,0,0},
        {0,0,1, 0,1,0, -1,0,0, 0,0,0},
        {1,0,0, 0,0,-1, 0,1,0, 0,0,0}
    };
    Vec3 eye = {-2.5f, -10, -2.5f};
    float raw_eye[3] = {eye.x, eye.y, eye.z};
    int x, y;
    unsigned matrix;
    for (matrix = 0; matrix < 3; ++matrix) {
        for (x = -20; x < 900; x += 73) for (y = -10; y < 700; y += 61) {
            struct { unsigned before; struct Ray3 value; unsigned after; } ray;
            float expected[6], actual[6], norm;
            memset(&ray, 0xa5, sizeof ray);
            renderer_cursor_ray(renderer, x, y, &eye, inverse[matrix], &ray.value);
            reference_CRenderer_CursorRay(renderer, x, y, raw_eye, inverse[matrix], expected);
            actual[0] = ray.value.origin.x; actual[1] = ray.value.origin.y; actual[2] = ray.value.origin.z;
            actual[3] = ray.value.dir.x; actual[4] = ray.value.dir.y; actual[5] = ray.value.dir.z;
            assert(!memcmp(actual, expected, sizeof actual));
            assert(ray.value.length == 0 && ray.before == 0xa5a5a5a5 && ray.after == 0xa5a5a5a5);
            norm = ray.value.dir.x * ray.value.dir.x + ray.value.dir.y * ray.value.dir.y + ray.value.dir.z * ray.value.dir.z;
            assert(fabsf(norm - 1) < 0.000001f);
        }
    }
    {
        struct AttrCell cells[4] = {0};
        struct C3dAttr grid = {0};
        struct Ray3 ray;
        int i;
        grid.width = grid.height = 2; grid.zoom = 5; grid.cells = cells;
        for (i = 0; i < 4; ++i) cells[0].height[i] = 3;
        renderer_cursor_ray(renderer, renderer->centreX, renderer->centreY, &eye, inverse[2], &ray);
        assert(ground_ray_distance(&grid, &ray, 0, 0) == 13);
    }
}

int main(void)
{
    struct Renderer renderer = {0};
    renderer.proj[0] = 2400.25f; renderer.proj[1] = 1800.5f;
    renderer.halfW = 400; renderer.halfH = 300;
    renderer.centreX = 320; renderer.centreY = 240;
    renderer.scaleX = 1.25f;
    same_float(renderer_scale_x(&renderer), reference_CRenderer_GetScaleX(&renderer));
    test_vertices(&renderer);
    test_cursor(&renderer);
    test_screen();
    puts("projection: archived coordinates/depth/rays, distinct centres, vertex payload preservation and real ground picking passed");
    puts("renderer screen: named dimensions, clear colour, fog blending, depth clear and frame restore passed");
    return 0;
}
