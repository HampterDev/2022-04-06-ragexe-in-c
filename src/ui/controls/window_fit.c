#include "ui/window.h"
#include "ui/window_paint.h"
#include "ui/window_manager_state.h"
#include "ui/chat/readable_links.h"
#include "ui/chat/navi_text.h"
#include "core/text_markup.h"
#include "core/utf8.h"
#include "render/textdc.h"
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "ui/controls/window_fit.h"

static int text_cx(struct UIWindow *self, const char *text, int face, int height, int arg)
{
    struct TextDC dc;
    SIZE size;
    int len;
    if (!text || (len = (int)strlen(text)) == 0) return 0;
    text_dc_construct(&dc, self->canvas);
    text_dc_select_font(&dc, face, height, arg, 0);
    text_dc_text_extent(&dc, text, len, &size);
    text_dc_destruct(&dc);
    return size.cx;
}

static void measure_dots(struct UIWindow *self, int face, int height, int arg)
{
    struct TextDC dc;
    SIZE size;
    text_dc_construct(&dc, self->canvas);
    text_dc_select_font(&dc, face, height, arg, 0);
    text_dc_text_extent(&dc, "...", 3, &size);
    text_dc_destruct(&dc);
}

/* Drops whole UTF-8 characters from the end and appends "..." until the text fits. */
static void shrink(struct UIWindow *self, struct String *out, int width, int face, int height, int arg)
{
    struct String source;
    size_t keep;

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
        if (last || width >= text_cx(self, string_cstr(out), face, height, arg))
            break;
    }
    string_free(&source);
}

static void unwrap(struct String *text, const char *open, const char *close)
{
    if (text->data) text->size = text_unwrap_tag(text->data, text->size, open, close);
}

struct String *ui_window_fit_text(struct UIWindow *self, struct String *out,
                                   int width, const char *text, int font_face,
                                   int font_height, int font_arg3)
{
    struct String result = {0};
    int measured;
    string_assign_cstr(&result, text);
    unwrap(&result, "<URL>", "</URL>");
    unwrap(&result, "<NAVI>", "</NAVI>");
    unwrap(&result, "<TIPBOX>", "</TIPBOX>");
    unwrap(&result, "<ITEM>", "</ITEM>");
    if (result.data) result.size = text_strip_info(result.data, result.size);
    if (g_windowMgr.new_chat_wnd)
        ui_new_chat_wnd_readable_links(g_windowMgr.new_chat_wnd, &result, string_view(&result));
    navi_links_readable_links(&result, string_view(&result));
    unwrap(&result, "<QUEST>", "</QUEST>");
    measured = text_cx(self, string_cstr(&result), font_face, font_height, font_arg3);
    measure_dots(self, font_face, font_height, font_arg3);
    if (width < measured)
        shrink(self, &result, width, font_face, font_height, font_arg3);
    string_free(out);
    *out = result;
    return out;
}
