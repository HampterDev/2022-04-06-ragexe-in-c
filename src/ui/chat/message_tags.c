#include "ui/chat/markup_links.h"
#include "core/msg_str.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { MESSAGE_EXPANSIONS_MAX = 4096, MESSAGE_BYTES_MAX = 1024 * 1024 };

static size_t conversion_end(struct TextView format)
{
    size_t at = 0;

    while (at < format.size) {
        unsigned width = 0;

        if (format.data[at++] != '%') continue;
        if (at < format.size && format.data[at] == '%') { ++at; continue; }
        while (at < format.size && format.data[at] && strchr("-+ #0", format.data[at])) ++at;
        while (at < format.size && format.data[at] >= '0' && format.data[at] <= '9') {
            width = width * 10 + format.data[at++] - '0';
            if (width > MESSAGE_BYTES_MAX) return 0;
        }
        if (at < format.size && format.data[at] == '.') {
            ++at;
            width = 0;
            while (at < format.size && format.data[at] >= '0' && format.data[at] <= '9') {
                width = width * 10 + format.data[at++] - '0';
                if (width > MESSAGE_BYTES_MAX) return 0;
            }
        }
        if (at < format.size && (format.data[at] == 's' || format.data[at] == 'd' || format.data[at] == 'f'))
            return at + 1;
        return 0;
    }
    return 0;
}

static int render_argument(char *out, size_t capacity, const char *format, char conversion, const char *arg)
{
    if (conversion == 's') return snprintf(out, capacity, format, arg);
    if (conversion == 'd') return snprintf(out, capacity, format, atoi(arg));
    return snprintf(out, capacity, format, atof(arg));
}

static int format_message(struct String *out, struct TextView payload)
{
    struct String argument = {0}, format = {0}, rendered = {0};
    size_t pos = text_view_find(payload, text_view(",", 1), 0);
    int ok = 1;

    string_assign(&argument, payload.data, payload.size);
    string_assign_cstr(out, msg_str(atoi(string_cstr(&argument))));
    if (pos != (size_t)-1) {
        ++pos;
        while (pos < payload.size) {
            size_t comma = text_view_find(payload, text_view(",", 1), pos);
            size_t end = conversion_end(string_view(out));
            char conversion;
            int size;

            if (!end) break;
            string_assign(&argument, payload.data + pos, (comma == (size_t)-1 ? payload.size : comma) - pos);
            conversion = out->data[end - 1];
            if (conversion == 's' && argument.size >= 8 &&
                string_find(&argument, "MsgStr(", 0) != STRING_NPOS) {
                string_assign(&argument, argument.data + 7, argument.size - 8);
                string_assign_cstr(&argument, msg_str(atoi(string_cstr(&argument))));
            }
            string_assign(&format, out->data, end);
            size = render_argument(NULL, 0, string_cstr(&format), conversion, string_cstr(&argument));
            if (size < 0 || (unsigned)size > MESSAGE_BYTES_MAX || out->size - end > MESSAGE_BYTES_MAX - (unsigned)size) {
                ok = 0;
                break;
            }
            string_reserve(&rendered, size ? (size_t)size : 1);
            render_argument(rendered.data, rendered.capacity + 1, string_cstr(&format), conversion, string_cstr(&argument));
            rendered.size = (size_t)size;
            string_append(&rendered, out->data + end, out->size - end);
            string_assign(out, string_cstr(&rendered), rendered.size);
            if (comma == (size_t)-1) break;
            pos = comma + 1;
        }
    }
    string_free(&rendered);
    string_free(&format);
    string_free(&argument);
    return ok;
}

struct String *navi_links_expand_msg_tags(struct String *out, struct TextView text)
{
    struct String current = {0}, next = {0}, message = {0};
    unsigned expansions = 0;

    string_assign(&current, text.data, text.size);
    for (;;) {
        struct TextView view = string_view(&current);
        size_t start = text_view_find(view, text_view("<MSG>", 5), 0);
        size_t end, suffix_size;

        if (start == (size_t)-1) break;
        end = text_view_find(view, text_view("</MSG>", 6), start + 5);
        if (end == (size_t)-1 || expansions++ == MESSAGE_EXPANSIONS_MAX) goto original;
        if (!format_message(&message, text_view(view.data + start + 5, end - start - 5))) goto original;
        suffix_size = view.size - end - 6;
        if (start > MESSAGE_BYTES_MAX || message.size > MESSAGE_BYTES_MAX - start ||
            suffix_size > MESSAGE_BYTES_MAX - start - message.size) goto original;
        string_assign(&next, view.data, start);
        string_append(&next, string_cstr(&message), message.size);
        string_append(&next, view.data + end + 6, suffix_size);
        if (next.size == current.size && !memcmp(string_cstr(&next), view.data, view.size)) goto original;
        string_assign(&current, string_cstr(&next), next.size);
    }
    string_assign(out, string_cstr(&current), current.size);
    goto done;
original:
    string_assign(out, text.data, text.size);
done:
    string_free(&message);
    string_free(&next);
    string_free(&current);
    return out;
}
