#ifndef NAVI_NAVI_TEXT_LINES_H
#define NAVI_NAVI_TEXT_LINES_H

#include <stddef.h>

#define NAVI_TEXT_MAX_UNITS 100
#define NAVI_TEXT_LINE_UNITS 11
#define NAVI_TEXT_LINE_BYTES 40
#define NAVI_TEXT_MAX_LINES (NAVI_TEXT_MAX_UNITS / 12 + 1)

/* Splits UTF-8 text into the lines the navigation window shows a name in: at most 11
   UTF-16 units each, never inside a character, so a surrogate pair stays on one line.
   The text is cut to its first 100 units, and there are len / 12 + 1 lines for len
   units, as before. Returns the number of lines. */
int navi_text_lines(const char *text, size_t size, char lines[NAVI_TEXT_MAX_LINES][NAVI_TEXT_LINE_BYTES]);

#endif
