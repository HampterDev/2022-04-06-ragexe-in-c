#ifndef RENDER_FONT_FACES_H
#define RENDER_FONT_FACES_H

/* The font finder's rules as pure functions (render/font_match_scan.c reads the fonts, these
   decide): which family names a face answers to, the order a request ranks the faces in, the
   languages a face covers, a face's family names from its name table, and where a font the Windows
   registry lists is. Nothing here reads a file or asks the system. */

#include <stddef.h>
#include <stdint.h>

/* A face as the finder knows it. */
struct FontFace {
    const char *family;       /* its family name (the name table's name 1) in English, as UTF-8 */
    const char *local_family; /* the same in the display language, or NULL */
    int weight;               /* the OS/2 weight class: 400 regular, 700 bold */
    int slanted, fixed_pitch;
    unsigned langs;           /* bit i: it covers language i (font_faces_language) */
};

/* What a font is asked for. */
struct FontRequest {
    const char *const *families; /* the families asked, best first, ending with NULL (or NULL) */
    const char *lang;            /* the text's language: "en", "ko", "ja", "zh-cn", ...; any other
                                    value counts as no language */
    int bold, italic;
    int fixed_pitch;             /* after the families, fixed-pitch faces before the others */
};

/* The languages a face can cover: lang's index, or -1 when there is no such language. */
int font_faces_language(const char *lang);
/* The characters a face must all have to cover language: the i-th, 0 past the last. */
unsigned font_faces_language_probe(int language, int i);

/* The index in request->families of the first family face answers to, by its English or
   display-language family name compared without ASCII case (as GDI matches a face name), or -1. */
int font_faces_family(const struct FontFace *face, const struct FontRequest *request);

/* order[0..count) becomes the indices of faces, best first: the faces of the asked families in
   their order, then, for a fixed-pitch request, fixed-pitch faces, then those that cover the
   language, then those of the asked slant, then the closest weight, then the faces' own order.
   keys is room for count values. */
void font_faces_sort(const struct FontFace *faces, int count, const struct FontRequest *request, uint64_t *keys,
                     int *order);

/* A record of a font's name table, as FreeType gives it (FT_SfntName): the text is UTF-16BE for
   the Windows platform (3). */
struct FontNameRecord {
    unsigned platform, encoding, language, name;
    const unsigned char *text;
    unsigned length;
};

/* Of a face's name records, the one to take as its family name in English (*english) and in the
   display language, a Windows language ID (*local; none when that is English or 0): the Windows
   platform's family names (name 1), English (United States) before any other English and any
   other English before the first other; the display language's own before another of its primary
   language. -1 when there is none. */
void font_faces_pick_family(const struct FontNameRecord *records, int count, unsigned display_language,
                            int *english, int *local);
/* A Windows name record's text as UTF-8 in out (size > 0), cut at a whole character where it does
   not fit; a unit that is not part of valid UTF-16 becomes U+FFFD and a NUL ends the text. Returns
   the length. */
size_t font_faces_name_text(const struct FontNameRecord *record, char *out, size_t size);

/* The file of a font the Windows registry lists (the data of a value under
   SOFTWARE\Microsoft\Windows NT\CurrentVersion\Fonts): value itself when it is a full path (a drive
   and a root, or a network share), otherwise value in fonts_folder. 0 when it is empty or does not
   fit in out. */
int font_faces_registry_path(const char *fonts_folder, const char *value, char *out, size_t size);

#endif
