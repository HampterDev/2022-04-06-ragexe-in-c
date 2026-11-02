#include "ui/draw.h"
#include "core/utf8.h"
#include "core/text_format.h"
#include "core/hex.h"
#include "platform/language.h"
#include <stdlib.h>
#include <string.h>

static int width12(struct UIWindow *self, const char *text, int length)
{
    return ui_window_text_width(self, text, length, 0, 12, 0, 0);
}

static int measured_as_colour_code(const char *p, const char *end)
{
    unsigned value = 0;
    int i;

    if (*p != '^')
        return 0;
    for (i = 1; i <= 6; ++i)
        value = value << 4 | (p + i < end ? hex_digit_value((unsigned char)p[i]) : 0);
    return value != 0 || (end - p >= 7 && memcmp(p, "^000000", 7) == 0);
}

void ui_window_break_lines(struct UIWindow *self, const char *text,
                         struct TextArray *lines, int width)
{
    const char *end, *start;
    unsigned rounds = 0;

    if (!text || !*text) return;
    end = text + strlen(text);
    for (start = text; start < end && rounds < 1001; ++rounds) {
        const char *p = start;
        const char *line_end = start;
        long long measured = 0;
        unsigned chars = 0;

        while (p < end && chars < 1001) {
            const char *next = utf8_step(p, end);
            int count;

            ++chars;
            count = (int)(next - p);
            if (count == 1 && (*p == '\r' || *p == '\n')) {
                line_end = p;
                if (*p == '\r' && next < end && *next == '\n') ++next;
                p = next;
                break;
            }
            measured += width12(self, p, count);
            if (measured_as_colour_code(p, end))
                measured -= width12(self, "^000000", 7);
            if (*p == '[') {
                const char *close = strstr(p, "]");
                if (close) {
                    int group_width = width12(self, p, (int)(close - p) + 1);
                    int char_width = width12(self, p, count);
                    if (group_width - char_width + measured > width && group_width <= width)
                        break;
                }
            }
            if (measured > width) {
                if (p == start) line_end = p = next;
                break;
            }
            line_end = p = next;
        }
        if (!text_array_append(lines, line_end == start ? text_view(" ", 1)
                                                        : text_view(start, line_end - start)))
            abort();
        start = p;
    }
}
