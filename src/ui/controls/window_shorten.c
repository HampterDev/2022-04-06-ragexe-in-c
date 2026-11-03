#include "ui/draw.h"
#include "core/utf8.h"
#include "platform/language.h"
#include "render/textdc.h"
#include <stdint.h>
#include <string.h>

const char *ui_window_shorten_text(struct UIWindow *self, struct String *out,
                                const char *text, int max_width, int font_face,
                                int font_height, int font_arg)
{
    struct TextDC dc;
    SIZE dots;
    const char *first, *pos, *end;
    size_t bytes = 0;
    int total;
    int64_t width = 0, available;

    if (!text) {
        string_clear(out);
        return NULL;
    }
    string_assign_cstr(out, text);
    first = string_cstr(out);
    total = ui_window_text_width(self, first, (int)out->size,
                              font_face, font_height, font_arg, 0);
    text_dc_construct(&dc, self->canvas);
    text_dc_select_font(&dc, font_face, font_height, font_arg, 0);
    text_dc_text_extent(&dc, "...", 3, &dots);
    text_dc_destruct(&dc);
    if (total <= max_width || !out->size)
        return first;

    available = (int64_t)max_width - dots.cx;
    pos = first;
    end = first + out->size;
    while (pos < end) {
        const char *next = utf8_step(pos, end);
        width += ui_window_text_width(self, pos, (int)(next - pos),
                                    font_face, font_height, font_arg, 0);
        if (width > available) break;
        bytes = (size_t)(next - first);
        pos = next;
    }
    if (bytes) {
        string_truncate(out, bytes);
        string_append_cstr(out, "...");
    }
    return string_cstr(out);
}
