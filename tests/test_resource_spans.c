#include "render/texture_cache.h"
#include "fileio/file.h"
#include "resources/gnd.h"
#include "resources/gnd_format.h"
#include "resources/ezeffect.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "owned_storage_test_allocator.h"
#define malloc checked_malloc
#define free checked_free
#include "../src/resources/gnd.c"
#include "../src/resources/ezeffect.c"
#undef malloc
#undef free

static unsigned char input[32768];
static size_t input_size;
static int missing, buffers, locks, unlocks, queries;
static char last_texture[256];
static struct Texture texture;
struct GameFileMgr g_fileMgr;
struct TexCache g_textureCache;
struct Texture g_whiteTexture;

BYTE *game_file_mgr_get_data(struct GameFileMgr *mgr, const char *name, int *size, BYTE disk)
{
    BYTE *copy;
    (void)mgr; (void)name; (void)disk;
    *size = 0;
    if (missing) return NULL;
    copy = malloc(input_size ? input_size : 1);
    assert(copy);
    memcpy(copy, input, input_size);
    *size = (int)input_size;
    ++buffers;
    return copy;
}
void game_file_mgr_release_data(struct GameFileMgr *mgr, void *data)
{ (void)mgr; assert(data && buffers > 0); --buffers; free(data); }
BYTE game_file_mgr_is_file_exist(struct GameFileMgr *mgr, const char *name, BYTE disk)
{ (void)mgr; (void)name; (void)disk; return !missing; }
void error_msg(const char *message, int flags) { assert(message && !flags); }
struct Res *res_construct(struct Res *res) { memset(res, 0, sizeof *res); return res; }
void res_destruct(struct Res *res) { res->ops = NULL; }
struct Texture *texture_cache_get(struct TexCache *cache, const char *name, int a, int b)
{
    assert(cache == &g_textureCache && a == 1 && b == 0 && strlen(name) < sizeof last_texture);
    strcpy(last_texture, name); ++queries; return &texture;
}
void texture_lock(struct Texture *p) { assert(p == &texture); ++locks; }
void texture_unlock(struct Texture *p) { assert(p == &texture && locks > unlocks); ++unlocks; }

static size_t put(const void *data, size_t size)
{
    size_t offset = input_size;
    assert(size <= sizeof input - input_size);
    memcpy(input + input_size, data, size); input_size += size;
    return offset;
}
static size_t put_int(int value) { return put(&value, sizeof value); }
static void patch_int(size_t at, int value) { memcpy(input + at, &value, sizeof value); }
static size_t put_zero(size_t size)
{
    size_t offset = input_size;
    assert(size <= sizeof input - input_size);
    memset(input + input_size, 0, size); input_size += size;
    return offset;
}
static size_t width_at, height_at, textures_at, name_size_at, name_at;
static size_t lights_at, surfaces_at, surface_at, cube_at, water_x_at, water_y_at;
static size_t layers_at, keys_at;

static void ground_file(unsigned char minor)
{
    unsigned char version[2] = {1, minor};
    float zoom = 10.0f;
    struct GndSurfaceRec surface = {0};
    struct GndCellRec cube = {{0, 1, 2, 3}, 0, -1, -1};
    struct GndWaterRec water = {3.0f, 1, 2.0f, 4.0f, 6.0f, 8};
    input_size = 0;
    put_int(GND_MAGIC); put(version, sizeof version);
    width_at = put_int(1); height_at = put_int(1); put(&zoom, sizeof zoom);
    textures_at = put_int(1); name_size_at = put_int(16);
    name_at = put_zero(16); memcpy(input + name_at, "tile.bmp", 9);
    lights_at = put_int(1); put_int(8); put_int(8); put_int(1);
    memset(input + put_zero(GND_LIGHTMAP_SIZE), 0x42, GND_LIGHTMAP_SIZE);
    surfaces_at = put_int(1); surface_at = put(&surface, sizeof surface);
    cube_at = put(&cube, sizeof cube);
    if (minor >= 8) {
        put(&water, sizeof water); water_x_at = put_int(2); water_y_at = put_int(1);
        water.level = 5.0f;
        put(&water, minor == 8 ? sizeof(float) : sizeof water);
        water.level = 6.0f;
        put(&water, minor == 8 ? sizeof(float) : sizeof water);
    }
}

static void assert_empty_ground(const struct GndRes *res)
{
    assert(!buffers && !res->file.buffer);
    assert(!res->width && !res->height && !res->textureCount && !res->surfaceCount);
    assert(!res->cubes && !res->surfaces && !res->lightmaps);
    assert(!res->textureNames && !res->waters && !res->waterCount);
}

static void test_ground(void)
{
    struct GndRes res;
    unsigned char lightmap[GND_LIGHTMAP_SIZE];
    size_t size, i, bad[8];
    int minor;
    gnd_init(&res);
    for (minor = 7; minor <= 9; ++minor) {
        ground_file((unsigned char)minor); size = input_size;
        for (i = 0; i < size; ++i) {
            input_size = i;
            assert(!gnd_res_load(&res, "test.gnd"));
            assert_empty_ground(&res);
        }
        input_size = size;
        assert(gnd_res_load(&res, "test.gnd") && buffers == 1);
        assert(res.width == 1 && res.height == 1 && res.zoom == 10.0f);
        assert(strcmp(res.textureNames[0], "tile.bmp") == 0);
        assert(surface_at % 4 && cube_at % 4);
        assert((uintptr_t)res.surfaces % 4 == 0 && (uintptr_t)res.cubes % 4 == 0);
        assert((const BYTE *)res.surfaces != res.file.buffer + surface_at);
        assert((const BYTE *)res.cubes != res.file.buffer + cube_at);
        assert(res.cubes[0].height[3] == 3.0f && res.surfaces[0].texture == 0);
        gnd_res_get_lightmap(&res, 0, lightmap);
        assert(lightmap[0] == 0x42 && lightmap[255] == 0x42);
        if (minor >= 8) {
            assert(res.waterCount == 2);
            assert(res.waters[0].level == 5.0f && res.waters[1].level == 6.0f);
            assert(res.waters[0].type == 1 && res.water.type == 1);
        }
        assert(gnd_res_load(&res, "again.gnd") && buffers == 1);
        assert(res.waterCount == (minor >= 8 ? 2u : 0u));
        gnd_res_get_lightmap(&res, -1, lightmap);
        for (i = 0; i < sizeof lightmap; ++i) assert(lightmap[i] == 0);
        memset(lightmap, 0xff, sizeof lightmap);
        gnd_res_get_lightmap(&res, INT_MAX, lightmap);
        for (i = 0; i < sizeof lightmap; ++i) assert(lightmap[i] == 0);
        ground_resource_clear(&res); ground_resource_clear(&res); assert_empty_ground(&res);
        memset(lightmap, 0xff, sizeof lightmap);
        gnd_res_get_lightmap(&res, 0, lightmap);
        for (i = 0; i < sizeof lightmap; ++i) assert(lightmap[i] == 0);
    }
    ground_file(9);
    bad[0] = width_at; bad[1] = height_at; bad[2] = textures_at; bad[3] = name_size_at;
    bad[4] = lights_at; bad[5] = surfaces_at; bad[6] = water_x_at; bad[7] = water_y_at;
    for (i = 0; i < 8; ++i) {
        ground_file(9); patch_int(bad[i], -1);
        assert(!gnd_res_load(&res, "negative.gnd")); assert_empty_ground(&res);
        ground_file(9); patch_int(bad[i], INT_MAX);
        assert(!gnd_res_load(&res, "overflow.gnd")); assert_empty_ground(&res);
    }
    ground_file(7); memset(input + name_at, 'x', 16);
    assert(gnd_res_load(&res, "name.gnd") && strlen(res.textureNames[0]) == 17);
    assert(!memcmp(res.textureNames[0], "xxxxxxxxxxxxxxxx", 16) && res.textureNames[0][16] == 1);
    ground_file(7); patch_int(name_size_at, 0);
    memmove(input + name_at, input + name_at + 16, input_size - name_at - 16); input_size -= 16;
    assert(gnd_res_load(&res, "unnamed.gnd") && res.textureCount == 1);
    assert(res.textureNames[0][0] == 1 && !res.textureNames[0][1]);
    ground_file(7); input[surface_at + offsetof(struct GndSurfaceRec, texture)] = 1;
    assert(!gnd_res_load(&res, "texture.gnd")); assert_empty_ground(&res);
    ground_file(7); input[surface_at + offsetof(struct GndSurfaceRec, lightmap)] = 1;
    assert(!gnd_res_load(&res, "lightmap.gnd")); assert_empty_ground(&res);
    ground_file(7); patch_int(cube_at + offsetof(struct GndCellRec, top), INT_MAX);
    assert(gnd_res_load(&res, "skipped.gnd"));
    ground_file(7); patch_int(width_at, 0); patch_int(height_at, 0); patch_int(textures_at, 0);
    input_size = name_at; put_int(0); put_int(8); put_int(8); put_int(1); put_int(0);
    assert(gnd_res_load(&res, "empty.gnd"));
    patch_int(name_size_at, 0);
    assert(gnd_res_load(&res, "empty-names.gnd") && !res.textureCount);
    patch_int(name_size_at, -1);
    assert(gnd_res_load(&res, "negative-names.gnd") && !res.textureCount);
    ground_file(6);
    assert(!gnd_res_load(&res, "old-version.gnd")); assert_empty_ground(&res);
    ground_file(7); patch_int(0, 0);
    assert(!gnd_res_load(&res, "magic.gnd")); assert_empty_ground(&res);
    missing = 1; assert(!gnd_res_load(&res, "missing.gnd")); missing = 0;
    assert_empty_ground(&res);
    ground_resource_finish(&res); ground_resource_finish(&res);
    assert(!outstanding && !buffers && !res.res.ops);
}

static void effect_file(void)
{
    struct StrKey keys[2] = {0};
    keys[0].frame = 2; keys[0].s.x = 4.0f;
    keys[1].frame = 3; keys[1].type = 1; keys[1].s.x = 1.0f;
    input_size = 0;
    put_int(0x4d525453); put_int(STR_VERSION); put_int(60); put_int(5);
    layers_at = put_int(1); put_zero(16); textures_at = put_int(1);
    name_at = put_zero(128); memcpy(input + name_at, "sprite", 7);
    keys_at = put_int(2); put(keys, sizeof keys);
}

static void assert_empty_effect(const struct EzEffectRes *res)
{
    assert(!buffers && !res->file.buffer && !res->anim.layer_count);
    assert(!res->anim.layers[0].key_count && !res->anim.layers[0].keys);
    assert(locks == unlocks);
}

static void test_effect(void)
{
    struct EzEffectRes res;
    size_t size, i, bad[3];
    char long_folder[260];
    ezeffect_init(&res);
    effect_file(); size = input_size;
    for (i = 0; i < size; ++i) {
        input_size = i;
        assert(!e_zeffect_res_load(&res, "test.str")); assert_empty_effect(&res);
    }
    input_size = size;
    assert(e_zeffect_res_load(&res, "test.str") && res.anim.layer_count == 1);
    assert(res.anim.layers[0].key_count == 2 && buffers == 1);
    assert(res.anim.layers[0].keys[0].frame == 2 && res.anim.layers[0].keys[0].s.x == 4.0f);
    assert(res.anim.layers[0].keys[1].type == 1 && res.anim.layers[0].keys[1].s.x == 1.0f);
    assert(str_layer_get_texture(&res.anim.layers[0], -1) == &g_whiteTexture);
    assert(str_layer_get_texture(&res.anim.layers[0], 1) == &g_whiteTexture);
    assert(str_layer_get_texture(&res.anim.layers[0], 0) == &texture);
    assert(strcmp(last_texture, "effect\\sprite.bmp") == 0 && locks == 1 && queries == 1);
    assert(str_layer_get_texture(&res.anim.layers[0], 0) == &texture && queries == 1);
    assert(e_zeffect_res_load(&res, "reload.str") && unlocks == 1);
    assert(str_layer_get_set_texture(&res.anim.layers[0], 0, "set\\") == &texture);
    assert(strcmp(last_texture, "effect\\set\\sprite.bmp") == 0);
    effect_resource_clear(&res); effect_resource_clear(&res); assert_empty_effect(&res);
    assert(e_zeffect_res_load(&res, "test.str"));
    memset(long_folder, 'x', sizeof long_folder - 1); long_folder[sizeof long_folder - 1] = 0;
    assert(str_layer_get_set_texture(&res.anim.layers[0], 0, long_folder) == &g_whiteTexture);
    assert(locks == unlocks);
    long_folder[239] = 0;
    assert(str_layer_get_set_texture(&res.anim.layers[0], 0, long_folder) == &g_whiteTexture);
    assert(locks == unlocks);
    effect_file(); patch_int(layers_at, STR_LAYERS); input_size = textures_at;
    put_int(STR_LAYER_TEXTURES);
    for (i = 0; i < STR_LAYER_TEXTURES; ++i) {
        size_t name = put_zero(128); memcpy(input + name, "last.bmp", 9);
    }
    put_int(0);
    for (i = 1; i < STR_LAYERS; ++i) { put_int(0); put_int(0); }
    assert(e_zeffect_res_load(&res, "full.str"));
    assert(res.anim.layer_count == STR_LAYERS && res.anim.layers[0].texture_count == STR_LAYER_TEXTURES);
    assert(str_layer_get_texture(&res.anim.layers[0], STR_LAYER_TEXTURES - 1) == &texture);
    effect_file(); input_size = keys_at;
    assert(!e_zeffect_res_load(&res, "failed-reload.str")); assert_empty_effect(&res);
    effect_file();
    bad[0] = layers_at; bad[1] = textures_at; bad[2] = keys_at;
    for (i = 0; i < 3; ++i) {
        effect_file(); patch_int(bad[i], -1);
        assert(!e_zeffect_res_load(&res, "negative.str")); assert_empty_effect(&res);
        effect_file(); patch_int(bad[i], INT_MAX);
        assert(!e_zeffect_res_load(&res, "overflow.str")); assert_empty_effect(&res);
    }
    effect_file(); patch_int(layers_at, STR_LAYERS + 1);
    assert(!e_zeffect_res_load(&res, "layers.str")); assert_empty_effect(&res);
    effect_file(); patch_int(textures_at, STR_LAYER_TEXTURES + 1);
    assert(!e_zeffect_res_load(&res, "textures.str")); assert_empty_effect(&res);
    effect_file(); memset(input + name_at, 'x', 128);
    assert(!e_zeffect_res_load(&res, "name.str")); assert_empty_effect(&res);
    effect_file(); patch_int(4, STR_VERSION + 1);
    assert(!e_zeffect_res_load(&res, "version.str")); assert_empty_effect(&res);
    effect_file(); patch_int(textures_at, 0); input_size = textures_at + 4; put_int(0);
    assert(e_zeffect_res_load(&res, "empty-layer.str"));
    assert(!res.anim.layers[0].key_count && !res.anim.layers[0].keys);
    missing = 1; assert(!e_zeffect_res_load(&res, "missing.str")); missing = 0;
    assert_empty_effect(&res);
    effect_file(); assert(e_zeffect_res_load(&res, "final.str"));
    assert(str_layer_get_texture(&res.anim.layers[0], 0) == &texture);
    effect_resource_finish(&res); effect_resource_finish(&res);
    assert(!buffers && !outstanding && locks == unlocks && !res.res.ops);
}

static void test_owned_lifetimes(void)
{
    struct Res *owner;
    unsigned calls, failure;

    ground_file(9);
    allocation_calls = 0;
    owner = ground_resource_create();
    assert(owner && resource_load(owner, "owned.gnd"));
    calls = allocation_calls;
    assert(calls > 1 && buffers == 1);
    resource_destroy(owner);
    assert(!outstanding && !buffers);
    for (failure = 1; failure <= calls; ++failure) {
        allocation_calls = 0; fail_at = failure;
        owner = ground_resource_create();
        if (failure == 1) {
            assert(!owner);
        } else {
            assert(owner && !resource_load(owner, "failed.gnd"));
            assert_empty_ground((struct GndRes *)owner);
            assert(outstanding == 1);
            resource_destroy(owner);
        }
        assert(!outstanding && !buffers);
    }
    allocation_calls = 0; fail_at = 1;
    assert(!effect_resource_create() && !outstanding);
    fail_at = 0;
    effect_file();
    owner = effect_resource_create();
    assert(owner && resource_load(owner, "owned.str"));
    assert(str_layer_get_texture(&((struct EzEffectRes *)owner)->anim.layers[0], 0) == &texture);
    assert(locks == unlocks + 1);
    resource_destroy(owner);
    assert(!outstanding && !buffers && locks == unlocks);
    puts("resource lifetime: stack finish, consuming destruction and every ground/effect allocation failure passed");
}

int main(void)
{
    test_ground(); test_effect(); test_owned_lifetimes();
    puts("resource spans: truncation, count overflow, names, references, reload and texture ownership passed");
    return 0;
}
