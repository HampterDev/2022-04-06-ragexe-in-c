#include "core/text_markup.h"
#include <string.h>

static size_t find_tag(const char *text, size_t length, size_t start,
                       const char *tag, size_t tag_length)
{
    size_t at;
    if (tag_length > length - start)
        return length;
    for (at = start; at <= length - tag_length; ++at)
        if (text[at] == tag[0] && memcmp(text + at, tag, tag_length) == 0)
            return at;
    return length;
}

static size_t remove_tags(char *text, size_t length, const char *open,
                          const char *close, int remove_content)
{
    size_t open_length = strlen(open), close_length = strlen(close);
    size_t read = 0, written = 0;

    if (open_length && close_length) {
        while (read < length) {
            size_t begin = find_tag(text, length, read, open, open_length);
            size_t inner, end;
            if (begin == length)
                break;
            inner = begin + open_length;
            end = find_tag(text, length, inner, close, close_length);
            if (end == length)
                break;
            memmove(text + written, text + read, begin - read);
            written += begin - read;
            if (!remove_content) {
                memmove(text + written, text + inner, end - inner);
                written += end - inner;
            }
            read = end + close_length;
        }
    }
    memmove(text + written, text + read, length - read);
    written += length - read;
    text[written] = '\0';
    return written;
}

size_t text_unwrap_tag(char *text, size_t length, const char *open, const char *close)
{
    return remove_tags(text, length, open, close, 0);
}

size_t text_strip_info(char *text, size_t length)
{
    return remove_tags(text, length, "<INFO>", "</INFO>", 1);
}

int text_has_markup_tags(const char *text, size_t length)
{
    static const char *const tags[] = { "<URL>", "<ITEM>", "<TIPBOX>", "<NAVI>", "<ITEML>", "<QUEST>" };
    size_t i;
    for (i = 0; i < sizeof(tags) / sizeof(tags[0]); ++i)
        if (find_tag(text, length, 0, tags[i], strlen(tags[i])) != length)
            return 1;
    return 0;
}

size_t text_unescape(char *text, size_t length)
{
    static const char escaped[] = {'r', 'n', '"'};
    static const char decoded[] = {'\r', '\n', '"'};
    size_t pass;
    if (!text) return 0;
    for (pass = 0; pass < sizeof escaped; ++pass) {
        size_t read, written = 0;
        for (read = 0; read < length; ++read) {
            char ch = text[read];
            while (written && text[written - 1] == '\\' && ch == escaped[pass]) {
                --written;
                ch = decoded[pass];
            }
            text[written++] = ch;
        }
        length = written;
    }
    text[length] = 0;
    return length;
}
