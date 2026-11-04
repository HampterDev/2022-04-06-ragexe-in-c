#include "core/utf8.h"
#include "ui/controls/static.h"
#include "ui/draw.h"
#include <stdlib.h>
#include <string.h>

struct String *ui_window_ellipsize_text(struct UIWindow *self, struct String *out,
                                      int max_width, const char *text, int font_face,
                                      int font_height, int font_arg)
{
    size_t keep = strlen(text);
    int width = ui_window_text_width(self, text, 0, font_face, font_height, font_arg, 0);
    ui_window_text_width(self, "...", 0, font_face, font_height, font_arg, 0);
    string_init(out);
    string_assign_cstr(out, text);
    if (max_width >= width)
        return out;
    for (;;) {
        int last = utf8_prev(text, text + keep) == text;
        if (!last)
            keep = (size_t)(utf8_prev(text, text + keep) - text);
        string_assign(out, text, keep);
        string_append_cstr(out, "...");
        if (last || max_width >= ui_window_text_width(self, string_cstr(out), 0,
                                                    font_face, font_height, font_arg, 0))
            break;
    }
    return out;
}

void ui_static_text_set_text_ellipsis_wide(struct UIWindow *wnd, const char *text)
{
    struct UIStaticText *self = (struct UIStaticText *)wnd;
    string_assign_cstr(&self->tooltip, text);
    self->has_tooltip = 0;
    if (wnd->canvas) {
        int width = ui_window_text_width(wnd, text, 0, self->font_face, self->font_height, 0, 0);
        self->has_tooltip = wnd->width < width;
    }
    if (self->has_tooltip) {
        struct String shown;
        ui_window_ellipsize_text(wnd, &shown, wnd->width, text, self->font_face, self->font_height, 0);
        string_free(&self->text);
        string_move(&self->text, &shown);
    } else {
        string_assign_cstr(&self->text, text);
    }
    ui_window_invalidate(wnd);
}
