#include "core/text_format.h"
#include "core/hex.h"
#include "core/utf8.h"
#include <windows.h>
#include <stdlib.h>
#include <string.h>

static int hex_byte(unsigned char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

int text_is_colour_code(const char *text, const char *end)
{
    size_t i;
    if (end - text < 7 || text[0] != '^')
        return 0;
    for (i = 1; i < 7; ++i)
        if (!hex_byte((unsigned char)text[i]))
            return 0;
    return 1;
}

const char *text_find_colour_code(const char *text, const char *end)
{
    while (text < end) {
        if (text_is_colour_code(text, end))
            return text;
        text = utf8_step(text, end);
    }
    return end;
}

const char *text_read_colour_code(const char *text, const char *end, unsigned long *colour)
{
    unsigned long r, g, b;
    if (!text_is_colour_code(text, end))
        return text;
    r = hex_digit_value((unsigned char)text[1]) * 16 + hex_digit_value((unsigned char)text[2]);
    g = hex_digit_value((unsigned char)text[3]) * 16 + hex_digit_value((unsigned char)text[4]);
    b = hex_digit_value((unsigned char)text[5]) * 16 + hex_digit_value((unsigned char)text[6]);
    *colour = r | (g << 8) | (b << 16);
    return text + 7;
}

size_t text_find_charset_tag(const char *text, size_t length, int enabled)
{
    size_t i;
    if (enabled)
        for (i = 0; i < length && text[i]; ++i)
            if (length - i >= 3 && text[i] == '|' && hex_byte((unsigned char)text[i + 1]) &&
                hex_byte((unsigned char)text[i + 2]))
                return i;
    return length;
}

size_t text_strip_colour_codes(char *text, size_t length)
{
    size_t read = 0, written = 0;
    while (read < length) {
        size_t end = (size_t)(text_find_colour_code(text + read, text + length) - text);
        memmove(text + written, text + read, end - read);
        written += end - read;
        read = end < length ? end + 7 : end;
    }
    text[written] = '\0';
    return written;
}

int text_copy_without_colours(struct OwnedText *out, struct TextView text, int charset_tags)
{
    char *copy;
    size_t read = 0, written = 0, tag;
    if (text.size > (size_t)-1 - 4)
        return 0;
    if (!text.size) {
        owned_text_clear(out);
        return 1;
    }
    copy = malloc(text.size + 4);
    if (!copy)
        return 0;
    tag = text_find_charset_tag(text.data, text.size, charset_tags);
    while (read < text.size) {
        size_t end;
        if (text_is_colour_code(text.data + read, text.data + text.size))
            read += 7;
        end = (size_t)(text_find_colour_code(text.data + read, text.data + text.size) - text.data);
        if (end != read) {
            if (tag < text.size) {
                memcpy(copy + written, text.data + tag, 3);
                written += 3;
            }
            memcpy(copy + written, text.data + read, end - read);
            written += end - read;
        }
        read = end;
    }
    copy[written] = '\0';
    free(out->data);
    out->data = copy;
    out->size = written;
    return 1;
}
