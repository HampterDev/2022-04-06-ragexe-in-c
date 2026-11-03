#include "render/fonts.h"
#include "render/device.h"
#include "render/font_family.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define TEST_DC   ((HDC)(uintptr_t)0x345)
#define DC_FONT   ((HGDIOBJ)(uintptr_t)0x777)
#define GULIM_KR  L"\xad74\xb9bc"

struct C3dDevice g_3dDevice;
static int startup_fails, create_fails, add_fails, dc_fails, alloc_fails, family_fails;
static int select_fails, glyph_fails, metric_fails;
static int starts, stops, creates, deletes, adds, removes, dc_gets, dc_releases, enums, reads, allocs;
static int selects, glyph_calls, metric_calls;
static int live_fonts, live_resources[2], active_dc;
static uintptr_t active_token;
static int alive[4096], resource_for_font[4096];
static LOGFONTW fonts[4096], last_font;
static int last_indirect, last_name_null, last_enum_charset;
static const wchar_t *available[6];
static const wchar_t *family_name = L"Test Family";
static HGDIOBJ selected = DC_FONT;
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

/* Like the GDI+ backend: the name is written only when it fits. */
int font_file_read_family(const wchar_t *path, wchar_t *name, size_t capacity)
{
    assert(active_token && capacity == LF_FACESIZE);
    assert(!wcscmp(path, L"system\\font\\SCDream4.otf") || !wcscmp(path, L"system\\font\\SCDream6.otf"));
    ++reads;
    if (family_fails || wcslen(family_name) + 1 > capacity)
        return 0;
    wcscpy(name, family_name);
    return 1;
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

int WINAPI font_cache_test_enum(HDC dc, LPLOGFONTW query, FONTENUMPROCW callback, LPARAM context, DWORD flags)
{
    int i, result = 0;
    assert(active_dc && dc == (HDC)(uintptr_t)0x234 && !flags);
    assert(!query->lfFaceName[0] && !query->lfPitchAndFamily);
    ++enums;
    last_enum_charset = query->lfCharSet;
    for (i = 0; available[i]; ++i) {
        LOGFONTW font = {0};
        wcscpy(font.lfFaceName, available[i]);
        result = callback(&font, NULL, TRUETYPE_FONTTYPE, context);
        if (!result)
            break;
    }
    return result;
}

static HFONT created(const LOGFONTW *font, int indirect, int null_name)
{
    ++creates;
    last_font = *font;
    last_indirect = indirect;
    last_name_null = null_name;
    assert(creates < (int)(sizeof(alive) / sizeof(alive[0])));
    if (create_fails)
        return NULL;
    fonts[creates] = *font;
    alive[creates] = 1;
    resource_for_font[creates] = -1;
    ++live_fonts;
    return (HFONT)(uintptr_t)(0x1000 + creates);
}

HFONT WINAPI font_cache_test_create(int height, int width, int escapement, int orientation, int weight,
                                  DWORD italic, DWORD underline, DWORD strikeout, DWORD charset,
                                  DWORD output, DWORD clipping, DWORD quality, DWORD pitch, LPCWSTR name)
{
    LOGFONTW font = {0};
    assert(!width && !escapement && !orientation && !underline && !strikeout && !output && !clipping && !pitch);
    assert(quality == NONANTIALIASED_QUALITY);
    font.lfHeight = height;
    font.lfWeight = weight;
    font.lfItalic = (BYTE)italic;
    font.lfCharSet = (BYTE)charset;
    if (name)
        wcscpy(font.lfFaceName, name);
    return created(&font, 0, name == NULL);
}

HFONT WINAPI font_cache_test_create_indirect(const LOGFONTW *font)
{
    return created(font, 1, 0);
}

static int resource_index(const wchar_t *path)
{
    if (!wcscmp(path, L"system\\font\\SCDream4.otf"))
        return 0;
    assert(!wcscmp(path, L"system\\font\\SCDream6.otf"));
    return 1;
}

int WINAPI font_cache_test_add(LPCWSTR path, DWORD flags, PVOID reserved)
{
    int index = resource_index(path);
    assert(flags == FR_PRIVATE && !reserved);
    ++adds;
    if (add_fails)
        return 0;
    ++live_resources[index];
    return 2;
}

BOOL WINAPI font_cache_test_remove(LPCWSTR path, DWORD flags, PVOID reserved)
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

HGDIOBJ WINAPI font_cache_test_select(HDC dc, HGDIOBJ object)
{
    HGDIOBJ previous = selected;
    assert(dc == TEST_DC);
    ++selects;
    if (select_fails)
        return NULL;
    selected = object;
    return previous;
}

static const LOGFONTW *selected_font(void)
{
    int id = (int)(uintptr_t)selected - 0x1000;
    assert(id > 0 && id <= creates && alive[id]);
    return &fonts[id];
}

/* Gulim and Malgun Gothic have every sample but Thai; "Primary" has only Thai. */
DWORD WINAPI font_cache_test_glyphs(HDC dc, LPCWSTR text, int count, LPWORD glyphs, DWORD flags)
{
    const wchar_t *name = selected_font()->lfFaceName;
    int korean = !_wcsicmp(name, L"Gulim") || !wcscmp(name, GULIM_KR) || !_wcsicmp(name, L"Malgun Gothic");
    assert(dc == TEST_DC && count == 1 && flags == GGI_MARK_NONEXISTING_GLYPHS);
    ++glyph_calls;
    if (glyph_fails)
        return GDI_ERROR;
    glyphs[0] = (text[0] == 0x0e01 ? !wcscmp(name, L"Primary") : korean) ? 7 : 0xffff;
    return 1;
}

BOOL WINAPI font_cache_test_metrics(HDC dc, LPTEXTMETRICW metric)
{
    assert(dc == TEST_DC);
    ++metric_calls;
    if (metric_fails)
        return FALSE;
    memset(metric, 0, sizeof(*metric));
    metric->tmAscent = abs(selected_font()->lfHeight) * 4 / 5;
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
    int deleted, removed, stopped, i;
    font_cache_destruct(cache);
    assert(!cache->entries && !cache->count && !cache->capacity && !cache->token);
    for (i = 0; i < TEXT_SCRIPT_COUNT; ++i)
        assert(!cache->fallback[i]);
    assert(!live_fonts && !live_resources[0] && !live_resources[1] && !active_token && !active_dc);
    assert(selected == DC_FONT);
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
    available[0] = L"unrelated"; available[1] = L"PRIMARY"; available[2] = NULL;
    first = font_cache_get_font(&cache, 0, 13, 129, 0, 0);
    assert(first && !last_indirect && !wcscmp(last_font.lfFaceName, L"Primary") && last_enum_charset == 129);
    assert(last_font.lfHeight == 13 && last_font.lfWeight == 400 && last_font.lfCharSet == 129 && !last_font.lfItalic);
    c = creates; a = allocs; d = dc_gets; e = enums;
    assert(font_cache_get_font(&cache, 0, 13, 129, 256, 256) == first);
    assert(creates == c && allocs == a && dc_gets == d && enums == e);
    assert(font_cache_get_font(&cache, 1, 13, 129, 0, 0) != first);
    assert(font_cache_get_font(&cache, 0, 14, 129, 0, 0) != first);
    assert(font_cache_get_font(&cache, 0, 13, 0, 0, 0) != first && last_enum_charset == 0);
    assert(font_cache_get_font(&cache, 0, 13, 129, 1, 0) != first);
    assert(font_cache_get_font(&cache, 0, 13, 129, 0, 1) != first);
    assert(font_cache_get_font(&cache, 0, 13, 129, 0, 2) != first);
    assert(last_font.lfItalic == 1);
    available[1] = L"fallback";
    assert(font_cache_get_font(&cache, 0, 15, 0, 0, 0));
    assert(!wcscmp(last_font.lfFaceName, L"Fallback"));
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
    static const wchar_t *const names[] = {L"RixSquirrel_10.eot", L"RixLoveangel_10.eot", L"RixNHCgogo_10.eot",
        L"RixDiary_10.eot", L"RixMiniHeart_10.eot", L"RixFreshman_10.eot", L"RixKid_10.eot", L"RixMagic_10.eot",
        L"RixJJangu_10.eot"};
    struct FontCache cache;
    int i, d = dc_gets, a = adds;
    font_cache_construct(&cache);
    for (i = 0; i < 9; ++i) {
        assert(font_cache_get_font(&cache, 20 + i, 99, 129, 1, 2));
        assert(last_indirect && !wcscmp(last_font.lfFaceName, names[i]));
        assert(last_font.lfHeight == -13 && last_font.lfWeight == 400 && last_font.lfCharSet == 1);
        assert(!last_font.lfItalic && last_font.lfOutPrecision == 6 && last_font.lfQuality == 2);
    }
    assert(dc_gets == d && adds == a);
    finish(&cache);
}

/* Modes: 0 reads the family, 1 has no GDI+, 2 fails the read, 3 reads a family too long
   for LOGFONT, 4 fails to add the file, 5 uses the regular file. */
static void file_fonts(void)
{
    struct FontCache cache;
    int mode, a, r, f;
    HFONT font;
    for (mode = 0; mode < 6; ++mode) {
        startup_fails = mode == 1; family_fails = mode == 2; add_fails = mode == 4;
        family_name = mode == 3 ? L"A family name that does not fit in LF_FACESIZE characters" : L"Test Family";
        font_cache_construct(&cache);
        a = adds; r = removes; f = reads;
        font = font_cache_get_font(&cache, 29, 19, 129, mode == 5 ? 2 : 1, 2);
        assert(font && adds == a + 1);
        assert(font_cache_get_font(&cache, 29, 19, 129, mode == 5 ? 2 : 1, 2) == font && adds == a + 1);
        assert(last_font.lfWeight == 700 && last_font.lfCharSet == 129);
        if (mode == 4) {
            assert(!last_indirect && last_name_null && last_font.lfHeight == 19 && last_font.lfItalic == 1);
        } else {
            assert(last_indirect && last_font.lfHeight == -19 && last_font.lfItalic == 2);
            assert(live_resources[mode == 5 ? 0 : 1] == 1);
            resource_for_font[(int)(uintptr_t)font - 0x1000] = mode == 5 ? 0 : 1;
            if (mode >= 1 && mode <= 3)
                assert(!last_font.lfFaceName[0]);
            else
                assert(!wcscmp(last_font.lfFaceName, L"Test Family"));
        }
        assert(reads == f + (!startup_fails && !add_fails));
        finish(&cache);
        assert(removes == r + (mode != 4));
    }
    startup_fails = family_fails = add_fails = 0;
    family_name = L"Test Family";
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

static void script_fonts(void)
{
    struct FontCache cache;
    HFONT primary, hangul;
    int c, e, s, g, m;
    font_cache_construct(&cache);
    g_3dDevice.faceNamesA[0] = L"Primary";
    available[0] = L"Primary"; available[1] = L"malgun gothic"; available[2] = NULL;
    primary = font_cache_get_font(&cache, 0, 13, 129, 0, 0);
    assert(primary && !wcscmp(last_font.lfFaceName, L"Primary"));
    /* The face lacks Hangul; finding out selects it once and restores the DC's font. */
    s = selects; g = glyph_calls;
    assert(!font_cache_covers(&cache, TEST_DC, primary, TEXT_SCRIPT_HANGUL));
    assert(selects == s + 2 && glyph_calls == g + 1 && selected == DC_FONT);
    assert(!font_cache_covers(&cache, TEST_DC, primary, TEXT_SCRIPT_HANGUL) && glyph_calls == g + 1);
    assert(font_cache_covers(&cache, TEST_DC, primary, TEXT_SCRIPT_THAI) && glyph_calls == g + 2);
    /* The default script, unknown fonts and bad scripts draw in the face; emoji never do. */
    assert(font_cache_covers(&cache, TEST_DC, primary, TEXT_SCRIPT_DEFAULT));
    assert(font_cache_covers(&cache, TEST_DC, primary, TEXT_SCRIPT_COUNT));
    assert(font_cache_covers(&cache, TEST_DC, (HFONT)(uintptr_t)0x999, TEXT_SCRIPT_HANGUL));
    assert(!font_cache_covers(&cache, TEST_DC, primary, TEXT_SCRIPT_EMOJI) && glyph_calls == g + 2);
    /* Without Gulim, Hangul falls back to Malgun Gothic, matched without case. */
    c = creates; e = enums;
    hangul = font_cache_get_script_font(&cache, TEXT_SCRIPT_HANGUL, 13, 1, 1);
    assert(hangul && hangul != primary && creates == c + 1 && !last_indirect);
    assert(!wcscmp(last_font.lfFaceName, L"Malgun Gothic") && last_font.lfCharSet == DEFAULT_CHARSET);
    assert(last_font.lfHeight == 13 && last_font.lfWeight == 700 && last_font.lfItalic == 1);
    assert(last_enum_charset == DEFAULT_CHARSET && enums == e + 3);
    assert(font_cache_get_script_font(&cache, TEXT_SCRIPT_HANGUL, 13, 1, 1) == hangul);
    assert(creates == c + 1 && enums == e + 3);
    assert(font_cache_get_script_font(&cache, TEXT_SCRIPT_HANGUL, 14, 1, 1) != hangul && enums == e + 3);
    assert(font_cache_get_font(&cache, FONT_FACE_FALLBACK, 13, DEFAULT_CHARSET, 1, 1) != hangul);
    assert(font_cache_covers(&cache, TEST_DC, hangul, TEXT_SCRIPT_HANGUL));
    /* Thai has no installed fallback, which is looked up once. */
    e = enums;
    assert(!font_cache_get_script_font(&cache, TEXT_SCRIPT_THAI, 13, 0, 0) && enums == e + 2);
    assert(!font_cache_get_script_font(&cache, TEXT_SCRIPT_THAI, 13, 0, 0) && enums == e + 2);
    assert(!font_cache_get_script_font(&cache, TEXT_SCRIPT_DEFAULT, 13, 0, 0));
    assert(!font_cache_get_script_font(&cache, TEXT_SCRIPT_COUNT, 13, 0, 0) && enums == e + 2);
    /* Ascents are measured once per font. */
    m = metric_calls;
    assert(font_cache_ascent(&cache, TEST_DC, primary) == 10 && selected == DC_FONT && metric_calls == m + 1);
    assert(font_cache_ascent(&cache, TEST_DC, primary) == 10 && metric_calls == m + 1);
    assert(!font_cache_ascent(&cache, TEST_DC, (HFONT)(uintptr_t)0x999) && metric_calls == m + 1);
    finish(&cache);

    /* Gulim comes first, here under its Korean name; symbols use it too. */
    font_cache_construct(&cache);
    available[0] = L"Malgun Gothic"; available[1] = GULIM_KR; available[2] = L"Segoe UI Emoji"; available[3] = NULL;
    assert(font_cache_get_script_font(&cache, TEXT_SCRIPT_HANGUL, 13, 0, 0) && !wcscmp(last_font.lfFaceName, GULIM_KR));
    assert(font_cache_get_script_font(&cache, TEXT_SCRIPT_SYMBOL, 13, 0, 0) && !wcscmp(last_font.lfFaceName, GULIM_KR));
    assert(font_cache_get_script_font(&cache, TEXT_SCRIPT_HAN, 13, 0, 0) && !wcscmp(last_font.lfFaceName, GULIM_KR));
    assert(font_cache_get_script_font(&cache, TEXT_SCRIPT_KANA, 13, 0, 0) && !wcscmp(last_font.lfFaceName, GULIM_KR));
    assert(font_cache_get_script_font(&cache, TEXT_SCRIPT_EMOJI, 13, 0, 0));
    assert(!wcscmp(last_font.lfFaceName, L"Segoe UI Emoji"));
    finish(&cache);

    /* Failures are retried. */
    font_cache_construct(&cache);
    available[0] = L"Gulim"; available[1] = NULL;
    alloc_fails = 1;
    assert(!font_cache_get_script_font(&cache, TEXT_SCRIPT_HANGUL, 13, 0, 0) && !cache.entries);
    alloc_fails = 0; dc_fails = 1; e = enums;
    assert(!font_cache_get_script_font(&cache, TEXT_SCRIPT_SYMBOL, 13, 0, 0) && enums == e);
    assert(!cache.fallback[TEXT_SCRIPT_SYMBOL]);
    dc_fails = 0; create_fails = 1;
    assert(!font_cache_get_script_font(&cache, TEXT_SCRIPT_SYMBOL, 13, 0, 0) && enums == e + 1 && !cache.count);
    create_fails = 0;
    assert(font_cache_get_script_font(&cache, TEXT_SCRIPT_SYMBOL, 13, 0, 0) && enums == e + 1 && cache.count == 1);
    primary = font_cache_get_font(&cache, 0, 13, 129, 0, 0);
    select_fails = 1; g = glyph_calls; m = metric_calls;
    assert(font_cache_covers(&cache, TEST_DC, primary, TEXT_SCRIPT_HANGUL) && glyph_calls == g);
    assert(!font_cache_ascent(&cache, TEST_DC, primary) && metric_calls == m);
    select_fails = 0; glyph_fails = 1;
    assert(font_cache_covers(&cache, TEST_DC, primary, TEXT_SCRIPT_HANGUL) && selected == DC_FONT);
    glyph_fails = 0;
    assert(!font_cache_covers(&cache, TEST_DC, primary, TEXT_SCRIPT_HANGUL) && glyph_calls == g + 2);
    metric_fails = 1;
    assert(!font_cache_ascent(&cache, TEST_DC, primary) && selected == DC_FONT);
    metric_fails = 0;
    assert(font_cache_ascent(&cache, TEST_DC, primary) == 10);
    assert(!font_cache_ascent(&cache, TEST_DC, font_cache_get_font(&cache, 0, 1, 129, 0, 0)));
    finish(&cache);
    available[0] = NULL;
}

int main(void)
{
    struct { unsigned before[4]; struct FontCache cache; unsigned after[4]; } guarded;
    unsigned i;
    int stopped;
    g_3dDevice.faceNamesA[0] = g_3dDevice.faceNamesA[1] = L"Primary";
    g_3dDevice.faceNamesB[0] = g_3dDevice.faceNamesB[1] = L"Fallback";
    memset(&guarded, 0xa5, sizeof(guarded));
    assert(font_cache_construct(&guarded.cache) == &guarded.cache);
    finish(&guarded.cache);
    for (i = 0; i < 4; ++i)
        assert(guarded.before[i] == 0xa5a5a5a5 && guarded.after[i] == 0xa5a5a5a5);
    installed_fonts(); eot_fonts(); file_fonts(); shared_file_resources(); retry_failures(); script_fonts();
    stopped = stops;
    gdiplus_init();
    assert(exit_callback == gdiplus_destroy && g_fontCache.token == active_token);
    exit_callback(); exit_callback();
    assert(stops == stopped + 1 && !g_fontCache.entries && !g_fontCache.count);
    assert(dc_gets == dc_releases + 2 && !active_dc);
    puts("Font cache: wide names, six-field keys, installed/EOT/file fonts, script fallbacks, glyph coverage, ascents, growth, failure retry and resource cleanup passed.");
    return 0;
}
