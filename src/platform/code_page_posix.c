#ifndef _WIN32
#include "platform/code_page.h"
#include "core/utf8.h"
#include <errno.h>
#include "platform/iconv_port.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

enum { CODE_PAGE_WESTERN = 1252 };

struct DoubleBytePage {
    unsigned int code_page;
    unsigned char lead[5], trail[5], single[5];
};

static const struct DoubleBytePage double_byte_pages[] = {
    {932, {0x81, 0x9f, 0xe0, 0xfc}, {0x40, 0x7e, 0x80, 0xfc}, {0x20, 0x7e, 0xa1, 0xdf}},
    {936, {0x81, 0xfe}, {0x40, 0xfe}, {0x20, 0x7e}},
    {949, {0x81, 0xfe}, {0x41, 0xfe}, {0x20, 0x7e}},
    {950, {0x81, 0xfe}, {0x40, 0x7e, 0xa1, 0xfe}, {0x20, 0x7e}},
};

struct CaseRange {
    uint16_t first, last, step;
    int delta;
};

static const struct CaseRange upper_case_ranges[] = {
    {0x0041, 0x005a, 1, 0x20}, {0x00c0, 0x00d6, 1, 0x20}, {0x00d8, 0x00de, 1, 0x20},
    {0x0100, 0x012e, 2, 1}, {0x0132, 0x0136, 2, 1}, {0x0139, 0x0147, 2, 1}, {0x014a, 0x0176, 2, 1},
    {0x0178, 0x0178, 1, -0x79}, {0x0179, 0x017d, 2, 1}, {0x01a0, 0x01a0, 1, 1}, {0x01af, 0x01af, 1, 1},
    {0x0386, 0x0386, 1, 0x26}, {0x0388, 0x038a, 1, 0x25}, {0x038c, 0x038c, 1, 0x40}, {0x038e, 0x038f, 1, 0x3f},
    {0x0391, 0x03a1, 1, 0x20}, {0x03a3, 0x03ab, 1, 0x20}, {0x0400, 0x040f, 1, 0x50}, {0x0410, 0x042f, 1, 0x20},
    {0x0460, 0x0480, 2, 1}, {0x048a, 0x04be, 2, 1}, {0x1ea0, 0x1ef8, 2, 1}, {0x2160, 0x216f, 1, 0x10},
    {0xff21, 0xff3a, 1, 0x20},
};

static unsigned int multibyte_code_page = CODE_PAGE_SYSTEM;

static unsigned int resolve(unsigned int code_page)
{
    if (code_page != CODE_PAGE_SYSTEM && code_page != CODE_PAGE_THREAD)
        return code_page;
    return g_codePage ? g_codePage : CODE_PAGE_WESTERN;
}

static const struct DoubleBytePage *double_byte_page(unsigned int code_page)
{
    size_t i;
    code_page = resolve(code_page);
    for (i = 0; i < sizeof double_byte_pages / sizeof *double_byte_pages; ++i)
        if (double_byte_pages[i].code_page == code_page)
            return &double_byte_pages[i];
    return NULL;
}

static int in_ranges(const unsigned char *ranges, unsigned char byte)
{
    for (; ranges[0]; ranges += 2)
        if (byte >= ranges[0] && byte <= ranges[1])
            return 1;
    return 0;
}

static int is_lead_byte(const struct DoubleBytePage *page, unsigned char byte)
{
    return page && in_ranges(page->lead, byte);
}

int platform_code_page_is_system_lead_byte(unsigned char byte)
{
    return is_lead_byte(double_byte_page(CODE_PAGE_SYSTEM), byte);
}

char *platform_code_page_next(unsigned int code_page, const char *text)
{
    if (!*text)
        return (char *)text;
    return (char *)text + (is_lead_byte(double_byte_page(code_page), (unsigned char)*text) && text[1] ? 2 : 1);
}

char *platform_code_page_prev(unsigned int code_page, const char *start, const char *text)
{
    while (*start && start < text) {
        const char *next = platform_code_page_next(code_page, start);
        if (next >= text)
            break;
        start = next;
    }
    return (char *)start;
}

void platform_code_page_set_multibyte(unsigned int code_page)
{
    multibyte_code_page = code_page;
}

int platform_code_page_byte_type(const char *text, size_t index)
{
    const struct DoubleBytePage *page = double_byte_page(multibyte_code_page);
    int type = CODE_PAGE_BYTE_ILLEGAL;
    size_t i;
    if (!page)
        return CODE_PAGE_BYTE_SINGLE;
    for (i = 0; i <= index; ++i) {
        unsigned char byte = (unsigned char)text[i];
        if (!byte)
            return CODE_PAGE_BYTE_ILLEGAL;
        if (type == CODE_PAGE_BYTE_LEAD)
            type = in_ranges(page->trail, byte) ? CODE_PAGE_BYTE_TRAIL : CODE_PAGE_BYTE_ILLEGAL;
        else if (in_ranges(page->lead, byte))
            type = CODE_PAGE_BYTE_LEAD;
        else
            type = in_ranges(page->single, byte) ? CODE_PAGE_BYTE_SINGLE : CODE_PAGE_BYTE_ILLEGAL;
    }
    return type;
}

char *platform_code_page_find(const char *text, const char *needle)
{
    const struct DoubleBytePage *page = double_byte_page(multibyte_code_page);
    size_t text_length, needle_length;
    const char *at, *last;
    if (!page)
        return strstr(text, needle);
    if (!*needle)
        return (char *)text;
    text_length = strlen(text);
    needle_length = strlen(needle);
    if (needle_length > text_length)
        return NULL;
    last = text + (text_length - needle_length);
    for (at = text; *at && at <= last;) {
        const char *a = at, *b = needle;
        while (*a && *b && *a == *b)
            ++a, ++b;
        if (!*b)
            return (char *)at;
        if (is_lead_byte(page, (unsigned char)*at++) && *at)
            ++at;
    }
    return NULL;
}

static const char *charset_name(unsigned int code_page, char *name, size_t size)
{
    code_page = resolve(code_page);
    if (code_page == CODE_PAGE_UTF8)
        return "UTF-8";
    snprintf(name, size, "CP%u", code_page);
    return name;
}

static const char *utf16_name(void)
{
    const uint16_t order = 1;
    return *(const unsigned char *)&order ? "UTF-16LE" : "UTF-16BE";
}

static size_t transcode(const char *to, const char *from, const char *input, size_t size,
                        char *output, size_t capacity)
{
    char scratch[256], *in = (char *)input;
    size_t total = 0;
    int error = 0;
    iconv_t converter = iconv_open(to, from);
    if (converter == (iconv_t)-1)
        return (size_t)-1;
    while (size && !error) {
        char *out = output ? output + total : scratch;
        size_t room = output ? capacity - total : sizeof scratch, before = room;
        size_t converted = iconv(converter, &in, &size, &out, &room);
        total += before - room;
        if (converted == (size_t)-1) {
            if (errno != E2BIG || output)
                error = errno;
        } else if (converted) {
            error = EILSEQ;
        }
    }
    iconv_close(converter);
    if (error) {
        errno = error;
        return (size_t)-1;
    }
    return total;
}

static int converted_units(size_t bytes, void *out, int capacity, size_t unit)
{
    if (bytes == (size_t)-1) {
        if (errno != E2BIG && capacity)
            memset(out, 0, unit);
        return 0;
    }
    if (bytes / unit > INT_MAX) {
        errno = EOVERFLOW;
        return 0;
    }
    return (int)(bytes / unit);
}

/* UTF-8, the client's text, converts as Windows converts CP_UTF8 without MB_ERR_INVALID_CHARS:
   an invalid byte becomes U+FFFD (here one per byte, as core/utf8.c steps over them) and an
   unpaired surrogate becomes U+FFFD, so that text never disappears for one bad byte. A capacity
   that is too small gives 0 and E2BIG. */
static int utf8_to_utf16(const char *text, size_t size, uint16_t *out, int capacity)
{
    size_t at = 0, units = 0, step;
    while (at < size) {
        long code = utf8_decode(text + at, size - at, &step);
        unsigned long point = code == UTF8_INVALID ? UTF8_REPLACEMENT : (unsigned long)code;
        size_t need = point >= 0x10000 ? 2 : 1;
        if (units + need > (size_t)INT_MAX) {
            errno = EOVERFLOW;
            return 0;
        }
        if (capacity) {
            if (units + need > (size_t)capacity) {
                errno = E2BIG;
                return 0;
            }
            if (need == 2) {
                out[units] = (uint16_t)(0xd800 + ((point - 0x10000) >> 10));
                out[units + 1] = (uint16_t)(0xdc00 + ((point - 0x10000) & 0x3ff));
            } else {
                out[units] = (uint16_t)point;
            }
        }
        units += need;
        at += step;
    }
    return (int)units;
}

static int utf16_to_utf8(const uint16_t *text, size_t length, char *out, int capacity)
{
    size_t at = 0, bytes = 0;
    while (at < length) {
        unsigned long point = text[at++];
        char encoded[UTF8_CHAR_MAX];
        size_t need;
        if (point >= 0xd800 && point <= 0xdbff && at < length && text[at] >= 0xdc00 && text[at] <= 0xdfff)
            point = 0x10000 + ((point - 0xd800) << 10) + (text[at++] - 0xdc00u);
        else if (point >= 0xd800 && point <= 0xdfff)
            point = UTF8_REPLACEMENT;
        need = utf8_encode(point, encoded);
        if (bytes + need > (size_t)INT_MAX) {
            errno = EOVERFLOW;
            return 0;
        }
        if (capacity) {
            if (bytes + need > (size_t)capacity) {
                errno = E2BIG;
                return 0;
            }
            memcpy(out + bytes, encoded, need);
        }
        bytes += need;
    }
    return (int)bytes;
}

int platform_code_page_is_valid(unsigned int code_page, const char *text)
{
    if (resolve(code_page) == CODE_PAGE_UTF8)
        return utf8_valid(text, strlen(text) + 1);
    return platform_code_page_to_utf16(code_page, text, -1, NULL, 0) != 0;
}

int platform_code_page_to_utf16(unsigned int code_page, const char *text, int length, uint16_t *out, int capacity)
{
    char name[16];
    size_t size;
    if (!text || !length || capacity < 0 || (capacity && !out)) {
        errno = EINVAL;
        return 0;
    }
    size = length < 0 ? strlen(text) + 1 : (size_t)length;
    if (resolve(code_page) == CODE_PAGE_UTF8)
        return utf8_to_utf16(text, size, out, capacity);
    return converted_units(transcode(utf16_name(), charset_name(code_page, name, sizeof name), text, size,
                                     capacity ? (char *)out : NULL, (size_t)capacity * sizeof *out),
                           out, capacity, sizeof *out);
}

int platform_code_page_from_utf16(unsigned int code_page, const uint16_t *text, int length, char *out, int capacity)
{
    char name[16];
    size_t size = 0;
    if (!text || !length || capacity < 0 || (capacity && !out)) {
        errno = EINVAL;
        return 0;
    }
    if (length < 0) {
        while (text[size])
            ++size;
        ++size;
    } else {
        size = (size_t)length;
    }
    if (resolve(code_page) == CODE_PAGE_UTF8)
        return utf16_to_utf8(text, size, out, capacity);
    return converted_units(transcode(charset_name(code_page, name, sizeof name), utf16_name(),
                                     (const char *)text, size * sizeof *text,
                                     capacity ? out : NULL, (size_t)capacity),
                           out, capacity, 1);
}

static uint16_t fold_case(uint16_t unit)
{
    size_t i;
    for (i = 0; i < sizeof upper_case_ranges / sizeof *upper_case_ranges; ++i) {
        const struct CaseRange *range = &upper_case_ranges[i];
        if (unit >= range->first && unit <= range->last && (unit - range->first) % range->step == 0)
            return (uint16_t)(unit + range->delta);
    }
    return unit;
}

uint16_t *platform_utf16_find_ignore_case(const uint16_t *text, const uint16_t *needle)
{
    size_t length = 0, i;
    if (!text || !needle || !*needle)
        return NULL;
    while (needle[length])
        ++length;
    for (; *text; ++text) {
        for (i = 0; i < length && text[i] && fold_case(text[i]) == fold_case(needle[i]); ++i)
            ;
        if (i == length)
            return (uint16_t *)text;
    }
    return NULL;
}
#endif
