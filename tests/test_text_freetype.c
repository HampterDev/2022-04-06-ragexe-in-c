#include "render/cdc.h"
#include "render/device.h"
#include "render/fonts.h"
#include "render/textdc.h"
#include "render/renderer.h"
#include "render/sprite_atlas.h"
#include "render/texture_cache.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include "platform/clientinfo.h"
#include "platform/code_page.h"
#include "platform/language.h"
#include "render/font_match.h"
#include "diag/perf.h"
#include <assert.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
static void set_variable(const char *name, const char *value) { assert(!_putenv_s(name, value)); }
#else
static void set_variable(const char *name, const char *value) { assert(!setenv(name, value, 1)); }
#endif

/* The installed fonts: the system's, through the client's font finder (the registry's fonts on
   Windows, Fontconfig elsewhere). */
struct C3dDevice g_3dDevice = {
    .faceNamesB = {L"Gulim", L"Arial", L"MS Gothic", L"SimSun", L"MingLiU", L"Tahoma",
                   L"Arial", L"Verdana", L"Verdana", L"Arial", L"Arial", L"Tahoma"},
    .faceNamesA = {L"\xad74\xb9bc", L"Arial", L"\xff2d\xff33 \xff30\x30b4\x30b7\x30c3\x30af", L"\x5b8b\x4f53",
                   L"\x7d30\x660e\x9ad4", L"Tahoma", L"Arial", L"Verdana", L"Verdana", L"Arial", L"Arial", L"Tahoma"},
    .faceCharsets = {CHARSET_HANGUL, CHARSET_ANSI, CHARSET_SHIFTJIS, CHARSET_GB2312, CHARSET_CHINESEBIG5,
                     CHARSET_THAI, CHARSET_VIETNAMESE, CHARSET_ANSI, CHARSET_ANSI, CHARSET_RUSSIAN,
                     CHARSET_VIETNAMESE + 1, CHARSET_ARABIC},
};
struct Language *g_language;
struct Renderer *g_renderer;
struct perf_counters g_perf;
unsigned int g_codePage = CODE_PAGE_UTF8;
char g_clientDir[0x88] = "/nonexistent";

void perf_begin(int phase) { (void)phase; }
void perf_end(int phase) { (void)phase; }
void surface_destroy(struct Surface *surface) { (void)surface; abort(); }
struct Texture *texture_create(int w, int h, int format) { (void)w; (void)h; (void)format; abort(); }
struct Texture *texture_from_pixels(int w, int h, const DWORD *pixels, int a, int b, int c)
{ (void)w; (void)h; (void)pixels; (void)a; (void)b; (void)c; abort(); }
int texture_upload_pixels(struct Texture *tex, int x, int y, int w, int h, const DWORD *pixels, int keyed, int pitch)
{ (void)tex; (void)x; (void)y; (void)w; (void)h; (void)pixels; (void)keyed; (void)pitch; abort(); }
void texture_draw_surface(struct Texture *tex, int x, int y, int w, int h, DWORD colour)
{ (void)tex; (void)x; (void)y; (void)w; (void)h; (void)colour; abort(); }
void texture_draw_surface_on_top(struct Texture *tex, int x, int y, int w, int h, DWORD colour)
{ (void)tex; (void)x; (void)y; (void)w; (void)h; (void)colour; abort(); }
struct SprClip *motion_get_clip(const struct Motion *motion, unsigned index) { (void)motion; (void)index; abort(); }
struct Texture *sprite_atlas_resolve(struct Renderer *renderer, const struct SprImg *image, const DWORD *palette,
                                     struct SpriteRegion *region)
{ (void)renderer; (void)image; (void)palette; (void)region; abort(); }
void dc_bitmap_draw_texels_scaled(struct DcBitmap *self, int dx0, int dy0, int dw, int dh, struct Texture *tex,
                                  int sw, int sh, float scaleX, float scaleY, DWORD colour, char mirror, int offX,
                                  int offY, int sx, int sy)
{
    (void)self; (void)dx0; (void)dy0; (void)dw; (void)dh; (void)tex; (void)sw; (void)sh; (void)scaleX;
    (void)scaleY; (void)colour; (void)mirror; (void)offX; (void)offY; (void)sx; (void)sy; abort();
}

#define BACKGROUND 0x77777777u
#define INK RGB(0x12, 0x34, 0x56)
#define INK_PIXEL 0x00123456u

/* "가나다": the client's text is UTF-8, three bytes a syllable. */
static const char hangul[] = "\xea\xb0\x80\xeb\x82\x98\xeb\x8b\xa4";

static struct DcBitmap *canvas(int width, int height)
{
    struct DcBitmap *bitmap = dc_bitmap_create(width, height);
    int i;
    assert(bitmap && bitmap->bits && bitmap->width == width && bitmap->height == height);
    for (i = 0; i < width * height; ++i)
        bitmap->bits[i] = BACKGROUND;
    bitmap->changed = 0;
    return bitmap;
}

static SIZE extent(int face, int size, int bold, int italic, const char *text, int length)
{
    struct TextDC dc;
    SIZE result;
    text_dc_construct(&dc, NULL);
    text_dc_select_font(&dc, face, size, bold, italic);
    text_dc_text_extent(&dc, text, length, &result);
    text_dc_destruct(&dc);
    return result;
}

static int draw(struct DcBitmap *bitmap, int face, int size, int bold, int x, int y, const char *text, int length,
                const COLORREF *background)
{
    struct TextDC dc;
    int i, set = 0;
    text_dc_construct(&dc, bitmap);
    assert(bitmap->changed == 1);
    text_dc_select_font(&dc, face, size, bold, 0);
    text_dc_set_text_color(&dc, INK);
    if (background) {
        text_dc_set_bk_color(&dc, *background);
        assert(text_dc_set_bk_mode(&dc, TEXT_DC_OPAQUE) == TEXT_DC_TRANSPARENT);
    }
    text_dc_text_out(&dc, x, y, text, length);
    text_dc_destruct(&dc);
    for (i = 0; i < bitmap->width * bitmap->height; ++i)
        set += bitmap->bits[i] == INK_PIXEL;
    return set;
}

static void check_inside(const struct DcBitmap *bitmap, int x, int y, SIZE box)
{
    int px, py;
    for (py = 0; py < bitmap->height; ++py)
        for (px = 0; px < bitmap->width; ++px) {
            DWORD pixel = bitmap->bits[py * bitmap->width + px];
            /* The last glyph may overhang its advance by a pixel, as in GDI (Apple SD Gothic Neo does). */
            int inside = px >= x && px <= x + box.cx && py >= y && py < y + box.cy;
            if (pixel != BACKGROUND && !(pixel == INK_PIXEL && inside))
                fprintf(stderr, "pixel %06lx at %d,%d outside the box %d,%d %dx%d\n", (unsigned long)pixel, px, py,
                        x, y, (int)box.cx, (int)box.cy);
            assert(pixel == BACKGROUND || (pixel == INK_PIXEL && inside));
        }
}

static void check_latin(void)
{
    struct DcBitmap *bitmap = canvas(160, 40);
    SIZE whole = extent(1, 12, 0, 0, "Hello", 5), head = extent(1, 12, 0, 0, "Hel", 3), tail = extent(1, 12, 0, 0, "lo", 2);
    SIZE bold = extent(1, 12, 1, 0, "Hello", 5);
    assert(whole.cx > 10 && whole.cx < 80 && whole.cy >= 10 && whole.cy <= 16);
    assert(head.cx + tail.cx == whole.cx && head.cy == whole.cy);
    assert(bold.cx > 10 && bold.cy >= 10);
    assert(extent(1, 0x18, 0, 0, "Hello", 5).cx > whole.cx);
    assert(draw(bitmap, 1, 12, 0, 7, 9, "Hello", 5, NULL) > 20);
    check_inside(bitmap, 7, 9, whole);
    dc_bitmap_destroy(bitmap);
    bitmap = canvas(160, 40);
    assert(draw(bitmap, 1, 12, 1, 7, 9, "Hello", 5, NULL) > 20);
    check_inside(bitmap, 7, 9, bold);
    dc_bitmap_destroy(bitmap);
    bitmap = canvas(8, 8);
    draw(bitmap, 1, 12, 0, -3, -4, "Hello", 5, NULL);
    dc_bitmap_destroy(bitmap);
}

static int count_ink(const struct DcBitmap *bitmap, int left, int right)
{
    int x, y, set = 0;
    for (y = 0; y < bitmap->height; ++y)
        for (x = left; x < right; ++x)
            set += bitmap->bits[y * bitmap->width + x] == INK_PIXEL;
    return set;
}

static void check_korean(void)
{
    struct DcBitmap *bitmap = canvas(160, 40);
    SIZE all = extent(0, 12, 0, 0, hangul, 9), ga = extent(0, 12, 0, 0, hangul, 3), na = extent(0, 12, 0, 0, hangul + 3, 3);
    int x, y, differ = 0;
    assert(all.cx == 3 * ga.cx && ga.cx == na.cx && ga.cx >= 8 && ga.cx <= 16);
    assert(draw(bitmap, 0, 12, 0, 4, 2, hangul, 9, NULL) > 30);
    check_inside(bitmap, 4, 2, all);
    assert(count_ink(bitmap, 4, 4 + ga.cx) > 8 && count_ink(bitmap, 4 + ga.cx, 4 + 2 * ga.cx) > 8);
    for (y = 0; y < bitmap->height; ++y)
        for (x = 0; x < ga.cx; ++x)
            differ += (bitmap->bits[y * bitmap->width + 4 + x] == INK_PIXEL)
                    != (bitmap->bits[y * bitmap->width + 4 + ga.cx + x] == INK_PIXEL);
    assert(differ > 4);
    dc_bitmap_destroy(bitmap);
}

static void check_opaque(void)
{
    struct DcBitmap *bitmap = canvas(80, 30);
    COLORREF paper = RGB(1, 2, 3);
    SIZE box = extent(1, 12, 0, 0, "Hi", 2);
    int x, y;
    draw(bitmap, 1, 12, 0, 5, 6, "Hi", 2, &paper);
    for (y = 0; y < bitmap->height; ++y)
        for (x = 0; x < bitmap->width; ++x) {
            DWORD pixel = bitmap->bits[y * bitmap->width + x];
            if (x >= 5 && x < 5 + box.cx && y >= 6 && y < 6 + box.cy)
                assert(pixel == INK_PIXEL || pixel == 0x00010203u);
            else
                assert(pixel == BACKGROUND);
        }
    assert(count_ink(bitmap, 0, bitmap->width) > 5);
    dc_bitmap_destroy(bitmap);
}

/* The Latin face (Arial, or a stand-in for it) has no Hangul, so a Hangul run is measured and drawn
   in the Hangul fallback font (Gulim, then Malgun Gothic, then the free Korean fonts), as GDI does. */
static void check_runs(void)
{
    static const char mixed[] = "AB\xea\xb0\x80\xeb\x82\x98";
    SIZE runs, latin = extent(1, 12, 0, 0, "AB", 2), korean = extent(1, 12, 0, 0, hangul, 6);
    FontHandle face = font_cache_get_font(&g_fontCache, 1, 12, CHARSET_ANSI, 0, 0);
    FontHandle fallback = font_cache_get_script_font(&g_fontCache, TEXT_SCRIPT_HANGUL, 12, 0, 0);
    assert(face && !font_covers(face, TEXT_SCRIPT_HANGUL));
    assert(fallback && font_covers(fallback, TEXT_SCRIPT_HANGUL) && font_ascent(fallback) > 0);
    assert(fallback == font_cache_get_script_font(&g_fontCache, TEXT_SCRIPT_HANGUL, 12, 0, 0));
    assert(!font_covers(face, TEXT_SCRIPT_EMOJI));
    runs = extent(1, 12, 0, 0, mixed, 8);
    assert(runs.cx == latin.cx + korean.cx);
    assert(runs.cy == (latin.cy > korean.cy ? latin.cy : korean.cy));
}

static void check_italic(void)
{
    SIZE upright = extent(1, 12, 0, 0, "Hello", 5), slanted = extent(1, 12, 0, 1, "Hello", 5);
    assert(slanted.cx > 0 && slanted.cy == upright.cy);
}

static long freetype_width(const char *file, int ppem, const char *text)
{
    FT_Library library;
    FT_Face face;
    long width = 0;
    assert(!FT_Init_FreeType(&library) && !FT_New_Face(library, file, 0, &face));
    assert(!FT_Set_Pixel_Sizes(face, 0, (FT_UInt)ppem));
    for (; *text; ++text) {
        assert(!FT_Load_Char(face, (unsigned char)*text, FT_LOAD_TARGET_MONO));
        width += (face->glyph->advance.x + 32) >> 6;
    }
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    return width;
}

/* The first file of a sans-serif face, as the font finder sorts the installed fonts. */
static void sans_file(char *out, size_t size)
{
    static const char *const families[] = {"Arial", "Liberation Sans", "DejaVu Sans", "Noto Sans", NULL};
    struct FontRequest request = {families, "en", 0, 0, 0};
    struct FontMatches *matches = font_match_sort(&request);
    int i, index;
    assert(matches);
    *out = 0;
    for (i = 0; i < font_match_count(matches) && !*out; ++i) {
        const char *file = font_match_file(matches, i, &index);
        if (file && !index && strlen(file) < size)
            strcpy(out, file);
    }
    font_match_free(matches);
    assert(*out);
    fprintf(stderr, "sans-serif -> %s\n", out);
}

static void check_override(void)
{
    char file[4096];
    struct DcBitmap *bitmap;
    int plain;
    long width;
    sans_file(file, sizeof file);
    width = freetype_width(file, 13, "Hello");
    set_variable("RAGEXE_FONT_7", file);
    set_variable("RAGEXE_FONT", "/nonexistent/font.ttf");
    assert(extent(7, -13, 0, 0, "Hello", 5).cx == width);
    assert(extent(7, -13, 1, 0, "Hello", 5).cx == width + 5);
    bitmap = canvas(160, 40);
    plain = draw(bitmap, 7, -13, 0, 4, 4, "Hello", 5, NULL);
    dc_bitmap_destroy(bitmap);
    bitmap = canvas(160, 40);
    assert(draw(bitmap, 7, -13, 1, 4, 4, "Hello", 5, NULL) > plain);
    dc_bitmap_destroy(bitmap);
    assert(extent(8, 12, 0, 0, "Hello", 5).cx > 0);
    set_variable("RAGEXE_FONT_7", "");
    set_variable("RAGEXE_FONT", file);
    assert(extent(9, -13, 0, 0, "Hello", 5).cx == width);
    set_variable("RAGEXE_FONT", "");
}

/* The overlay's fixed-pitch font: Lucida Console where it is installed, otherwise a fixed-pitch
   stand-in. */
static void check_monospace(void)
{
    FontHandle font = font_cache_get_fixed(&g_fontCache, "Lucida Console", -12);
    uint16_t wide[] = {'W', 'W'}, thin[] = {'i', 'i'};
    SIZE a, b;
    assert(font && font == font_cache_get_fixed(&g_fontCache, "Lucida Console", -12));
    font_text_extent(font, wide, 2, 0, &a);
    font_text_extent(font, thin, 2, 0, &b);
    assert(a.cx == b.cx && a.cx > 0);
}

/* The plain text of the serial number: the selected face alone, measured as drawn. */
static void check_plain(void)
{
    struct DcBitmap *bitmap = canvas(200, 40);
    struct TextDC dc;
    SIZE plain, runs;
    int x, y, inked = 0;
    text_dc_construct(&dc, bitmap);
    text_dc_select_font(&dc, 0, 12, 0, 0);
    text_dc_text_extent_plain(&dc, "XX-XX", 5, &plain);
    text_dc_text_extent(&dc, "XX-XX", 5, &runs);
    assert(plain.cx == runs.cx && plain.cy == runs.cy && plain.cx > 0);
    text_dc_set_text_color(&dc, INK);
    text_dc_text_out_plain(&dc, 1, 1, "XX-XX", 5);
    text_dc_text_out_plain(&dc, 1, 1, NULL, 5);
    text_dc_destruct(&dc);
    for (y = 0; y < bitmap->height; ++y)
        for (x = 0; x < bitmap->width; ++x)
            inked += bitmap->bits[y * bitmap->width + x] == INK_PIXEL;
    assert(inked > 10);
    dc_bitmap_destroy(bitmap);
}

int main(void)
{
    fonts_init();
    check_latin();
    check_korean();
    check_opaque();
    check_runs();
    check_italic();
    check_override();
    check_monospace();
    check_plain();
    fonts_shutdown();
    puts("FreeType text on the system's fonts: extents, glyph placement, Korean, opaque cells, script runs, "
         "overrides, the fixed-pitch font and plain text passed.");
    return 0;
}
