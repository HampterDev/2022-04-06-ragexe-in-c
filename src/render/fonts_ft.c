#include "render/fonts.h"
#include "render/device.h"
#include "render/font_match.h"
#include "platform/code_page.h"
#include "core/client_path.h"
#include "core/cstr.h"
#include "core/utf8.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H
#include FT_TRUETYPE_TAGS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FONT_SCDREAM_BOLD "system\\font\\SCDream6.otf"
#define FONT_SCDREAM      "system\\font\\SCDream4.otf"
#define FONT_EOT_HEIGHT   (-13)
#define FONT_PATH_MAX     0x800
#define GLYPH_PAGE_SIZE   256
/* Pages cover every code point up to U+10FFFF; text arrives as UTF-16, with surrogate pairs. */
#define GLYPH_PAGES       0x1100
#define FALLBACK_FACES    8
#define REQUEST_FAMILIES  16
#define OBLIQUE_SHEAR     0x0366a

struct Glyph {
    unsigned char *mask;
    int left, top, width, rows, advance, black_box;
    unsigned char loaded;
};

struct GlyphPage {
    struct Glyph glyphs[GLYPH_PAGE_SIZE];
};

/* A face glyphs come from: the font's own, or a fallback for the characters it lacks. Its own
   TrueType instructions hint its glyphs, as GDI's did, or FreeType's autohinter does. */
struct Source {
    FT_Face face;
    int candidate, own_hints;
};

/* The families a request asks for: first the Windows families GDI looked up by name (the first
   `windows` of them), then the free fonts that stand in for them on other platforms. em_cell: the
   Windows families have an em as tall as their cell (Gulim, MS Gothic, SimSun, MingLiU), so a
   stand-in asked for a cell height gets that height as its em, as its Windows family would. */
struct Families {
    const char *const *names;
    int windows, em_cell;
};

struct CharsetFonts {
    int charset;
    const char *lang;
    struct Families families;
};

struct Font {
    int face, size, charset, script;
    BYTE bold, italic;
    const char *family;
    const char *lang;
    struct Source primary;
    int fallback_count;
    struct Source fallbacks[FALLBACK_FACES];
    struct FontMatches *candidates;
    int windows, ppem, ascent, descent, embolden, oblique, em_cell;
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

/* The families for a face's charset after the face's own names: the charset's Windows family, the
   one GDI's font mapper gives text of that charset when the face is not installed, then its
   stand-ins. */
static const struct CharsetFonts charset_fonts[] = {
    {CHARSET_ANSI, "en", {latin_families, 1, 0}},
    {CHARSET_HANGUL, "ko", {korean_families, 1, 1}},
    {CHARSET_SHIFTJIS, "ja", {japanese_families, 1, 1}},
    {CHARSET_GB2312, "zh-cn", {simplified_chinese_families, 1, 1}},
    {CHARSET_CHINESEBIG5, "zh-tw", {traditional_chinese_families, 1, 1}},
    {CHARSET_THAI, "th", {thai_families, 1, 0}},
    {CHARSET_VIETNAMESE, "vi", {latin_families, 1, 0}},
    {CHARSET_RUSSIAN, "ru", {latin_families, 1, 0}},
    {CHARSET_GREEK, "el", {latin_families, 1, 0}},
    {CHARSET_TURKISH, "tr", {latin_families, 1, 0}},
    {CHARSET_HEBREW, "he", {latin_families, 1, 0}},
    {CHARSET_ARABIC, "ar", {latin_families, 1, 0}},
    {CHARSET_BALTIC, "lt", {latin_families, 1, 0}},
    {CHARSET_EASTEUROPE, "pl", {latin_families, 1, 0}},
};

/* The fallback fonts of each script (render/text_runs.h), for text the selected face cannot draw:
   the Windows fonts GDI tried in order, then the free fonts that stand in for them elsewhere; with
   the language the rest are sorted by and a character the selected face must have to draw the
   script itself (0 when the client's faces never do). */
static const char *const hangul_script_families[] = {
    "Gulim", "Malgun Gothic", "NanumGothic", "Noto Sans CJK KR", "Noto Sans KR", "UnDotum", "Baekmuk Gulim", NULL,
};
static const char *const han_script_families[] = {
    "Gulim", "MS Gothic", "SimSun", "Microsoft YaHei", "Malgun Gothic", "Noto Sans CJK KR", "Noto Sans CJK SC",
    "WenQuanYi Zen Hei", NULL,
};
static const char *const kana_script_families[] = {
    "MS PGothic", "MS Gothic", "Meiryo", "Yu Gothic", "Gulim", "Malgun Gothic", "Noto Sans CJK JP", "IPAGothic",
    "VL Gothic", NULL,
};
static const char *const thai_script_families[] = {"Tahoma", "Leelawadee UI", "Noto Sans Thai", "Loma", NULL};
static const char *const symbol_script_families[] = {
    "Gulim", "Malgun Gothic", "Segoe UI Symbol", "NanumGothic", "Noto Sans CJK KR", "DejaVu Sans", "Noto Sans Symbols2",
    NULL,
};
static const char *const emoji_script_families[] = {
    "Segoe UI Emoji", "Segoe UI Symbol", "Noto Color Emoji", "Noto Emoji", "Symbola", NULL,
};

static const struct {
    const char *lang;
    struct Families families;
    unsigned sample;
} script_fonts[TEXT_SCRIPT_COUNT] = {
    [TEXT_SCRIPT_HANGUL] = {"ko", {hangul_script_families, 2, 1}, 0xd55c},
    [TEXT_SCRIPT_HAN] = {"ko", {han_script_families, 5, 1}, 0x4e2d},
    [TEXT_SCRIPT_KANA] = {"ja", {kana_script_families, 6, 1}, 0x3042},
    [TEXT_SCRIPT_THAI] = {"th", {thai_script_families, 2, 0}, 0x0e01},
    [TEXT_SCRIPT_SYMBOL] = {"ko", {symbol_script_families, 3, 1}, 0x2605},
    [TEXT_SCRIPT_EMOJI] = {"und-zsye", {emoji_script_families, 2, 0}, 0},
};

/* After a fixed-pitch family the caller names: Courier New, which GDI's font mapper gives a
   fixed-pitch request whose family is not installed, then its stand-ins. */
static const char *const fixed_family_names[] = {
    "Courier New", "DejaVu Sans Mono", "Liberation Mono", "Noto Sans Mono", "Droid Sans Mono", NULL,
};
static const struct Families fixed_families = {fixed_family_names, 1, 0};

static const struct CharsetFonts *fonts_for_charset(int charset)
{
    size_t i;
    for (i = 0; i < sizeof charset_fonts / sizeof charset_fonts[0]; ++i)
        if (charset_fonts[i].charset == charset)
            return &charset_fonts[i];
    return &charset_fonts[0];
}

/* A face's family name from one of the device's UTF-16 face tables, as UTF-8 in out. */
static const char *face_name(const wchar_t *const *table, int face, char *out, size_t size)
{
    const wchar_t *name;
    size_t used = 0;
    if (face < 0 || face >= (int)(sizeof g_3dDevice.faceNamesB / sizeof g_3dDevice.faceNamesB[0])
        || !(name = table[face]) || !size)
        return NULL;
    for (; *name; ++name) {
        char bytes[UTF8_CHAR_MAX];
        size_t length = utf8_encode((unsigned long)*name, bytes);
        if (!length || used + length >= size)
            return NULL;
        memcpy(out + used, bytes, length);
        used += length;
    }
    out[used] = 0;
    return out;
}

static const char *font_variable(const char *name)
{
    const char *path = getenv(name);
    return path && *path ? path : NULL;
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

/* Whether candidate i is a face of one of the request's Windows families. */
static int windows_candidate(const struct Font *font, int i)
{
    int asked = font_match_family(font->candidates, i);
    return asked >= 0 && asked < font->windows;
}

/* A script's fallback font is the first of its Windows families that is installed, as GDI's font
   cache took it whatever it covered (the candidates come in the families' order). Otherwise, and
   for a face, the first candidate that covers the request's language, as GDI's charset wins over
   the name; the first of all when none does. */
static FT_Face open_matched(struct Font *font, int bold, int italic)
{
    int i = 0, count = font_match_count(font->candidates), is_bold, is_slanted;
    FT_Face face;
    if (font->script == TEXT_SCRIPT_DEFAULT || !windows_candidate(font, 0)) {
        for (i = 0; i < count; ++i)
            if (font_match_covers_language(font->candidates, i, font->lang))
                break;
        if (i == count)
            i = 0;
    }
    face = open_candidate(font->candidates, i);
    if (!face)
        return NULL;
    font->primary.candidate = i;
    font->primary.own_hints = windows_candidate(font, i);
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

static int round_scaled(long long value, long long numerator, long long denominator)
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

/* The VDMX group for a square pixel. GDI reads only the groups for the Windows ANSI subset (a
   character set of 1): Tahoma's, made for all its glyphs, it leaves alone. */
static size_t vdmx_group(const FT_Byte *table, FT_ULong length)
{
    size_t ratios = read_u16(table + 4), i;
    for (i = 0; i < ratios && 6 + 6 * ratios <= length; ++i) {
        const FT_Byte *ratio = table + 6 + 4 * i;
        if (!ratio[0])
            continue;
        if ((!ratio[1] && !ratio[2] && !ratio[3]) || (ratio[1] == 1 && ratio[2] <= 1 && ratio[3] >= 1))
            return read_u16(table + 6 + 4 * ratios + 2 * i);
    }
    return 0;
}

/* The size and its ascent and descent from the face's VDMX table, as GDI takes them: for a cell
   height, the first size whose glyphs are that tall, or else the size before the first that is
   taller; for an em height, that size's record. 0 when the table has no answer. */
static int vdmx_metrics(struct Font *font, FT_Face face, int height)
{
    FT_ULong length = 0;
    FT_Byte *table;
    size_t group, records, i;
    const FT_Byte *found = NULL;
    if (FT_Load_Sfnt_Table(face, TTAG_VDMX, 0, NULL, &length) || length < 6 || !(table = malloc(length)))
        return 0;
    if (!FT_Load_Sfnt_Table(face, TTAG_VDMX, 0, table, &length) && (group = vdmx_group(table, length))
        && group + 4 <= length) {
        records = read_u16(table + group);
        for (i = 0; i < records && group + 10 + 6 * i <= length && !found; ++i) {
            const FT_Byte *record = table + group + 4 + 6 * i;
            int tall = read_s16(record + 2) - read_s16(record + 4);
            if (height > 0 ? tall == height : (int)read_u16(record) == -height)
                found = record;
            else if (height > 0 && tall > height)
                break;
        }
        if (!found && height > 0 && i > 0 && i < records)
            found = table + group + 4 + 6 * (i - 1);
    }
    if (found) {
        font->ppem = (int)read_u16(found);
        font->ascent = read_s16(found + 2);
        font->descent = -read_s16(found + 4);
    }
    free(table);
    return found != NULL;
}

static int apply_size(struct Font *font, FT_Face face)
{
    if (FT_IS_SCALABLE(face))
        return FT_Set_Pixel_Sizes(face, 0, (FT_UInt)font->ppem) == 0;
    return face->num_fixed_sizes > 0 && select_bitmap_size(face, font->ppem);
}

/* GDI's size from the face's Windows ascent and descent in units of an em of em: for a cell height,
   the largest em whose ascent and descent, each rounded, fit in the cell, and the cell's ascent
   rounded with the descent the rest; for an em height, that em with its ascent and descent
   rounded. */
static void scaled_metrics(struct Font *font, int height, long ascent, long descent, long em)
{
    int most;
    if (height < 0) {
        font->ppem = -height;
        font->ascent = round_scaled(ascent, -height, em);
        font->descent = round_scaled(descent, -height, em);
        return;
    }
    font->ppem = round_scaled(height, em, ascent + descent);
    most = font->ppem * 2 + 2;
    while (font->ppem > 1 && round_scaled(ascent, font->ppem, em) + round_scaled(descent, font->ppem, em) > height)
        --font->ppem;
    while (font->ppem < most
           && round_scaled(ascent, font->ppem + 1, em) + round_scaled(descent, font->ppem + 1, em) <= height)
        ++font->ppem;
    font->ascent = round_scaled(ascent, height, ascent + descent);
    font->descent = height - font->ascent;
}

/* GDI's CreateFont heights: a positive height is the cell, sized by the face's VDMX table or its
   OS/2 Windows ascent and descent (a stand-in for a family whose em is its cell takes the height as
   its em); a negative height is the em; 0 is 16 pixels of cell. */
static int set_height(struct Font *font, int height)
{
    FT_Face face = font->primary.face;
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
    /* a font's metrics are data from outside: none, or nonsense, make the em the cell */
    if (em <= 0)
        return 0;
    if (ascent < 0 || descent < 0 || ascent + descent <= 0) {
        ascent = em;
        descent = 0;
    }
    if (!height)
        height = 16;
    if (font->em_cell && height > 0) {
        font->ppem = height;
        font->ascent = round_scaled(ascent, height, ascent + descent);
        font->descent = height - font->ascent;
    } else if (!vdmx_metrics(font, face, height)) {
        scaled_metrics(font, height, ascent, descent, em);
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
    if (font->primary.face)
        FT_Done_Face(font->primary.face);
    font_match_free(font->candidates);
    free(font);
}

/* Whether a font file the player chose is of one of the request's Windows families. */
static int windows_file(const char *const *names, int windows, FT_Face face)
{
    int i;
    for (i = 0; i < windows && face->family_name; ++i)
        if (!cstr_compare_nocase(names[i], face->family_name))
            return 1;
    return 0;
}

/* A face's font (script TEXT_SCRIPT_DEFAULT), a script's fallback font (face FONT_FACE_FALLBACK)
   or a fixed-pitch family's (face -1), asked for as GDI asked: a face by its name in the display
   language (render/device.h's faceNamesA), then its English one (faceNamesB). */
static struct Font *font_open(int face, const char *family, int size, int charset, int script, int bold, int italic)
{
    struct Font *font = calloc(1, sizeof *font);
    const char *names[REQUEST_FAMILIES], *path;
    const struct Families *list;
    struct FontRequest request = {names, NULL, 0, 0, 0};
    char variable[32], local_name[64], english_name[64];
    int height = size, count = 0, i;
    if (!font)
        return NULL;
    font->face = face;
    font->family = family;
    font->size = size;
    font->charset = charset;
    font->script = script;
    font->bold = (BYTE)bold;
    font->italic = (BYTE)italic;
    font->lang = fonts_for_charset(charset)->lang;
    font->primary.candidate = -1;
    if (face >= FONT_FACE_EOT_FIRST && face < FONT_FACE_SCDREAM) {
        height = FONT_EOT_HEIGHT;
        bold = italic = 0;
    }
    if (script != TEXT_SCRIPT_DEFAULT) {
        list = &script_fonts[script].families;
        font->lang = script_fonts[script].lang;
    } else if (family) {
        names[count++] = family;
        list = &fixed_families;
        request.fixed_pitch = 1;
    } else {
        if (face_name(g_3dDevice.faceNamesA, face, local_name, sizeof local_name))
            names[count++] = local_name;
        if (face_name(g_3dDevice.faceNamesB, face, english_name, sizeof english_name)
            && (!count || strcmp(local_name, english_name)))
            names[count++] = english_name;
        list = &fonts_for_charset(charset)->families;
    }
    font->windows = count + list->windows;
    for (i = 0; list->names[i] && count < REQUEST_FAMILIES - 1; ++i)
        names[count++] = list->names[i];
    names[count] = NULL;
    request.lang = font->lang;
    request.bold = bold;
    request.italic = italic;
    font->candidates = font_match_sort(&request);
    snprintf(variable, sizeof variable, "RAGEXE_FONT_%d", face);
    if (face >= 0 && (path = font_variable(variable)))
        font->primary.face = open_file(font, path, bold, italic);
    if (!font->primary.face && face == FONT_FACE_SCDREAM)
        font->primary.face = open_scdream(font, bold, italic);
    if (font->primary.face && face == FONT_FACE_SCDREAM)
        height = -size;
    if (!font->primary.face && face >= 0 && (path = font_variable("RAGEXE_FONT")))
        font->primary.face = open_file(font, path, bold, italic);
    if (font->primary.face) {
        font->primary.own_hints = 1;
        font->em_cell = list->em_cell && !windows_file(names, font->windows, font->primary.face);
    } else if (font->candidates && (font->primary.face = open_matched(font, bold, italic))) {
        font->em_cell = list->em_cell && !font->primary.own_hints;
    }
    if (!font->primary.face || !set_height(font, height)) {
        font_free(font);
        return NULL;
    }
    slant_face(font, font->primary.face);
    return font;
}

static int is_open(const struct Font *font, int candidate)
{
    int i;
    if (candidate == font->primary.candidate)
        return 1;
    for (i = 0; i < font->fallback_count; ++i)
        if (font->fallbacks[i].candidate == candidate)
            return 1;
    return 0;
}

/* The next candidate that has the character, at the font's size. */
static const struct Source *open_fallback(struct Font *font, unsigned unit, FT_UInt *index)
{
    int i;
    if (unit < 0x20 || (unit >= 0xd800 && unit <= 0xdfff) || !font->candidates
        || font->fallback_count == FALLBACK_FACES)
        return NULL;
    for (i = 0; i < font_match_count(font->candidates); ++i) {
        struct Source *source = &font->fallbacks[font->fallback_count];
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
        source->face = face;
        source->candidate = i;
        source->own_hints = windows_candidate(font, i);
        ++font->fallback_count;
        return source;
    }
    return NULL;
}

static const struct Source *glyph_source(struct Font *font, unsigned unit, FT_UInt *index)
{
    const struct Source *source;
    int i;
    if ((*index = FT_Get_Char_Index(font->primary.face, unit)))
        return &font->primary;
    for (i = 0; i < font->fallback_count; ++i)
        if ((*index = FT_Get_Char_Index(font->fallbacks[i].face, unit)))
            return &font->fallbacks[i];
    source = open_fallback(font, unit, index);
    if (source)
        return source;
    *index = 0;
    return &font->primary;
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

/* The advance GDI gives a glyph just loaded, in pixels: its hinted width, or its embedded bitmap's;
   but in a fixed-pitch face a glyph twice as wide as the average character advances twice the
   average's pixels, as GDI keeps a full-width character two half-width ones wide (MS Gothic's and
   SimSun's at odd sizes). */
static int gdi_advance(FT_Face face, FT_GlyphSlot slot, int ppem)
{
    TT_OS2 *os2 = FT_Get_Sfnt_Table(face, FT_SFNT_OS2);
    int advance = (int)((slot->advance.x + 32) >> 6);
    if (FT_IS_FIXED_WIDTH(face) && os2 && os2->version != 0xffff && os2->xAvgCharWidth > 0
        && face->units_per_EM > 0) {
        int average = round_scaled(os2->xAvgCharWidth, ppem, face->units_per_EM);
        if (average > 0 && advance == round_scaled(2L * os2->xAvgCharWidth, ppem, face->units_per_EM))
            advance = 2 * average;
    }
    return advance;
}

static void load_glyph(struct Font *font, unsigned unit, struct Glyph *glyph)
{
    FT_UInt index;
    const struct Source *source = glyph_source(font, unit, &index);
    FT_Face face = source->face;
    FT_GlyphSlot slot = face->glyph;
    int embedded;
    FT_Int32 flags = FT_LOAD_TARGET_MONO;
    glyph->loaded = 1;
    if (!source->own_hints)
        flags |= FT_LOAD_FORCE_AUTOHINT;
    if (font->oblique)
        flags |= FT_LOAD_NO_BITMAP;
    if (FT_Load_Glyph(face, index, flags))
        return;
    glyph->advance = gdi_advance(face, slot, font->ppem) + font->embolden;
    embedded = slot->format == FT_GLYPH_FORMAT_BITMAP;
    if (!embedded && FT_Render_Glyph(slot, FT_RENDER_MODE_MONO))
        return;
    store_mask(glyph, slot, font->embolden);
    glyph->black_box = embedded && glyph->width ? glyph->left + glyph->width
                     : slot->bitmap_left + (slot->bitmap.width ? (int)slot->bitmap.width : 1) + font->embolden;
}

static const struct Glyph *font_glyph(struct Font *font, unsigned unit)
{
    struct GlyphPage **page;
    struct Glyph *glyph;
    if (unit >= (unsigned)GLYPH_PAGES * GLYPH_PAGE_SIZE)
        return NULL;
    page = &font->pages[unit >> 8];
    if (!*page && !(*page = calloc(1, sizeof **page)))
        return NULL;
    glyph = &(*page)->glyphs[unit & (GLYPH_PAGE_SIZE - 1)];
    if (!glyph->loaded)
        load_glyph(font, unit, glyph);
    return glyph;
}

/* The code point at text[*i], a surrogate pair joined; *i ends on its last unit. */
static unsigned next_code_point(const uint16_t *text, int length, int *i)
{
    unsigned unit = text[*i];
    if (unit >= 0xd800 && unit <= 0xdbff && *i + 1 < length && text[*i + 1] >= 0xdc00 && text[*i + 1] <= 0xdfff) {
        unit = 0x10000 + ((unit - 0xd800) << 10) + (text[*i + 1] - 0xdc00u);
        ++*i;
    }
    return unit;
}

/* With black_box, as GDI measures italic text: each character to the end of its black box, a
   character outside the BMP by its advance, a surrogate pair measured whole. */
void font_text_extent(FontHandle font, const uint16_t *text, int length, int black_box, SIZE *out)
{
    long width = 0;
    int i;
    for (i = 0; i < length; ++i) {
        unsigned code = next_code_point(text, length, &i);
        const struct Glyph *glyph = font_glyph(font, code);
        if (glyph)
            width += black_box && code < 0x10000 ? glyph->black_box : glyph->advance;
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
        const struct Glyph *glyph = font_glyph(font, next_code_point(text, length, &i));
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
                              int script, int bold, int italic)
{
    struct Font *font;
    size_t i;
    for (i = 0; i < self->count; ++i) {
        font = self->fonts[i];
        if (font->face == face && font->size == size && font->charset == charset && font->script == script
            && font->bold == (BYTE)bold
            && font->italic == (BYTE)italic
            && (font->family == family || (font->family && family && !strcmp(font->family, family))))
            return font;
    }
    if (!library || !reserve_font(self))
        return NULL;
    font = font_open(face, family, size, charset, script, bold, italic);
    if (font)
        self->fonts[self->count++] = font;
    return font;
}

FontHandle font_cache_get_font(struct FontCache *self, int face, int size, int charset, int bold, int italic)
{
    return cached_font(self, face, NULL, size, charset, TEXT_SCRIPT_DEFAULT, bold, italic);
}

FontHandle font_cache_get_script_font(struct FontCache *self, int script, int size, int bold, int italic)
{
    if (script <= TEXT_SCRIPT_DEFAULT || script >= TEXT_SCRIPT_COUNT)
        return NULL;
    return cached_font(self, FONT_FACE_FALLBACK, NULL, size, CHARSET_DEFAULT, script, bold, italic);
}

FontHandle font_cache_get_fixed(struct FontCache *self, const char *family, int size)
{
    return cached_font(self, -1, family, size, CHARSET_ANSI, TEXT_SCRIPT_DEFAULT, 0, 0);
}

/* By the script's one sample character in the font's own face; emoji always fall back. */
int font_covers(FontHandle font, int script)
{
    if (!font || script <= TEXT_SCRIPT_DEFAULT || script >= TEXT_SCRIPT_COUNT)
        return 1;
    return script_fonts[script].sample && FT_Get_Char_Index(font->primary.face, script_fonts[script].sample) != 0;
}

int font_ascent(FontHandle font)
{
    return font ? font->ascent : 0;
}

static void font_cache_construct(struct FontCache *self)
{
    FT_Library opened;
    memset(self, 0, sizeof(*self));
    if (!library && !FT_Init_FreeType(&opened))
        library = opened;
    font_match_init(0);
}

static void font_cache_destruct(struct FontCache *self)
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
    font_match_shutdown();
}

void fonts_shutdown(void)
{
    font_cache_destruct(&g_fontCache);
}

void fonts_init(void)
{
    static int registered;
    font_cache_construct(&g_fontCache);
    if (!registered)
        registered = atexit(fonts_shutdown) == 0;
}
