#include "render/fonts.h"
#include "render/device.h"
#include "render/font_family.h"
#include "core/text_encoding.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

struct C3dDevice g_3dDevice;
static int startup_fails, create_fails, add_fails, dc_fails, alloc_fails;
static int family_fails, wide_fails, text_fails;
static int starts, stops, creates, deletes, adds, removes, dc_gets, dc_releases, enums, reads, allocs;
static int live_fonts, live_resources[2], active_dc;
static uintptr_t active_token;
static int alive[4096], resource_for_font[4096];
static LOGFONTA last_font;
static int last_indirect, last_name_null;
static const char *available[4];
static const wchar_t *family_name = L"Test Family";
static const char *family_text = "Test Family";
static void (*exit_callback)(void);

int font_backend_start(uintptr_t *token)
{
    ++starts;
    if (startup_fails)
        return 0;
    assert(!active_token);
    *token = active_token = 0x12345678;
    return 1;
}

void font_backend_stop(uintptr_t token)
{
    if (token) {
        assert(token == active_token && !live_fonts && !live_resources[0] && !live_resources[1]);
        active_token = 0;
        ++stops;
    }
}

int font_file_read_family(const wchar_t *path, wchar_t *name, size_t capacity)
{
    assert(active_token && capacity == LF_FACESIZE);
    assert(!wcscmp(path, L"system\\font\\SCDream4.otf") || !wcscmp(path, L"system\\font\\SCDream6.otf"));
    ++reads;
    if (family_fails)
        return 0;
    wcscpy(name, family_name);
    return 1;
}

wchar_t *encoding_to_wide(unsigned code_page, const char *text)
{
    wchar_t *result;
    size_t i, n = strlen(text) + 1;
    assert(code_page == CP_THREAD_ACP);
    if (wide_fails)
        return NULL;
    result = malloc(n * sizeof(*result));
    assert(result);
    for (i = 0; i < n; ++i)
        result[i] = (unsigned char)text[i];
    return result;
}

char *encoding_from_wide(unsigned code_page, const wchar_t *text)
{
    assert(code_page == CP_THREAD_ACP && !wcscmp(text, family_name));
    return text_fails ? NULL : _strdup(family_text);
}

void *font_cache_test_realloc(void *data, size_t size)
{
    ++allocs;
    return alloc_fails ? NULL : realloc(data, size);
}

HDC WINAPI font_cache_test_get_dc(HWND window)
{
    assert(!window && !active_dc);
    ++dc_gets;
    if (dc_fails)
        return NULL;
    active_dc = 1;
    return (HDC)(uintptr_t)0x234;
}

int WINAPI font_cache_test_release_dc(HWND window, HDC dc)
{
    assert(!window && active_dc && dc == (HDC)(uintptr_t)0x234);
    ++dc_releases;
    active_dc = 0;
    return 1;
}

int WINAPI font_cache_test_enum(HDC dc, LPLOGFONTA query, FONTENUMPROCA callback, LPARAM context, DWORD flags)
{
    int i, result = 0;
    assert(active_dc && dc == (HDC)(uintptr_t)0x234 && !flags);
    assert(!query->lfFaceName[0] && !query->lfPitchAndFamily);
    ++enums;
    for (i = 0; available[i]; ++i) {
        LOGFONTA font = {0};
        strcpy(font.lfFaceName, available[i]);
        result = callback(&font, NULL, TRUETYPE_FONTTYPE, context);
        if (!result)
            break;
    }
    return result;
}

static HFONT created(const LOGFONTA *font, int indirect, int null_name)
{
    ++creates;
    last_font = *font;
    last_indirect = indirect;
    last_name_null = null_name;
    assert(creates < (int)(sizeof(alive) / sizeof(alive[0])));
    if (create_fails)
        return NULL;
    alive[creates] = 1;
    resource_for_font[creates] = -1;
    ++live_fonts;
    return (HFONT)(uintptr_t)(0x1000 + creates);
}

HFONT WINAPI font_cache_test_create(int height, int width, int escapement, int orientation, int weight,
                                  DWORD italic, DWORD underline, DWORD strikeout, DWORD charset,
                                  DWORD output, DWORD clipping, DWORD quality, DWORD pitch, LPCSTR name)
{
    LOGFONTA font = {0};
    assert(!width && !escapement && !orientation && !underline && !strikeout && !output && !clipping && !pitch);
    assert(quality == NONANTIALIASED_QUALITY);
    font.lfHeight = height;
    font.lfWeight = weight;
    font.lfItalic = (BYTE)italic;
    font.lfCharSet = (BYTE)charset;
    if (name)
        strcpy(font.lfFaceName, name);
    return created(&font, 0, name == NULL);
}

HFONT WINAPI font_cache_test_create_indirect(const LOGFONTA *font)
{
    return created(font, 1, 0);
}

static int resource_index(const char *path)
{
    if (!strcmp(path, "system\\font\\SCDream4.otf"))
        return 0;
    assert(!strcmp(path, "system\\font\\SCDream6.otf"));
    return 1;
}

int WINAPI font_cache_test_add(LPCSTR path, DWORD flags, PVOID reserved)
{
    int index = resource_index(path);
    assert(flags == FR_PRIVATE && !reserved);
    ++adds;
    if (add_fails)
        return 0;
    ++live_resources[index];
    return 2;
}

BOOL WINAPI font_cache_test_remove(LPCSTR path, DWORD flags, PVOID reserved)
{
    int i, outstanding = 0, index = resource_index(path);
    assert(flags == FR_PRIVATE && !reserved && live_resources[index]);
    for (i = 1; i <= creates; ++i)
        outstanding += alive[i] && resource_for_font[i] == index;
    assert(live_resources[index] > outstanding);
    --live_resources[index];
    ++removes;
    return TRUE;
}

BOOL WINAPI font_cache_test_delete_object(HGDIOBJ object)
{
    int id = (int)(uintptr_t)object - 0x1000;
    assert(id > 0 && id <= creates && alive[id]);
    alive[id] = 0;
    --live_fonts;
    ++deletes;
    return TRUE;
}

int font_cache_test_atexit(void (*callback)(void))
{
    assert(!exit_callback);
    exit_callback = callback;
    return 0;
}

static void finish(struct FontCache *cache)
{
    int deleted, removed, stopped;
    font_cache_destruct(cache);
    assert(!cache->entries && !cache->count && !cache->capacity && !cache->token);
    assert(!live_fonts && !live_resources[0] && !live_resources[1] && !active_token && !active_dc);
    deleted = deletes; removed = removes; stopped = stops;
    font_cache_destruct(cache);
    assert(deletes == deleted && removes == removed && stops == stopped);
}

static void installed_fonts(void)
{
    struct FontCache cache;
    HFONT first;
    int c, a, d, e, i;
    font_cache_construct(&cache);
    available[0] = "unrelated"; available[1] = "PRIMARY";
    first = font_cache_get_font(&cache, 0, 13, 129, 0, 0);
    assert(first && !last_indirect && !strcmp(last_font.lfFaceName, "Primary"));
    assert(last_font.lfHeight == 13 && last_font.lfWeight == 400 && last_font.lfCharSet == 129 && !last_font.lfItalic);
    c = creates; a = allocs; d = dc_gets; e = enums;
    assert(font_cache_get_font(&cache, 0, 13, 129, 256, 256) == first);
    assert(creates == c && allocs == a && dc_gets == d && enums == e);
    assert(font_cache_get_font(&cache, 1, 13, 129, 0, 0) != first);
    assert(font_cache_get_font(&cache, 0, 14, 129, 0, 0) != first);
    assert(font_cache_get_font(&cache, 0, 13, 0, 0, 0) != first);
    assert(font_cache_get_font(&cache, 0, 13, 129, 1, 0) != first);
    assert(font_cache_get_font(&cache, 0, 13, 129, 0, 1) != first);
    assert(font_cache_get_font(&cache, 0, 13, 129, 0, 2) != first);
    assert(last_font.lfItalic == 1);
    available[1] = "fallback";
    assert(font_cache_get_font(&cache, 0, 15, 0, 0, 0));
    assert(!strcmp(last_font.lfFaceName, "Fallback"));
    available[1] = NULL;
    assert(font_cache_get_font(&cache, 0, 16, 0, 0, 0) && last_name_null);
    available[0] = NULL;
    assert(font_cache_get_font(&cache, 0, 17, 0, 0, 0) && last_name_null);
    g_3dDevice.faceNamesA[1] = g_3dDevice.faceNamesB[1] = NULL;
    e = enums;
    assert(font_cache_get_font(&cache, 1, 17, 0, 0, 0) && last_name_null && enums == e);
    d = dc_gets;
    assert(font_cache_get_font(&cache, -1, 17, 0, 0, 0) && last_name_null);
    assert(font_cache_get_font(&cache, 12, 17, 0, 0, 0) && last_name_null);
    assert(font_cache_get_font(&cache, 1000000, 17, 0, 0, 0) && last_name_null && dc_gets == d);
    dc_fails = 1;
    e = enums;
    assert(font_cache_get_font(&cache, 0, 18, 0, 0, 0) && last_name_null && enums == e);
    dc_fails = 0;
    for (i = 0; i < 500; ++i)
        assert(font_cache_get_font(&cache, 0, 100 + i, 0, i % 2, i % 3));
    assert(font_cache_get_font(&cache, 0, 13, 129, 0, 0) == first);
    finish(&cache);
}

static void eot_fonts(void)
{
    static const char *const names[] = {"RixSquirrel_10.eot", "RixLoveangel_10.eot", "RixNHCgogo_10.eot",
        "RixDiary_10.eot", "RixMiniHeart_10.eot", "RixFreshman_10.eot", "RixKid_10.eot", "RixMagic_10.eot", "RixJJangu_10.eot"};
    struct FontCache cache;
    int i, d = dc_gets, a = adds;
    font_cache_construct(&cache);
    for (i = 0; i < 9; ++i) {
        assert(font_cache_get_font(&cache, 20 + i, 99, 129, 1, 2));
        assert(last_indirect && !strcmp(last_font.lfFaceName, names[i]));
        assert(last_font.lfHeight == -13 && last_font.lfWeight == 400 && last_font.lfCharSet == 1);
        assert(!last_font.lfItalic && last_font.lfOutPrecision == 6 && last_font.lfQuality == 2);
    }
    assert(dc_gets == d && adds == a);
    finish(&cache);
}

static void file_fonts(void)
{
    struct FontCache cache;
    int mode, a, r, f;
    HFONT font;
    for (mode = 0; mode < 8; ++mode) {
        startup_fails = mode == 1; family_fails = mode == 2; wide_fails = mode == 3;
        text_fails = mode == 4; add_fails = mode == 5;
        family_text = mode == 6 ? "A very long family name that requires truncation at LF_FACESIZE" : "Test Family";
        font_cache_construct(&cache);
        a = adds; r = removes; f = reads;
        font = font_cache_get_font(&cache, 29, 19, 129, mode == 7 ? 2 : 1, 2);
        assert(font && adds == a + 1);
        assert(font_cache_get_font(&cache, 29, 19, 129, mode == 7 ? 2 : 1, 2) == font && adds == a + 1);
        assert(last_font.lfWeight == 700 && last_font.lfCharSet == 129);
        if (mode == 5) {
            assert(!last_indirect && last_name_null && last_font.lfHeight == 19 && last_font.lfItalic == 1);
        } else {
            assert(last_indirect && last_font.lfHeight == -19 && last_font.lfItalic == 2);
            assert(live_resources[mode == 7 ? 0 : 1] == 1);
            resource_for_font[(int)(uintptr_t)font - 0x1000] = mode == 7 ? 0 : 1;
            if (mode >= 1 && mode <= 4)
                assert(!last_font.lfFaceName[0]);
            else
                assert(!strncmp(last_font.lfFaceName, family_text, LF_FACESIZE - 1) && !last_font.lfFaceName[LF_FACESIZE - 1]);
        }
        if (startup_fails || wide_fails || add_fails)
            assert(reads == f);
        finish(&cache);
        assert(removes == r + (mode != 5));
    }
    startup_fails = family_fails = wide_fails = text_fails = add_fails = 0;
    family_text = "Test Family";
}

static void shared_file_resources(void)
{
    struct FontCache cache;
    int i, a = adds, r = removes;
    font_cache_construct(&cache);
    for (i = 0; i < 12; ++i) {
        HFONT font = font_cache_get_font(&cache, 29, 10 + i, 129, i % 2, 0);
        assert(font);
        resource_for_font[(int)(uintptr_t)font - 0x1000] = i % 2;
    }
    assert(live_resources[0] == 6 && live_resources[1] == 6);
    finish(&cache);
    assert(adds == a + 12 && removes == r + 12);
}

static void retry_failures(void)
{
    struct FontCache cache;
    HFONT old;
    size_t count, capacity;
    int c, a, r, face;
    font_cache_construct(&cache);
    alloc_fails = 1; c = creates; a = adds;
    assert(!font_cache_get_font(&cache, 29, 10, 0, 0, 0));
    assert(!cache.entries && !cache.count && !cache.capacity && creates == c && adds == a);
    alloc_fails = 0;
    old = font_cache_get_font(&cache, 0, 10, 0, 0, 0);
    assert(old);
    while (cache.count < cache.capacity)
        assert(font_cache_get_font(&cache, 0, 100 + (int)cache.count, 0, 0, 0));
    capacity = cache.capacity;
    alloc_fails = 1; c = creates;
    assert(!font_cache_get_font(&cache, 0, 4000, 0, 0, 0));
    assert(cache.count == capacity && cache.capacity == capacity && creates == c);
    assert(font_cache_get_font(&cache, 0, 10, 0, 0, 0) == old);
    alloc_fails = 0;
    assert(font_cache_get_font(&cache, 0, 4000, 0, 0, 0) && cache.capacity > capacity);
    for (face = 0; face <= 29; face += face == 0 ? 20 : 9) {
        count = cache.count; a = adds; r = removes;
        create_fails = 1;
        assert(!font_cache_get_font(&cache, face, 1234, 0, 1, 0) && cache.count == count);
        assert(adds == a + (face == 29) && removes == r + (face == 29));
        assert(!live_resources[0] && !live_resources[1]);
        create_fails = 0;
        assert(font_cache_get_font(&cache, face, 1234, 0, 1, 0) && cache.count == count + 1);
    }
    finish(&cache);
}

int main(void)
{
    struct { unsigned before[4]; struct FontCache cache; unsigned after[4]; } guarded;
    unsigned i;
    int stopped;
    g_3dDevice.faceNamesA[0] = g_3dDevice.faceNamesA[1] = "Primary";
    g_3dDevice.faceNamesB[0] = g_3dDevice.faceNamesB[1] = "Fallback";
    memset(&guarded, 0xa5, sizeof(guarded));
    assert(font_cache_construct(&guarded.cache) == &guarded.cache);
    finish(&guarded.cache);
    for (i = 0; i < 4; ++i)
        assert(guarded.before[i] == 0xa5a5a5a5 && guarded.after[i] == 0xa5a5a5a5);
    installed_fonts(); eot_fonts(); file_fonts(); shared_file_resources(); retry_failures();
    stopped = stops;
    gdiplus_init();
    assert(exit_callback == gdiplus_destroy && g_fontCache.token == active_token);
    exit_callback(); exit_callback();
    assert(stops == stopped + 1 && !g_fontCache.entries && !g_fontCache.count);
    assert(dc_gets == dc_releases + 1 && !active_dc);
    puts("Font cache: C storage, five-field keys, installed/EOT/file fonts, growth, failure retry and resource cleanup passed.");
    return 0;
}
