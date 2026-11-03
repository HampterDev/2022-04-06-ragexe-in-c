#include "navi/navi_text_lines.h"
#include "core/utf8.h"
#include <string.h>
#include <wchar.h>

/* At most capacity UTF-16 units of text, whole characters only; an invalid byte becomes
   U+FFFD. */
static int widen_prefix(const char *text, size_t size, wchar_t *wide, int capacity)
{
    size_t at = 0, step;
    int units = 0;

    while (at < size) {
        long code_point = utf8_decode(text + at, size - at, &step);
        unsigned long value = code_point == UTF8_INVALID ? 0xfffd : (unsigned long)code_point;
        int need = value > 0xffff ? 2 : 1;

        if (units + need > capacity)
            break;
        if (need == 2) {
            value -= 0x10000;
            wide[units++] = (wchar_t)(0xd800 + (value >> 10));
            wide[units++] = (wchar_t)(0xdc00 + (value & 0x3ff));
        } else {
            wide[units++] = (wchar_t)value;
        }
        at += step;
    }
    return units;
}

static int is_high(wchar_t unit) { return unit >= 0xd800 && unit <= 0xdbff; }
static int is_low(wchar_t unit) { return unit >= 0xdc00 && unit <= 0xdfff; }

int navi_text_lines(const char *text, size_t size, char lines[NAVI_TEXT_MAX_LINES][NAVI_TEXT_LINE_BYTES])
{
    wchar_t wide[NAVI_TEXT_MAX_UNITS];
    int len = widen_prefix(text, size, wide, NAVI_TEXT_MAX_UNITS), count = len / 12 + 1, i;
    const wchar_t *start = wide, *limit = wide + len;

    for (i = 0; i < count; i++) {
        const wchar_t *stop = limit - start > NAVI_TEXT_LINE_UNITS ? start + NAVI_TEXT_LINE_UNITS : limit;

        if (stop < limit && stop > start + 1 && is_high(stop[-1]) && is_low(*stop))
            --stop;
        utf8_from_utf16(lines[i], NAVI_TEXT_LINE_BYTES, start, (size_t)(stop - start));
        start = stop;
    }
    return count;
}
