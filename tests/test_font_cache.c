/* The font cache (render/fonts_ft.c) over fonts this test makes, found by the font finder's folder
   scan: a face asked by its display-language name then its English one; GDI's sizes (a cell
   height through the face's own Windows ascent and descent, an em height, a stand-in for a CJK face
   with its em as the cell); the face's own hints for a Windows family and the autohinter for a
   stand-in; the Hangul fallback; the fixed-pitch family and its fallbacks; drawing and measuring;
   every allocation failing in turn, and many caches made and destroyed, without a leak. */
#include "font_fixture.h"
#include "render/device.h"
#include "render/fonts.h"
#include "platform/code_page.h"
#include "platform/filesystem.h"
#include <assert.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_TABLES_H
#include FT_TRUETYPE_TAGS_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#define SEPARATOR ";"
static void set_variable(const char *name, const char *value) { assert(!_putenv_s(name, value)); }
#else
#define SEPARATOR ":"
static void set_variable(const char *name, const char *value) { assert(!setenv(name, value, 1)); }
#endif

struct C3dDevice g_3dDevice = {
    .faceNamesB = {L"Gulim", L"Fixture Sans", L"Fixture Gothic"},
    .faceNamesA = {L"Gulim", L"Missing Face", L"Fixture Pair"},
    .faceCharsets = {CHARSET_HANGUL, CHARSET_ANSI, CHARSET_SHIFTJIS},
};
char g_clientDir[0x88] = "nonexistent";

static long live, calls, fail_at;

static void *test_malloc(size_t size)
{
    void *data;
    if (++calls == fail_at)
        return NULL;
    data = malloc(size);
    live += data != NULL;
    return data;
}

static void *test_calloc(size_t count, size_t size)
{
    void *data;
    if (++calls == fail_at)
        return NULL;
    data = calloc(count, size);
    live += data != NULL;
    return data;
}

static void *test_realloc(void *data, size_t size)
{
    void *grown;
    if (++calls == fail_at)
        return NULL;
    grown = realloc(data, size);
    live += grown && !data;
    return grown;
}

static void test_free(void *data)
{
    live -= data != NULL;
    free(data);
}

#define malloc test_malloc
#define calloc test_calloc
#define realloc test_realloc
#define free test_free
#include "../src/render/fonts_ft.c"
/* Each file has a FreeType library of its own. */
#define library finder_library
#include "../src/render/font_match_scan.c"
#undef library
#undef malloc
#undef calloc
#undef realloc
#undef free

#define LATIN 'A', 'z', 'a', 'Z', ' ', 'W', 'i'
static const unsigned latin[] = {LATIN, 0};
static const unsigned japanese[] = {LATIN, 0x3042, 0x30a2, 0x65e5, 0};
static const unsigned hangul[] = {' ', 0xac00, 0xd7a3, 0xd55c, 0x2605, 0};
/* "가나" */
static const uint16_t ga_na[] = {0xac00, 0xac00};
static const uint16_t hello[] = {'A', 'z', 'a', 'W', 'i'};

static void folder(const char *path)
{
    assert(platform_directory_create(path) || platform_path_is_directory(path));
}

static void save(const char *path, const char *family, int weight, int fixed, int em, int ascent, int descent,
                 const unsigned *chars)
{
    const struct FixtureFont font = {family, NULL, 0, NULL, 0, weight, 0, fixed, em, ascent, descent, chars};
    assert(font_fixture_save(path, &font, 1));
}

/* windows: Windows fonts the client asks for by name; standin: a free font that stands in for one;
   mono: a fixed-pitch font of no family the client names; lucida: the overlay's own. Malgun
   Gothic's cell is 1.33 of its em, as the real font's is; Fixture Gothic's cell is its em, as
   Gulim's and MS Gothic's are. */
static void make_fonts(void)
{
    folder("windows");
    folder("standin");
    folder("mono");
    folder("lucida");
    save("windows/sans.ttf", "Fixture Sans", 400, 0, 2048, 1854, 434, latin);
    save("windows/sans-bold.ttf", "Fixture Sans", 700, 0, 2048, 1854, 434, latin);
    save("windows/gothic.ttf", "Fixture Gothic", 400, 0, 256, 220, 36, japanese);
    save("windows/pair.ttf", "Fixture Pair", 400, 0, 1000, 900, 300, japanese);
    save("windows/malgun.ttf", "Malgun Gothic", 400, 0, 2048, 2210, 514, hangul);
    save("windows/courier.ttf", "Courier New", 400, 1, 2048, 1705, 615, latin);
    save("windows/msgothic.ttf", "MS Gothic", 400, 0, 256, 220, 36, latin);
    save("standin/nanum.ttf", "NanumGothic", 400, 0, 2048, 2210, 514, hangul);
    save("standin/sans.ttf", "Fixture Sans", 400, 0, 2048, 1854, 434, latin);
    save("mono/mono.ttf", "Fixture Mono", 400, 1, 1000, 800, 200, latin);
    save("mono/sans.ttf", "Fixture Sans", 400, 0, 2048, 1854, 434, latin);
    save("lucida/lucida.ttf", "Lucida Console", 400, 1, 2048, 1616, 432, latin);
    save("lucida/courier.ttf", "Courier New", 400, 1, 2048, 1705, 615, latin);
    folder("flat");
    save("flat/flat.ttf", "Arial", 400, 0, 1000, -1, -1, latin);
}

static int ink_rows(FontHandle font, const uint16_t *text, int length);

/* A font without an ascent or a descent (data from outside) takes its em as its cell. */
static void flat_font(void)
{
    FontHandle font;
    SIZE extent;
    set_variable("RAGE_FONT_DIRS", "flat");
    fonts_init();
    font = font_cache_get_font(&g_fontCache, 1, 12, CHARSET_ANSI, 0, 0);
    assert(font && font->ppem == 12 && font->ascent == 12 && !font->descent);
    font_text_extent(font, hello, 5, 0, &extent);
    assert(extent.cy == 12);
    ink_rows(font, hello, 5);
    fonts_shutdown();
    assert(!live);
}

static const char *family_name(const struct Source *source)
{
    return source->face->family_name;
}

static int ink_rows(FontHandle font, const uint16_t *text, int length)
{
    static DWORD pixels[64 * 64];
    struct FontTarget target = {(BYTE *)pixels, 64, 64, 64 * 4};
    int x, y, rows = 0;
    memset(pixels, 0, sizeof pixels);
    font_text_out(font, &target, 2, 2, text, length, RGB(1, 2, 3), NULL);
    for (y = 0; y < 64; ++y)
        for (x = 0; x < 64; ++x)
            if (pixels[y * 64 + x]) {
                assert(pixels[y * 64 + x] == 0x010203);
                ++rows;
                break;
            }
    return rows;
}

static void windows_fonts(void)
{
    FontHandle sans, gothic, pair, hangul_font, fixed;
    SIZE extent;
    set_variable("RAGE_FONT_DIRS", "windows");
    fonts_init();
    /* Face 1: "Missing Face" is not installed, so its English name, Fixture Sans, at a cell of 15:
       ppem round(15 * 2048 / 2288) = 13, ascent round(1854 * 15 / 2288) = 12, descent 3. */
    sans = font_cache_get_font(&g_fontCache, 1, 15, CHARSET_ANSI, 0, 0);
    assert(sans && sans == font_cache_get_font(&g_fontCache, 1, 15, CHARSET_ANSI, 0, 0));
    assert(!strcmp(family_name(&sans->primary), "Fixture Sans") && sans->primary.own_hints && !sans->em_cell);
    assert(sans->ppem == 13 && sans->ascent == 12 && sans->descent == 3 && font_ascent(sans) == 12);
    font_text_extent(sans, hello, 5, 0, &extent);
    assert(extent.cy == 15 && extent.cx > 5 * 5);
    assert(!font_covers(sans, TEXT_SCRIPT_HANGUL) && !font_covers(sans, TEXT_SCRIPT_EMOJI));
    /* An em height: 13 pixels of em. */
    sans = font_cache_get_font(&g_fontCache, 1, -13, CHARSET_ANSI, 1, 0);
    assert(sans->ppem == 13 && sans->ascent == 12 && sans->descent == 3 && !sans->embolden);
    /* Face 2: "Fixture Pair", its first name, before Fixture Gothic. */
    pair = font_cache_get_font(&g_fontCache, 2, 12, CHARSET_SHIFTJIS, 0, 0);
    assert(pair && !strcmp(family_name(&pair->primary), "Fixture Pair") && pair->primary.own_hints);
    /* A cell of 12 on a 1,200-unit cell of a 1,000-unit em: ppem 10. */
    assert(pair->ppem == 10 && pair->ascent == 9 && pair->descent == 3 && !pair->em_cell);
    g_3dDevice.faceNamesA[2] = L"Missing Too";
    gothic = font_cache_get_font(&g_fontCache, 2, 13, CHARSET_SHIFTJIS, 0, 0);
    assert(gothic && !strcmp(family_name(&gothic->primary), "Fixture Gothic") && gothic->primary.own_hints);
    assert(gothic->ppem == 13 && gothic->ascent == 11 && gothic->descent == 2);
    g_3dDevice.faceNamesA[2] = L"Fixture Pair";
    /* Hangul: Gulim is missing, Malgun Gothic is the next Windows font, sized as GDI sizes it, by
       its own cell: ppem round(12 * 2048 / 2724) = 9. */
    hangul_font = font_cache_get_script_font(&g_fontCache, TEXT_SCRIPT_HANGUL, 12, 0, 0);
    assert(hangul_font && !strcmp(family_name(&hangul_font->primary), "Malgun Gothic"));
    assert(hangul_font->primary.own_hints && !hangul_font->em_cell);
    assert(hangul_font->ppem == 9 && hangul_font->ascent == 10 && hangul_font->descent == 2);
    assert(font_covers(hangul_font, TEXT_SCRIPT_HANGUL) && font_covers(hangul_font, TEXT_SCRIPT_SYMBOL));
    assert(ink_rows(hangul_font, ga_na, 2) == 6);
    assert(font_cache_get_script_font(&g_fontCache, TEXT_SCRIPT_SYMBOL, 12, 0, 0)->ppem == 9);
    /* Han: Gulim is missing, MS Gothic is the next Windows font and is taken, as GDI took the first
       installed, though it has no Han here; its em is its cell */
    hangul_font = font_cache_get_script_font(&g_fontCache, TEXT_SCRIPT_HAN, 12, 0, 0);
    assert(hangul_font && !strcmp(family_name(&hangul_font->primary), "MS Gothic"));
    assert(hangul_font->primary.own_hints && hangul_font->ppem == 12);
    assert(!font_cache_get_script_font(&g_fontCache, TEXT_SCRIPT_DEFAULT, 12, 0, 0));
    /* No Lucida Console: Courier New, as GDI's mapper gives, with its own hints. */
    fixed = font_cache_get_fixed(&g_fontCache, "Lucida Console", -12);
    assert(fixed && !strcmp(family_name(&fixed->primary), "Courier New") && fixed->primary.own_hints);
    assert(fixed->ppem == 12 && fixed == font_cache_get_fixed(&g_fontCache, "Lucida Console", -12));
    fonts_shutdown();
    assert(!live);
}

static void stand_ins(void)
{
    FontHandle hangul_font, gothic, fixed;
    SIZE wide, thin;
    set_variable("RAGE_FONT_DIRS", "standin" SEPARATOR "mono");
    fonts_init();
    /* NanumGothic stands in for Gulim, whose em is its cell: ppem 12, autohinted. */
    hangul_font = font_cache_get_script_font(&g_fontCache, TEXT_SCRIPT_HANGUL, 12, 0, 0);
    assert(hangul_font && !strcmp(family_name(&hangul_font->primary), "NanumGothic"));
    assert(!hangul_font->primary.own_hints && hangul_font->em_cell);
    assert(hangul_font->ppem == 12 && hangul_font->ascent == 10 && hangul_font->descent == 2);
    assert(ink_rows(hangul_font, ga_na, 2) == 8);
    /* Face 2 has no font of its language here: the first face, a stand-in at its cell as its em. */
    gothic = font_cache_get_font(&g_fontCache, 2, 12, CHARSET_SHIFTJIS, 0, 0);
    assert(gothic && !gothic->primary.own_hints && gothic->em_cell && gothic->ppem == 12);
    /* No Lucida Console and no Courier New: a fixed-pitch face. */
    fixed = font_cache_get_fixed(&g_fontCache, "Lucida Console", -12);
    assert(fixed && !strcmp(family_name(&fixed->primary), "Fixture Mono") && !fixed->primary.own_hints);
    font_text_extent(fixed, hello + 3, 1, 0, &wide);
    font_text_extent(fixed, hello + 4, 1, 0, &thin);
    assert(wide.cx == thin.cx && wide.cx > 0);
    fonts_shutdown();
    assert(!live);
    set_variable("RAGE_FONT_DIRS", "lucida");
    fonts_init();
    fixed = font_cache_get_fixed(&g_fontCache, "Lucida Console", -12);
    assert(fixed && !strcmp(family_name(&fixed->primary), "Lucida Console") && fixed->primary.own_hints);
    /* GDI's em of 12: ascent round(1616 * 12 / 2048) = 9, descent 3. */
    assert(fixed->ppem == 12 && fixed->ascent == 9 && fixed->descent == 3);
    fonts_shutdown();
    assert(!live);
}

/* One cache's life: fonts, a fallback face for a character, measuring and drawing. */
static void life(void)
{
    static const uint16_t mixed[] = {'A', 0xac00, 0x2605};
    FontHandle sans, hangul_font, fixed;
    SIZE extent;
    fonts_init();
    sans = font_cache_get_font(&g_fontCache, 1, 12, CHARSET_ANSI, 1, 1);
    hangul_font = font_cache_get_script_font(&g_fontCache, TEXT_SCRIPT_HANGUL, 15, 0, 0);
    fixed = font_cache_get_fixed(&g_fontCache, "Lucida Console", -12);
    if (sans) {
        font_text_extent(sans, mixed, 3, 1, &extent);
        ink_rows(sans, mixed, 3);
    }
    if (hangul_font)
        ink_rows(hangul_font, mixed, 3);
    if (fixed)
        ink_rows(fixed, hello, 5);
    fonts_shutdown();
    assert(!live);
}

static void allocation_failures(void)
{
    long total, i;
    set_variable("RAGE_FONT_DIRS", "windows");
    calls = fail_at = 0;
    life();
    total = calls;
    assert(total > 20);
    for (i = 1; i <= total; ++i) {
        calls = 0;
        fail_at = i;
        life();
    }
    fail_at = 0;
    for (i = 0; i < 32; ++i)
        life();
}

int main(void)
{
    make_fonts();
    windows_fonts();
    stand_ins();
    flat_font();
    allocation_failures();
    puts("Font cache: faces by their first then second name, GDI's cell and em sizes, own hints for "
         "Windows families and the autohinter for stand-ins, Hangul's and the fixed-pitch fallbacks, "
         "every allocation failing in turn and 32 caches made and destroyed without a leak passed.");
    return 0;
}
