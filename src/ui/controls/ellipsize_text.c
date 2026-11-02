#include "core/text_encoding.h"
#include "platform/code_page.h"
#include "ui/controls/static.h"
#include "ui/draw.h"
#include <stdlib.h>
#include <string.h>

struct String *ui_window_ellipsize_text(struct UIWindow *self, struct String *out,
                                      int max_width, const char *text, int font_face,
                                      int font_height, int font_arg)
{
    uint16_t *wide;
    int width = ui_window_text_width(self, text, 0, font_face, font_height, font_arg, 0);
    ui_window_text_width(self, "...", 0, font_face, font_height, font_arg, 0);
    string_init(out);
    string_assign_cstr(out, text);
    if (max_width >= width)
        return out;
    wide = encoding_to_wide(CODE_PAGE_THREAD, text);
    if (!wide)
        return out;
    for (;;) {
        size_t count = utf16_length(wide);
        int last = count <= 1;
        char *narrow;
        if (!last) {
            size_t keep = count - 1;
            if ((wide[count - 2] & 0x80) && (wide[count - 1] & 0x80))
                keep = count - 2;
            wide[keep] = 0;
        }
        narrow = encoding_from_wide(CODE_PAGE_THREAD, wide);
        if (!narrow)
            break;
        string_assign_cstr(out, narrow);
        string_append_cstr(out, "...");
        free(narrow);
        if (last || max_width >= ui_window_text_width(self, string_cstr(out), 0,
                                                    font_face, font_height, font_arg, 0))
            break;
    }
    free(wide);
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
