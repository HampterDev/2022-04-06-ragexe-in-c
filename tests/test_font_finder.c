/* The font finder (render/font_match_scan.c) over fonts this test makes: family, style and
   language matching, family names in the display language, fixed-pitch fallbacks, collections,
   folders and their depth, and font files that are broken, truncated, empty, too large a
   collection or oddly named; every allocation failing in turn, and nothing left allocated. */
#include "font_fixture.h"
#include "platform/filesystem.h"
#include <assert.h>
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
#define SEPARATOR ";"
static void set_variable(const char *name, const char *value) { assert(!_putenv_s(name, value)); }
#else
#define SEPARATOR ":"
static void set_variable(const char *name, const char *value) { assert(!setenv(name, value, 1)); }
#endif

static long live, calls, fail_at;

static void *test_malloc(size_t size)
{
    void *data;
    if (++calls == fail_at)
        return NULL;
    data = malloc(size);
    live += data != NULL;
    return data;
}

static void *test_calloc(size_t count, size_t size)
{
    void *data;
    if (++calls == fail_at)
        return NULL;
    data = calloc(count, size);
    live += data != NULL;
    return data;
}

static void *test_realloc(void *data, size_t size)
{
    void *grown;
    if (++calls == fail_at)
        return NULL;
    grown = realloc(data, size);
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
#include "../src/render/font_match_scan.c"
#undef malloc
#undef calloc
#undef realloc
#undef free

#define LATIN 'A', 'z', 'a', 'Z', ' '
static const unsigned latin[] = {LATIN, 0};
static const unsigned japanese[] = {LATIN, 0x3042, 0x30a2, 0x65e5, 0};
static const unsigned hangul[] = {' ', 0xac00, 0xd7a3, 0xd55c, 0};
/* "フィクスチャゴシック" */
#define GOTHIC_JA "\xe3\x83\x95\xe3\x82\xa3\xe3\x82\xaf\xe3\x82\xb9\xe3\x83\x81\xe3\x83\xa3" \
                  "\xe3\x82\xb4\xe3\x82\xb7\xe3\x83\x83\xe3\x82\xaf"
static const unsigned char lone_surrogate[] = {0, 'B', 0, 'a', 0, 'd', 0xd8, 0x00, 0, 'N', 0, 'a', 0, 'm', 0, 'e'};
static char long_name[301];
static unsigned char bytes[1 << 17];

static void folder(const char *path)
{
    assert(platform_directory_create(path) || platform_path_is_directory(path));
}

static void save(const char *path, const struct FixtureFont *fonts, int count)
{
    assert(font_fixture_save(path, fonts, count));
}

static void save_bytes(const char *path, const void *data, size_t size)
{
    assert(font_fixture_write(path, data, size));
}

/* The folders: fonts (the catalogue) and small (the allocation failures). */
static void make_fonts(void)
{
    const struct FixtureFont sans = {"Fixture Sans", NULL, 0, NULL, 0, 400, 0, 0, 0, 0, 0, latin};
    const struct FixtureFont bold = {"Fixture Sans", NULL, 0, NULL, 0, 700, 0, 0, 0, 0, 0, latin};
    const struct FixtureFont italic = {"Fixture Sans", NULL, 0, NULL, 0, 400, 1, 0, 0, 0, 0, latin};
    const struct FixtureFont mono = {"Fixture Mono", NULL, 0, NULL, 0, 400, 0, 1, 0, 0, 0, latin};
    const struct FixtureFont gothic = {"Fixture Gothic", NULL, 0, GOTHIC_JA, 0x0411, 400, 0, 0, 2048, 1802, 246, japanese};
    const struct FixtureFont korean = {"Fixture Hangul", NULL, 0, NULL, 0, 400, 0, 0, 0, 0, 0, hangul};
    const struct FixtureFont pair[2] = {{"Fixture Pair A", NULL, 0, NULL, 0, 400, 0, 0, 0, 0, 0, latin},
                                        {"Fixture Pair B", NULL, 0, NULL, 0, 400, 0, 0, 0, 0, 0, latin}};
    const struct FixtureFont odd = {"Odd %s%n%x", NULL, 0, NULL, 0, 400, 0, 0, 0, 0, 0, latin};
    const struct FixtureFont tall = {long_name, NULL, 0, NULL, 0, 400, 0, 0, 0, 0, 0, latin};
    const struct FixtureFont broken_name = {NULL, lone_surrogate, sizeof lone_surrogate, NULL, 0, 400, 0, 0, 0, 0, 0,
                                            latin};
    const struct FixtureFont smile = {"Fixture \xf0\x9f\x98\x80 Smile", NULL, 0, NULL, 0, 400, 0, 0, 0, 0, 0, latin};
    const struct FixtureFont nameless = {NULL, NULL, 0, NULL, 0, 400, 0, 0, 0, 0, 0, latin};
    const struct FixtureFont deep = {"Fixture Deep", NULL, 0, NULL, 0, 400, 0, 0, 0, 0, 0, latin};
    const struct FixtureFont deeper = {"Fixture Deeper", NULL, 0, NULL, 0, 400, 0, 0, 0, 0, 0, latin};
    const struct FixtureFont second = {"Fixture Second", NULL, 0, NULL, 0, 400, 0, 0, 0, 0, 0, latin};
    unsigned char garbage[1000], header[12];
    size_t size, i;
    memset(long_name, 'L', 300);
    folder("fonts");
    folder("fonts/nested");
    folder("fonts/nested/a");
    folder("fonts/nested/a/b");
    folder("fonts/nested/a/b/c");
    folder("fonts/nested/a/b/c/d");
    folder("second");
    folder("small");
    save("fonts/sans.ttf", &sans, 1);
    save("fonts/sans-bold.ttf", &bold, 1);
    save("fonts/sans-italic.TTF", &italic, 1);
    save("fonts/mono.otf", &mono, 1);
    save("fonts/gothic.ttf", &gothic, 1);
    save("fonts/hangul.ttf", &korean, 1);
    save("fonts/pair.ttc", pair, 2);
    save("fonts/%s%n.ttf", &odd, 1);
    save("fonts/long.ttf", &tall, 1);
    save("fonts/surrogate.ttf", &broken_name, 1);
    save("fonts/smile.ttf", &smile, 1);
    save("fonts/nameless.ttf", &nameless, 1);
    save("fonts/.hidden.ttf", &deep, 1);
    save("fonts/sans.txt", &deep, 1);
    save("fonts/nested/a/b/c/deep.ttf", &deep, 1);
    save("fonts/nested/a/b/c/d/deeper.ttf", &deeper, 1);
    save("second/second.ttf", &second, 1);
    for (i = 0; i < sizeof garbage; ++i)
        garbage[i] = (unsigned char)(i * 37 + 11);
    save_bytes("fonts/broken.ttf", garbage, sizeof garbage);
    save_bytes("fonts/empty.ttf", garbage, 0);
    size = font_fixture_build(&sans, 1, bytes, sizeof bytes);
    assert(size > 400);
    save_bytes("fonts/truncated.ttf", bytes, 100);
    save_bytes("fonts/half.ttf", bytes, size / 2);
    memcpy(header, "ttcf\0\1\0\0\x7f\xff\xff\xff", 12);
    save_bytes("fonts/many.ttc", header, sizeof header);
    save("small/sans.ttf", &sans, 1);
    save("small/gothic.ttf", &gothic, 1);
    save("small/pair.ttc", pair, 2);
    save_bytes("small/broken.ttf", garbage, sizeof garbage);
}

/* A collection that says it holds 2,000 fonts, each the same one: the finder reads 1,024. FreeType
   takes a collection only if the file could hold that many fonts' headers, so it is padded to 64 KB. */
static void make_crowd(void)
{
    const struct FixtureFont crowd = {"Fixture Crowd", NULL, 0, NULL, 0, 400, 0, 0, 0, 0, 0, latin};
    static unsigned char file[12 + 4 * 2000 + sizeof bytes];
    const size_t padded = 2000 * 32 + 12;
    size_t size = font_fixture_build(&crowd, 1, bytes, sizeof bytes), i, at = 12 + 4 * 2000, t;
    assert(size);
    memcpy(file, "ttcf\0\1\0\0\0\0\x07\xd0", 12);
    for (i = 0; i < 2000; ++i) {
        file[12 + 4 * i] = (unsigned char)(at >> 24);
        file[13 + 4 * i] = (unsigned char)(at >> 16);
        file[14 + 4 * i] = (unsigned char)(at >> 8);
        file[15 + 4 * i] = (unsigned char)at;
    }
    memcpy(file + at, bytes, size);
    /* The table offsets in the copy count from the start of the file. */
    for (t = 0; t < 10; ++t) {
        unsigned char *offset = file + at + 12 + 16 * t + 8;
        unsigned long value = (unsigned long)offset[0] << 24 | (unsigned long)offset[1] << 16
                            | (unsigned long)offset[2] << 8 | offset[3];
        value += (unsigned long)at;
        offset[0] = (unsigned char)(value >> 24);
        offset[1] = (unsigned char)(value >> 16);
        offset[2] = (unsigned char)(value >> 8);
        offset[3] = (unsigned char)value;
    }
    folder("crowd");
    assert(at + size <= padded && padded <= sizeof file);
    save_bytes("crowd/crowd.ttc", file, padded);
}

static struct FontMatches *sorted(const char *const *families, const char *lang, int bold, int italic, int fixed)
{
    struct FontRequest request = {families, lang, bold, italic, fixed};
    struct FontMatches *matches = font_match_sort(&request);
    assert(matches);
    return matches;
}

static const char *family_of(const struct FontMatches *matches, int i)
{
    return faces[matches->order[i]].family;
}

static int count_family(const char *family)
{
    int i, count = 0;
    for (i = 0; i < face_count; ++i)
        count += !strcmp(faces[i].family, family);
    return count;
}

static void catalogue(void)
{
    static const char *const sans[] = {"fixture sans", NULL};
    static const char *const localized[] = {GOTHIC_JA, "Fixture Gothic", NULL};
    static const char *const missing_mono[] = {"No Such Mono", NULL};
    static const char *const odd[] = {"Nope", "Odd %s%n%x", NULL};
    static const char *const pair_b[] = {"Fixture Pair B", NULL};
    static const char *const surrogate[] = {"Bad\xef\xbf\xbdName", NULL};
    static const char *const smile[] = {"Fixture \xf0\x9f\x98\x80 Smile", NULL};
    static char truncated[256];
    const char *truncated_name[] = {truncated, NULL};
    struct FontMatches *matches;
    int bold, slanted, index;
    const char *file;
    set_variable("RAGE_FONT_DIRS", "fonts" SEPARATOR SEPARATOR "second" SEPARATOR "missing");
    font_match_init(0x0411);
    for (index = 0; index < face_count; ++index)
        fprintf(stderr, "%s %d: %.40s\n", files[index].file, files[index].index, faces[index].family);
    /* sans x3, mono, gothic, hangul, pair x2, odd, long, surrogate, smile, deep, second (14); not the
       nameless face, the hidden or misnamed files, the one five folders down, nor the broken ones. */
    assert(face_count == 14);
    assert(count_family("Fixture Sans") == 3 && count_family("Fixture Deep") == 1 && !count_family("Fixture Deeper"));
    assert(count_family("Fixture Second") == 1);
    matches = sorted(sans, "en", 0, 0, 0);
    assert(font_match_count(matches) == 14 && font_match_family(matches, 0) == 0);
    font_match_style(matches, 0, &bold, &slanted);
    assert(!bold && !slanted && font_match_family(matches, 3) == -1);
    assert(font_match_covers_language(matches, 0, "en") && !font_match_covers_language(matches, 0, "ko"));
    assert(font_match_has_char(matches, 0, 'A') && !font_match_has_char(matches, 0, 0xac00));
    assert(!font_match_has_char(matches, 0, 0x1f600));
    font_match_free(matches);
    matches = sorted(sans, "en", 1, 0, 0);
    font_match_style(matches, 0, &bold, &slanted);
    assert(bold && !slanted && !strcmp(family_of(matches, 0), "Fixture Sans"));
    font_match_free(matches);
    matches = sorted(sans, "en", 0, 1, 0);
    font_match_style(matches, 0, &bold, &slanted);
    assert(!bold && slanted);
    font_match_free(matches);
    /* The Japanese name answers in Japanese, the English name always. */
    matches = sorted(localized, "ja", 0, 0, 0);
    assert(!strcmp(family_of(matches, 0), "Fixture Gothic") && font_match_family(matches, 0) == 0);
    assert(font_match_covers_language(matches, 0, "ja") && font_match_has_char(matches, 0, 0x3042));
    font_match_free(matches);
    matches = sorted(missing_mono, "en", 0, 0, 1);
    assert(!strcmp(family_of(matches, 0), "Fixture Mono") && font_match_family(matches, 0) == -1);
    font_match_free(matches);
    matches = sorted(NULL, "ko", 0, 0, 0);
    assert(!strcmp(family_of(matches, 0), "Fixture Hangul") && font_match_covers_language(matches, 0, "ko"));
    assert(font_match_has_char(matches, 0, 0xd55c));
    font_match_free(matches);
    matches = sorted(pair_b, "en", 0, 0, 0);
    file = font_match_file(matches, 0, &index);
    assert(index == 1 && strstr(file, "pair.ttc"));
    font_match_free(matches);
    matches = sorted(odd, "en", 0, 0, 0);
    file = font_match_file(matches, 0, &index);
    assert(font_match_family(matches, 0) == 1 && !index && strstr(file, "%s%n.ttf"));
    font_match_free(matches);
    matches = sorted(surrogate, "en", 0, 0, 0);
    assert(font_match_family(matches, 0) == 0);
    font_match_free(matches);
    matches = sorted(smile, "en", 0, 0, 0);
    assert(font_match_family(matches, 0) == 0);
    font_match_free(matches);
    /* A name longer than the finder keeps answers to its first 255 bytes. */
    memset(truncated, 'L', 255);
    matches = sorted(truncated_name, "en", 0, 0, 0);
    assert(font_match_family(matches, 0) == 0 && strlen(family_of(matches, 0)) == 255);
    font_match_free(matches);
    font_match_shutdown();
    assert(!live && !face_count && !library);
    /* In English, the Japanese name is no name of the face. */
    font_match_init(0x0409);
    matches = sorted(localized, "ja", 0, 0, 0);
    assert(!strcmp(family_of(matches, 0), "Fixture Gothic") && font_match_family(matches, 0) == 1);
    font_match_free(matches);
    font_match_shutdown();
    assert(!live);
}

static void crowd(void)
{
    static const char *const families[] = {"Fixture Crowd", NULL};
    struct FontMatches *matches;
    int index;
    set_variable("RAGE_FONT_DIRS", "crowd");
    font_match_init(0);
    assert(face_count == FACES_PER_FILE);
    matches = sorted(families, "en", 0, 0, 0);
    font_match_file(matches, FACES_PER_FILE - 1, &index);
    assert(index == FACES_PER_FILE - 1 && font_match_family(matches, FACES_PER_FILE - 1) == 0);
    font_match_free(matches);
    font_match_shutdown();
    assert(!live);
}

/* One run over the small folder; with fail_at, that allocation fails. */
static int small_run(void)
{
    static const char *const families[] = {"Fixture Pair B", NULL};
    struct FontMatches *matches;
    int found = 0;
    font_match_init(0x0411);
    matches = font_match_sort(&(struct FontRequest){families, "en", 0, 0, 0});
    if (matches) {
        assert(font_match_count(matches) <= 4);
        found = font_match_family(matches, 0) == 0;
        font_match_has_char(matches, 0, 'A');
        font_match_free(matches);
    }
    font_match_shutdown();
    assert(!live);
    return found;
}

static void allocation_failures(void)
{
    long total, i;
    set_variable("RAGE_FONT_DIRS", "small");
    calls = fail_at = 0;
    assert(small_run());
    total = calls;
    assert(total > 10);
    for (i = 1; i <= total; ++i) {
        calls = 0;
        fail_at = i;
        small_run();
    }
    fail_at = 0;
    assert(small_run());
}

int main(void)
{
    make_fonts();
    make_crowd();
    catalogue();
    crowd();
    allocation_failures();
    puts("Font finder: generated fonts by family, style, language, display-language names and pitch; "
         "collections, folder depth, broken, truncated, empty and oversized files, odd names; "
         "every allocation failing in turn without a leak passed.");
    return 0;
}
