#include "ui/draw.h"
#include "core/utf8.h"
#include <string.h>

struct String *ui_window_fit_plain_text(struct UIWindow *self, struct String *out,
                                        struct TextView text, int width)
{
    struct String source;
    size_t keep;
    int measured;

    if (!self->canvas)
        ui_window_resize(self, width, 15);
    string_assign(out, text.data, text.size);
    measured = out->size ? ui_window_text_width(self, string_cstr(out), 0, 0, 12, 0, 0) : 0;
    ui_window_text_width(self, "...", 3, 0, 12, 0, 0);
    if (width >= measured)
        return out;
    string_init(&source);
    string_assign(&source, string_cstr(out), strlen(string_cstr(out)));
    keep = source.size;
    for (;;) {
        const char *start = string_cstr(&source);
        int last = utf8_prev(start, start + keep) == start;
        if (!last)
            keep = (size_t)(utf8_prev(start, start + keep) - start);
        string_assign(out, start, keep);
        string_append_cstr(out, "...");
        if (last || width >= ui_window_text_width(self, string_cstr(out), 0, 0, 12, 0, 0))
            break;
    }
    string_free(&source);
    return out;
}
