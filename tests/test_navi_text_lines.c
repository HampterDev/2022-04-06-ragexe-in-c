#include "navi/navi_text_lines.h"
#include "utf8_text.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define REPLACEMENT "\xef\xbf\xbd" /* U+FFFD */

static char lines[NAVI_TEXT_MAX_LINES][NAVI_TEXT_LINE_BYTES];

static int split(const char *text)
{
    memset(lines, 0x55, sizeof lines);
    return navi_text_lines(text, strlen(text), lines);
}

static void repeat(char *out, const char *piece, int count)
{
    size_t n = strlen(piece);
    int i;
    for (i = 0; i < count; i++)
        memcpy(out + i * n, piece, n);
    out[count * n] = 0;
}

int main(void)
{
    char text[512], expected[64];
    int i;

    /* Short text is one line. */
    assert(split("[Prontera") == 1 && !strcmp(lines[0], "[Prontera"));
    assert(split("") == 1 && !lines[0][0]);
    /* 11 units a line; len / 12 + 1 lines, so 12 units take two lines. */
    assert(split("[abcdefghijk") == 2 && !strcmp(lines[0], "[abcdefghij") && !strcmp(lines[1], "k"));
    assert(split("[abcdefghij") == 1 && !strcmp(lines[0], "[abcdefghij"));
    repeat(text, U8_HAN, 23);
    assert(split(text) == 2);
    repeat(expected, U8_HAN, 11);
    assert(!strcmp(lines[0], expected) && !strcmp(lines[1], expected));
    /* A surrogate pair at a line end moves to the next line whole. */
    strcpy(text, "[abcdefghi" U8_GRIN "xyz");
    assert(split(text) == 2);
    assert(!strcmp(lines[0], "[abcdefghi") && !strcmp(lines[1], U8_GRIN "xyz"));
    strcpy(text, "[abcdefgh" U8_GRIN "xyz");
    assert(split(text) == 2);
    assert(!strcmp(lines[0], "[abcdefgh" U8_GRIN) && !strcmp(lines[1], "xyz"));
    repeat(text, U8_GRIN, 12);
    assert(split(text) == 3);
    for (i = 0; i < 2; i++) {
        repeat(expected, U8_GRIN, 5);
        assert(!strcmp(lines[i], expected));
    }
    repeat(expected, U8_GRIN, 2);
    assert(!strcmp(lines[2], expected));
    /* At most 100 units, whole characters: 50 pairs fill them, the 51st is dropped. */
    repeat(text, U8_GRIN, 51);
    assert(split(text) == 9);
    for (i = 0; i < 9; i++)
        assert(strlen(lines[i]) <= 33 && !(strlen(lines[i]) % 4));
    strcpy(text, "a");
    repeat(text + 1, U8_GRIN, 50);
    assert(split(text) == 9);
    /* Invalid bytes are one unit each. */
    assert(split("[\xff\xfe") == 1 && !strcmp(lines[0], "[" REPLACEMENT REPLACEMENT));
    puts("Navigation text lines: 11 units a line, surrogate pairs kept whole, the 100-unit limit and invalid bytes passed.");
    return 0;
}
