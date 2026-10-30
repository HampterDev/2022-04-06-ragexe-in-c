#include "ui/window.h"
#include "ui/window_paint.h"
#include "ui/window_manager_state.h"
#include "ui/chat/readable_links.h"
#include "ui/chat/navi_text.h"
#include "core/text_markup.h"
#include "core/text_encoding.h"
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

static int dotted(struct String *out, const wchar_t *wide)
{
    char *text = encoding_from_wide(CP_THREAD_ACP, wide);
    if (!text) return 0;
    string_assign_cstr(out, text);
    string_append_cstr(out, "...");
    free(text);
    return 1;
}

static void shrink(struct UIWindow *self, struct String *out, wchar_t *wide,
                    int width, int face, int height, int arg)
{
    size_t count = wcslen(wide);
    for (;;) {
        if (count <= 1) { dotted(out, wide); return; }
        count -= ((unsigned char)wide[count - 2] & 0x80) &&
                 ((unsigned char)wide[count - 1] & 0x80) ? 2 : 1;
        wide[count] = 0;
        if (!dotted(out, wide)) return;
        if (width >= text_cx(self, string_cstr(out), face, height, arg)) return;
    }
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
    if (width < measured) {
        wchar_t *wide = encoding_to_wide(CP_THREAD_ACP, string_cstr(&result));
        if (wide) {
            shrink(self, &result, wide, width, font_face, font_height, font_arg3);
            free(wide);
        }
    }
    string_free(out);
    *out = result;
    return out;
}

void ui_window_move_centred_for(struct UIWindow *self, int width, int height)
{
    int y = (int)((double)(height - self->height) * 0.5 + (double)self->y);
    int x = (int)((double)(width - self->width) * 0.5 + (double)self->x);

    ui_window_move_base(self, x, y);
}
