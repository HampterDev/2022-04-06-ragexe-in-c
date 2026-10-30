#include "ui/chat/navi_text.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const char open_tag[] = "<NAVIL>";
static const char close_tag[] = "</NAVIL>";

static size_t find_tag(struct TextView text, const char *tag, size_t size)
{
    size_t i;

    if (size <= text.size)
        for (i = 0; i <= text.size - size; ++i)
            if (!memcmp(text.data + i, tag, size))
                return i;
    return SIZE_MAX;
}

int navi_links_base62(struct TextView text)
{
    uint32_t value = 0;
    size_t i;

    for (i = 0; i < text.size; ++i) {
        unsigned char c = (unsigned char)text.data[i];
        unsigned digit = 0;

        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'a' && c <= 'z') digit = c - 'a' + 10;
        else if (c >= 'A' && c <= 'Z') digit = c - 'A' + 36;
        value = value * 62u + digit;
    }
    return value <= INT32_MAX ? (int)value : -1 - (int)(UINT32_MAX - value);
}

struct String *navi_links_readable_link(struct String *out,
                                         struct TextView link, int bracketed)
{
    struct String result = {0};
    struct TextView name = text_view("", 0);
    size_t end = find_tag(link, close_tag, sizeof close_tag - 1);
    int x = 0, y = 0;

    if (end != SIZE_MAX && end >= 4) {
        size_t start;
        struct TextView head = text_view(link.data, end - 4);

        x = navi_links_base62(text_view(link.data + end - 4, 2));
        y = navi_links_base62(text_view(link.data + end - 2, 2));
        start = find_tag(head, open_tag, sizeof open_tag - 1);
        if (start != SIZE_MAX) {
            start += sizeof open_tag - 1;
            name = text_view(head.data + start, head.size - start);
        }
    }
    if (bracketed) string_append_cstr(&result, "<");
    string_append(&result, name.data, name.size);
    if (x <= 999 || y <= 999) {
        char coords[32];
        int size = snprintf(coords, sizeof coords, " %d,%d", x, y);

        string_append(&result, coords, (size_t)size);
    }
    if (bracketed) string_append_cstr(&result, ">");
    string_free(out);
    *out = result;
    return out;
}

struct String *navi_links_readable_links(struct String *out,
                                          struct TextView text)
{
    struct String result = {0};
    size_t start = find_tag(text, open_tag, sizeof open_tag - 1);

    if (start == SIZE_MAX) {
        string_assign(out, text.data, text.size);
        return out;
    }
    {
        struct TextView suffix = text_view(text.data + start, text.size - start);
        size_t end = find_tag(suffix, close_tag, sizeof close_tag - 1);

        if (end != SIZE_MAX) {
            struct String readable = {0};

            end += sizeof close_tag - 1;
            navi_links_readable_link(&readable, text_view(suffix.data, end), 1);
            string_append(&result, text.data, start);
            string_append(&result, string_cstr(&readable), readable.size);
            string_append(&result, suffix.data + end, suffix.size - end);
            string_free(&readable);
        }
    }
    string_free(out);
    *out = result;
    return out;
}

struct String *navi_links_to_base62(struct String *out, unsigned value)
{
    static const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char encoded[16], *end = encoded + sizeof encoded, *p = end;

    do {
        *--p = digits[value % 62];
        value /= 62;
    } while (value);
    if (end - p == 1) *--p = '0';
    string_assign(out, p, (size_t)(end - p));
    return out;
}

int navi_links_has_link(struct TextView text)
{
    return find_tag(text, open_tag, sizeof open_tag - 1) != SIZE_MAX ||
           find_tag(text, close_tag, sizeof close_tag - 1) != SIZE_MAX ||
           find_tag(text, "<ITEML>", 7) != SIZE_MAX ||
           find_tag(text, "</ITEML>", 8) != SIZE_MAX;
}

struct TextView navi_links_extract_link(struct TextView text)
{
    size_t start = find_tag(text, open_tag, sizeof open_tag - 1);

    if (start != SIZE_MAX) {
        size_t end;

        text = text_view(text.data + start, text.size - start);
        end = find_tag(text, close_tag, sizeof close_tag - 1);
        if (end != SIZE_MAX) text.size = end + sizeof close_tag - 1;
    }
    return text;
}

int navi_links_strip_nr(struct String *out, struct TextView text)
{
    size_t at = find_tag(text, "<NR>", 4);
    struct String result = {0};

    if (at == SIZE_MAX) {
        string_assign(out, text.data, text.size);
        return 0;
    }
    string_append(&result, text.data, at);
    string_append(&result, text.data + at + 4, text.size - at - 4);
    string_free(out);
    *out = result;
    return 1;
}
