#include "render/fonts.h"
#include "render/device.h"
#include "render/font_family.h"
#include "core/text_encoding.h"
#include <stdlib.h>
#include <string.h>
#include "core/cstr.h"

#define FONT_SCDREAM_BOLD "system\\font\\SCDream6.otf"
#define FONT_SCDREAM      "system\\font\\SCDream4.otf"

struct FontEntry {
    int face, size, charset;
    BYTE bold, italic;
    HFONT font;
    const char *resource_path;
};

struct FontCache g_fontCache;

struct FontSearch {
    const char *name;
    int found;
};

static int CALLBACK font_enum_match(const LOGFONTA *font, const TEXTMETRICA *metric,
                                    DWORD type, LPARAM context)
{
    struct FontSearch *search = (struct FontSearch *)context;
    (void)metric;
    (void)type;
    search->found = cstr_compare_nocase(search->name, font->lfFaceName) == 0;
    return !search->found;
}

static const char *installed_face_name(int face, int charset)
{
    LOGFONTA lf = {0};
    HDC dc;
    struct FontSearch search = {0};
    if (face < 0 || (size_t)face >= sizeof(g_3dDevice.faceNamesA) / sizeof(g_3dDevice.faceNamesA[0]))
        return NULL;
    dc = GetDC(NULL);
    if (!dc)
        return NULL;
    lf.lfCharSet = (BYTE)charset;
    search.name = g_3dDevice.faceNamesA[face];
    if (search.name)
        EnumFontFamiliesExA(dc, &lf, font_enum_match, (LPARAM)&search, 0);
    if (!search.found) {
        search.name = g_3dDevice.faceNamesB[face];
        if (search.name)
            EnumFontFamiliesExA(dc, &lf, font_enum_match, (LPARAM)&search, 0);
    }
    ReleaseDC(NULL, dc);
    return search.found ? search.name : NULL;
}

static void eot_logfont(LOGFONTA *lf, const char *name)
{
    memset(lf, 0, sizeof(*lf));
    lf->lfWeight = 400;
    lf->lfCharSet = 1;
    lf->lfOutPrecision = 6;
    lf->lfQuality = 2;
    strcpy(lf->lfFaceName, name);
    lf->lfHeight = -13;
}

static const char *const eot_names[] = {
    "RixSquirrel_10.eot", "RixLoveangel_10.eot", "RixNHCgogo_10.eot", "RixDiary_10.eot",
    "RixMiniHeart_10.eot", "RixFreshman_10.eot", "RixKid_10.eot", "RixMagic_10.eot", "RixJJangu_10.eot",
};

static char *font_file_family_name(const char *path)
{
    wchar_t name[LF_FACESIZE];
    wchar_t *wide_path = encoding_to_wide(CP_THREAD_ACP, path);
    char *text = NULL;
    if (!wide_path)
        return NULL;
    if (font_file_read_family(wide_path, name, LF_FACESIZE))
        text = encoding_from_wide(CP_THREAD_ACP, name);
    free(wide_path);
    return text;
}

static HFONT file_font(const struct FontEntry *entry, int backend_active)
{
    LOGFONTA lf = {0};
    char *name = backend_active ? font_file_family_name(entry->resource_path) : NULL;
    lf.lfHeight = -entry->size;
    lf.lfWeight = entry->bold ? 700 : 400;
    lf.lfItalic = entry->italic;
    lf.lfCharSet = (BYTE)entry->charset;
    if (name) {
        cstr_copy(lf.lfFaceName, sizeof(lf.lfFaceName), name);
        free(name);
    }
    return CreateFontIndirectA(&lf);
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

HFONT font_cache_get_font(struct FontCache *self, int face, int size, int charset,
                       int bold, int italic)
{
    struct FontEntry entry = {0};
    size_t i;
    LOGFONTA lf;
    for (i = 0; i < self->count; ++i) {
        const struct FontEntry *it = &self->entries[i];
        if (it->face == face && it->size == size && it->charset == charset
            && it->bold == (BYTE)bold && it->italic == (BYTE)italic)
            return it->font;
    }
    if (!reserve_font_entry(self))
        return NULL;
    entry.face = face;
    entry.size = size;
    entry.charset = charset;
    entry.bold = (BYTE)bold;
    entry.italic = (BYTE)italic;
    if (face >= FONT_FACE_EOT_FIRST && face < FONT_FACE_SCDREAM) {
        eot_logfont(&lf, eot_names[face - FONT_FACE_EOT_FIRST]);
        entry.font = CreateFontIndirectA(&lf);
    } else {
        if (face == FONT_FACE_SCDREAM) {
            const char *path = entry.bold == 1 ? FONT_SCDREAM_BOLD : FONT_SCDREAM;
            if (AddFontResourceExA(path, FR_PRIVATE, 0) > 0) {
                entry.resource_path = path;
                entry.font = file_font(&entry, self->token != 0);
            }
        }
        if (!entry.resource_path)
            entry.font = CreateFontA(size, 0, 0, 0, entry.bold ? 700 : 400,
                                    entry.italic != 0, 0, 0, charset, 0, 0,
                                    NONANTIALIASED_QUALITY, 0, installed_face_name(face, charset));
    }
    if (!entry.font) {
        if (entry.resource_path)
            RemoveFontResourceExA(entry.resource_path, FR_PRIVATE, 0);
        return NULL;
    }
    self->entries[self->count++] = entry;
    return entry.font;
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
            RemoveFontResourceExA(entry->resource_path, FR_PRIVATE, 0);
    }
    free(self->entries);
    self->entries = NULL;
    self->count = self->capacity = 0;
    font_backend_stop(self->token);
    self->token = 0;
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
