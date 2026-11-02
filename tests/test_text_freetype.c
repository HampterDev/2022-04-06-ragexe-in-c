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
#include "diag/perf.h"
#include <assert.h>
#include <fontconfig/fontconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct C3dDevice g_3dDevice = {
    .faceNamesB = {"Gulim", "Arial", "MS Gothic", "SimSun", "MingLiU", "Tahoma",
                   "Arial", "Verdana", "Verdana", "Arial", "Arial", "Tahoma"},
    .faceCharsets = {CHARSET_HANGUL, CHARSET_ANSI, CHARSET_SHIFTJIS, CHARSET_GB2312, CHARSET_CHINESEBIG5,
                     CHARSET_THAI, CHARSET_VIETNAMESE, CHARSET_ANSI, CHARSET_ANSI, CHARSET_RUSSIAN,
                     CHARSET_VIETNAMESE + 1, CHARSET_ARABIC},
};
struct Language *g_language;
struct Renderer *g_renderer;
struct perf_counters g_perf;
int g_languageIndex, g_serviceType;
unsigned int g_codePage = 949;
unsigned char g_useCharsetTag;
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

static DWORD surface_pixels[64 * 32];
static int surface_refs, surface_locks, surface_unlocks;
static struct gfx_surface *const test_surface = (struct gfx_surface *)(uintptr_t)8;

void gfx_addref(gfx_surface *surface) { assert(surface == test_surface); ++surface_refs; }
void gfx_release(gfx_surface *surface) { assert(surface == test_surface && surface_refs > 0); --surface_refs; }
int gfx_lock(gfx_surface *surface, const RECT *r, int flags, struct gfx_lock *out)
{
    assert(surface == test_surface && !r && flags == GFX_LOCK_WAIT);
    ++surface_locks;
    out->bits = surface_pixels;
    out->pitch = 64 * 4;
    out->width = 64;
    out->height = 32;
    out->bpp = 32;
    return 0;
}
int gfx_unlock(gfx_surface *surface) { assert(surface == test_surface); ++surface_unlocks; return 0; }

#define BACKGROUND 0x77777777u
#define INK RGB(0x12, 0x34, 0x56)
#define INK_PIXEL 0x00123456u

static const char hangul[] = "\260\241\263\252\264\331";

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
    text_dc_from_surface(&dc, NULL);
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
            int inside = px >= x && px < x + box.cx && py >= y && py < y + box.cy;
            assert(pixel == BACKGROUND || (pixel == INK_PIXEL && inside));
        }
}

static void check_latin(void)
{
    struct DcBitmap *bitmap = canvas(160, 40);
    SIZE whole = extent(1, 12, 0, 0, "Hello", 5), head = extent(1, 12, 0, 0, "Hel", 3), tail = extent(1, 12, 0, 0, "lo", 2);
    SIZE bold = extent(1, 12, 1, 0, "Hello", 5);
    int plain, heavy;
    assert(whole.cx > 10 && whole.cx < 80 && whole.cy >= 10 && whole.cy <= 16);
    assert(head.cx + tail.cx == whole.cx && head.cy == whole.cy);
    assert(bold.cx >= whole.cx && bold.cy == whole.cy);
    assert(extent(1, 0x18, 0, 0, "Hello", 5).cx > whole.cx);
    plain = draw(bitmap, 1, 12, 0, 7, 9, "Hello", 5, NULL);
    assert(plain > 20);
    check_inside(bitmap, 7, 9, whole);
    dc_bitmap_destroy(bitmap);
    bitmap = canvas(160, 40);
    heavy = draw(bitmap, 1, 12, 1, 7, 9, "Hello", 5, NULL);
    assert(heavy > plain);
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
    SIZE all = extent(0, 12, 0, 0, hangul, 6), ga = extent(0, 12, 0, 0, hangul, 2), na = extent(0, 12, 0, 0, hangul + 2, 2);
    int x, y, differ = 0;
    assert(all.cx == 3 * ga.cx && ga.cx == na.cx && ga.cx >= 8 && ga.cx <= 16);
    assert(draw(bitmap, 0, 12, 0, 4, 2, hangul, 6, NULL) > 30);
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

static void check_tags(void)
{
    SIZE tagged, latin = extent(1, 12, 0, 0, "AB", 2), korean = extent(0, 12, 0, 0, hangul, 4);
    g_useCharsetTag = 1;
    tagged = extent(1, 12, 0, 0, "AB|81\260\241\263\252", 9);
    g_useCharsetTag = 0;
    assert(tagged.cx == latin.cx + korean.cx);
    assert(tagged.cy == (latin.cy > korean.cy ? latin.cy : korean.cy));
}

static void check_italic(void)
{
    SIZE upright = extent(1, 12, 0, 0, "Hello", 5), slanted = extent(1, 12, 0, 1, "Hello", 5);
    assert(slanted.cx > 0 && slanted.cy == upright.cy);
}

static void check_surface(void)
{
    struct TextDC dc;
    SIZE size;
    int i, set = 0;
    text_dc_from_surface(&dc, test_surface);
    text_dc_select_font(&dc, 1, 12, 0, 0);
    text_dc_text_extent(&dc, "Hi", 2, &size);
    assert(surface_refs == 1 && !surface_locks);
    text_dc_set_text_color(&dc, INK);
    text_dc_text_out_plain(&dc, 2, 2, "Hi", 2);
    text_dc_text_out(&dc, 2, 16, "Hi", 2);
    assert(surface_locks == 1 && !surface_unlocks);
    text_dc_destruct(&dc);
    assert(!surface_refs && surface_unlocks == 1);
    for (i = 0; i < 64 * 32; ++i)
        set += surface_pixels[i] == INK_PIXEL;
    assert(set > 10);
}

static void check_override(void)
{
    FcPattern *pattern = FcNameParse((const FcChar8 *)"DejaVu Sans"), *match;
    FcResult result;
    FcChar8 *file;
    SIZE named, chosen;
    FontHandle font;
    uint16_t text[] = {'H', 'e', 'l', 'l', 'o'};
    FcConfigSubstitute(NULL, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);
    match = FcFontMatch(NULL, pattern, &result);
    assert(match && FcPatternGetString(match, FC_FILE, 0, &file) == FcResultMatch);
    setenv("RAGEXE_FONT_7", (const char *)file, 1);
    chosen = extent(7, 13, 0, 0, "Hello", 5);
    font = font_cache_get_family(&g_fontCache, "DejaVu Sans", 13);
    assert(font);
    font_text_extent(font, text, 5, 0, &named);
    assert(named.cx == chosen.cx && named.cy == chosen.cy);
    unsetenv("RAGEXE_FONT_7");
    FcPatternDestroy(match);
    FcPatternDestroy(pattern);
}

static void check_monospace(void)
{
    FontHandle font = font_cache_get_family(&g_fontCache, "monospace", -12);
    uint16_t wide[] = {'W', 'W'}, thin[] = {'i', 'i'};
    SIZE a, b;
    assert(font && font == font_cache_get_family(&g_fontCache, "monospace", -12));
    font_text_extent(font, wide, 2, 0, &a);
    font_text_extent(font, thin, 2, 0, &b);
    assert(a.cx == b.cx && a.cx > 0);
}

static void check_triangle(void)
{
    static const char *const expected[] = {
        ".............#",
        "............##",
        "...........###",
        "...........###",
        "..........####",
        ".........#####",
        "........######",
        ".........#####",
        "..........####",
        "...........###",
        "...........###",
        "............##",
        ".............#",
    };
    struct DcBitmap *bitmap = canvas(20, 30);
    POINT points[3] = {{8, 0x12}, {0xe, 0xa}, {0xe, 0x1a}};
    POINT clipped[3] = {{-5, -5}, {40, 10}, {-5, 40}};
    int x, y;
    dc_bitmap_fill_triangle(bitmap, points, INK);
    assert(bitmap->changed == 1);
    for (y = 0; y < bitmap->height; ++y)
        for (x = 0; x < bitmap->width; ++x) {
            int row = y - 12, want = row >= 0 && row < 13 && x < 14 && expected[row][x] == '#';
            assert((bitmap->bits[y * bitmap->width + x] == INK_PIXEL) == want);
            assert(want || bitmap->bits[y * bitmap->width + x] == BACKGROUND);
        }
    dc_bitmap_fill_triangle(bitmap, clipped, INK);
    assert(bitmap->bits[0] == INK_PIXEL && bitmap->bits[19] == BACKGROUND);
    dc_bitmap_destroy(bitmap);
}

int main(void)
{
    fonts_init();
    check_latin();
    check_korean();
    check_opaque();
    check_tags();
    check_italic();
    check_surface();
    check_override();
    check_monospace();
    check_triangle();
    fonts_shutdown();
    FcFini();
    puts("FreeType text: extents, glyph placement, Korean, opaque cells, charset tags, surfaces, overrides and triangles passed.");
    return 0;
}
