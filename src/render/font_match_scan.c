#include "render/font_match.h"
#include "render/fonts.h"
#if defined(RAGE_FONT_SCAN) && !defined(TEXT_BACKEND_GDI)
#include "core/cstr.h"
#include "platform/filesystem.h"
#include "platform/thread.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The fonts are the faces in the font folders (RAGE_FONT_DIRS, separated by ':', on Windows by ';',
   or the system's), read when the font cache starts. A search sorts them as Fontconfig would for
   these requests: the asked families in order, then the faces that cover the language, then the
   closest slant and weight. */
#ifdef _WIN32
#define FOLDER_SEPARATOR ';'
#elif defined(__ANDROID__)
#define FOLDER_SEPARATOR ':'
#define SYSTEM_FONT_DIRS "/system/fonts:/product/fonts"
#elif defined(__APPLE__)
#define FOLDER_SEPARATOR ':'
#define SYSTEM_FONT_DIRS "/System/Library/Fonts:/Library/Fonts"
#else
#define FOLDER_SEPARATOR ':'
#define SYSTEM_FONT_DIRS "/usr/share/fonts:/usr/local/share/fonts"
#endif
#define FOLDER_DEPTH 4
#define WEIGHT_REGULAR 400
#define WEIGHT_BOLD 700
#define WEIGHT_DEMIBOLD 600
#define BMP_WORDS (0x10000 / 32)

struct Language {
    const char *lang;
    uint16_t probes[3];
};

/* The characters a face must have to count as covering a language. */
static const struct Language languages[] = {
    {"en", {0x0041, 0x007a}},
    {"ko", {0xac00, 0xd7a3}},
    {"ja", {0x3042, 0x30a2, 0x65e5}},
    {"zh-cn", {0x4e2d, 0x56fd}},
    {"zh-tw", {0x4e2d, 0x570b}},
    {"th", {0x0e01, 0x0e3f}},
    {"vi", {0x1ea0, 0x01b0}},
    {"ru", {0x0416, 0x044f}},
    {"el", {0x03a9, 0x03c9}},
    {"tr", {0x011f, 0x0131}},
    {"he", {0x05d0, 0x05ea}},
    {"ar", {0x0627, 0x064a}},
    {"lt", {0x0117, 0x012f}},
    {"pl", {0x0142, 0x017c}},
};

struct Face {
    char *file;
    int index;
    char *family;
    int weight, slanted;
    unsigned langs;
    uint32_t *coverage;
    int coverage_failed;
};

struct FontMatches {
    int count;
    int order[];
};

static FT_Library library;
static struct Face *faces;
static size_t face_count, face_capacity;
static int catalog_read;
static struct PlatformMutex *coverage_lock;

static char *copy_text(const char *text)
{
    size_t size = strlen(text) + 1;
    char *copy = malloc(size);
    return copy ? memcpy(copy, text, size) : NULL;
}

static int language_index(const char *lang)
{
    size_t i;
    for (i = 0; i < sizeof languages / sizeof languages[0]; ++i)
        if (!cstr_compare_nocase(languages[i].lang, lang))
            return (int)i;
    return -1;
}

static unsigned face_languages(FT_Face face)
{
    unsigned langs = 0;
    size_t i, j;
    for (i = 0; i < sizeof languages / sizeof languages[0]; ++i) {
        for (j = 0; j < 3 && languages[i].probes[j]; ++j)
            if (!FT_Get_Char_Index(face, languages[i].probes[j]))
                break;
        if (j == 3 || !languages[i].probes[j])
            langs |= 1u << i;
    }
    return langs;
}

static void add_face(const char *file, int index, FT_Face face)
{
    TT_OS2 *os2 = FT_Get_Sfnt_Table(face, FT_SFNT_OS2);
    struct Face *entry;
    if (!face->family_name || !face->charmap)
        return;
    if (face_count == face_capacity) {
        size_t capacity = face_capacity ? face_capacity * 2 : 256;
        struct Face *grown = realloc(faces, capacity * sizeof *grown);
        if (!grown)
            return;
        faces = grown;
        face_capacity = capacity;
    }
    entry = &faces[face_count];
    memset(entry, 0, sizeof *entry);
    entry->file = copy_text(file);
    entry->family = copy_text(face->family_name);
    if (!entry->file || !entry->family) {
        free(entry->file);
        free(entry->family);
        return;
    }
    entry->index = index;
    entry->weight = os2 && os2->version != 0xffff && os2->usWeightClass
                  ? os2->usWeightClass : (face->style_flags & FT_STYLE_FLAG_BOLD ? WEIGHT_BOLD : WEIGHT_REGULAR);
    entry->slanted = (face->style_flags & FT_STYLE_FLAG_ITALIC) != 0;
    entry->langs = face_languages(face);
    ++face_count;
}

static int is_font_file(const char *name)
{
    const char *dot = strrchr(name, '.');
    return dot && (!cstr_compare_nocase(dot, ".ttf") || !cstr_compare_nocase(dot, ".otf")
                   || !cstr_compare_nocase(dot, ".ttc") || !cstr_compare_nocase(dot, ".otc"));
}

static void add_file(const char *file)
{
    FT_Face face;
    FT_Long count, i;
    if (FT_New_Face(library, file, -1, &face))
        return;
    count = face->num_faces;
    FT_Done_Face(face);
    for (i = 0; i < count; ++i)
        if (!FT_New_Face(library, file, i, &face)) {
            add_face(file, (int)i, face);
            FT_Done_Face(face);
        }
}

struct Listing {
    char **names; /* each name after a byte that says whether it is a folder */
    size_t count, capacity;
};

static void list_entry(void *context, const char *name, int is_directory)
{
    struct Listing *listing = context;
    size_t size = strlen(name) + 2;
    char *entry;
    if (name[0] == '.')
        return;
    if (listing->count == listing->capacity) {
        size_t capacity = listing->capacity ? listing->capacity * 2 : 64;
        char **grown = realloc(listing->names, capacity * sizeof *grown);
        if (!grown)
            return;
        listing->names = grown;
        listing->capacity = capacity;
    }
    if (!(entry = malloc(size)))
        return;
    entry[0] = is_directory ? 'd' : 'f';
    memcpy(entry + 1, name, size - 1);
    listing->names[listing->count++] = entry;
}

static int compare_names(const void *a, const void *b)
{
    return strcmp(*(char *const *)a + 1, *(char *const *)b + 1);
}

static void scan_folder(const char *folder, int depth)
{
    struct Listing listing = {0};
    size_t i;
    if (!platform_native_list(folder, list_entry, &listing))
        return;
    qsort(listing.names, listing.count, sizeof *listing.names, compare_names);
    for (i = 0; i < listing.count; ++i) {
        const char *name = listing.names[i] + 1;
        char path[4096];
        if ((size_t)snprintf(path, sizeof path, "%s/%s", folder, name) < sizeof path) {
            if (listing.names[i][0] == 'd' && depth < FOLDER_DEPTH)
                scan_folder(path, depth + 1);
            else if (listing.names[i][0] == 'f' && is_font_file(name))
                add_file(path);
        }
        free(listing.names[i]);
    }
    free(listing.names);
}

static const char *system_font_dirs(char *out, size_t size)
{
#ifdef _WIN32
    const char *windows = getenv("WINDIR");
    if (windows && (size_t)snprintf(out, size, "%s\\Fonts", windows) < size)
        return out;
    return "C:\\Windows\\Fonts";
#else
    (void)out;
    (void)size;
    return SYSTEM_FONT_DIRS;
#endif
}

static void read_catalog(void)
{
    const char *folders = getenv("RAGE_FONT_DIRS");
    char system[1024], *list, *folder, *end;
    if (FT_Init_FreeType(&library))
        return;
    list = copy_text(folders && *folders ? folders : system_font_dirs(system, sizeof system));
    if (!list)
        return;
    for (folder = list; folder; folder = end ? end + 1 : NULL) {
        end = strchr(folder, FOLDER_SEPARATOR);
        if (end)
            *end = 0;
        if (*folder)
            scan_folder(folder, 0);
    }
    free(list);
}

void font_match_init(void)
{
    if (catalog_read)
        return;
    catalog_read = 1;
    coverage_lock = platform_mutex_create();
    read_catalog();
}

/* The generic family the client asks by name, resolved as Fontconfig's configuration does. */
static const char *const monospace_families[] = {
    "Lucida Console", "Consolas", "DejaVu Sans Mono", "Liberation Mono", "Noto Sans Mono", "Droid Sans Mono",
    "Courier New", NULL,
};

struct SortKey {
    int index, family, lang, slant, weight;
};

static int compare_keys(const void *a, const void *b)
{
    const struct SortKey *x = a, *y = b;
    if (x->family != y->family)
        return x->family - y->family;
    if (x->lang != y->lang)
        return x->lang - y->lang;
    if (x->slant != y->slant)
        return x->slant - y->slant;
    if (x->weight != y->weight)
        return x->weight - y->weight;
    return x->index - y->index;
}

static int family_rank(const char *name, const char *family, const char *const *fallbacks)
{
    int rank = 0;
    if (family) {
        if (!cstr_compare_nocase(name, family))
            return 0;
        ++rank;
    }
    for (; fallbacks && *fallbacks; ++fallbacks, ++rank)
        if (!cstr_compare_nocase(name, *fallbacks))
            return rank;
    return rank;
}

struct FontMatches *font_match_sort(const char *family, const char *const *fallbacks, const char *lang, int bold,
                                    int italic)
{
    struct FontMatches *matches;
    struct SortKey *keys;
    int language = language_index(lang), wanted = bold ? WEIGHT_BOLD : WEIGHT_REGULAR;
    size_t i;
    if (!face_count)
        return NULL;
    if (family && !cstr_compare_nocase(family, "monospace")) {
        family = NULL;
        fallbacks = monospace_families;
    }
    keys = malloc(face_count * sizeof *keys);
    matches = malloc(sizeof *matches + face_count * sizeof matches->order[0]);
    if (!keys || !matches) {
        free(keys);
        free(matches);
        return NULL;
    }
    for (i = 0; i < face_count; ++i) {
        keys[i].index = (int)i;
        keys[i].family = family_rank(faces[i].family, family, fallbacks);
        keys[i].lang = !(language >= 0 && (faces[i].langs >> language & 1));
        keys[i].slant = faces[i].slanted != (italic != 0);
        keys[i].weight = abs(faces[i].weight - wanted);
    }
    qsort(keys, face_count, sizeof *keys, compare_keys);
    matches->count = (int)face_count;
    for (i = 0; i < face_count; ++i)
        matches->order[i] = keys[i].index;
    free(keys);
    return matches;
}

int font_match_count(const struct FontMatches *matches)
{
    return matches->count;
}

const char *font_match_file(const struct FontMatches *matches, int i, int *face_index)
{
    const struct Face *face = &faces[matches->order[i]];
    *face_index = face->index;
    return face->file;
}

int font_match_covers_language(const struct FontMatches *matches, int i, const char *lang)
{
    int language = language_index(lang);
    return language >= 0 && (faces[matches->order[i]].langs >> language & 1);
}

/* Which characters of the Basic Multilingual Plane a face has, read on the first question. */
static const uint32_t *face_coverage(struct Face *face)
{
    FT_Face opened;
    FT_ULong code;
    FT_UInt glyph;
    if (face->coverage || face->coverage_failed)
        return face->coverage;
    face->coverage_failed = 1;
    if (FT_New_Face(library, face->file, face->index, &opened))
        return NULL;
    face->coverage = calloc(BMP_WORDS, sizeof *face->coverage);
    if (face->coverage) {
        face->coverage_failed = 0;
        for (code = FT_Get_First_Char(opened, &glyph); glyph; code = FT_Get_Next_Char(opened, code, &glyph))
            if (code < 0x10000)
                face->coverage[code >> 5] |= 1u << (code & 31);
    }
    FT_Done_Face(opened);
    return face->coverage;
}

int font_match_has_char(struct FontMatches *matches, int i, unsigned code)
{
    const uint32_t *coverage;
    int has;
    if (code >= 0x10000)
        return 0;
    platform_mutex_lock(coverage_lock);
    coverage = face_coverage(&faces[matches->order[i]]);
    has = coverage && (coverage[code >> 5] >> (code & 31) & 1);
    platform_mutex_unlock(coverage_lock);
    return has;
}

void font_match_style(const struct FontMatches *matches, int i, int *bold, int *slanted)
{
    const struct Face *face = &faces[matches->order[i]];
    *bold = face->weight >= WEIGHT_DEMIBOLD;
    *slanted = face->slanted;
}

void font_match_free(struct FontMatches *matches)
{
    free(matches);
}
#endif
