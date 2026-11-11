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

struct GameFileMgr g_fileMgr;
struct C3dDevice g_3dDevice;
struct Renderer *g_renderer;
void sprite_atlas_invalidate_palette(struct Renderer *renderer, const DWORD *palette)
{ assert(!renderer && palette); }

static BYTE *input;
static int input_size, file_present = 1, borrowed_buffers, decode_failure;
static unsigned decode_calls, closed_buffers, sound_frees;
static void *sounds[16];
static unsigned sound_count;

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

/* The decoder (platform/audio_sdl.c, tested by test_audio) sees the file's own bytes while the
   file is open; a sound is an allocation the wave owns. */
struct AudioSound *audio_sound_decode(const void *data, size_t size)
{
    void *sound;
    assert(data == input && size == (size_t)input_size && borrowed_buffers == 1);
    ++decode_calls;
    if (decode_failure)
        return NULL;
    sound = checked_malloc(16);
    if (!sound)
        return NULL;
    assert(sound_count < 16); sounds[sound_count++] = sound;
    return sound;
}
void audio_sound_free(struct AudioSound *sound)
{
    unsigned i;
    if (!sound)
        return;
    for (i = 0; i < sound_count && sounds[i] != (void *)sound; ++i) {}
    assert(i < sound_count);
    sounds[i] = sounds[--sound_count]; ++sound_frees; checked_free(sound);
}

static void set_input(const void *data, int size)
{ assert(!borrowed_buffers); input = (BYTE *)data; input_size = size; }
static void require_clean(void) { assert(!borrowed_buffers && !outstanding && !sound_count); }

static void test_wave_replacement(void)
{
    struct Res *base = wave_resource_create();
    struct Wave *wave = (struct Wave *)base;
    void *old;
    unsigned before;
    assert(base && base->ops == &wave_resource_ops && !wave->sound);
    set_input(pcm_wave, sizeof pcm_wave);
    assert(resource_load(base, "test.wav") && wave->sound && sound_count == 1 && outstanding == 2);
    old = wave->sound; file_present = 0; before = decode_calls;
    assert(!resource_load(base, "missing.wav") && wave->sound == old && decode_calls == before);
    file_present = 1; assert(!resource_load(base, NULL) && wave->sound == old && decode_calls == before);
    set_input(ima_wave, sizeof ima_wave);
    assert(resource_load(base, "test.wav") && wave->sound && sound_count == 1 && outstanding == 2);
    /* a file that reads but does not decode loads without a sound, so it plays nothing */
    decode_failure = 1;
    assert(resource_load(base, "broken.wav") && !wave->sound && !sound_count && outstanding == 1);
    decode_failure = 0;
    assert(resource_load(base, "test.wav") && wave->sound);
    wave_resource_reset(wave); wave_resource_reset(wave);
    assert(!wave->sound && !sound_count && outstanding == 1);
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
    assert(resource_load(base, "test.pal") && !memcmp(palette, &saved, sizeof saved));
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
    unsigned n, before = decode_calls;
    GetSystemInfo(&sys); assert(size < sys.dwPageSize);
    pages = VirtualAlloc(NULL, sys.dwPageSize * 2, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE); assert(pages);
    guard = pages + sys.dwPageSize; assert(VirtualProtect(guard, sys.dwPageSize, PAGE_NOACCESS, &old));
    for (n = 0; n < size; ++n) {
        at = guard - n; memcpy(at, data, n); set_input(at, n);
        if (palette) {
            static const BYTE zeros[1024];
            assert(resource_load(resource, "test.pal") && !borrowed_buffers);
            palette_matches((struct PaletteRes *)resource, zeros);
            continue;
        }
        if (n >= 12) {
            assert(resource_load(resource, "test.wav") && !borrowed_buffers && decode_calls == ++before);
            continue;
        }
        assert(!resource_load(resource, "test.wav"));
        assert(!borrowed_buffers && decode_calls == before);
    }
    set_input(pages, -1); assert(!resource_load(resource, "negative"));
    resource_destroy(resource); assert(VirtualFree(pages, 0, MEM_RELEASE)); require_clean();
    return size;
}

static void test_allocation_failures(void)
{
    struct Res *base;
    struct Wave *wave;
    fail_at = allocation_calls + 1; assert(!wave_resource_create()); require_clean();
    fail_at = allocation_calls + 1; assert(!palette_resource_create()); require_clean();
    fail_at = 0;
    base = wave_resource_create(); wave = (struct Wave *)base;
    set_input(pcm_wave, sizeof pcm_wave);
    assert(resource_load(base, "old.wav") && wave->sound);
    fail_at = allocation_calls + 1;
    assert(resource_load(base, "replacement.wav") && !wave->sound);
    assert(outstanding == 1 && !borrowed_buffers && !sound_count);
    fail_at = 0; resource_destroy(base); require_clean();
}

static int corpus(const char *name)
{
    FILE *file = fopen(name, "rb"); unsigned counts[3] = {0}; uint32_t header[2];
    assert(file);
    while (fread(header, sizeof header, 1, file) == 1) {
        BYTE *data = malloc(header[1]); struct Res *base; unsigned before = decode_calls;
        assert(header[0] == 1 || header[0] == 2); assert(data && header[1] <= INT_MAX);
        assert(fread(data, header[1], 1, file) == 1); set_input(data, (int)header[1]);
        base = header[0] == 1 ? palette_resource_create() : wave_resource_create(); assert(base);
        assert(resource_load(base, header[0] == 1 ? "asset.pal" : "asset.wav"));
        if (header[0] == 1) palette_matches((struct PaletteRes *)base, data);
        else assert(decode_calls == before + (header[1] >= 12));
        ++counts[header[0]]; resource_destroy(base); free(data); require_clean();
    }
    assert(feof(file)); fclose(file);
    printf("Client assets: %u palettes preserve source and converted colors; %u sounds reach the decoder whole\n", counts[1], counts[2]);
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
    prefixes += test_guarded_prefixes(palette, sizeof palette, 1);
    test_allocation_failures(); require_clean();
    assert(closed_buffers && decode_calls && sound_frees);
    printf("Wave/palette ownership: transactional replacement, real CFile/conversion, %u guarded prefixes, decode failures, allocator pairing and every owner allocation failure passed\n", prefixes);
    return 0;
}
