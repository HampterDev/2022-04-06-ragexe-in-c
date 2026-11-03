#include "render/fonts.h"
#include "render/device.h"
#include "render/font_family.h"
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define FONT_SCDREAM_BOLD L"system\\font\\SCDream6.otf"
#define FONT_SCDREAM      L"system\\font\\SCDream4.otf"

/* Coverage of a script by a font: not checked yet, has the script's sample glyph, lacks it. */
enum { COVERAGE_UNKNOWN, COVERAGE_YES, COVERAGE_NO };

struct FontEntry {
    int face, size, charset, script, ascent;
    BYTE bold, italic;
    HFONT font;
    const wchar_t *resource_path;
    BYTE coverage[TEXT_SCRIPT_COUNT];
};

struct FontCache g_fontCache;

/* Windows fonts tried in order for text the selected face cannot draw. Names are listed
   in English and in their own language, because Windows reports either: 굴림 is Gulim,
   맑은 고딕 Malgun Gothic, ＭＳ ゴシック MS Gothic, ＭＳ Ｐゴシック MS PGothic, 宋体 SimSun. */
#define GULIM_NAMES  L"Gulim", L"\xad74\xb9bc"
#define MALGUN_NAMES L"Malgun Gothic", L"\xb9d1\xc740 \xace0\xb515"
static const wchar_t *const hangul_faces[] = { GULIM_NAMES, MALGUN_NAMES, NULL };
static const wchar_t *const han_faces[] = {
    GULIM_NAMES, L"MS Gothic", L"\xff2d\xff33 \x30b4\x30b7\x30c3\x30af", L"SimSun", L"\x5b8b\x4f53",
    L"Microsoft YaHei", MALGUN_NAMES, NULL
};
static const wchar_t *const kana_faces[] = {
    L"MS PGothic", L"\xff2d\xff33 \xff30\x30b4\x30b7\x30c3\x30af", L"MS Gothic", L"Meiryo", L"Yu Gothic",
    GULIM_NAMES, MALGUN_NAMES, NULL
};
static const wchar_t *const thai_faces[] = { L"Tahoma", L"Leelawadee UI", NULL };
static const wchar_t *const symbol_faces[] = { GULIM_NAMES, MALGUN_NAMES, L"Segoe UI Symbol", NULL };
static const wchar_t *const emoji_faces[] = { L"Segoe UI Emoji", L"Segoe UI Symbol", NULL };

/* The fallback faces of each script, and a character the selected face must have to draw
   the script itself; 0 when the game's faces never have the script. */
static const struct {
    const wchar_t *const *faces;
    WCHAR sample;
} scripts[TEXT_SCRIPT_COUNT] = {
    [TEXT_SCRIPT_HANGUL] = { hangul_faces, 0xd55c },
    [TEXT_SCRIPT_HAN] = { han_faces, 0x4e2d },
    [TEXT_SCRIPT_KANA] = { kana_faces, 0x3042 },
    [TEXT_SCRIPT_THAI] = { thai_faces, 0x0e01 },
    [TEXT_SCRIPT_SYMBOL] = { symbol_faces, 0x2605 },
    [TEXT_SCRIPT_EMOJI] = { emoji_faces, 0 },
};

struct FontSearch {
    const wchar_t *name;
    int found;
};

static int CALLBACK font_enum_match(const LOGFONTW *font, const TEXTMETRICW *metric,
                                    DWORD type, LPARAM context)
{
    struct FontSearch *search = (struct FontSearch *)context;
    (void)metric;
    (void)type;
    search->found = _wcsicmp(search->name, font->lfFaceName) == 0;
    return !search->found;
}

/* Whether a font family called name, compared without case, exists in charset. */
static int face_installed(HDC dc, const wchar_t *name, int charset)
{
    LOGFONTW lf = {0};
    struct FontSearch search = {0};

    if (!name)
        return 0;
    lf.lfCharSet = (BYTE)charset;
    search.name = name;
    EnumFontFamiliesExW(dc, &lf, font_enum_match, (LPARAM)&search, 0);
    return search.found;
}

static const wchar_t *installed_face_name(int face, int charset)
{
    const wchar_t *name = NULL;
    HDC dc;

    if (face < 0 || (size_t)face >= sizeof(g_3dDevice.faceNamesA) / sizeof(g_3dDevice.faceNamesA[0]))
        return NULL;
    dc = GetDC(NULL);
    if (!dc)
        return NULL;
    if (face_installed(dc, g_3dDevice.faceNamesA[face], charset))
        name = g_3dDevice.faceNamesA[face];
    else if (face_installed(dc, g_3dDevice.faceNamesB[face], charset))
        name = g_3dDevice.faceNamesB[face];
    ReleaseDC(NULL, dc);
    return name;
}

static void eot_logfont(LOGFONTW *lf, const wchar_t *name)
{
    memset(lf, 0, sizeof(*lf));
    lf->lfWeight = 400;
    lf->lfCharSet = 1;
    lf->lfOutPrecision = 6;
    lf->lfQuality = 2;
    wcscpy(lf->lfFaceName, name);
    lf->lfHeight = -MulDiv(13, 72, 72);
}

static const wchar_t *const eot_names[] = {
    L"RixSquirrel_10.eot", L"RixLoveangel_10.eot", L"RixNHCgogo_10.eot", L"RixDiary_10.eot",
    L"RixMiniHeart_10.eot", L"RixFreshman_10.eot", L"RixKid_10.eot", L"RixMagic_10.eot", L"RixJJangu_10.eot",
};

static HFONT file_font(const struct FontEntry *entry, int backend_active)
{
    LOGFONTW lf = {0};
    lf.lfHeight = -MulDiv(entry->size, 72, 72);
    lf.lfWeight = entry->bold ? 700 : 400;
    lf.lfItalic = entry->italic;
    lf.lfCharSet = (BYTE)entry->charset;
    if (!backend_active || !font_file_read_family(entry->resource_path, lf.lfFaceName, LF_FACESIZE))
        lf.lfFaceName[0] = 0;
    return CreateFontIndirectW(&lf);
}

static int reserve_font_entry(struct FontCache *self)
{
    size_t capacity, limit = SIZE_MAX / sizeof(*self->entries);
    struct FontEntry *entries;
    if (self->count < self->capacity)
        return 1;
    if (self->capacity == limit)
        return 0;
    capacity = self->capacity > limit / 2 ? limit : self->capacity * 2;
    if (!capacity)
        capacity = 8;
    entries = realloc(self->entries, capacity * sizeof(*entries));
    if (!entries)
        return 0;
    self->entries = entries;
    self->capacity = capacity;
    return 1;
}

static HFONT plain_font(const struct FontEntry *entry, const wchar_t *name)
{
    return CreateFontW(entry->size, 0, 0, 0, entry->bold ? 700 : 400, entry->italic != 0, 0, 0,
                       (DWORD)entry->charset, 0, 0, NONANTIALIASED_QUALITY, 0, name);
}

static HFONT cached_font(const struct FontCache *self, int face, int size, int charset, int script,
                         int bold, int italic)
{
    size_t i;
    for (i = 0; i < self->count; ++i) {
        const struct FontEntry *it = &self->entries[i];
        if (it->face == face && it->size == size && it->charset == charset && it->script == script
            && it->bold == (BYTE)bold && it->italic == (BYTE)italic)
            return it->font;
    }
    return NULL;
}

HFONT font_cache_get_font(struct FontCache *self, int face, int size, int charset,
                       int bold, int italic)
{
    struct FontEntry entry = {0};
    HFONT font = cached_font(self, face, size, charset, TEXT_SCRIPT_DEFAULT, bold, italic);
    LOGFONTW lf;
    if (font)
        return font;
    if (!reserve_font_entry(self))
        return NULL;
    entry.face = face;
    entry.size = size;
    entry.charset = charset;
    entry.bold = (BYTE)bold;
    entry.italic = (BYTE)italic;
    if (face >= FONT_FACE_EOT_FIRST && face < FONT_FACE_SCDREAM) {
        eot_logfont(&lf, eot_names[face - FONT_FACE_EOT_FIRST]);
        entry.font = CreateFontIndirectW(&lf);
    } else {
        if (face == FONT_FACE_SCDREAM) {
            const wchar_t *path = entry.bold == 1 ? FONT_SCDREAM_BOLD : FONT_SCDREAM;
            if (AddFontResourceExW(path, FR_PRIVATE, 0) > 0) {
                entry.resource_path = path;
                entry.font = file_font(&entry, self->token != 0);
            }
        }
        if (!entry.resource_path)
            entry.font = plain_font(&entry, installed_face_name(face, charset));
    }
    if (!entry.font) {
        if (entry.resource_path)
            RemoveFontResourceExW(entry.resource_path, FR_PRIVATE, 0);
        return NULL;
    }
    self->entries[self->count++] = entry;
    return entry.font;
}

/* The first installed fallback face of script, looked up once per cache: fallback holds
   0 before the lookup, then the face's index plus one, or -1 when none is installed. */
static const wchar_t *fallback_face(struct FontCache *self, int script)
{
    const wchar_t *const *faces = scripts[script].faces;
    HDC dc;
    int i;

    if (!self->fallback[script]) {
        dc = GetDC(NULL);
        if (!dc)
            return NULL;
        self->fallback[script] = -1;
        for (i = 0; faces[i]; ++i)
            if (face_installed(dc, faces[i], DEFAULT_CHARSET)) {
                self->fallback[script] = (signed char)(i + 1);
                break;
            }
        ReleaseDC(NULL, dc);
    }
    return self->fallback[script] > 0 ? faces[self->fallback[script] - 1] : NULL;
}

HFONT font_cache_get_script_font(struct FontCache *self, int script, int size, int bold, int italic)
{
    struct FontEntry entry = {0};
    const wchar_t *name;
    HFONT font;

    if (script <= TEXT_SCRIPT_DEFAULT || script >= TEXT_SCRIPT_COUNT)
        return NULL;
    font = cached_font(self, FONT_FACE_FALLBACK, size, DEFAULT_CHARSET, script, bold, italic);
    if (font)
        return font;
    name = fallback_face(self, script);
    if (!name || !reserve_font_entry(self))
        return NULL;
    entry.face = FONT_FACE_FALLBACK;
    entry.size = size;
    entry.charset = DEFAULT_CHARSET;
    entry.script = script;
    entry.bold = (BYTE)bold;
    entry.italic = (BYTE)italic;
    entry.font = plain_font(&entry, name);
    if (!entry.font)
        return NULL;
    self->entries[self->count++] = entry;
    return entry.font;
}

static struct FontEntry *find_entry(struct FontCache *self, HFONT font)
{
    size_t i;
    for (i = 0; i < self->count; ++i)
        if (self->entries[i].font == font)
            return &self->entries[i];
    return NULL;
}

int font_cache_covers(struct FontCache *self, HDC dc, HFONT font, int script)
{
    struct FontEntry *entry;

    if (script <= TEXT_SCRIPT_DEFAULT || script >= TEXT_SCRIPT_COUNT || !(entry = find_entry(self, font)))
        return 1;
    if (!scripts[script].sample)
        return 0;
    if (entry->coverage[script] == COVERAGE_UNKNOWN) {
        HGDIOBJ previous = SelectObject(dc, font);
        WORD glyph = 0xffff;
        DWORD found;

        if (!previous || previous == HGDI_ERROR)
            return 1;
        found = GetGlyphIndicesW(dc, &scripts[script].sample, 1, &glyph, GGI_MARK_NONEXISTING_GLYPHS);
        SelectObject(dc, previous);
        if (found != 1)
            return 1;
        entry->coverage[script] = glyph != 0xffff ? COVERAGE_YES : COVERAGE_NO;
    }
    return entry->coverage[script] == COVERAGE_YES;
}

int font_cache_ascent(struct FontCache *self, HDC dc, HFONT font)
{
    struct FontEntry *entry = find_entry(self, font);

    if (!entry)
        return 0;
    if (!entry->ascent) {
        HGDIOBJ previous = SelectObject(dc, font);
        TEXTMETRICW metric;
        BOOL measured;

        if (!previous || previous == HGDI_ERROR)
            return 0;
        measured = GetTextMetricsW(dc, &metric);
        SelectObject(dc, previous);
        if (!measured || metric.tmAscent <= 0)
            return 0;
        entry->ascent = metric.tmAscent;
    }
    return entry->ascent;
}

struct FontCache *font_cache_construct(struct FontCache *self)
{
    memset(self, 0, sizeof(*self));
    font_backend_start(&self->token);
    return self;
}

void font_cache_destruct(struct FontCache *self)
{
    size_t i;
    for (i = 0; i < self->count; ++i) {
        struct FontEntry *entry = &self->entries[i];
        DeleteObject(entry->font);
        if (entry->resource_path)
            RemoveFontResourceExW(entry->resource_path, FR_PRIVATE, 0);
    }
    free(self->entries);
    self->entries = NULL;
    self->count = self->capacity = 0;
    memset(self->fallback, 0, sizeof(self->fallback));
    font_backend_stop(self->token);
    self->token = 0;
}

void gdiplus_destroy(void)
{
    font_cache_destruct(&g_fontCache);
}

void gdiplus_init(void)
{
    font_cache_construct(&g_fontCache);
    atexit(gdiplus_destroy);
}
