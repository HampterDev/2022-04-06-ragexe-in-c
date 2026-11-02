#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <windows.h>
#include "fileio/file.h"
#include "fileio/file_api.h"
#include "owned_storage_test_allocator.h"
#include "wave_format_fixtures.h"

static int fail_read;
static BYTE checked_read(struct File *file, void *out, int size)
{
    if (fail_read) {
        assert(file_read(file, out, size / 2));
        return 0;
    }
    return file_read(file, out, size);
}
static void *checked_calloc(size_t count, size_t size)
{
    void *p;
    assert(!size || count <= SIZE_MAX / size);
    p = checked_malloc(count * size);
    if (p) memset(p, 0, count * size);
    return p;
}
#define MSS_IMPORT
#define malloc checked_malloc
#define calloc checked_calloc
#define free checked_free
#define file_read checked_read
#include "../src/resources/wave.c"
#include "../src/resources/palette.c"
#undef file_read
#undef malloc
#undef calloc
#undef free

#define AIL_FILE_WAV 1

struct GameFileMgr g_fileMgr;
struct C3dDevice g_3dDevice;
struct Renderer *g_renderer;
void sprite_atlas_invalidate_palette(struct Renderer *renderer, const DWORD *palette)
{ assert(!renderer && palette); }

static BYTE *input;
static int input_size, file_present = 1, borrowed_buffers;
static int file_kind = AIL_FILE_WAV, info_failure, decode_failure, null_output, empty_output, bad_info;
static unsigned type_calls, info_calls, decode_calls, closed_buffers, sdk_frees;
static void *sdk_blocks[16];
static unsigned sdk_count;

struct Res *res_construct(struct Res *resource)
{ memset(resource, 0, sizeof *resource); resource->typeIndex = -1; return resource; }
void error_msg(const char *text, int flags) { (void)text; (void)flags; abort(); }
BYTE *game_file_mgr_get_data(struct GameFileMgr *mgr, const char *name, int *size, BYTE disk)
{
    assert(mgr == &g_fileMgr && name && !disk && !borrowed_buffers);
    if (!file_present) { *size = 0; return NULL; }
    *size = input_size; ++borrowed_buffers; return input;
}
void game_file_mgr_release_data(struct GameFileMgr *mgr, void *data)
{ assert(mgr == &g_fileMgr && data == input && borrowed_buffers == 1); --borrowed_buffers; ++closed_buffers; }
BYTE game_file_mgr_is_file_exist(struct GameFileMgr *mgr, const char *name, BYTE disk)
{ (void)mgr; (void)name; (void)disk; return (BYTE)file_present; }

int __stdcall _AIL_file_type(const void *data, unsigned size)
{
    assert(data && size == (unsigned)input_size && !memcmp(data, input, size));
    ++type_calls; return file_kind;
}
int __stdcall _AIL_WAV_info(const void *data, struct AilSoundInfo *info)
{
    ++info_calls;
    if (info_failure) return 0;
    assert(input_size == sizeof ima_wave);
    info->data = (const BYTE *)data + 60; info->dataLen = 8;
    info->format = 17; info->bits = 4; info->channels = 1; info->rate = 8000;
    info->samples = 9; info->blockSize = 8; info->initial = data;
    if (bad_info == 1) info->data = (const void *)((uintptr_t)data - 1);
    if (bad_info == 2) info->dataLen = UINT_MAX;
    if (bad_info == 3) info->initial = (const BYTE *)data + input_size;
    if (bad_info == 4) info->channels = 0;
    if (bad_info == 5) info->samples = UINT_MAX;
    if (bad_info == 6) info->blockSize = 0;
    return 1;
}
static int decode(void **out, unsigned *size)
{
    ++decode_calls; assert(out && !*out && size && !*size);
    if (null_output) return !decode_failure;
    *out = checked_malloc(16);
    if (!*out) return 0;
    memset(*out, 0x39, 16); assert(sdk_count < 16); sdk_blocks[sdk_count++] = *out;
    *size = empty_output ? 0 : 16;
    return !decode_failure;
}
int __stdcall _AIL_decompress_ADPCM(const struct AilSoundInfo *info, void **out, unsigned *size)
{ assert(info && info->dataLen == 8); return decode(out, size); }
int __stdcall _AIL_decompress_ASI(const void *data, unsigned size, const char *name,
    void **out, unsigned *out_size, void *callback)
{ assert(data && size == (unsigned)input_size && name && !callback); return decode(out, out_size); }
void __stdcall _AIL_mem_free_lock(void *p)
{
    unsigned i;
    assert(p);
    for (i = 0; i < sdk_count && sdk_blocks[i] != p; ++i) {}
    assert(i < sdk_count);
    sdk_blocks[i] = sdk_blocks[--sdk_count]; ++sdk_frees; checked_free(p);
}

static void set_input(const void *data, int size)
{ assert(!borrowed_buffers); input = (BYTE *)data; input_size = size; }
static void require_clean(void) { assert(!borrowed_buffers && !outstanding && !sdk_count); }

static void test_wave_replacement(void)
{
    struct Res *base = wave_resource_create();
    struct Wave *wave = (struct Wave *)base;
    void *old;
    unsigned before;
    int failure, kind;
    assert(base && base->ops == &wave_resource_ops && !wave->data && !wave->milesOwned);
    set_input(pcm_wave, sizeof pcm_wave);
    assert(resource_load(base, "test.wav"));
    assert(!wave->milesOwned && !memcmp(wave->data, pcm_wave, sizeof pcm_wave));
    old = wave->data; file_present = 0;
    assert(!resource_load(base, "missing.wav") && wave->data == old);
    file_present = 1; fail_read = 1;
    assert(!resource_load(base, "test.wav") && wave->data == old);
    fail_read = 0; assert(!resource_load(base, NULL) && wave->data == old);
    for (kind = 0; kind < 2; ++kind) {
        file_kind = kind ? AIL_FILE_ASI : AIL_FILE_ADPCM;
        set_input(ima_wave, sizeof ima_wave);
        for (failure = 0; failure < 3; ++failure) {
            decode_failure = failure == 0; null_output = failure == 1; empty_output = failure == 2;
            assert(!resource_load(base, "test.wav") && wave->data == old && !sdk_count);
            assert(outstanding == 2 && !borrowed_buffers);
        }
        decode_failure = null_output = empty_output = 0;
    }
    file_kind = AIL_FILE_ADPCM;
    info_failure = 1; before = decode_calls;
    assert(!resource_load(base, "test.wav") && wave->data == old && decode_calls == before);
    info_failure = 0;
    for (bad_info = 1; bad_info <= 6; ++bad_info)
        assert(!resource_load(base, "test.wav") && wave->data == old && decode_calls == before);
    bad_info = 0;
    assert(resource_load(base, "test.wav") && wave->milesOwned && sdk_count == 1 && outstanding == 2);
    old = wave->data; decode_failure = 1;
    assert(!resource_load(base, "test.wav") && wave->data == old && sdk_count == 1 && outstanding == 2);
    decode_failure = 0;
    assert(resource_load(base, "test.wav") && wave->data != old && sdk_count == 1);
    wave_resource_reset(wave); wave_resource_reset(wave);
    assert(!wave->data && !wave->milesOwned && !sdk_count && outstanding == 1);
    file_kind = AIL_FILE_WAV; set_input(pcm_wave, sizeof pcm_wave);
    assert(resource_load(base, "test.wav"));
    resource_destroy(base); require_clean();
}

static DWORD expected_color(const BYTE *rgb)
{
    if (rgb[0] >= 254 && rgb[1] < 4 && rgb[2] >= 254) return g_3dDevice.colorKeyPixel;
    return 0x8000u | (DWORD)(rgb[0] >> 3) << 10 | (DWORD)(rgb[1] >> 3) << 5 | rgb[2] >> 3;
}
static void palette_matches(struct PaletteRes *palette, const BYTE *raw)
{
    unsigned i;
    assert(!memcmp(palette->raw, raw, sizeof palette->raw));
    for (i = 0; i < 256; ++i) assert(palette->palette[i] == expected_color(raw + i * 4));
}
static void test_palette(void)
{
    BYTE raw[1028]; struct PaletteRes saved;
    struct Res *base = palette_resource_create();
    struct PaletteRes *palette = (struct PaletteRes *)base;
    DWORD fallback[256]; unsigned i;
    assert(base && base->ops == &palette_resource_ops);
    for (i = 0; i < 256; ++i) assert(!palette->palette[i]);
    for (i = 0; i < sizeof raw; ++i) raw[i] = (BYTE)(i * 31 + i / 4);
    raw[0] = raw[2] = 254; raw[1] = 3;
    raw[4] = raw[6] = 255; raw[5] = 4;
    set_input(raw, sizeof raw);
    assert(resource_load(base, "test.pal")); palette_matches(palette, raw);
    assert(palette_resource_colors(palette, fallback) == palette->palette);
    assert(palette_resource_colors(NULL, fallback) == fallback && !palette_resource_colors(NULL, NULL));
    saved = *palette; memset(raw, 0xff, sizeof raw); fail_read = 1;
    assert(!resource_load(base, "test.pal") && !memcmp(palette, &saved, sizeof saved));
    fail_read = 0; file_present = 0;
    assert(!resource_load(base, "missing.pal") && !memcmp(palette, &saved, sizeof saved));
    file_present = 1; assert(!resource_load(base, NULL) && !memcmp(palette, &saved, sizeof saved));
    assert(resource_load(base, "new.pal")); palette_matches(palette, raw);
    resource_destroy(base); require_clean();
}

static unsigned test_guarded_prefixes(const BYTE *data, unsigned size, int palette)
{
    SYSTEM_INFO sys; DWORD old; BYTE *pages, *guard, *at;
    struct Res *resource = palette ? palette_resource_create() : wave_resource_create();
    unsigned n, before = type_calls, info_before = info_calls;
    GetSystemInfo(&sys); assert(size < sys.dwPageSize);
    pages = VirtualAlloc(NULL, sys.dwPageSize * 2, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE); assert(pages);
    guard = pages + sys.dwPageSize; assert(VirtualProtect(guard, sys.dwPageSize, PAGE_NOACCESS, &old));
    for (n = 0; n < size; ++n) {
        at = guard - n; memcpy(at, data, n); set_input(at, n);
        if (!palette && n >= 12 && file_kind == AIL_FILE_WAV) {
            assert(resource_load(resource, "test.wav") && !borrowed_buffers);
            assert(!memcmp(((struct Wave *)resource)->data, data, n) && type_calls == ++before);
            continue;
        }
        assert(!resource_load(resource, palette ? "test.pal" : "test.wav"));
        assert(!borrowed_buffers && outstanding == 1 && info_calls == info_before);
        if (n < 12) assert(type_calls == before);
        else before = type_calls;
    }
    set_input(pages, -1); assert(!resource_load(resource, "negative"));
    resource_destroy(resource); assert(VirtualFree(pages, 0, MEM_RELEASE)); require_clean();
    return size;
}

static void test_malformed_chunks(void)
{
    BYTE data[sizeof pcm_wave > sizeof ima_wave ? sizeof pcm_wave : sizeof ima_wave];
    struct Res *resource = wave_resource_create();
    unsigned before = info_calls;
    const unsigned offsets[] = {4, 16, 56}; unsigned i;
    file_kind = AIL_FILE_ADPCM;
    for (i = 0; i < sizeof offsets / sizeof *offsets; ++i) {
        memcpy(data, ima_wave, sizeof ima_wave); memset(data + offsets[i], 255, 4);
        set_input(data, sizeof ima_wave); assert(!resource_load(resource, "bad.wav"));
    }
    memcpy(data, ima_wave, sizeof ima_wave); data[36] = 255;
    set_input(data, sizeof ima_wave); assert(!resource_load(resource, "bad.wav"));
    assert(info_calls == before);
    file_kind = AIL_FILE_WAV;
    for (i = 0; i < 4; ++i) {
        memcpy(data, pcm_wave, sizeof pcm_wave);
        if (i == 0) memset(data + 4, 255, 4);
        if (i == 1) data[4] = (BYTE)(sizeof pcm_wave);
        if (i == 2) memset(data + 40, 0, 4);
        if (i == 3) memset(data + 40, 255, 4);
        set_input(data, sizeof pcm_wave); assert(resource_load(resource, "sloppy.wav"));
        assert(!memcmp(((struct Wave *)resource)->data, data, sizeof pcm_wave));
    }
    resource_destroy(resource); require_clean();
}

static void test_allocation_failures(void)
{
    unsigned point; int kind;
    fail_at = allocation_calls + 1; assert(!wave_resource_create()); require_clean();
    fail_at = allocation_calls + 1; assert(!palette_resource_create()); require_clean();
    fail_at = 0;
    for (kind = 0; kind < 3; ++kind) {
        for (point = 1; point <= (kind ? 2u : 1u); ++point) {
            struct Res *base = wave_resource_create(); struct Wave *wave = (struct Wave *)base; void *old;
            file_kind = AIL_FILE_WAV; set_input(pcm_wave, sizeof pcm_wave);
            assert(resource_load(base, "old.wav")); old = wave->data;
            file_kind = kind == 0 ? AIL_FILE_WAV : kind == 1 ? AIL_FILE_ADPCM : AIL_FILE_ASI;
            if (kind) set_input(ima_wave, sizeof ima_wave);
            fail_at = allocation_calls + point;
            assert(!resource_load(base, "replacement.wav") && wave->data == old && !wave->milesOwned);
            assert(outstanding == 2 && !borrowed_buffers && !sdk_count);
            fail_at = 0; resource_destroy(base); require_clean();
        }
    }
}

static int corpus(const char *name)
{
    FILE *file = fopen(name, "rb"); unsigned counts[3] = {0}; uint32_t header[2];
    assert(file);
    while (fread(header, sizeof header, 1, file) == 1) {
        BYTE *data = malloc(header[1]); struct Res *base;
        assert(header[0] == 1 || header[0] == 2); assert(data && header[1] <= INT_MAX);
        assert(fread(data, header[1], 1, file) == 1); set_input(data, (int)header[1]);
        base = header[0] == 1 ? palette_resource_create() : wave_resource_create(); assert(base);
        file_kind = AIL_FILE_WAV; assert(resource_load(base, header[0] == 1 ? "asset.pal" : "asset.wav"));
        if (header[0] == 1) palette_matches((struct PaletteRes *)base, data);
        else assert(!memcmp(((struct Wave *)base)->data, data, header[1]));
        ++counts[header[0]]; resource_destroy(base); free(data); require_clean();
    }
    assert(feof(file)); fclose(file);
    printf("Client assets: %u palettes preserve source and converted colors; %u PCM sounds preserve every byte\n", counts[1], counts[2]);
    return 0;
}

int main(int argc, char **argv)
{
    BYTE palette[1024] = {0}; unsigned prefixes;
    g_3dDevice.colorKeyPixel = 0x12345678;
    if (argc == 2) return corpus(argv[1]);
    test_wave_replacement(); test_palette();
    prefixes = test_guarded_prefixes(pcm_wave, sizeof pcm_wave, 0);
    prefixes += test_guarded_prefixes(ima_wave, sizeof ima_wave, 0);
    file_kind = AIL_FILE_ADPCM; prefixes += test_guarded_prefixes(ima_wave, sizeof ima_wave, 0);
    file_kind = AIL_FILE_WAV;
    prefixes += test_guarded_prefixes(palette, sizeof palette, 1);
    test_malformed_chunks(); test_allocation_failures(); require_clean();
    assert(closed_buffers && info_calls && decode_calls && sdk_frees);
    printf("Wave/palette ownership: transactional replacement, real CFile/conversion, %u guarded prefixes, SDK failures, allocator pairing and every owner allocation failure passed\n", prefixes);
    return 0;
}
