#include "render/font_match.h"
#ifdef RAGE_FONT_SCAN
#include "core/cstr.h"
#include "platform/filesystem.h"
#include "platform/thread.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_TABLES_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

/* The installed faces, read when the font cache starts: on Windows from the fonts the registry
   lists, as GDI loads them (the machine's under HKEY_LOCAL_MACHINE, then the user's under
   HKEY_CURRENT_USER); elsewhere from the system's font folders; with RAGE_FONT_DIRS (folders
   separated by ':', on Windows by ';') from those folders instead, four levels deep. Each face is
   kept with its family names, weight, slant, pitch and languages; render/font_faces.c sorts them.
   Font files are data from outside: one that FreeType cannot open, or a face without a family name
   or a character map, is left out. */
#ifdef _WIN32
#define FOLDER_SEPARATOR ';'
#define FONTS_KEY L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts"
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
#define PATH_SIZE 4096
#define NAME_SIZE 256
/* More faces than any real collection has: a file that claims more is read no further. */
#define FACES_PER_FILE 1024
#define WEIGHT_REGULAR 400
#define WEIGHT_BOLD 700
#define WEIGHT_DEMIBOLD 600
#define BMP_WORDS (0x10000 / 32)

struct FaceFile {
    char *file;
    int index;
    uint32_t *coverage;
    int coverage_failed;
};

struct FontMatches {
    int count;
    int *family;
    int order[];
};

static FT_Library library;
static struct FontFace *faces;
static struct FaceFile *files;
static int face_count, face_capacity;
static unsigned display_language;
static struct PlatformMutex *coverage_lock;

static char *copy_text(const char *text)
{
    size_t size = strlen(text) + 1;
    char *copy = malloc(size);
    return copy ? memcpy(copy, text, size) : NULL;
}

static unsigned face_languages(FT_Face face)
{
    unsigned langs = 0;
    int language, i;
    for (language = 0; font_faces_language_probe(language, 0); ++language) {
        for (i = 0; font_faces_language_probe(language, i); ++i)
            if (!FT_Get_Char_Index(face, font_faces_language_probe(language, i)))
                break;
        if (!font_faces_language_probe(language, i))
            langs |= 1u << language;
    }
    return langs;
}

/* The face's family names in English (english, never empty) and in the display language (local,
   empty when it has none of its own); 0 when it has no family name. */
static int face_names(FT_Face face, char *english, char *local)
{
    FT_UInt count = FT_Get_Sfnt_Name_Count(face), i;
    struct FontNameRecord *records = count ? malloc(count * sizeof *records) : NULL;
    int found = 0, english_record, local_record;
    *english = *local = 0;
    for (i = 0; records && i < count; ++i) {
        FT_SfntName name;
        if (FT_Get_Sfnt_Name(face, i, &name))
            continue;
        records[found].platform = name.platform_id;
        records[found].encoding = name.encoding_id;
        records[found].language = name.language_id;
        records[found].name = name.name_id;
        records[found].text = name.string;
        records[found++].length = name.string_len;
    }
    font_faces_pick_family(records, found, display_language, &english_record, &local_record);
    if (english_record >= 0)
        font_faces_name_text(&records[english_record], english, NAME_SIZE);
    if (!*english && face->family_name && strlen(face->family_name) < NAME_SIZE)
        strcpy(english, face->family_name);
    if (local_record >= 0 && font_faces_name_text(&records[local_record], local, NAME_SIZE)
        && !strcmp(local, english))
        *local = 0;
    free(records);
    return *english != 0;
}

static int reserve_face(void)
{
    int capacity;
    struct FontFace *grown_faces;
    struct FaceFile *grown_files;
    if (face_count < face_capacity)
        return 1;
    capacity = face_capacity ? face_capacity * 2 : 256;
    grown_faces = realloc(faces, (size_t)capacity * sizeof *grown_faces);
    if (grown_faces)
        faces = grown_faces;
    grown_files = realloc(files, (size_t)capacity * sizeof *grown_files);
    if (grown_files)
        files = grown_files;
    if (!grown_faces || !grown_files)
        return 0;
    face_capacity = capacity;
    return 1;
}

static void add_face(const char *file, int index, FT_Face face)
{
    TT_OS2 *os2 = FT_Get_Sfnt_Table(face, FT_SFNT_OS2);
    char english[NAME_SIZE], local[NAME_SIZE];
    struct FontFace *entry;
    struct FaceFile *source;
    if (!face->charmap || !face_names(face, english, local) || !reserve_face())
        return;
    entry = &faces[face_count];
    source = &files[face_count];
    memset(entry, 0, sizeof *entry);
    memset(source, 0, sizeof *source);
    source->file = copy_text(file);
    entry->family = copy_text(english);
    entry->local_family = *local ? copy_text(local) : NULL;
    if (!source->file || !entry->family || (*local && !entry->local_family)) {
        free(source->file);
        free((char *)entry->family);
        free((char *)entry->local_family);
        return;
    }
    source->index = index;
    entry->weight = os2 && os2->version != 0xffff && os2->usWeightClass
                  ? os2->usWeightClass : (face->style_flags & FT_STYLE_FLAG_BOLD ? WEIGHT_BOLD : WEIGHT_REGULAR);
    if (entry->weight < 10)
        entry->weight *= 100;
    entry->slanted = (face->style_flags & FT_STYLE_FLAG_ITALIC) != 0;
    entry->fixed_pitch = FT_IS_FIXED_WIDTH(face) != 0;
    entry->langs = face_languages(face);
    ++face_count;
}

static void add_file(const char *file)
{
    FT_Face face;
    FT_Long count, i;
    if (FT_New_Face(library, file, -1, &face))
        return;
    count = face->num_faces < FACES_PER_FILE ? face->num_faces : FACES_PER_FILE;
    FT_Done_Face(face);
    for (i = 0; i < count; ++i)
        if (!FT_New_Face(library, file, i, &face)) {
            add_face(file, (int)i, face);
            FT_Done_Face(face);
        }
}

static int is_font_file(const char *name)
{
    const char *dot = strrchr(name, '.');
    return dot && (!cstr_compare_nocase(dot, ".ttf") || !cstr_compare_nocase(dot, ".otf")
                   || !cstr_compare_nocase(dot, ".ttc") || !cstr_compare_nocase(dot, ".otc"));
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
        char path[PATH_SIZE];
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

static void scan_folders(const char *folders)
{
    char *list = copy_text(folders), *folder, *end;
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

#ifdef _WIN32
/* A path as the system's code page names it, which is how FreeType and the platform's files open
   one; failing that (a character the code page lacks), its short form. 0 when neither has it. */
static int native_path(const wchar_t *path, char *out, int size)
{
    wchar_t short_path[MAX_PATH];
    BOOL lossy = FALSE;
    DWORD length;
    if (WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, path, -1, out, size, NULL, &lossy) > 0 && !lossy)
        return 1;
    length = GetShortPathNameW(path, short_path, MAX_PATH);
    lossy = FALSE;
    return length && length < MAX_PATH
        && WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, short_path, -1, out, size, NULL, &lossy) > 0 && !lossy;
}

static int is_full_path(const wchar_t *path)
{
    if ((path[0] == L'\\' || path[0] == L'/') && (path[1] == L'\\' || path[1] == L'/'))
        return 1;
    return ((path[0] >= L'A' && path[0] <= L'Z') || (path[0] >= L'a' && path[0] <= L'z')) && path[1] == L':'
        && (path[2] == L'\\' || path[2] == L'/');
}

/* The fonts one registry key lists: each value's data is a file in the fonts folder or a full path. */
static void scan_key(HKEY root, const wchar_t *fonts_folder)
{
    static wchar_t name[16384], data[PATH_SIZE / 2], expanded[PATH_SIZE / 2], full[PATH_SIZE / 2];
    HKEY key;
    DWORD i;
    if (RegOpenKeyExW(root, FONTS_KEY, 0, KEY_READ, &key) != ERROR_SUCCESS)
        return;
    for (i = 0;; ++i) {
        DWORD name_size = sizeof name / sizeof name[0], data_size = sizeof data - sizeof data[0], type;
        char native[PATH_SIZE], folder[PATH_SIZE], value[PATH_SIZE];
        LONG result = RegEnumValueW(key, i, name, &name_size, NULL, &type, (BYTE *)data, &data_size);
        const wchar_t *text = data;
        if (result == ERROR_NO_MORE_ITEMS)
            break;
        if (result != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ))
            continue;
        data[data_size / sizeof data[0]] = 0;
        if (type == REG_EXPAND_SZ) {
            DWORD length = ExpandEnvironmentStringsW(data, expanded, sizeof expanded / sizeof expanded[0]);
            if (!length || length > sizeof expanded / sizeof expanded[0])
                continue;
            text = expanded;
        }
        if (native_path(fonts_folder, folder, sizeof folder) && native_path(text, value, sizeof value)
            && font_faces_registry_path(folder, value, native, sizeof native)) {
            add_file(native);
            continue;
        }
        /* A name the code page cannot spell: the full path in its short form, if it has one. */
        if ((size_t)_snwprintf(full, sizeof full / sizeof full[0], L"%ls\\%ls", fonts_folder, text)
            >= sizeof full / sizeof full[0])
            continue;
        if (native_path(is_full_path(text) ? text : full, native, sizeof native))
            add_file(native);
    }
    RegCloseKey(key);
}

static void scan_system(void)
{
    wchar_t fonts_folder[MAX_PATH];
    UINT length = GetWindowsDirectoryW(fonts_folder, MAX_PATH - 6);
    if (!length || length >= MAX_PATH - 6)
        return;
    wcscat(fonts_folder, L"\\Fonts");
    scan_key(HKEY_LOCAL_MACHINE, fonts_folder);
    scan_key(HKEY_CURRENT_USER, fonts_folder);
}

static unsigned system_display_language(void)
{
    return GetUserDefaultUILanguage();
}
#else
static void scan_system(void)
{
    scan_folders(SYSTEM_FONT_DIRS);
}

static unsigned system_display_language(void)
{
    return 0;
}
#endif

void font_match_init(unsigned language)
{
    const char *folders = getenv("RAGE_FONT_DIRS");
    FT_Library opened;
    if (library || FT_Init_FreeType(&opened))
        return;
    coverage_lock = platform_mutex_create();
    if (!coverage_lock) {
        FT_Done_FreeType(opened);
        return;
    }
    library = opened;
    display_language = language ? language : system_display_language();
    if (folders && *folders)
        scan_folders(folders);
    else
        scan_system();
}

void font_match_shutdown(void)
{
    int i;
    for (i = 0; i < face_count; ++i) {
        free((char *)faces[i].family);
        free((char *)faces[i].local_family);
        free(files[i].file);
        free(files[i].coverage);
    }
    free(faces);
    free(files);
    faces = NULL;
    files = NULL;
    face_count = face_capacity = 0;
    if (coverage_lock)
        platform_mutex_destroy(coverage_lock);
    coverage_lock = NULL;
    if (library)
        FT_Done_FreeType(library);
    library = NULL;
}

struct FontMatches *font_match_sort(const struct FontRequest *request)
{
    struct FontMatches *matches;
    uint64_t *keys;
    int i;
    if (!face_count)
        return NULL;
    keys = malloc((size_t)face_count * sizeof *keys);
    matches = malloc(sizeof *matches + 2 * (size_t)face_count * sizeof matches->order[0]);
    if (!keys || !matches) {
        free(keys);
        free(matches);
        return NULL;
    }
    font_faces_sort(faces, face_count, request, keys, matches->order);
    free(keys);
    matches->count = face_count;
    matches->family = matches->order + face_count;
    for (i = 0; i < face_count; ++i)
        matches->family[i] = font_faces_family(&faces[matches->order[i]], request);
    return matches;
}

int font_match_count(const struct FontMatches *matches)
{
    return matches->count;
}

const char *font_match_file(const struct FontMatches *matches, int i, int *face_index)
{
    const struct FaceFile *file = &files[matches->order[i]];
    *face_index = file->index;
    return file->file;
}

int font_match_family(const struct FontMatches *matches, int i)
{
    return matches->family[i];
}

int font_match_covers_language(const struct FontMatches *matches, int i, const char *lang)
{
    int language = font_faces_language(lang);
    return language >= 0 && (faces[matches->order[i]].langs >> language & 1);
}

/* Which characters of the Basic Multilingual Plane a face has, read on the first question. */
static const uint32_t *face_coverage(struct FaceFile *file)
{
    FT_Face opened;
    FT_ULong code;
    FT_UInt glyph;
    if (file->coverage || file->coverage_failed)
        return file->coverage;
    file->coverage_failed = 1;
    if (FT_New_Face(library, file->file, file->index, &opened))
        return NULL;
    file->coverage = calloc(BMP_WORDS, sizeof *file->coverage);
    if (file->coverage) {
        file->coverage_failed = 0;
        for (code = FT_Get_First_Char(opened, &glyph); glyph; code = FT_Get_Next_Char(opened, code, &glyph))
            if (code < 0x10000)
                file->coverage[code >> 5] |= 1u << (code & 31);
    }
    FT_Done_Face(opened);
    return file->coverage;
}

int font_match_has_char(struct FontMatches *matches, int i, unsigned code)
{
    const uint32_t *coverage;
    int has;
    if (code >= 0x10000)
        return 0;
    platform_mutex_lock(coverage_lock);
    coverage = face_coverage(&files[matches->order[i]]);
    has = coverage && (coverage[code >> 5] >> (code & 31) & 1);
    platform_mutex_unlock(coverage_lock);
    return has;
}

void font_match_style(const struct FontMatches *matches, int i, int *bold, int *slanted)
{
    const struct FontFace *face = &faces[matches->order[i]];
    *bold = face->weight >= WEIGHT_DEMIBOLD;
    *slanted = face->slanted;
}

void font_match_free(struct FontMatches *matches)
{
    free(matches);
}
#endif
