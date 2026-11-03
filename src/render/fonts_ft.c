#ifndef _WIN32
#include "render/fonts.h"
#include "render/device.h"
#include "platform/code_page.h"
#include "core/client_path.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H
#include FT_TRUETYPE_TAGS_H
#include "render/font_match.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define FONT_SCDREAM_BOLD "system\\font\\SCDream6.otf"
#define FONT_SCDREAM      "system\\font\\SCDream4.otf"
#define FONT_EOT_HEIGHT   (-13)
#define FONT_PATH_MAX     0x800
#define GLYPH_PAGE_SIZE   256
#define GLYPH_PAGES       256
#define FALLBACK_FACES    8
#define OBLIQUE_SHEAR     0x0366a

struct Glyph {
    unsigned char *mask;
    int left, top, width, rows, advance, black_box;
    unsigned char loaded;
};

struct GlyphPage {
    struct Glyph glyphs[GLYPH_PAGE_SIZE];
};

struct Fallback {
    FT_Face face;
    int candidate;
};

struct CharsetFonts {
    int charset;
    const char *lang;
    const char *const *families;
};

struct Font {
    int face, size, charset;
    BYTE bold, italic;
    const char *family;
    const struct CharsetFonts *fonts;
    FT_Face primary;
    int primary_candidate, fallback_count;
    struct Fallback fallbacks[FALLBACK_FACES];
    struct FontMatches *candidates;
    int ppem, ascent, descent, embolden, oblique, em_cell, own_hints;
    struct GlyphPage *pages[GLYPH_PAGES];
};

struct FontCache g_fontCache;
static FT_Library library;

static const char *const latin_families[] = {"Arial", "Liberation Sans", "DejaVu Sans", "Roboto", NULL};
static const char *const korean_families[] = {
    "Gulim", "NanumGothic", "Noto Sans CJK KR", "Noto Sans KR", "UnDotum", "Baekmuk Gulim", NULL,
};
static const char *const japanese_families[] = {"MS Gothic", "Noto Sans CJK JP", "IPAGothic", "VL Gothic", NULL};
static const char *const simplified_chinese_families[] = {
    "SimSun", "Noto Sans CJK SC", "WenQuanYi Zen Hei", "WenQuanYi Micro Hei", NULL,
};
static const char *const traditional_chinese_families[] = {"MingLiU", "Noto Sans CJK TC", "AR PL UMing TW", NULL};
static const char *const thai_families[] = {"Tahoma", "Noto Sans Thai", "Loma", NULL};

static const struct CharsetFonts charset_fonts[] = {
    {CHARSET_ANSI, "en", latin_families},
    {CHARSET_HANGUL, "ko", korean_families},
    {CHARSET_SHIFTJIS, "ja", japanese_families},
    {CHARSET_GB2312, "zh-cn", simplified_chinese_families},
    {CHARSET_CHINESEBIG5, "zh-tw", traditional_chinese_families},
    {CHARSET_THAI, "th", thai_families},
    {CHARSET_VIETNAMESE, "vi", latin_families},
    {CHARSET_RUSSIAN, "ru", latin_families},
    {CHARSET_GREEK, "el", latin_families},
    {CHARSET_TURKISH, "tr", latin_families},
    {CHARSET_HEBREW, "he", latin_families},
    {CHARSET_ARABIC, "ar", latin_families},
    {CHARSET_BALTIC, "lt", latin_families},
    {CHARSET_EASTEUROPE, "pl", latin_families},
};

static const struct CharsetFonts *fonts_for_charset(int charset)
{
    size_t i;
    for (i = 0; i < sizeof charset_fonts / sizeof charset_fonts[0]; ++i)
        if (charset_fonts[i].charset == charset)
            return &charset_fonts[i];
    return &charset_fonts[0];
}

static const char *face_family(int face)
{
    if (face < 0 || (size_t)face >= sizeof g_3dDevice.faceNamesB / sizeof g_3dDevice.faceNamesB[0])
        return NULL;
    return g_3dDevice.faceNamesB[face];
}

static const char *font_variable(const char *name)
{
    const char *path = getenv(name);
    return path && *path ? path : NULL;
}

static int is_cjk(int charset)
{
    return charset == CHARSET_HANGUL || charset == CHARSET_SHIFTJIS || charset == CHARSET_GB2312
        || charset == CHARSET_CHINESEBIG5;
}

static FT_Face open_face(const char *path, int index)
{
    FT_Face face;
    return FT_New_Face(library, path, index, &face) ? NULL : face;
}

static FT_Face open_candidate(const struct FontMatches *candidates, int i)
{
    int index;
    const char *file = font_match_file(candidates, i, &index);
    return file ? open_face(file, index) : NULL;
}

static FT_Face open_matched(struct Font *font, int bold, int italic)
{
    int i, count = font_match_count(font->candidates), is_bold, is_slanted;
    FT_Face face;
    for (i = 0; i < count; ++i)
        if (font_match_covers_language(font->candidates, i, font->fonts->lang))
            break;
    if (i == count)
        i = 0;
    face = open_candidate(font->candidates, i);
    if (!face)
        return NULL;
    font->primary_candidate = i;
    font_match_style(font->candidates, i, &is_bold, &is_slanted);
    font->embolden = bold && !is_bold;
    font->oblique = italic && !is_slanted;
    return face;
}

static FT_Face open_file(struct Font *font, const char *path, int bold, int italic)
{
    FT_Face face = open_face(path, 0);
    if (!face)
        return NULL;
    font->embolden = bold && !(face->style_flags & FT_STYLE_FLAG_BOLD);
    font->oblique = italic && !(face->style_flags & FT_STYLE_FLAG_ITALIC);
    return face;
}

static FT_Face open_scdream(struct Font *font, int bold, int italic)
{
    char path[FONT_PATH_MAX];
    FT_Face face;
    if (!client_path(path, sizeof path, "%s", bold ? FONT_SCDREAM_BOLD : FONT_SCDREAM))
        return NULL;
    face = open_face(path, 0);
    if (face)
        font->oblique = italic && !(face->style_flags & FT_STYLE_FLAG_ITALIC);
    return face;
}

static int round_scaled(long value, long numerator, long denominator)
{
    return (int)((2 * value * numerator + denominator) / (2 * denominator));
}

static int select_bitmap_size(FT_Face face, int height)
{
    int i, best = 0;
    for (i = 1; i < face->num_fixed_sizes; ++i)
        if (abs(face->available_sizes[i].height - abs(height)) < abs(face->available_sizes[best].height - abs(height)))
            best = i;
    return FT_Select_Size(face, best) == 0;
}

static unsigned read_u16(const FT_Byte *bytes)
{
    return (unsigned)bytes[0] << 8 | bytes[1];
}

static int read_s16(const FT_Byte *bytes)
{
    unsigned value = read_u16(bytes);
    return value < 0x8000 ? (int)value : (int)value - 0x10000;
}

static size_t vdmx_group(const FT_Byte *table, FT_ULong length)
{
    size_t ratios = read_u16(table + 4), i;
    for (i = 0; i < ratios && 6 + 6 * ratios <= length; ++i) {
        const FT_Byte *ratio = table + 6 + 4 * i;
        if ((!ratio[1] && !ratio[2] && !ratio[3]) || (ratio[1] == 1 && ratio[2] <= 1 && ratio[3] >= 1))
            return read_u16(table + 6 + 4 * ratios + 2 * i);
    }
    return 0;
}

static int vdmx_metrics(struct Font *font, FT_Face face, int height)
{
    FT_ULong length = 0;
    FT_Byte *table;
    size_t group, records, i;
    int found = 0;
    if (FT_Load_Sfnt_Table(face, TTAG_VDMX, 0, NULL, &length) || length < 6 || !(table = malloc(length)))
        return 0;
    if (!FT_Load_Sfnt_Table(face, TTAG_VDMX, 0, table, &length) && (group = vdmx_group(table, length))
        && group + 4 <= length) {
        records = read_u16(table + group);
        for (i = 0; i < records && group + 10 + 6 * i <= length; ++i) {
            const FT_Byte *record = table + group + 4 + 6 * i;
            int ppem = (int)read_u16(record), top = read_s16(record + 2), bottom = read_s16(record + 4);
            if (height > 0 ? top - bottom == height : ppem == -height) {
                font->ppem = ppem;
                font->ascent = top;
                font->descent = -bottom;
                found = 1;
                break;
            }
        }
    }
    free(table);
    return found;
}

static int apply_size(struct Font *font, FT_Face face)
{
    if (FT_IS_SCALABLE(face))
        return FT_Set_Pixel_Sizes(face, 0, (FT_UInt)font->ppem) == 0;
    return face->num_fixed_sizes > 0 && select_bitmap_size(face, font->ppem);
}

static int set_height(struct Font *font, int height)
{
    FT_Face face = font->primary;
    TT_OS2 *os2 = FT_Get_Sfnt_Table(face, FT_SFNT_OS2);
    long ascent = face->ascender, descent = -face->descender, em = face->units_per_EM;
    if (!FT_IS_SCALABLE(face)) {
        if (!face->num_fixed_sizes || !select_bitmap_size(face, height))
            return 0;
        font->ppem = face->size->metrics.y_ppem;
        font->ascent = (int)((face->size->metrics.ascender + 32) >> 6);
        font->descent = (int)((-face->size->metrics.descender + 32) >> 6);
        return 1;
    }
    if (os2 && os2->version != 0xffff && os2->usWinAscent + os2->usWinDescent) {
        ascent = os2->usWinAscent;
        descent = os2->usWinDescent;
    }
    if (!height)
        height = 16;
    if (font->em_cell && height > 0) {
        font->ppem = height;
        font->ascent = round_scaled(ascent, height, ascent + descent);
        font->descent = height - font->ascent;
    } else if (!vdmx_metrics(font, face, height)) {
        long scale = height > 0 ? height : -height, units = height > 0 ? ascent + descent : em;
        font->ppem = height > 0 ? round_scaled(height, em, units) : -height;
        font->ascent = round_scaled(ascent, scale, units);
        font->descent = round_scaled(descent, scale, units);
    }
    if (font->ppem < 1)
        font->ppem = 1;
    return apply_size(font, face);
}

static void slant_face(const struct Font *font, FT_Face face)
{
    FT_Matrix shear = {0x10000, OBLIQUE_SHEAR, 0, 0x10000};
    if (font->oblique)
        FT_Set_Transform(face, &shear, NULL);
}

static void font_free(struct Font *font)
{
    int i, j;
    for (i = 0; i < GLYPH_PAGES; ++i) {
        if (!font->pages[i])
            continue;
        for (j = 0; j < GLYPH_PAGE_SIZE; ++j)
            free(font->pages[i]->glyphs[j].mask);
        free(font->pages[i]);
    }
    for (i = 0; i < font->fallback_count; ++i)
        FT_Done_Face(font->fallbacks[i].face);
    if (font->primary)
        FT_Done_Face(font->primary);
    font_match_free(font->candidates);
    free(font);
}

static struct Font *font_open(int face, const char *family, int size, int charset, int bold, int italic)
{
    struct Font *font = calloc(1, sizeof *font);
    const char *path, *const *fallbacks = NULL;
    char name[32];
    int height = size;
    if (!font)
        return NULL;
    font->face = face;
    font->family = family;
    font->size = size;
    font->charset = charset;
    font->bold = (BYTE)bold;
    font->italic = (BYTE)italic;
    font->fonts = fonts_for_charset(charset);
    font->primary_candidate = -1;
    if (face >= FONT_FACE_EOT_FIRST && face < FONT_FACE_SCDREAM) {
        height = FONT_EOT_HEIGHT;
        bold = italic = 0;
    }
    if (!family) {
        family = face_family(face);
        fallbacks = font->fonts->families;
    }
    font->candidates = font_match_sort(family, fallbacks, font->fonts->lang, bold, italic);
    snprintf(name, sizeof name, "RAGEXE_FONT_%d", face);
    if (face >= 0 && (path = font_variable(name)))
        font->primary = open_file(font, path, bold, italic);
    if (!font->primary && face == FONT_FACE_SCDREAM)
        font->primary = open_scdream(font, bold, italic);
    if (font->primary && face == FONT_FACE_SCDREAM)
        height = -size;
    if (!font->primary && face >= 0 && (path = font_variable("RAGEXE_FONT")))
        font->primary = open_file(font, path, bold, italic);
    font->own_hints = font->primary != NULL;
    if (!font->primary && font->candidates) {
        font->primary = open_matched(font, bold, italic);
        font->own_hints = font->primary && family && font->primary->family_name
                          && !strcasecmp(family, font->primary->family_name);
    }
    if (font->primary && face >= 0 && is_cjk(charset))
        font->em_cell = !family || !font->primary->family_name || strcasecmp(family, font->primary->family_name);
    if (!font->primary || !set_height(font, height)) {
        font_free(font);
        return NULL;
    }
    slant_face(font, font->primary);
    return font;
}

static int is_open(const struct Font *font, int candidate)
{
    int i;
    if (candidate == font->primary_candidate)
        return 1;
    for (i = 0; i < font->fallback_count; ++i)
        if (font->fallbacks[i].candidate == candidate)
            return 1;
    return 0;
}

static FT_Face open_fallback(struct Font *font, uint16_t unit, FT_UInt *index)
{
    int i;
    if (unit < 0x20 || (unit >= 0xd800 && unit <= 0xdfff) || !font->candidates
        || font->fallback_count == FALLBACK_FACES)
        return NULL;
    for (i = 0; i < font_match_count(font->candidates); ++i) {
        FT_Face face;
        if (is_open(font, i) || !font_match_has_char(font->candidates, i, unit))
            continue;
        face = open_candidate(font->candidates, i);
        if (!face)
            continue;
        if (!apply_size(font, face) || !(*index = FT_Get_Char_Index(face, unit))) {
            FT_Done_Face(face);
            continue;
        }
        slant_face(font, face);
        font->fallbacks[font->fallback_count].face = face;
        font->fallbacks[font->fallback_count++].candidate = i;
        return face;
    }
    return NULL;
}

static FT_Face glyph_face(struct Font *font, uint16_t unit, FT_UInt *index)
{
    FT_Face face;
    int i;
    if ((*index = FT_Get_Char_Index(font->primary, unit)))
        return font->primary;
    for (i = 0; i < font->fallback_count; ++i)
        if ((*index = FT_Get_Char_Index(font->fallbacks[i].face, unit)))
            return font->fallbacks[i].face;
    face = open_fallback(font, unit, index);
    if (face)
        return face;
    *index = 0;
    return font->primary;
}

static int bitmap_bit(const FT_Bitmap *bitmap, int x, int y)
{
    const unsigned char *row = bitmap->buffer + (bitmap->pitch >= 0 ? (long)y * bitmap->pitch
                                                 : (long)(bitmap->rows - 1 - y) * -bitmap->pitch);
    switch (bitmap->pixel_mode) {
    case FT_PIXEL_MODE_MONO:
        return (row[x >> 3] >> (7 - (x & 7))) & 1;
    case FT_PIXEL_MODE_GRAY:
        return row[x] >= 0x80;
    case FT_PIXEL_MODE_BGRA:
        return row[x * 4 + 3] >= 0x80;
    default:
        return 0;
    }
}

static void store_mask(struct Glyph *glyph, const FT_GlyphSlot slot, int embolden)
{
    const FT_Bitmap *bitmap = &slot->bitmap;
    int left = (int)bitmap->width, right = -1, top = (int)bitmap->rows, bottom = -1, x, y;
    for (y = 0; y < (int)bitmap->rows; ++y)
        for (x = 0; x < (int)bitmap->width; ++x)
            if (bitmap_bit(bitmap, x, y)) {
                left = x < left ? x : left;
                right = x > right ? x : right;
                top = y < top ? y : top;
                bottom = y;
            }
    if (right < 0)
        return;
    glyph->width = right - left + 1 + embolden;
    glyph->rows = bottom - top + 1;
    glyph->left = slot->bitmap_left + left;
    glyph->top = slot->bitmap_top - top;
    glyph->mask = calloc((size_t)glyph->width * (size_t)glyph->rows, 1);
    if (!glyph->mask)
        return;
    for (y = top; y <= bottom; ++y)
        for (x = left; x <= right; ++x)
            if (bitmap_bit(bitmap, x, y)) {
                glyph->mask[(y - top) * glyph->width + x - left] = 1;
                if (embolden)
                    glyph->mask[(y - top) * glyph->width + x - left + 1] = 1;
            }
}

static void load_glyph(struct Font *font, uint16_t unit, struct Glyph *glyph)
{
    FT_UInt index;
    FT_Face face = glyph_face(font, unit, &index);
    FT_GlyphSlot slot = face->glyph;
    int embedded;
    FT_Int32 flags = FT_LOAD_TARGET_MONO;
    glyph->loaded = 1;
    if (face != font->primary || !font->own_hints)
        flags |= FT_LOAD_FORCE_AUTOHINT;
    if (font->oblique)
        flags |= FT_LOAD_NO_BITMAP;
    if (FT_Load_Glyph(face, index, flags))
        return;
    glyph->advance = (int)((slot->advance.x + 32) >> 6) + font->embolden;
    embedded = slot->format == FT_GLYPH_FORMAT_BITMAP;
    if (!embedded && FT_Render_Glyph(slot, FT_RENDER_MODE_MONO))
        return;
    store_mask(glyph, slot, font->embolden);
    glyph->black_box = embedded && glyph->width ? glyph->left + glyph->width
                     : slot->bitmap_left + (slot->bitmap.width ? (int)slot->bitmap.width : 1) + font->embolden;
}

static const struct Glyph *font_glyph(struct Font *font, uint16_t unit)
{
    struct GlyphPage **page = &font->pages[unit >> 8];
    struct Glyph *glyph;
    if (!*page && !(*page = calloc(1, sizeof **page)))
        return NULL;
    glyph = &(*page)->glyphs[unit & (GLYPH_PAGE_SIZE - 1)];
    if (!glyph->loaded)
        load_glyph(font, unit, glyph);
    return glyph;
}

void font_text_extent(FontHandle font, const uint16_t *text, int length, int black_box, SIZE *out)
{
    long width = 0;
    int i;
    for (i = 0; i < length; ++i) {
        const struct Glyph *glyph = font_glyph(font, text[i]);
        if (glyph)
            width += black_box ? glyph->black_box : glyph->advance;
    }
    out->cx = (LONG)width;
    out->cy = font->ascent + font->descent;
}

static DWORD colour_pixel(COLORREF colour)
{
    return (DWORD)GetRValue(colour) << 16 | (DWORD)GetGValue(colour) << 8 | GetBValue(colour);
}

static void fill_box(const struct FontTarget *target, int x, int y, int width, int height, DWORD pixel)
{
    int left = x < 0 ? 0 : x, top = y < 0 ? 0 : y;
    int right = x + width < target->width ? x + width : target->width;
    int bottom = y + height < target->height ? y + height : target->height;
    for (; top < bottom; ++top) {
        DWORD *row = (DWORD *)(target->bits + (size_t)top * (size_t)target->pitch);
        int column;
        for (column = left; column < right; ++column)
            row[column] = pixel;
    }
}

static void draw_glyph(const struct FontTarget *target, const struct Glyph *glyph, int x, int y, DWORD pixel)
{
    int row, column;
    for (row = 0; row < glyph->rows; ++row) {
        const unsigned char *mask = glyph->mask + (size_t)row * (size_t)glyph->width;
        DWORD *line;
        if (y + row < 0 || y + row >= target->height)
            continue;
        line = (DWORD *)(target->bits + (size_t)(y + row) * (size_t)target->pitch);
        for (column = 0; column < glyph->width; ++column)
            if (mask[column] && x + column >= 0 && x + column < target->width)
                line[x + column] = pixel;
    }
}

void font_text_out(FontHandle font, const struct FontTarget *target, int x, int y, const uint16_t *text, int length,
                   COLORREF colour, const COLORREF *background)
{
    DWORD pixel = colour_pixel(colour);
    int baseline = y + font->ascent, i;
    if (background) {
        SIZE extent;
        font_text_extent(font, text, length, 0, &extent);
        fill_box(target, x, y, extent.cx, extent.cy, colour_pixel(*background));
    }
    for (i = 0; i < length; ++i) {
        const struct Glyph *glyph = font_glyph(font, text[i]);
        if (!glyph)
            continue;
        if (glyph->mask)
            draw_glyph(target, glyph, x + glyph->left, baseline - glyph->top, pixel);
        x += glyph->advance;
    }
}

static int reserve_font(struct FontCache *self)
{
    size_t capacity;
    struct Font **fonts;
    if (self->count < self->capacity)
        return 1;
    capacity = self->capacity ? self->capacity * 2 : 8;
    fonts = realloc(self->fonts, capacity * sizeof *fonts);
    if (!fonts)
        return 0;
    self->fonts = fonts;
    self->capacity = capacity;
    return 1;
}

static FontHandle cached_font(struct FontCache *self, int face, const char *family, int size, int charset,
                              int bold, int italic)
{
    struct Font *font;
    size_t i;
    for (i = 0; i < self->count; ++i) {
        font = self->fonts[i];
        if (font->face == face && font->size == size && font->charset == charset && font->bold == (BYTE)bold
            && font->italic == (BYTE)italic
            && (font->family == family || (font->family && family && !strcmp(font->family, family))))
            return font;
    }
    if (!library || !reserve_font(self))
        return NULL;
    font = font_open(face, family, size, charset, bold, italic);
    if (font)
        self->fonts[self->count++] = font;
    return font;
}

FontHandle font_cache_get_font(struct FontCache *self, int face, int size, int charset, int bold, int italic)
{
    return cached_font(self, face, NULL, size, charset, bold, italic);
}

FontHandle font_cache_get_family(struct FontCache *self, const char *family, int size)
{
    return cached_font(self, -1, family, size, CHARSET_ANSI, 0, 0);
}

struct FontCache *font_cache_construct(struct FontCache *self)
{
    memset(self, 0, sizeof(*self));
    if (!library && FT_Init_FreeType(&library))
        library = NULL;
    font_match_init();
    return self;
}

void font_cache_destruct(struct FontCache *self)
{
    size_t i;
    for (i = 0; i < self->count; ++i)
        font_free(self->fonts[i]);
    free(self->fonts);
    self->fonts = NULL;
    self->count = self->capacity = 0;
    if (library)
        FT_Done_FreeType(library);
    library = NULL;
}

void fonts_shutdown(void)
{
    font_cache_destruct(&g_fontCache);
}

void fonts_init(void)
{
    font_cache_construct(&g_fontCache);
    atexit(fonts_shutdown);
}
#endif
