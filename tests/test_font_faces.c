#include "render/font_faces.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define KO (1u << 1)
#define EN 1u

static void languages(void)
{
    int language;
    assert(font_faces_language("en") == 0 && font_faces_language("KO") == 1);
    assert(font_faces_language("und-zsye") == -1 && font_faces_language(NULL) == -1 && font_faces_language("") == -1);
    assert(font_faces_language_probe(1, 0) == 0xac00 && font_faces_language_probe(1, 1) == 0xd7a3);
    assert(!font_faces_language_probe(1, 2) && !font_faces_language_probe(1, 3) && !font_faces_language_probe(-1, 0));
    for (language = 0; font_faces_language_probe(language, 0); ++language)
        assert(font_faces_language_probe(language, 1));
    assert(language == 14 && !font_faces_language_probe(99, 0));
}

static void families(void)
{
    static const char *const asked[] = {"MS PGothic", "\xef\xbc\xad\xef\xbc\xb3 \xef\xbc\xb0Gothic", "Arial", NULL};
    const struct FontRequest request = {asked, "ja", 0, 0, 0}, nothing = {NULL, "en", 0, 0, 0};
    struct FontFace face = {"ms pgothic", NULL, 400, 0, 0, 0};
    assert(font_faces_family(&face, &request) == 0);
    face.family = "Other";
    face.local_family = "\xef\xbc\xad\xef\xbc\xb3 \xef\xbc\xb0Gothic";
    assert(font_faces_family(&face, &request) == 1);
    face.local_family = "\xef\xbc\xad\xef\xbc\xb3 \xef\xbc\xb0gothic";
    assert(font_faces_family(&face, &request) == 1);
    face.family = "ARIAL";
    face.local_family = NULL;
    assert(font_faces_family(&face, &request) == 2 && font_faces_family(&face, &nothing) == -1);
    face.family = "Arial Black";
    assert(font_faces_family(&face, &request) == -1);
    face.family = NULL;
    assert(font_faces_family(&face, &request) == -1);
}

/* The order: asked families first, then fixed pitch for a fixed request, language, slant, weight and
   the catalogue's order. */
static void sort(void)
{
    static const char *const asked[] = {"Wanted", "Second", NULL};
    static const struct FontFace faces[] = {
        {"Other", NULL, 400, 0, 0, KO},           /* 0 */
        {"Second", NULL, 400, 0, 0, EN},          /* 1 */
        {"Wanted", NULL, 700, 0, 0, EN},          /* 2 */
        {"Wanted", NULL, 400, 1, 0, EN},          /* 3 */
        {"Wanted", NULL, 400, 0, 0, EN},          /* 4 */
        {"Mono", NULL, 400, 0, 1, EN},            /* 5 */
        {"Korean", NULL, 300, 0, 0, KO | EN},     /* 6 */
        {"Wanted", "Local", 400, 0, 0, KO},       /* 7 */
        {"Heavy", NULL, 900, 0, 0, EN},           /* 8 */
        {"Odd %s%n", NULL, 2000, 0, 0, 0},        /* 9 */
    };
    enum { COUNT = sizeof faces / sizeof faces[0] };
    struct FontRequest request = {asked, "en", 0, 0, 0};
    uint64_t keys[COUNT];
    int order[COUNT], i, seen[COUNT] = {0};
    static const int regular[] = {4, 2, 3, 7, 1, 5, 6, 8, 0, 9};
    static const int bold_italic[] = {3, 2, 4, 7, 1, 8, 5, 6, 0, 9};
    static const int korean_fixed[] = {7, 4, 2, 3, 1, 5, 0, 6, 8, 9};
    static const int no_family[] = {0, 1, 4, 5, 7, 6, 2, 8, 9, 3};
    font_faces_sort(faces, COUNT, &request, keys, order);
    for (i = 0; i < COUNT; ++i) {
        assert(order[i] == regular[i]);
        seen[order[i]]++;
    }
    for (i = 0; i < COUNT; ++i)
        assert(seen[i] == 1);
    request.bold = request.italic = 1;
    font_faces_sort(faces, COUNT, &request, keys, order);
    for (i = 0; i < COUNT; ++i)
        assert(order[i] == bold_italic[i]);
    request.bold = request.italic = 0;
    request.lang = "ko";
    request.fixed_pitch = 1;
    font_faces_sort(faces, COUNT, &request, keys, order);
    for (i = 0; i < COUNT; ++i)
        assert(order[i] == korean_fixed[i]);
    request.families = NULL;
    request.lang = "xx";
    request.fixed_pitch = 0;
    font_faces_sort(faces, COUNT, &request, keys, order);
    for (i = 0; i < COUNT; ++i)
        assert(order[i] == no_family[i]);
    font_faces_sort(faces, 0, &request, keys, order);
}

static void pick(void)
{
    static const unsigned char gulim_ko[] = {0xad, 0x74, 0xb9, 0xbc}, gulim[] = {0, 'G', 0, 'u', 0, 'l', 0, 'i', 0, 'm'};
    struct FontNameRecord records[] = {
        {1, 0, 0, 1, (const unsigned char *)"Mac", 3},      /* another platform */
        {3, 1, 0x0412, 1, gulim_ko, sizeof gulim_ko},     /* Korean */
        {3, 1, 0x0809, 1, gulim, sizeof gulim},           /* British English */
        {3, 1, 0x0409, 2, gulim, sizeof gulim},           /* a subfamily */
        {3, 1, 0x0409, 1, gulim, 0},                      /* empty */
        {3, 1, 0x0409, 1, gulim, sizeof gulim},           /* English (United States) */
        {3, 1, 0x0812, 1, gulim_ko, sizeof gulim_ko},     /* Korean (Johab) */
    };
    int english, local;
    font_faces_pick_family(records, 7, 0x0412, &english, &local);
    assert(english == 5 && local == 1);
    font_faces_pick_family(records, 7, 0x0812, &english, &local);
    assert(english == 5 && local == 6);
    font_faces_pick_family(records, 7, 0x0409, &english, &local);
    assert(english == 5 && local == -1);
    font_faces_pick_family(records, 7, 0x0411, &english, &local);
    assert(local == -1);
    font_faces_pick_family(records, 5, 0, &english, &local);
    assert(english == 2 && local == -1);
    font_faces_pick_family(records, 2, 0x0412, &english, &local);
    assert(english == 1 && local == 1);
    font_faces_pick_family(records, 1, 0x0412, &english, &local);
    assert(english == -1 && local == -1);
    font_faces_pick_family(NULL, 0, 0x0412, &english, &local);
    assert(english == -1 && local == -1);
}

static size_t text_of(const unsigned char *bytes, unsigned length, char *out, size_t size)
{
    struct FontNameRecord record = {3, 1, 0x0409, 1, bytes, length};
    return font_faces_name_text(&record, out, size);
}

static void name_text(void)
{
    static const unsigned char gulim_ko[] = {0xad, 0x74, 0xb9, 0xbc};
    static const unsigned char pair[] = {0, 'a', 0xd8, 0x3d, 0xde, 0x00, 0, 'b'};
    static const unsigned char lone[] = {0xd8, 0x3d, 0, 'x', 0xdc, 0x00, 0xd8, 0x00};
    static const unsigned char nul[] = {0, 'a', 0, 0, 0, 'b'};
    static const unsigned char odd[] = {0, 'a', 0};
    char out[64];
    assert(text_of(gulim_ko, 4, out, sizeof out) == 6 && !strcmp(out, "\xea\xb5\xb4\xeb\xa6\xbc"));
    assert(text_of(pair, 8, out, sizeof out) == 6 && !strcmp(out, "a\xf0\x9f\x98\x80" "b"));
    assert(!strcmp(out + 0, "a\xf0\x9f\x98\x80" "b"));
    assert(text_of(lone, 8, out, sizeof out) == 10 && !strcmp(out, "\xef\xbf\xbdx\xef\xbf\xbd\xef\xbf\xbd"));
    assert(text_of(nul, 6, out, sizeof out) == 1 && !strcmp(out, "a"));
    assert(text_of(odd, 3, out, sizeof out) == 1 && !strcmp(out, "a"));
    assert(text_of(gulim_ko, 4, out, 4) == 3 && !strcmp(out, "\xea\xb5\xb4"));
    assert(text_of(gulim_ko, 4, out, 3) == 0 && !*out);
    assert(text_of(gulim_ko, 0, out, 1) == 0 && !*out);
}

static void registry_path(void)
{
    char out[32];
    assert(font_faces_registry_path("C:\\Windows\\Fonts", "arial.ttf", out, sizeof out)
           && !strcmp(out, "C:\\Windows\\Fonts\\arial.ttf"));
    assert(font_faces_registry_path("C:\\Windows\\Fonts", "D:\\u\\gulim.ttc", out, sizeof out)
           && !strcmp(out, "D:\\u\\gulim.ttc"));
    assert(font_faces_registry_path("C:\\Windows\\Fonts", "d:/u/g.ttc", out, sizeof out) && !strcmp(out, "d:/u/g.ttc"));
    assert(font_faces_registry_path("C:\\F", "\\\\server\\f.ttf", out, sizeof out) && !strcmp(out, "\\\\server\\f.ttf"));
    assert(font_faces_registry_path("C:\\F", "C:relative.ttf", out, sizeof out) && !strcmp(out, "C:\\F\\C:relative.ttf"));
    assert(font_faces_registry_path("C:\\F", "%s%n.ttf", out, sizeof out) && !strcmp(out, "C:\\F\\%s%n.ttf"));
    assert(!font_faces_registry_path("C:\\F", "", out, sizeof out) && !font_faces_registry_path("C:\\F", NULL, out, 8));
    assert(!font_faces_registry_path("", "a.ttf", out, sizeof out));
    assert(!font_faces_registry_path("C:\\Windows\\Fonts", "a-name-that-is-too-long.ttf", out, sizeof out));
    assert(font_faces_registry_path("C:\\F", "1234567890123456789012345.ttf", out, sizeof out) == 0);
    assert(font_faces_registry_path("C:\\F", "123456789012345678901.ttf", out, sizeof out) && strlen(out) == 30);
    assert(!font_faces_registry_path("C:\\F", "a.ttf", out, 0));
}

int main(void)
{
    languages();
    families();
    sort();
    pick();
    name_text();
    registry_path();
    puts("Font faces: languages, family names in English and the display language, the sort, name records, "
         "name text and registry paths passed.");
    return 0;
}
