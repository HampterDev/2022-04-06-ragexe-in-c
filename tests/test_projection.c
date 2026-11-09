#include "render/projection.h"
#include "render/renderer.h"
#include "render/render_api.h"
#include "render/primitive_types.h"
#include "resources/attr.h"
#include "fixtures/projection_reference.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DWORD g_fogColor;
float g_fogNear, g_fogFar;
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

int gfx_shadows_supported(void) { return 1; }
int gfx_world_supported(void) { return 1; }

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

/* rp_vertex_fog_alpha (render/rp_pool.c), which this test does not link */
static int cpu_fog_alpha(float rhw)
{
    float range = g_fogFar - g_fogNear, t = (1.0f / rhw - 10.0f) / 1490.0f;
    if (range != 0.0f)
        t = (t - g_fogNear) / range;
    t = t < 0.0f ? 0.0f : t > 1.0f ? 1.0f : t;
    return (int)((1.0f - t) * 255.0f);
}

/* custom: a static mesh drawn by the device (render/gfx.h) lands where renderer_project_vertex puts
   the same point, with its depth and fog */
static void test_world_view(struct Renderer *renderer)
{
    static const float view[12] = {0.8f, -0.3f, 0.52f, 0.1f, 0.9f, 0.42f, -0.6f, -0.3f, 0.74f, 15, -40, 260};
    static const float fog[3][2] = {{0.2f, 0.8f}, {0.0f, 1.0f}, {0.5f, 0.5f}};
    static const Vec3 points[4] = {{0, 0, 0}, {120, 35, -80}, {-200, 10, 400}, {35, -60, 900}};
    struct GfxWorldView world;
    int f, i;
    for (f = 0; f < 3; ++f) {
        g_fogNear = fog[f][0];
        g_fogFar = fog[f][1];
        renderer_world_view(renderer, view, &world);
        assert(!memcmp(world.view, view, sizeof view));
        assert(world.nearZ == RENDER_NEAR_Z && world.depthScale == RENDER_DEPTH_SCALE);
        assert(world.centre[0] == (float)renderer->centreX && world.centre[1] == (float)renderer->centreY);
        for (i = 0; i < 4; ++i) {
            const Vec3 *p = &points[i];
            struct TexDrawVertex cpu;
            float vx = world.view[0] * p->x + world.view[3] * p->y + world.view[6] * p->z + world.view[9];
            float vy = world.view[1] * p->x + world.view[4] * p->y + world.view[7] * p->z + world.view[10];
            float vz = world.view[2] * p->x + world.view[5] * p->y + world.view[8] * p->z + world.view[11];
            float a = (world.fogEnd - vz) / (world.fogEnd - world.fogStart);
            int fogAlpha;
            renderer_project_vertex(renderer, p, view, &cpu);
            assert(vz > RENDER_NEAR_Z);
            assert(fabsf(vx * world.proj[0] / vz + world.centre[0] - cpu.x) < 0.01f);
            assert(fabsf(vy * world.proj[1] / vz + world.centre[1] - cpu.y) < 0.01f);
            assert(fabsf((vz - world.nearZ) * world.depthScale / vz - cpu.z) < 1e-6f);
            a = a < 0.0f ? 0.0f : a > 1.0f ? 1.0f : a;
            fogAlpha = (int)floorf(a * 255.0f);
            assert(abs(fogAlpha - cpu_fog_alpha(cpu.rhw)) <= 1);
        }
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
    test_world_view(&renderer);
    puts("projection: archived coordinates/depth/rays, distinct centres, vertex payload preservation and real ground picking passed");
    puts("renderer screen: named dimensions, clear colour, fog blending, depth clear and frame restore passed");
    puts("world view: device-projected meshes land on the CPU's pixels, depth and fog passed");
    return 0;
}
