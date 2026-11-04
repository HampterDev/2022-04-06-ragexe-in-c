#include "resources/world_res.h"
#include "resources/world_format.h"
#include "fileio/file.h"
#include "actor/weather.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct Block { unsigned char *data; size_t size; } blocks[32];
static unsigned live, allocation_calls, fail_at;
static void *new_block(size_t size)
{
    unsigned i;
    unsigned char *data = malloc(size + 32);
    assert(data);
    memset(data, 0xa5, 16); memset(data + 16, 0, size); memset(data + 16 + size, 0x5a, 16);
    for (i = 0; i < 32 && blocks[i].data; ++i) {}
    assert(i < 32); blocks[i].data = data; blocks[i].size = size; ++live;
    return data + 16;
}
static unsigned block_index(void *memory)
{
    unsigned i, j;
    for (i = 0; i < 32 && blocks[i].data != (unsigned char *)memory - 16; ++i) {}
    assert(i < 32);
    for (j = 0; j < 16; ++j) {
        assert(blocks[i].data[j] == 0xa5);
        assert(blocks[i].data[16 + blocks[i].size + j] == 0x5a);
    }
    return i;
}
static void observed_free(void *memory)
{
    unsigned i;
    if (!memory) return;
    i = block_index(memory); free(blocks[i].data); blocks[i].data = NULL; --live;
}
static void *observed_calloc(size_t count, size_t size)
{
    assert(count && size && count <= (size_t)-1 / size);
    if (++allocation_calls == fail_at) return NULL;
    return new_block(count * size);
}
static void *observed_realloc(void *memory, size_t size)
{
    void *next;
    size_t old_size;
    if (++allocation_calls == fail_at) return NULL;
    if (!memory) return new_block(size);
    old_size = blocks[block_index(memory)].size;
    next = new_block(size); memcpy(next, memory, old_size < size ? old_size : size);
    observed_free(memory);
    return next;
}
#define calloc observed_calloc
#define realloc observed_realloc
#define free observed_free
#include "../src/resources/world_res.c"
#undef free
#undef realloc
#undef calloc
#include "../src/actor/weather_hooks.c"

struct Weather g_weather;
struct GameFileMgr g_fileMgr;
static unsigned char input[100000];
static size_t input_size, count_offset, first_type_offset, first_model_offset, tree_offset;
static unsigned buffers;
BYTE *game_file_mgr_get_data(struct GameFileMgr *mgr, const char *name, int *size, BYTE disk)
{
    BYTE *copy = malloc(input_size ? input_size : 1);
    (void)mgr; (void)name; (void)disk; assert(copy);
    memcpy(copy, input, input_size); *size = (int)input_size; ++buffers; return copy;
}
void game_file_mgr_release_data(struct GameFileMgr *mgr, void *data)
{ (void)mgr; assert(data && buffers); --buffers; free(data); }
BYTE game_file_mgr_is_file_exist(struct GameFileMgr *mgr, const char *name, BYTE disk)
{ (void)mgr; (void)name; (void)disk; return 1; }
void error_msg(const char *message, int flags) { assert(message && !flags); }
struct Res *res_construct(struct Res *res) { memset(res, 0, sizeof *res); return res; }
void res_destruct(struct Res *res) { (void)res; }
static void put(const void *data, size_t size)
{ assert(input_size + size <= sizeof input); memcpy(input + input_size, data, size); input_size += size; }
static void put_int(int value) { put(&value, sizeof value); }
static void put_float(float value) { put(&value, sizeof value); }
static void text_field(char byte, size_t width)
{ char text[80]; assert(width <= sizeof text); memset(text, byte, width); put(text, width); }
static void fixture(BYTE major, BYTE minor, int build, int models)
{
    struct RswModelRec model = {0};
    struct RswLightRec light = {0};
    struct RswSoundRec sound = {0};
    struct RswEffectRec effect = {0};
    BYTE version[2] = {major, minor}, flag = 1;
    float diffuse[3] = {0.1f, 0.2f, 0.3f}, ambient[3] = {0.4f, 0.5f, 0.6f};
    int i;
    input_size = 0; put("GRSW", 4); put(version, 2);
    if (major == 2 && minor >= 5) put_int(build);
    if (major == 2 && minor >= 2) put(&flag, 1);
    text_field('I', 40); text_field('G', 40);
    if (major >= 2 || minor >= 4) text_field('A', 40);
    text_field('S', 40);
    if (!(major == 2 && minor >= 6)) {
        if (major >= 2 || minor >= 3) put_float(7);
        if (major >= 2 || minor >= 8) { put_int(4); put_float(3); put_float(2.5f); put_float(15.5f); }
        if (major >= 2 || minor >= 9) put_int(9);
    }
    if (major >= 2 || minor >= 5) {
        put_int(30); put_int(60); put(diffuse, sizeof diffuse); put(ambient, sizeof ambient);
    }
    if (major >= 2 || minor >= 7) put_int(0);
    if (major >= 2 || minor >= 6) { put_int(-10); put_int(20); put_int(-30); put_int(40); }
    count_offset = input_size; put_int(models + 3); first_type_offset = input_size;
    memset(model.fileName, 'M', sizeof model.fileName); memset(model.nodeName, 'N', sizeof model.nodeName);
    model.pos[0] = 1; model.pos[1] = 2; model.pos[2] = 3;
    model.rot[0] = 4; model.rot[1] = 5; model.rot[2] = 6;
    model.scale[0] = model.scale[1] = model.scale[2] = 1;
    for (i = 0; i < models; ++i) {
        put_int(1);
        if (major >= 2 || minor >= 3) {
            text_field('R', 40); put_int(2); put_float(1.5f); put_int(3);
            if (major == 2 && minor >= 6 && build >= 186) put(&flag, 1);
        }
        if (!i) first_model_offset = input_size;
        put(&model, sizeof model);
    }
    put_int(2); put(&light, sizeof light);
    memset(sound.name, 'B', sizeof sound.name); memset(sound.waveName, 'W', sizeof sound.waveName);
    sound.pos[0] = 11; sound.pos[1] = 12; sound.pos[2] = 13;
    sound.volume = 0.8f; sound.width = 7; sound.height = 8; sound.range = 9; sound.cycle = 2;
    put_int(3); put(&sound, major >= 2 ? sizeof sound : offsetof(struct RswSoundRec, cycle));
    memset(effect.name, 'E', sizeof effect.name); effect.pos[0] = 21; effect.pos[1] = 22; effect.pos[2] = 23;
    effect.id = 100; effect.emitSpeed = 50;
    for (i = 0; i < 4; ++i) effect.param[i] = (float)(i + 1);
    put_int(4); put(&effect, sizeof effect); tree_offset = input_size;
    if (major == 2 && minor >= 1) {
        for (i = 0; i < RSW_TREE_NODE_COUNT; ++i) {
            struct RswBounds box = {{0}};
            box.max[0] = (float)(i + 100); box.min[0] = (float)(-i - 100);
            box.half[0] = (float)i; box.centre[0] = (float)(i * 2);
            put(&box, sizeof box);
        }
    }
}
static void empty_resource(const struct C3dWorldRes *res, unsigned expected_live)
{
    assert(!res->models && !res->modelCount && !res->effects && !res->effectCount);
    assert(!res->sounds && !res->soundCount && !res->tree);
    assert(!res->gndFile[0] && !res->gatFile[0] && !res->srcFile[0]);
    assert(!res->waterLevel && !res->waterType && res->waveHeight == 1);
    assert(res->waveSpeed == 2 && res->wavePitch == 50 && res->waterAnimSpeed == 3);
    assert(!res->major && !res->minor && !res->twoPassAlpha);
    assert(live == expected_live && !buffers);
}
static void verify(const struct C3dWorldRes *res, int major, int minor, int build, size_t models)
{
    assert(!buffers && res->modelCount == models && res->effectCount == 1 && res->soundCount == 1);
    assert(strlen(res->gndFile) == 40 && strlen(res->srcFile) == 40);
    assert(strlen(res->gatFile) == (size_t)(major >= 2 || minor >= 4 ? 40 : 0));
    assert(strlen(res->models[0].fileName) == 80 && strlen(res->models[0].nodeName) == 80);
    assert(strlen(res->models[0].name) == (size_t)(major >= 2 || minor >= 3 ? 40 : 0));
    assert(res->models[0].pos[2] == 3 && res->models[0].rot[1] == 5 && res->models[0].scale[0] == 1);
    assert(res->models[0].flag == (major == 2 && minor >= 6 && build >= 186));
    assert(strlen(res->sounds[0].waveName) == 80 && res->sounds[0].pos[0] == 11);
    assert(res->sounds[0].width == 7 && res->sounds[0].cycle == (major >= 2 ? 2 : 4));
    assert(strlen(res->effects[0].name) == 80 && res->effects[0].id == 100);
    assert(res->effects[0].param[3] == 4 && res->effects[0].pos[2] == 23);
    assert(!memcmp(g_weather.tint, res->ambient, sizeof res->ambient));
    if (major == 2 && minor >= 1) {
        assert(res->tree && res->tree[0].max[0] == 100);
        assert(res->tree[RSW_TREE_NODE_COUNT - 1].centre[0] == (RSW_TREE_NODE_COUNT - 1) * 2);
    } else assert(!res->tree);
    if ((major == 1 && minor < 3) || (major == 2 && minor >= 6))
        assert(res->waterLevel == 0 && res->waterAnimSpeed == 3);
    else assert(res->waterLevel == 7);
}
int main(void)
{
    struct C3dWorldRes resource, *owned;
    const int versions[][3] = {{1,0,0}, {1,9,0}, {2,0,0}, {2,6,185}, {2,6,186}};
    unsigned i, failures;
    size_t complete, length;
    g_worldResLight = weather_take_map_light;   /* as client_install_hooks() does */
    world_res_init(&resource); empty_resource(&resource, 0);
    fail_at = 1; allocation_calls = 0; assert(!world_resource_create()); fail_at = 0;
    for (i = 0; i < sizeof versions / sizeof *versions; ++i) {
        fixture((BYTE)versions[i][0], (BYTE)versions[i][1], versions[i][2], 1);
        assert(c3d_world_res_load(&resource, "scene.rsw"));
        verify(&resource, versions[i][0], versions[i][1], versions[i][2], 1);
    }
    complete = input_size;
    for (length = 0; length < complete; ++length) {
        float tint[3]; memcpy(tint, g_weather.tint, sizeof tint);
        input_size = length; assert(!c3d_world_res_load(&resource, "short.rsw"));
        empty_resource(&resource, 0); assert(!memcmp(tint, g_weather.tint, sizeof tint));
    }
    input_size = complete;
    { int bad = -1; memcpy(input + count_offset, &bad, 4); }
    assert(!c3d_world_res_load(&resource, "negative-count.rsw")); empty_resource(&resource, 0);
    fixture(2,6,186,1);
    { int bad = 99; memcpy(input + first_type_offset, &bad, 4); }
    assert(!c3d_world_res_load(&resource, "unknown-object.rsw")); empty_resource(&resource, 0);
    fixture(2,6,186,1);
    { float bad = NAN; memcpy(input + first_model_offset + offsetof(struct RswModelRec, pos), &bad, 4); }
    assert(!c3d_world_res_load(&resource, "nan-model.rsw")); empty_resource(&resource, 0);
    fixture(2,6,186,1);
    { float bad = INFINITY; memcpy(input + tree_offset, &bad, 4); }
    assert(!c3d_world_res_load(&resource, "infinite-bounds.rsw")); empty_resource(&resource, 0);
    fixture(2,6,186,33); allocation_calls = 0;
    assert(c3d_world_res_load(&resource, "growth.rsw")); verify(&resource,2,6,186,33);
    failures = allocation_calls; assert(failures == 6);
    for (i = 1; i <= failures; ++i) {
        allocation_calls = 0; fail_at = i;
        assert(!c3d_world_res_load(&resource, "allocation-failure.rsw")); empty_resource(&resource, 0);
    }
    fail_at = 0; owned = (struct C3dWorldRes *)world_resource_create(); assert(owned);
    assert(c3d_world_res_load(owned, "owned.rsw"));
    resource_destroy(&owned->res); assert(!live && !buffers);
    assert(c3d_world_res_load(&resource, "direct-destruct.rsw"));
    world_resource_finish(&resource); empty_resource(&resource, 0);
    puts("scene resource: versions, full-width names, typed ownership, reloads, every-byte truncation, array failures and flat bounds passed");
    return 0;
}
