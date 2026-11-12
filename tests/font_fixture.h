#ifndef TESTS_FONT_FIXTURE_H
#define TESTS_FONT_FIXTURE_H

/* TrueType fonts made in the test, for the font finder and the font cache: every glyph but the
   missing one is the same filled box, so their faces differ only in what the finder reads (names,
   weight, slant, pitch, characters, metrics). */

#include <stddef.h>

struct FixtureFont {
    const char *family;              /* the family (name 1) in English, as UTF-8; NULL for none */
    const unsigned char *raw_family; /* instead, these UTF-16BE bytes as the English family */
    unsigned raw_length;
    const char *local_family;        /* the family in local_language too, or NULL */
    unsigned local_language;         /* a Windows language ID */
    int weight, italic, fixed_pitch; /* weight 0 is 400 */
    int em, win_ascent, win_descent; /* 0 is 1000, 800 and 200; a negative ascent or descent is 0 */
    const unsigned *chars;           /* the characters it has (BMP), ending with 0; NULL: ' ' to '~' */
};

/* The font file of count fonts (a collection when count > 1) in out: its size, or 0 when it does
   not fit in size bytes. */
size_t font_fixture_build(const struct FixtureFont *fonts, int count, unsigned char *out, size_t size);
/* Writes bytes to path; 0 on failure. */
int font_fixture_write(const char *path, const unsigned char *bytes, size_t size);
/* Builds the fonts and writes them to path; 0 on failure. */
int font_fixture_save(const char *path, const struct FixtureFont *fonts, int count);

#endif
