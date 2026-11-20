/* The font cache (render/fonts_ft.c) on this Windows's own fonts, against GDI, which drew the text
   before: for each request the client makes (its faces by the names of render/c3ddevice.c, the
   script fallbacks, the frame-time overlay's Lucida Console, the embedded faces) at the sizes it
   uses, the same face, ppem, ascent and descent as GDI's CreateFont, and the advance widths; the
   pixels of a line of text drawn by both are counted and printed. Also: the Hangul fallback lands on
   Gulim or Malgun Gothic, fonts installed for the user are found, the scan's time, and many caches
   made and destroyed leave no allocation and no handle behind. */
#include "render/device.h"
#include "render/fonts.h"
#include "platform/code_page.h"
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
#include <windows.h>

#define FACE_GULIM_KR   L"\xad74\xb9bc"
#define FACE_MS_PGOTHIC L"\xff2d\xff33 \xff30\x30b4\x30b7\x30c3\x30af"
#define FACE_SIMSUN_CN  L"\x5b8b\x4f53"
#define FACE_MINGLIU_TW L"\x7d30\x660e\x9ad4"
struct C3dDevice g_3dDevice = {
    .faceNamesB = {L"Gulim", L"Arial", L"MS Gothic", L"SimSun", L"MingLiU", L"Tahoma",
                   L"Arial", L"Verdana", L"Verdana", L"Arial", L"Arial", L"Tahoma"},
    .faceNamesA = {FACE_GULIM_KR, L"Arial", FACE_MS_PGOTHIC, FACE_SIMSUN_CN, FACE_MINGLIU_TW, L"Tahoma",
                   L"Arial", L"Verdana", L"Verdana", L"Arial", L"Arial", L"Tahoma"},
    .faceCharsets = {CHARSET_HANGUL, CHARSET_ANSI, CHARSET_SHIFTJIS, CHARSET_GB2312, CHARSET_CHINESEBIG5,
                     CHARSET_THAI, CHARSET_VIETNAMESE, CHARSET_ANSI, CHARSET_ANSI, CHARSET_RUSSIAN,
                     CHARSET_VIETNAMESE + 1, CHARSET_ARABIC},
};
char g_clientDir[0x88] = "nonexistent";

static long live;

static void *test_malloc(size_t size)
{
    void *data = malloc(size);
    live += data != NULL;
    return data;
}

static void *test_calloc(size_t count, size_t size)
{
    void *data = calloc(count, size);
    live += data != NULL;
    return data;
}

static void *test_realloc(void *data, size_t size)
{
    void *grown = realloc(data, size);
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

#define WIDTH 400
#define HEIGHT 48

/* What a request asked GDI for. */
struct Request {
    const char *what;
    const wchar_t *name; /* NULL: GDI's mapper chose by the charset */
    int charset, height, pitch, precision, quality;
};

/* What GDI made of a request, as the old font cache asked: CreateFont with the name and charset. */
struct Gdi {
    wchar_t face[LF_FACESIZE];
    TEXTMETRICW metrics;
};

static HDC dc;
static DWORD *gdi_bits;
static HBITMAP dib;

static int CALLBACK enum_match(const LOGFONTW *font, const TEXTMETRICW *metric, DWORD type, LPARAM context)
{
    (void)metric;
    (void)type;
    return _wcsicmp((const wchar_t *)context, font->lfFaceName) != 0;
}

/* The old font cache's lookup: a family name GDI enumerates in the charset. */
static int installed(const wchar_t *name, int charset)
{
    LOGFONTW query = {0};
    query.lfCharSet = (BYTE)charset;
    return name && !EnumFontFamiliesExW(dc, &query, enum_match, (LPARAM)name, 0);
}

static HFONT gdi_font(const struct Request *request)
{
    return CreateFontW(request->height, 0, 0, 0, FW_NORMAL, 0, 0, 0, (DWORD)request->charset,
                       (DWORD)request->precision, 0, (DWORD)request->quality, (DWORD)request->pitch, request->name);
}

static void gdi_measure(HFONT font, struct Gdi *out)
{
    HGDIOBJ previous = SelectObject(dc, font);
    assert(previous && previous != HGDI_ERROR);
    assert(GetTextFaceW(dc, LF_FACESIZE, out->face) && GetTextMetricsW(dc, &out->metrics));
    SelectObject(dc, previous);
}

/* text drawn white on black at (2, 2) by GDI into gdi_bits and by the font into ours; returns the
   pixels that differ. */
static int compare_pixels(HFONT gdi, FontHandle font, const wchar_t *text, int *extent_gdi, int *extent_ft)
{
    static DWORD ours[WIDTH * HEIGHT];
    struct FontTarget target = {(BYTE *)ours, WIDTH, HEIGHT, WIDTH * 4};
    HGDIOBJ previous = SelectObject(dc, gdi);
    int length = (int)wcslen(text), i, differ = 0;
    SIZE size;
    memset(gdi_bits, 0, WIDTH * HEIGHT * 4);
    memset(ours, 0, sizeof ours);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 255, 255));
    assert(TextOutW(dc, 2, 2, text, length) && GdiFlush());
    assert(GetTextExtentPoint32W(dc, text, length, &size));
    SelectObject(dc, previous);
    *extent_gdi = size.cx;
    font_text_out(font, &target, 2, 2, (const uint16_t *)text, length, RGB(255, 255, 255), NULL);
    font_text_extent(font, (const uint16_t *)text, length, 0, &size);
    *extent_ft = size.cx;
    for (i = 0; i < WIDTH * HEIGHT; ++i)
        differ += (gdi_bits[i] & 0xffffff) != (ours[i] & 0xffffff);
    return differ;
}

static char *utf8(const wchar_t *text)
{
    static char out[4][128];
    static int next;
    char *buffer = out[next++ & 3];
    WideCharToMultiByte(CP_UTF8, 0, text, -1, buffer, 128, NULL, NULL);
    return buffer;
}

/* The face drawn answers to GDI's face name, in English or in the display language. */
static int same_face(FontHandle font, const wchar_t *gdi_face)
{
    const char *name = utf8(gdi_face), *file;
    int i, index;
    if (font->primary.candidate < 0)
        return 0;
    file = font_match_file(font->candidates, font->primary.candidate, &index);
    if (font->primary.face->family_name && !cstr_compare_nocase(font->primary.face->family_name, name))
        return 1;
    for (i = 0; i < face_count; ++i)
        if (!strcmp(files[i].file, file) && files[i].index == index
            && ((faces[i].family && !cstr_compare_nocase(faces[i].family, name))
                || (faces[i].local_family && !cstr_compare_nocase(faces[i].local_family, name))))
            return 1;
    return 0;
}

static int requests, metric_mismatches, extent_mismatches, pixel_total, pixel_lines;

/* How much of a request must be GDI's: nothing (measured only), the face and its metrics, or the
   advance widths as well. */
enum { MEASURED, METRICS, WIDTHS };

/* One request: GDI's face, metrics and widths against the font's; the pixels are counted. */
static void check(const struct Request *request, FontHandle font, const wchar_t *text, int strict)
{
    HFONT gdi = gdi_font(request);
    struct Gdi made;
    int ppem, extent_gdi, extent_ft, differ, face_ok, metrics_ok;
    assert(gdi && font);
    gdi_measure(gdi, &made);
    ppem = made.metrics.tmHeight - made.metrics.tmInternalLeading;
    differ = compare_pixels(gdi, font, text, &extent_gdi, &extent_ft);
    face_ok = same_face(font, made.face);
    metrics_ok = font->ppem == ppem && font->ascent == made.metrics.tmAscent
              && font->descent == made.metrics.tmDescent;
    ++requests;
    pixel_total += differ;
    pixel_lines += differ > 0;
    metric_mismatches += !metrics_ok;
    extent_mismatches += extent_gdi != extent_ft;
    printf("%-26s %3d  GDI %-16s ppem %2d %2d/%d w %3d | FT %-16s ppem %2d %2d/%d w %3d %s | %4d px\n",
           request->what, request->height, utf8(made.face), ppem, (int)made.metrics.tmAscent,
           (int)made.metrics.tmDescent, extent_gdi, font->primary.face->family_name, font->ppem, font->ascent,
           font->descent, extent_ft, font->primary.own_hints ? "own" : "auto", differ);
    if (strict >= METRICS) {
        assert(face_ok);
        assert(metrics_ok);
    }
    if (strict >= WIDTHS)
        assert(extent_gdi == extent_ft);
    DeleteObject(gdi);
}

static const wchar_t latin_text[] = L"Base Lv. 99 Hello, World! 0123456789 gjpqy";
static const wchar_t hangul_text[] = L"\xc548\xb155\xd558\xc138\xc694 \xd55c\xae00 \xac00\xb098\xb2e4 \xb77c\xadf8\xb098\xb85c\xd06c";
static const wchar_t symbol_text[] = L"\x2605\x2606\x2665\x2192\x203b";
static const wchar_t thai_text[] = L"\x0e01\x0e02\x0e04\x0e07\x0e08";
static const wchar_t kana_text[] = L"\x3042\x3044\x3046\x30a2\x30a4\x30a6";
static const wchar_t han_text[] = L"\x4e2d\x6587\x6f22\x5b57\x65e5\x672c";

/* The client's faces at the sizes it asks for, by the old cache's names and charsets. */
static void faces_against_gdi(void)
{
    int face, height;
    for (face = 1; face < 12; ++face)
        for (height = 10; height <= 24; ++height) {
            const wchar_t *name = installed(g_3dDevice.faceNamesA[face], g_3dDevice.faceCharsets[face])
                                ? g_3dDevice.faceNamesA[face]
                                : installed(g_3dDevice.faceNamesB[face], g_3dDevice.faceCharsets[face])
                                ? g_3dDevice.faceNamesB[face] : NULL;
            struct Request request = {NULL, name, g_3dDevice.faceCharsets[face], height, 0, 0,
                                      NONANTIALIASED_QUALITY};
            char what[32];
            FontHandle font = font_cache_get_font(&g_fontCache, face, height, g_3dDevice.faceCharsets[face], 0, 0);
            snprintf(what, sizeof what, "face %d %s", face, name ? utf8(name) : "(not installed)");
            request.what = what;
            /* A face GDI could not find by name is GDI's mapper's choice, which the client does not
               copy (render/fonts_ft.c): measured, not required. */
            check(&request, font, latin_text, name ? WIDTHS : MEASURED);
        }
}

/* The old cache's names for the scripts, in English; it listed the localized names as well. */
static const wchar_t *const hangul_names[] = {L"Gulim", L"Malgun Gothic", NULL};
static const wchar_t *const symbol_names[] = {L"Gulim", L"Malgun Gothic", L"Segoe UI Symbol", NULL};
static const wchar_t *const thai_names[] = {L"Tahoma", L"Leelawadee UI", NULL};
static const wchar_t *const kana_names[] = {L"MS PGothic", L"MS Gothic", L"Meiryo", L"Yu Gothic", NULL};
static const wchar_t *const han_names[] = {L"Gulim", L"MS Gothic", L"SimSun", L"Microsoft YaHei", L"Malgun Gothic",
                                           NULL};
static const struct {
    int script;
    const wchar_t *const *names;
    const wchar_t *text;
    int strict;
} scripts[] = {
    {TEXT_SCRIPT_HANGUL, hangul_names, hangul_text, WIDTHS},
    {TEXT_SCRIPT_SYMBOL, symbol_names, symbol_text, WIDTHS},
    {TEXT_SCRIPT_THAI, thai_names, thai_text, WIDTHS},
    /* MS PGothic's kana at odd sizes, drawn from its embedded bitmaps: GDI advances some by the
       bitmap's width and some by the outline's, by a rule not found, and so they are measured */
    {TEXT_SCRIPT_KANA, kana_names, kana_text, METRICS},
    {TEXT_SCRIPT_HAN, han_names, han_text, WIDTHS},
};

/* A script's fallback font: the old cache's first installed name, at the client's sizes. */
static void scripts_against_gdi(void)
{
    size_t s;
    int height;
    for (s = 0; s < sizeof scripts / sizeof scripts[0]; ++s) {
        const wchar_t *name = NULL;
        int i;
        for (i = 0; scripts[s].names[i] && !name; ++i)
            if (installed(scripts[s].names[i], DEFAULT_CHARSET))
                name = scripts[s].names[i];
        if (!name) {
            printf("script %d: none of its Windows fonts is installed\n", scripts[s].script);
            continue;
        }
        for (height = 10; height <= 24; ++height) {
            struct Request request = {NULL, name, DEFAULT_CHARSET, height, 0, 0, NONANTIALIASED_QUALITY};
            char what[48];
            FontHandle font = font_cache_get_script_font(&g_fontCache, scripts[s].script, height, 0, 0);
            snprintf(what, sizeof what, "script %d %s", scripts[s].script, utf8(name));
            request.what = what;
            check(&request, font, scripts[s].text, scripts[s].strict);
        }
    }
}

/* The overlay's request, and the embedded faces' (an unknown name at an em of 13, raster precision
   and proof quality, which GDI's mapper gives the charset's font, smoothed as the system smooths
   text). */
static void others_against_gdi(void)
{
    struct Request overlay = {"overlay Lucida Console", L"Lucida Console", DEFAULT_CHARSET, -12,
                              FIXED_PITCH | FF_MODERN, 0, NONANTIALIASED_QUALITY};
    struct Request embedded = {"embedded face 20", L"RixSquirrel_10.eot", DEFAULT_CHARSET, -13, 0,
                               OUT_RASTER_PRECIS, PROOF_QUALITY};
    FontHandle fixed = font_cache_get_fixed(&g_fontCache, "Lucida Console", -12);
    assert(fixed && !strcmp(fixed->primary.face->family_name, "Lucida Console") && fixed->primary.own_hints);
    check(&overlay, fixed, L"draws 1174 (1954 calls)  prims 100336  modes 19 sent / 782 skipped", WIDTHS);
    check(&embedded, font_cache_get_font(&g_fontCache, 20, 12, CHARSET_ANSI, 0, 0), latin_text, WIDTHS);
}

/* Hangul in face 0's Arial falls back to Gulim or Malgun Gothic, as GDI's cache did. */
static void hangul_fallback(void)
{
    FontHandle face = font_cache_get_font(&g_fontCache, 1, 15, CHARSET_ANSI, 0, 0);
    FontHandle hangul_font = font_cache_get_script_font(&g_fontCache, TEXT_SCRIPT_HANGUL, 15, 0, 0);
    const char *family;
    assert(face && !font_covers(face, TEXT_SCRIPT_HANGUL));
    assert(hangul_font && hangul_font != face && font_covers(hangul_font, TEXT_SCRIPT_HANGUL));
    family = hangul_font->primary.face->family_name;
    assert(!strcmp(family, "Gulim") || !strcmp(family, "Malgun Gothic"));
    assert(hangul_font->primary.own_hints && !hangul_font->em_cell);
    printf("Hangul falls back to %s\n", family);
}

/* Every font the registry lists for the user that exists is in the catalogue. */
static void user_fonts(void)
{
    HKEY key;
    DWORD i, found = 0, listed = 0;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, FONTS_KEY, 0, KEY_READ, &key) != ERROR_SUCCESS) {
        puts("no fonts are installed for the user");
        return;
    }
    for (i = 0;; ++i) {
        wchar_t name[512], data[MAX_PATH];
        DWORD name_size = 512, data_size = sizeof data - sizeof data[0], type;
        char path[MAX_PATH];
        int f;
        BOOL lossy = FALSE;
        LONG result = RegEnumValueW(key, i, name, &name_size, NULL, &type, (BYTE *)data, &data_size);
        if (result == ERROR_NO_MORE_ITEMS)
            break;
        if (result != ERROR_SUCCESS || type != REG_SZ)
            continue;
        data[data_size / sizeof data[0]] = 0;
        if (GetFileAttributesW(data) == INVALID_FILE_ATTRIBUTES
            || !WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, data, -1, path, MAX_PATH, NULL, &lossy) || lossy)
            continue;
        ++listed;
        for (f = 0; f < face_count; ++f)
            if (!_stricmp(files[f].file, path)) {
                ++found;
                break;
            }
    }
    RegCloseKey(key);
    printf("fonts installed for the user: %lu listed, %lu found\n", listed, found);
    assert(found == listed);
}

static double seconds(void)
{
    LARGE_INTEGER now, frequency;
    QueryPerformanceCounter(&now);
    QueryPerformanceFrequency(&frequency);
    return (double)now.QuadPart / (double)frequency.QuadPart;
}

static void cycles(void)
{
    DWORD handles_before, handles_after;
    double start, total = 0;
    int i;
    GetProcessHandleCount(GetCurrentProcess(), &handles_before);
    for (i = 0; i < 32; ++i) {
        FontHandle face, hangul_font, fixed;
        SIZE size;
        start = seconds();
        fonts_init();
        total += seconds() - start;
        face = font_cache_get_font(&g_fontCache, 1, 15, CHARSET_ANSI, i & 1, i & 2);
        hangul_font = font_cache_get_script_font(&g_fontCache, TEXT_SCRIPT_HANGUL, 15, 0, 0);
        fixed = font_cache_get_fixed(&g_fontCache, "Lucida Console", -12);
        assert(face && hangul_font && fixed);
        font_text_extent(face, (const uint16_t *)hangul_text, (int)wcslen(hangul_text), 1, &size);
        font_text_extent(hangul_font, (const uint16_t *)hangul_text, (int)wcslen(hangul_text), 0, &size);
        assert(size.cx > 0);
        fonts_shutdown();
        assert(!live);
    }
    GetProcessHandleCount(GetCurrentProcess(), &handles_after);
    printf("32 caches made and destroyed: %lu handles before, %lu after; the font scan took %.1f ms on average\n",
           handles_before, handles_after, total * 1000 / 32);
    assert(handles_after == handles_before);
}

int main(void)
{
    BITMAPINFO info = {0};
    double start;
    setvbuf(stdout, NULL, _IONBF, 0);
    dc = CreateCompatibleDC(NULL);
    info.bmiHeader.biSize = sizeof info.bmiHeader;
    info.bmiHeader.biWidth = WIDTH;
    info.bmiHeader.biHeight = -HEIGHT;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    dib = CreateDIBSection(dc, &info, DIB_RGB_COLORS, (void **)&gdi_bits, NULL, 0);
    assert(dc && dib && SelectObject(dc, dib));
    start = seconds();
    fonts_init();
    printf("the first font scan took %.1f ms for %d faces (display language %04x)\n", (seconds() - start) * 1000,
           face_count, display_language);
    hangul_fallback();
    faces_against_gdi();
    scripts_against_gdi();
    others_against_gdi();
    user_fonts();
    printf("%d requests: %d with other metrics than GDI's, %d with other widths; %d lines with other pixels, "
           "%d pixels in all\n", requests, metric_mismatches, extent_mismatches, pixel_lines, pixel_total);
    fonts_shutdown();
    assert(!live);
    cycles();
    DeleteObject(dib);
    DeleteDC(dc);
    puts("Font system: the client's requests against GDI on this Windows's fonts, the Hangul fallback, the user's "
         "fonts and 32 caches without a leak passed.");
    return 0;
}
