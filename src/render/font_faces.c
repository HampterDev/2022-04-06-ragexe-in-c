#include "render/font_faces.h"
#include "core/cstr.h"
#include "core/utf8.h"
#include <stdlib.h>
#include <string.h>

#define WEIGHT_REGULAR 400
#define WEIGHT_BOLD 700
#define PLATFORM_WINDOWS 3
#define NAME_FAMILY 1
#define LANGUAGE_ENGLISH_US 0x0409
#define PRIMARY_LANGUAGE(id) ((id) & 0x3ff)
#define FAMILY_RANKS 0xff
#define WEIGHT_RANKS 0x3ff

struct Language {
    const char *lang;
    unsigned short probes[3];
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

int font_faces_language(const char *lang)
{
    size_t i;
    for (i = 0; lang && i < sizeof languages / sizeof languages[0]; ++i)
        if (!cstr_compare_nocase(languages[i].lang, lang))
            return (int)i;
    return -1;
}

unsigned font_faces_language_probe(int language, int i)
{
    if (language < 0 || (size_t)language >= sizeof languages / sizeof languages[0] || i < 0 || i >= 3)
        return 0;
    return languages[language].probes[i];
}

int font_faces_family(const struct FontFace *face, const struct FontRequest *request)
{
    int i;
    for (i = 0; request->families && request->families[i]; ++i)
        if ((face->family && !cstr_compare_nocase(face->family, request->families[i]))
            || (face->local_family && !cstr_compare_nocase(face->local_family, request->families[i])))
            return i;
    return -1;
}

static int compare_keys(const void *a, const void *b)
{
    uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
    return x < y ? -1 : x > y;
}

void font_faces_sort(const struct FontFace *faces, int count, const struct FontRequest *request, uint64_t *keys,
                     int *order)
{
    int language = font_faces_language(request->lang), wanted = request->bold ? WEIGHT_BOLD : WEIGHT_REGULAR, i;
    for (i = 0; i < count; ++i) {
        const struct FontFace *face = &faces[i];
        int family = font_faces_family(face, request), weight = abs(face->weight - wanted);
        uint64_t key;
        if (family < 0 || family > FAMILY_RANKS - 1)
            family = FAMILY_RANKS;
        if (weight > WEIGHT_RANKS)
            weight = WEIGHT_RANKS;
        key = (uint64_t)family;
        key = key << 1 | (request->fixed_pitch && !face->fixed_pitch);
        key = key << 1 | (language >= 0 && !(face->langs >> language & 1));
        key = key << 1 | ((face->slanted != 0) != (request->italic != 0));
        key = key << 10 | (unsigned)weight;
        keys[i] = key << 32 | (uint32_t)i;
    }
    qsort(keys, (size_t)count, sizeof *keys, compare_keys);
    for (i = 0; i < count; ++i)
        order[i] = (int)(keys[i] & 0xffffffffu);
}

/* How good a family record is: 0 for none, higher for better. */
static int english_rank(const struct FontNameRecord *record)
{
    if (record->language == LANGUAGE_ENGLISH_US)
        return 3;
    return PRIMARY_LANGUAGE(record->language) == PRIMARY_LANGUAGE(LANGUAGE_ENGLISH_US) ? 2 : 1;
}

static int local_rank(const struct FontNameRecord *record, unsigned display_language)
{
    if (record->language == display_language)
        return 2;
    return PRIMARY_LANGUAGE(record->language) == PRIMARY_LANGUAGE(display_language);
}

void font_faces_pick_family(const struct FontNameRecord *records, int count, unsigned display_language,
                            int *english, int *local)
{
    int i, best_english = 0, best_local = 0;
    int localized = display_language && PRIMARY_LANGUAGE(display_language) != PRIMARY_LANGUAGE(LANGUAGE_ENGLISH_US);
    *english = *local = -1;
    for (i = 0; i < count; ++i) {
        const struct FontNameRecord *record = &records[i];
        int rank;
        if (record->platform != PLATFORM_WINDOWS || record->name != NAME_FAMILY || !record->length)
            continue;
        rank = english_rank(record);
        if (rank > best_english) {
            best_english = rank;
            *english = i;
        }
        rank = localized ? local_rank(record, display_language) : 0;
        if (rank > best_local) {
            best_local = rank;
            *local = i;
        }
    }
}

size_t font_faces_name_text(const struct FontNameRecord *record, char *out, size_t size)
{
    size_t used = 0, at;
    for (at = 0; at + 1 < record->length; at += 2) {
        unsigned long unit = (unsigned long)record->text[at] << 8 | record->text[at + 1], code = unit;
        char bytes[UTF8_CHAR_MAX];
        size_t length;
        if (!unit)
            break;
        if (unit >= 0xd800 && unit <= 0xdbff && at + 3 < record->length) {
            unsigned long low = (unsigned long)record->text[at + 2] << 8 | record->text[at + 3];
            if (low >= 0xdc00 && low <= 0xdfff) {
                code = 0x10000 + ((unit - 0xd800) << 10) + (low - 0xdc00);
                at += 2;
            }
        }
        if (code >= 0xd800 && code <= 0xdfff)
            code = 0xfffd;
        length = utf8_encode(code, bytes);
        if (!length || used + length >= size)
            break;
        memcpy(out + used, bytes, length);
        used += length;
    }
    out[used] = 0;
    return used;
}

static int is_full_path(const char *path)
{
    if ((path[0] == '\\' || path[0] == '/') && (path[1] == '\\' || path[1] == '/'))
        return 1;
    return ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')) && path[1] == ':'
        && (path[2] == '\\' || path[2] == '/');
}

int font_faces_registry_path(const char *fonts_folder, const char *value, char *out, size_t size)
{
    size_t folder, name;
    if (!value || !*value || !size)
        return 0;
    name = strlen(value);
    if (is_full_path(value)) {
        if (name >= size)
            return 0;
        memcpy(out, value, name + 1);
        return 1;
    }
    folder = strlen(fonts_folder);
    if (!folder || folder + 1 + name >= size)
        return 0;
    memcpy(out, fonts_folder, folder);
    out[folder] = '\\';
    memcpy(out + folder + 1, value, name + 1);
    return 1;
}
