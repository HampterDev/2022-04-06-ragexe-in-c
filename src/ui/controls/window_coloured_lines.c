#include "ui/draw.h"
#include "core/text_array.h"
#include "core/byte_array.h"
#include "platform/language.h"
#include <stdlib.h>
#include <string.h>

void ui_window_break_coloured_lines(struct UIWindow *self, const char *text,
                                 struct TextArray *lines, struct ByteArray *flags, int width)
{
    const char *end, *start;
    if (!text || !*text || ui_window_text_width(self, text, 0, 0, 12, 0, 0) <= 0)
        return;
    end = text + strlen(text);
    for (start = text; start < end;) {
        const char *p = start;
        size_t bytes = 0;
        long long measured = 0;
        unsigned char cr = 0;
        while (p < end) {
            const char *next = language_char_next(p);
            size_t before = bytes;
            int count;
            if (next <= p || next > end) next = p + 1;
            count = (int)(next - p);
            measured += ui_window_text_width(self, p, count, 0, 12, 0, 0);
            bytes += count;
            if (count == 1 && *p == '\r') {
                cr = 1;
                if (next < end) ++bytes;
                break;
            }
            if (measured > width) {
                if (before) bytes = before;
                break;
            }
            p = next;
        }
        if (!text_array_append(lines, text_view(start, bytes))) abort();
        if (flags && !byte_array_append(flags, cr)) abort();
        start += bytes;
    }
}
