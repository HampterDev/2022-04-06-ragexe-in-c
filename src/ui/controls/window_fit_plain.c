#include "ui/draw.h"
#include "core/text_encoding.h"
#include "platform/code_page.h"
#include <stdlib.h>

struct String *ui_window_fit_plain_text(struct UIWindow *self, struct String *out,
                                        struct TextView text, int width)
{
    uint16_t *wide;
    size_t count;
    int measured;

    if (!self->canvas)
        ui_window_resize(self, width, 15);
    string_assign(out, text.data, text.size);
    measured = out->size ? ui_window_text_width(self, string_cstr(out), 0, 0, 12, 0, 0) : 0;
    ui_window_text_width(self, "...", 3, 0, 12, 0, 0);
    wide = encoding_to_wide(CODE_PAGE_THREAD, string_cstr(out));
    if (!wide) return out;
    count = utf16_length(wide);
    if (width < measured) {
        for (;;) {
            char *narrow;
            if (count > 1)
                count -= ((unsigned char)wide[count - 2] & 0x80) &&
                         ((unsigned char)wide[count - 1] & 0x80) ? 2 : 1;
            wide[count] = 0;
            narrow = encoding_from_wide(CODE_PAGE_THREAD, wide);
            if (!narrow) break;
            string_assign_cstr(out, narrow);
            string_append_cstr(out, "...");
            free(narrow);
            if (count <= 1 || width >= ui_window_text_width(self, string_cstr(out), 0, 0, 12, 0, 0))
                break;
        }
    }
    free(wide);
    return out;
}
