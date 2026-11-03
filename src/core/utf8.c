#include "core/utf8.h"
#include <string.h>

#define IS_CONT(b) (((b) & 0xc0) == 0x80)
#define IS_HIGH(u) ((u) >= 0xd800 && (u) <= 0xdbff)
#define IS_LOW(u) ((u) >= 0xdc00 && (u) <= 0xdfff)

long utf8_decode(const char *text, size_t length, size_t *size)
{
    const unsigned char *s = (const unsigned char *)text;
    unsigned char lo = 0x80, hi = 0xbf;
    unsigned long value;
    size_t need, i;

    if (size)
        *size = length ? 1 : 0;
    if (!length)
        return UTF8_INVALID;
    if (s[0] < 0x80)
        return s[0];
    if (s[0] >= 0xc2 && s[0] <= 0xdf) {
        need = 1;
        value = s[0] & 0x1f;
    } else if (s[0] >= 0xe0 && s[0] <= 0xef) {
        need = 2;
        value = s[0] & 0x0f;
        if (s[0] == 0xe0)
            lo = 0xa0;
        else if (s[0] == 0xed)
            hi = 0x9f;
    } else if (s[0] >= 0xf0 && s[0] <= 0xf4) {
        need = 3;
        value = s[0] & 0x07;
        if (s[0] == 0xf0)
            lo = 0x90;
        else if (s[0] == 0xf4)
            hi = 0x8f;
    } else {
        return UTF8_INVALID;
    }
    if (need >= length)
        return UTF8_INVALID;
    for (i = 1; i <= need; i++) {
        if (s[i] < lo || s[i] > hi)
            return UTF8_INVALID;
        value = value << 6 | (s[i] & 0x3f);
        lo = 0x80;
        hi = 0xbf;
    }
    if (size)
        *size = need + 1;
    return (long)value;
}

size_t utf8_char_size(const char *text, size_t length)
{
    size_t size;

    utf8_decode(text, length, &size);
    return size;
}

const char *utf8_next(const char *text)
{
    /* The terminator is not a continuation byte, so decoding stops at it. */
    return *text ? text + utf8_char_size(text, (size_t)-1) : text;
}

const char *utf8_step(const char *text, const char *end)
{
    return text < end ? text + utf8_char_size(text, (size_t)(end - text)) : end;
}

const char *utf8_prev(const char *start, const char *text)
{
    const char *lead;

    if (text <= start)
        return start;
    lead = text - 1;
    while (lead > start && text - lead < UTF8_CHAR_MAX && IS_CONT((unsigned char)*lead))
        lead--;
    if (utf8_char_size(lead, (size_t)(text - lead)) == (size_t)(text - lead))
        return lead;
    return text - 1;
}

int utf8_valid(const char *text, size_t length)
{
    size_t at = 0, size;

    while (at < length) {
        if (utf8_decode(text + at, length - at, &size) == UTF8_INVALID)
            return 0;
        at += size;
    }
    return 1;
}

size_t utf8_prefix(const char *text, size_t length, size_t max_bytes)
{
    size_t at = 0, size;

    if (length <= max_bytes)
        return length;
    while (at < length) {
        size = utf8_char_size(text + at, length - at);
        if (size > max_bytes - at)
            break;
        at += size;
    }
    return at;
}

static int is_wide(unsigned long c)
{
    static const unsigned long ranges[][2] = {
        {0x1100, 0x115f}, {0x2e80, 0x303e}, {0x3041, 0x33ff}, {0x3400, 0x4dbf},
        {0x4e00, 0x9fff}, {0xa000, 0xa4cf}, {0xa960, 0xa97f}, {0xac00, 0xd7a3},
        {0xf900, 0xfaff}, {0xfe10, 0xfe19}, {0xfe30, 0xfe6f}, {0xff00, 0xff60},
        {0xffe0, 0xffe6}, {0x1f300, 0x1f64f}, {0x1f900, 0x1f9ff}, {0x20000, 0x2fffd},
        {0x30000, 0x3fffd},
    };
    size_t i;

    for (i = 0; i < sizeof ranges / sizeof ranges[0]; i++)
        if (c >= ranges[i][0] && c <= ranges[i][1])
            return 1;
    return 0;
}

size_t utf8_columns(const char *text, size_t length)
{
    size_t at = 0, size, columns = 0;
    long c;

    while (at < length) {
        c = utf8_decode(text + at, length - at, &size);
        columns += c != UTF8_INVALID && is_wide((unsigned long)c) ? 2 : 1;
        at += size;
    }
    return columns;
}

size_t utf8_encode(unsigned long c, char out[UTF8_CHAR_MAX])
{
    if (c < 0x80) {
        out[0] = (char)c;
        return 1;
    }
    if (c < 0x800) {
        out[0] = (char)(0xc0 | c >> 6);
        out[1] = (char)(0x80 | (c & 0x3f));
        return 2;
    }
    if (c >= 0xd800 && c <= 0xdfff)
        return 0;
    if (c < 0x10000) {
        out[0] = (char)(0xe0 | c >> 12);
        out[1] = (char)(0x80 | (c >> 6 & 0x3f));
        out[2] = (char)(0x80 | (c & 0x3f));
        return 3;
    }
    if (c <= 0x10ffff) {
        out[0] = (char)(0xf0 | c >> 18);
        out[1] = (char)(0x80 | (c >> 12 & 0x3f));
        out[2] = (char)(0x80 | (c >> 6 & 0x3f));
        out[3] = (char)(0x80 | (c & 0x3f));
        return 4;
    }
    return 0;
}

size_t utf8_from_utf16(char *out, size_t capacity, const wchar_t *text, size_t count)
{
    char bytes[UTF8_CHAR_MAX];
    size_t written = 0, i, n;
    unsigned long c;

    if (!capacity)
        return 0;
    for (i = 0; i < count; i++) {
        c = (unsigned long)(unsigned short)text[i];
        if (IS_HIGH(c) && i + 1 < count && IS_LOW((unsigned short)text[i + 1])) {
            c = 0x10000 + ((c - 0xd800) << 10) + ((unsigned short)text[i + 1] - 0xdc00);
            i++;
        } else if (IS_HIGH(c) || IS_LOW(c)) {
            c = UTF8_REPLACEMENT;
        }
        n = utf8_encode(c, bytes);
        if (n > capacity - 1 - written)
            break;
        memcpy(out + written, bytes, n);
        written += n;
    }
    out[written] = 0;
    return written;
}

size_t utf16_feed(wchar_t *pending, wchar_t unit, unsigned long out[2])
{
    unsigned long u = (unsigned short)unit;
    size_t count = 0;

    if (*pending) {
        unsigned long high = (unsigned short)*pending;

        *pending = 0;
        if (IS_LOW(u)) {
            out[0] = 0x10000 + ((high - 0xd800) << 10) + (u - 0xdc00);
            return 1;
        }
        out[count++] = UTF8_REPLACEMENT;
    }
    if (IS_HIGH(u)) {
        *pending = unit;
        return count;
    }
    out[count++] = IS_LOW(u) ? UTF8_REPLACEMENT : u;
    return count;
}
