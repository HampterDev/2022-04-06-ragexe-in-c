#include "ui/chat/markup_links.h"
#include "core/msg_str.h"
#include "session/session_items.h"
#include "ui/window_mgr.h"

#include <string.h>

static int item_link_id(struct TextView link)
{
    static const char stops[] = "!#$%&'()*+,-/";
    struct String digits = {0};
    size_t i;
    int id = -1;

    for (i = 6; i <= link.size; ++i) {
        signed char c = i < link.size ? link.data[i] : 0;

        if (c >= '0') {
            string_append(&digits, link.data + i, 1);
        } else if ((!c || strchr(stops, c)) && digits.size) {
            id = navi_links_base62(string_view(&digits));
            break;
        }
    }
    string_free(&digits);
    return id;
}

int chat_filter_is_blocked(struct TextView text)
{
    size_t pos = 0;

    if (text_view_find(text, text_view("<ITEML>", 7), 0) == STRING_NPOS)
        return 0;
    for (;;) {
        size_t start = text_view_find(text, text_view("<ITEML>", 7), pos);
        size_t end = text_view_find(text, text_view("</ITEML>", 8), pos);
        size_t nested;
        struct ItemInfo item, *found;
        int id, blocked;

        if (end == (size_t)-1) return 0;
        if (start == (size_t)-1 || end < start) break;
        while ((nested = text_view_find(text, text_view("<ITEML>", 7), start + 7)) != (size_t)-1 && nested < end)
            start = nested;
        id = item_link_id(text_view(text.data + start + 7, end - start - 7));
        if (id == -1) break;
        found = session_get_inventory_item_by_id(&g_session1, &item, id, 0);
        blocked = found->index == 0 && !session_is_wearing_item_id(&g_session1, id) &&
                  !session_has_cart_item_id(&g_session1, id);
        item_info_destruct(&item);
        if (blocked) break;
        pos = end + 8;
    }
    ui_chat_line(msg_str(0xe8d), 0xff);
    return 1;
}
