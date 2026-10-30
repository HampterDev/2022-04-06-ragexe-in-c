#include "ui/chat/readable_links.h"
#include "ui/chat/chat_window.h"
#include "ui/chat/navi_text.h"
#include "iteminfo/item_link.h"
#include "iteminfo/item_api.h"
#include "ui/window.h"
#include "ui/window_paint.h"
#include "ui/draw.h"
#include <stdint.h>

void item_link_caption(struct UIWindow *window, struct String *out, struct TextView name)
{
    struct String fitted = {0};
    ui_window_fit_plain_text(window, &fitted, name, window->width / 2);
    string_assign_cstr(out, "<");
    string_append(out, string_cstr(&fitted), fitted.size);
    string_append_cstr(out, ">");
    string_free(&fitted);
}

struct String *ui_chat_his_box_readable_item_links(struct UIWindow *window,
                                                struct String *out, struct TextView text)
{
    struct String result = {0}, name = {0}, caption = {0};
    struct ItemLinkSpan span;
    size_t cursor = 0, copied = 0;

    while (item_link_next(text, &cursor, &span)) {
        struct ItemInfo item;
        item_info_construct(&item);
        if (item_link_read(&item, span.payload)) {
            item_info_get_display_name(&item, &name);
            item_link_caption(window, &caption, string_view(&name));
            string_append(&result, text.data + copied, span.begin - copied);
            string_append(&result, string_cstr(&caption), caption.size);
            copied = span.end;
        }
        item_info_destruct(&item);
    }
    string_append(&result, text.data + copied, text.size - copied);
    string_free(&caption);
    string_free(&name);
    string_free(out);
    *out = result;
    return out;
}

int item_link_build_line(struct UIWindow *window, struct String *out,
                         struct String *rest, const char *line,
                         ItemLinkLineCallback on_item, void *context)
{
    struct TextView text = string_view(rest);
    struct String result = {0}, name = {0}, caption = {0};
    struct ItemLinkSpan span;
    size_t cursor = 0, consumed = 0, limit = strlen(line);
    int stopped_at_link = 0;

    if (text_view_find(text, text_view("<ITEML>", 7), 0) == SIZE_MAX) {
        string_assign(out, line, limit);
        return 0;
    }
    while (result.size < limit && item_link_next(text, &cursor, &span)) {
        struct ItemInfo item;
        size_t plain;
        item_info_construct(&item);
        if (!item_link_read(&item, span.payload)) {
            item_info_destruct(&item);
            continue;
        }
        plain = span.begin - consumed;
        if (plain > limit - result.size) {
            item_info_destruct(&item);
            break;
        }
        string_append(&result, text.data + consumed, plain);
        consumed = span.begin;
        item_info_get_display_name(&item, &name);
        item_link_caption(window, &caption, string_view(&name));
        if (caption.size > limit - result.size) {
            stopped_at_link = 1;
            item_info_destruct(&item);
            break;
        }
        if (on_item)
            on_item(context, &item, string_cstr(&name), string_cstr(&caption), string_cstr(&result));
        string_append(&result, string_cstr(&caption), caption.size);
        consumed = span.end;
        item_info_destruct(&item);
    }
    if (!stopped_at_link) {
        size_t count = limit - result.size;
        if (count > text.size - consumed) count = text.size - consumed;
        string_append(&result, text.data + consumed, count);
        consumed += count;
    }
    string_assign(rest, text.data + consumed, text.size - consumed);
    string_free(&caption);
    string_free(&name);
    string_free(out);
    *out = result;
    return 1;
}

struct String *ui_new_chat_wnd_readable_links(struct UIWindow *window, struct String *out, struct TextView text)
{
    struct UINewChatWnd *self = (struct UINewChatWnd *)window;
    int i;
    navi_links_readable_links(out, text);
    for (i = 0; i < CHAT_BOX_COUNT(self); ++i)
        ui_chat_his_box_readable_item_links(CHAT_BOX(self, 0), out, string_view(out));
    return out;
}
