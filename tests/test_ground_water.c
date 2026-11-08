#include "render/texture_cache.h"
#include "world/ground.h"
#include "fileio/file.h"
#include "world_state_memory.h"
#include <limits.h>
#include <stdio.h>
#include <float.h>
#include <math.h>

static BYTE input[4096];
static size_t input_size;
static unsigned buffers;
struct GameFileMgr g_fileMgr;

struct TexCache g_textureCache;
static struct Texture picture_texture;
static int selected_picture, sampled_angle, fail_picture;
static unsigned picture_queries;
struct Texture *texture_cache_get(struct TexCache *cache, const char *name, int a, int b)
{
    size_t length = strlen(name);
    assert(cache == &g_textureCache && !a && !b && length >= 6 && length < 32);
    assert(!strcmp(name + length - 4, ".jpg"));
    selected_picture = (name[length - 6] - '0') * 10 + name[length - 5] - '0';
    assert(selected_picture >= 0 && selected_picture < 32);
    ++picture_queries;
    return fail_picture ? NULL : &picture_texture;
}
void surface_destroy(struct Surface *surface) { (void)surface; abort(); }
/* the ground's meshes are built on its first draw, which these tests do not reach */
void gfx_mesh_destroy(struct gfx_mesh *mesh) { (void)mesh; abort(); }
float get_sin_deg(int degrees)
{
    sampled_angle = degrees;
    return (float)sin((degrees % 360) * 3.14159265358979323846 / 180.0);
}

BYTE *game_file_mgr_get_data(struct GameFileMgr *mgr, const char *name, int *size, BYTE disk)
{
    BYTE *copy = malloc(input_size);
    (void)mgr; (void)name; (void)disk;
    assert(copy);
    memcpy(copy, input, input_size);
    *size = (int)input_size;
    ++buffers;
    return copy;
}
void game_file_mgr_release_data(struct GameFileMgr *mgr, void *data)
{ (void)mgr; assert(data && buffers); --buffers; free(data); }
BYTE game_file_mgr_is_file_exist(struct GameFileMgr *mgr, const char *name, BYTE disk)
{ (void)mgr; (void)name; (void)disk; return 1; }
void error_msg(const char *message, int flags) { assert(message && !flags); }
struct Res *res_construct(struct Res *res) { memset(res, 0, sizeof *res); return res; }
void res_destruct(struct Res *res) { (void)res; }
static void put(const void *data, size_t size)
{
    assert(size <= sizeof input - input_size);
    memcpy(input + input_size, data, size);
    input_size += size;
}
static void put_int(int value) { put(&value, sizeof value); }

static void ground_file(BYTE minor, int splits)
{
    BYTE version[2] = {1, minor};
    float zoom = 10.0f;
    struct GndCellRec cell = {{0}, -1, -1, -1};
    struct GndWaterRec water = {3.0f, 2, 4.0f, 12.0f, 6.0f, 8};
    int i;
    input_size = 0;
    put_int(GND_MAGIC); put(version, sizeof version);
    put_int(5); put_int(2); put(&zoom, sizeof zoom);
    put_int(0); put_int(16);
    put_int(0); put_int(8); put_int(8); put_int(1);
    put_int(0);
    for (i = 0; i < 10; ++i) put(&cell, sizeof cell);
    if (minor >= 8) {
        put(&water, sizeof water); put_int(splits); put_int(1);
        for (i = 0; i < splits; ++i) {
            water.level = (float)(20 + i);
            water.type = 10 + i;
            water.waveHeight = (float)(30 + i);
            water.waveSpeed = (float)(40 + i);
            water.wavePitch = (float)(50 + i);
            water.animSpeed = 60 + i;
            put(&water, minor == 8 ? sizeof(float) : sizeof water);
        }
    }
}

static void verify_water(const struct GndWater *actual, const struct GndWaterRec *expected)
{
    assert(actual->level == expected->level && actual->type == expected->type);
    assert(actual->waveHeight == expected->waveHeight && actual->phaseStep == expected->waveSpeed);
    assert(actual->wavePitch == expected->wavePitch && actual->stepsPerPicture == expected->animSpeed);
    assert(!actual->step && !actual->picture && !actual->phase && !actual->texture);
}

static void numeric_cases(void)
{
    struct C3dGround15 ground = {0};
    struct GndRes resource = {0};
    struct GndWaterRec records[2] = {{7, 2, 3, 1.25f, 30, 3}, {7, 2, 3, 1, 30, 3}};
    struct GroundWaterSettings scene = {7, 2, 3, 1.75f, 30.75f, 3};
    const float invalid[] = {NAN, INFINITY, -INFINITY};
    const int static_delays[] = {-1, INT_MIN};
    struct GndWater *previous;
    int delay, tick, field;
    unsigned i;
    resource.major = 1; resource.minor = 9;
    resource.waterSplitW = resource.waterSplitH = 1;
    resource.waters = records; resource.waterCount = 1; resource.water.type = 4;
    ground.width = ground.height = 2;
    for (delay = 1; delay <= 9; delay += 2) {
        float expected_phase = 0;
        records[0].animSpeed = delay;
        assert(c3d_ground15_load_water(&ground, &resource, NULL));
        for (tick = 1; tick <= delay * 32 * 3; ++tick) {
            c3d_ground15_update_water(&ground);
            expected_phase += 1.25f;
            if (expected_phase > 180) expected_phase -= 360;
            assert(selected_picture == (tick / delay) % 32);
            assert(ground.waters[0].step == tick % delay);
            assert(ground.waters[0].phase == expected_phase);
            assert(ground.waters[0].texture == &picture_texture);
        }
    }
    records[0].animSpeed = INT_MAX;
    records[0].type = INT_MIN;
    assert(c3d_ground15_load_water(&ground, &resource, NULL));
    ground.waters[0].step = INT_MAX - 2; ground.waters[0].picture = 31;
    c3d_ground15_update_water(&ground);
    assert(selected_picture == 31 && ground.waters[0].step == INT_MAX - 1);
    c3d_ground15_update_water(&ground);
    assert(!selected_picture && !ground.waters[0].step);
    fail_picture = 1;
    c3d_ground15_update_water(&ground);
    assert(!ground.waters[0].texture);
    fail_picture = 0;

    previous = ground.waters;
    resource.waterCount = 2; resource.waterSplitW = 2; resource.water.type = 99;
    records[1].animSpeed = 0;
    assert(!c3d_ground15_load_water(&ground, &resource, NULL));
    assert(ground.waters == previous && ground.waterCount == 1 && ground.waterType == 4);
    resource.waterCount = (size_t)-1;
    assert(!c3d_ground15_load_water(&ground, &resource, NULL));
    assert(!c3d_ground15_load_water(&ground, NULL, &scene));
    assert(ground.waters == previous);
    resource.waterCount = 2;
    for (i = 0; i < sizeof static_delays / sizeof *static_delays; ++i) {
        records[1].animSpeed = static_delays[i];
        assert(c3d_ground15_load_water(&ground, &resource, NULL));
        assert(ground.waterCount == 2 && ground.waterType == 99);
        for (tick = 0; tick < 70; ++tick) {
            c3d_ground15_update_water(&ground);
            assert(!ground.waters[1].picture && !ground.waters[1].step && !selected_picture);
        }
    }
    records[1].animSpeed = 3;
    for (field = 0; field < 4; ++field) {
        for (i = 0; i < sizeof invalid / sizeof *invalid; ++i) {
            struct GndWaterRec valid = records[1];
            if (field == 0) records[1].level = invalid[i];
            if (field == 1) records[1].waveHeight = invalid[i];
            if (field == 2) records[1].waveSpeed = invalid[i];
            if (field == 3) records[1].wavePitch = invalid[i];
            assert(c3d_ground15_load_water(&ground, &resource, NULL) && ground.waterCount == 2);
            c3d_ground15_update_water(&ground);
            (void)ground_water_height(&ground.waters[1], 5);
            (void)ground_water_height(&ground.waters[1], INT64_MIN);
            records[1] = valid;
        }
    }
    records[1].level = records[1].waveHeight = FLT_MAX;
    assert(c3d_ground15_load_water(&ground, &resource, NULL));
    assert(ground.waters[1].level == FLT_MAX && ground.waters[1].waveHeight == FLT_MAX);

    resource.waterCount = resource.waterSplitW = 1;
    records[0].animSpeed = 3; records[0].type = 2;
    records[0].waveSpeed = -725.5f;
    assert(c3d_ground15_load_water(&ground, &resource, NULL));
    for (tick = 0; tick < 400; ++tick) {
        c3d_ground15_update_water(&ground);
        assert(ground.waters[0].phase >= -180 && ground.waters[0].phase <= 180);
    }
    records[0].waveSpeed = FLT_MAX;
    records[0].wavePitch = FLT_MAX;
    assert(c3d_ground15_load_water(&ground, &resource, NULL));
    for (tick = 0; tick < 400; ++tick) {
        c3d_ground15_update_water(&ground);
        assert(isfinite(ground.waters[0].phase));
        assert(ground.waters[0].phase >= -180 && ground.waters[0].phase <= 180);
    }
    assert(isfinite(ground_water_height(ground.waters, INT64_MAX)));
    assert(sampled_angle > -360 && sampled_angle < 360);
    assert(isfinite(ground_water_height(ground.waters, INT64_MIN)));
    assert(sampled_angle > -360 && sampled_angle < 360);
    records[0].waveSpeed = 0; records[0].wavePitch = 30;
    assert(c3d_ground15_load_water(&ground, &resource, NULL));
    assert(ground_water_height(ground.waters, 3) == 10 && sampled_angle == 90);
    assert(ground_water_height(ground.waters, -3) == 4 && sampled_angle == -90);
    records[0].wavePitch = 6.25f;
    assert(c3d_ground15_load_water(&ground, &resource, NULL));
    ground.waters[0].phase = 1.25f;
    (void)ground_water_height(ground.waters, 37); assert(sampled_angle == 232);
    (void)ground_water_height(ground.waters, -37); assert(sampled_angle == -230);
    records[0].level = FLT_MAX; records[0].waveHeight = 0;
    assert(c3d_ground15_load_water(&ground, &resource, NULL));
    assert(ground_water_height(ground.waters, INT64_MAX) == FLT_MAX);

    resource.minor = 7;
    assert(c3d_ground15_load_water(&ground, &resource, &scene));
    assert(ground.waters[0].phaseStep == 1 && ground.waters[0].wavePitch == 30);
    previous = ground.waters;
    assert(!c3d_ground15_load_water(&ground, &resource, NULL));
    assert(ground.waters == previous);
    scene.waveSpeed = NAN;
    assert(c3d_ground15_load_water(&ground, &resource, &scene));
    assert(isnan(ground.waters[0].phaseStep));
    scene.waveSpeed = FLT_MAX; scene.wavePitch = -FLT_MAX;
    assert(c3d_ground15_load_water(&ground, &resource, &scene));
    c3d_ground15_update_water(&ground);
    assert(isfinite(ground_water_height(ground.waters, INT64_MIN)));

    resource.minor = 9; resource.waterCount = 0;
    resource.water.wavePitch = resource.water.waveSpeed = NAN;
    resource.water.animSpeed = 0;
    scene.frameDelay = 0;
    assert(c3d_ground15_load_water(&ground, &resource, &scene));
    assert(!ground.waters && !ground.waterCount);
    i = picture_queries; c3d_ground15_update_water(&ground);
    assert(picture_queries == i);
    c3d_ground15_clear(&ground);
}

int main(void)
{
    struct GndRes *resource = (struct GndRes *)ground_resource_create();
    struct C3dGround15 ground = {0};
    struct GndWaterRec expected[2];
    const struct GroundWaterSettings scene = {7, 3, 2, 4, 6, 8};
    struct GndWater *waters;
    int minor, pass;
    assert(resource);
    for (minor = 8; minor <= 9; ++minor) {
        ground_file((BYTE)minor, 2);
        assert(gnd_res_load(resource, "water.gnd"));
        ground.width = resource->width;
        ground.height = resource->height;
        memcpy(expected, resource->waters, sizeof expected);
        for (pass = 0; pass < 3; ++pass) {
            assert(c3d_ground15_load_water(&ground, resource, &scene));
            assert(ground.blocksX == 2 && ground.blocksY == 1 && ground.waterPerBlock);
            assert(ground.waterCount == 2);
            waters = ground.waters;
            verify_water(&waters[0], &expected[0]); verify_water(&waters[1], &expected[1]);
            assert(ground.waterType == 2);
            waters[0].step = 12; waters[0].phase = 90.0f;
            waters[0].texture = (struct Texture *)&ground;
        }
        assert(c3d_ground15_load_water(&ground, resource, &scene));
        ground_resource_clear(resource);
        assert(!buffers && !resource->waters && !resource->waterCount);
        waters = ground.waters;
        verify_water(&waters[0], &expected[0]); verify_water(&waters[1], &expected[1]);
        assert(c3d_ground15_get_block_index(&ground, 0, 0) == 0);
        assert(c3d_ground15_get_block_index(&ground, 1, 1) == 0);
        assert(c3d_ground15_get_block_index(&ground, 2, 1) == 1);
        assert(c3d_ground15_get_block_index(&ground, 4, 1) == 1);
        assert(c3d_ground15_get_block_index(&ground, INT_MAX, INT_MAX) == 1);
        assert(c3d_ground15_get_block_index(&ground, 0, -1) == -1);
    }
    ground_file(7, 0); assert(gnd_res_load(resource, "old-water.gnd"));
    expected[0].level = scene.level;
    expected[0].type = scene.type;
    expected[0].waveHeight = scene.waveHeight;
    expected[0].waveSpeed = scene.waveSpeed;
    expected[0].wavePitch = scene.wavePitch;
    expected[0].animSpeed = scene.frameDelay;
    for (pass = 0; pass < 3; ++pass) {
        assert(c3d_ground15_load_water(&ground, resource, &scene));
        assert(ground.blocksX == 1 && ground.blocksY == 1 && !ground.waterPerBlock);
        assert(ground.waterCount == 1);
        verify_water(ground.waters, &expected[0]);
    }
    ground_file(9, 0); assert(gnd_res_load(resource, "empty-water.gnd"));
    assert(c3d_ground15_load_water(&ground, resource, &scene));
    assert(!ground.waters && !ground.waterCount && !ground.waterPerBlock);
    assert(c3d_ground15_get_block_index(&ground, 0, 0) == -1);
    ground.blocksX = 6; ground.blocksY = 1;
    assert(c3d_ground15_get_block_index(&ground, 0, 0) == -1);
    ground.blocksX = -1;
    assert(c3d_ground15_get_block_index(&ground, 0, 0) == -1);
    ground.width = ground.height = ground.blocksX = ground.blocksY = INT_MAX;
    assert(c3d_ground15_get_block_index(&ground, INT_MAX, INT_MAX) == -1);
    c3d_ground15_clear(&ground);
    assert(!ground.waters && !ground.waterCount);
    ground.width = 0;
    assert(c3d_ground15_get_block_index(&ground, 0, 0) == -1);
    resource_destroy(&resource->res);
    assert(!live_blocks && !buffers);
    numeric_cases();
    puts("water numeric: atomic settings, a zero frame delay refused, negative delays static, non-finite values kept, phase wrapping and bounded angles passed");
    puts("ground water tests passed");
    return 0;
}
