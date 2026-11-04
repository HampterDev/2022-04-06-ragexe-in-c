#include "platform/iconv_port.h"
#ifdef RAGE_OWN_ICONV
#include "platform/code_page_tables.h"
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

enum Form { FORM_CODE_PAGE, FORM_UTF8, FORM_UTF16LE, FORM_UTF16BE };

struct Charset {
    enum Form form;
    const struct CodePageTable *table;
};

struct CodeConverter {
    struct Charset to, from;
};

/* A code page's encoder: its characters in order with their codes, built on first use. */
struct Encoder {
    const struct CodePageTable *table;
    uint32_t *pairs;
    size_t count;
};

static struct Encoder *encoders;
#ifdef _WIN32
static SRWLOCK encoder_lock = SRWLOCK_INIT;
#define LOCK() AcquireSRWLockExclusive(&encoder_lock)
#define UNLOCK() ReleaseSRWLockExclusive(&encoder_lock)
#else
static pthread_mutex_t encoder_lock = PTHREAD_MUTEX_INITIALIZER;
#define LOCK() pthread_mutex_lock(&encoder_lock)
#define UNLOCK() pthread_mutex_unlock(&encoder_lock)
#endif

#define DECODE_INVALID (-1)
#define DECODE_INCOMPLETE (-2)
#define ENCODE_UNMAPPED (-1)
#define ENCODE_NO_ROOM (-2)

static int same_name(const char *a, const char *b)
{
    for (; *a && *b; ++a, ++b)
        if (toupper((unsigned char)*a) != toupper((unsigned char)*b))
            return 0;
    return !*a && !*b;
}

static const struct CodePageTable *find_table(unsigned code_page)
{
    size_t i;
    for (i = 0; i < code_page_table_count; ++i)
        if (code_page_tables[i].code_page == code_page)
            return &code_page_tables[i];
    return NULL;
}

static int parse_charset(const char *name, struct Charset *out)
{
    const char *digits = NULL;
    char *end;
    unsigned long code_page;
    out->table = NULL;
    if (same_name(name, "UTF-8") || same_name(name, "UTF8")) {
        out->form = FORM_UTF8;
        return 1;
    }
    if (same_name(name, "UTF-16LE")) {
        out->form = FORM_UTF16LE;
        return 1;
    }
    if (same_name(name, "UTF-16BE")) {
        out->form = FORM_UTF16BE;
        return 1;
    }
    if (toupper((unsigned char)name[0]) == 'C' && toupper((unsigned char)name[1]) == 'P')
        digits = name + 2;
    else if (!strncmp(name, "WINDOWS-", 8) || !strncmp(name, "windows-", 8))
        digits = name + 8;
    if (!digits || !isdigit((unsigned char)*digits))
        return 0;
    code_page = strtoul(digits, &end, 10);
    if (*end || !(out->table = find_table((unsigned)code_page)))
        return 0;
    out->form = FORM_CODE_PAGE;
    return 1;
}

static int compare_pairs(const void *a, const void *b)
{
    uint32_t x = *(const uint32_t *)a, y = *(const uint32_t *)b;
    return x < y ? -1 : x > y;
}

static int is_decode_only(const struct CodePageTable *table, unsigned code)
{
    size_t low = 0, high = table->decode_only_count;
    while (low < high) {
        size_t middle = low + (high - low) / 2;
        if (table->decode_only[middle] == code)
            return 1;
        if (table->decode_only[middle] < code)
            low = middle + 1;
        else
            high = middle;
    }
    return 0;
}

/* Each pair is character << 16 | code, so sorting the pairs sorts them by character. */
static int build_encoder(struct Encoder *encoder, const struct CodePageTable *table)
{
    size_t count = 0, i;
    uint32_t *pairs = malloc((128 + (table->double_byte ? 126 * CODE_PAGE_TRAILS : 0)) * sizeof *pairs);
    if (!pairs)
        return 0;
    for (i = 0; i < 128; ++i)
        if (table->single[i] && !is_decode_only(table, (unsigned)(0x80 + i)))
            pairs[count++] = (uint32_t)table->single[i] << 16 | (uint32_t)(0x80 + i);
    if (table->double_byte)
        for (i = 0; i < 126 * CODE_PAGE_TRAILS; ++i) {
            unsigned code = (unsigned)(CODE_PAGE_LEAD_FIRST + i / CODE_PAGE_TRAILS) << 8
                          | (unsigned)(CODE_PAGE_TRAIL_FIRST + i % CODE_PAGE_TRAILS);
            if (table->double_byte[i] && !is_decode_only(table, code))
                pairs[count++] = (uint32_t)table->double_byte[i] << 16 | code;
        }
    qsort(pairs, count, sizeof *pairs, compare_pairs);
    encoder->table = table;
    encoder->pairs = pairs;
    encoder->count = count;
    return 1;
}

static const struct Encoder *encoder_for(const struct CodePageTable *table)
{
    const struct Encoder *found = NULL;
    size_t index = (size_t)(table - code_page_tables);
    LOCK();
    if (!encoders)
        encoders = calloc(code_page_table_count, sizeof *encoders);
    if (encoders && (encoders[index].pairs || build_encoder(&encoders[index], table)))
        found = &encoders[index];
    UNLOCK();
    return found;
}

static int decode_code_page(const struct CodePageTable *table, const unsigned char *in, size_t left, uint32_t *c)
{
    unsigned char lead = in[0], trail;
    uint16_t value;
    if (lead < 0x80) {
        *c = lead;
        return 1;
    }
    if (table->single[lead - 0x80]) {
        *c = table->single[lead - 0x80];
        return 1;
    }
    if (!table->double_byte || lead < CODE_PAGE_LEAD_FIRST || lead > CODE_PAGE_LEAD_LAST)
        return DECODE_INVALID;
    if (left < 2)
        return DECODE_INCOMPLETE;
    trail = in[1];
    if (trail < CODE_PAGE_TRAIL_FIRST || trail > CODE_PAGE_TRAIL_LAST)
        return DECODE_INVALID;
    value = table->double_byte[(lead - CODE_PAGE_LEAD_FIRST) * CODE_PAGE_TRAILS + trail - CODE_PAGE_TRAIL_FIRST];
    if (!value)
        return DECODE_INVALID;
    *c = value;
    return 2;
}

static int decode_utf8(const unsigned char *in, size_t left, uint32_t *c)
{
    static const uint32_t least[] = {0, 0, 0x80, 0x800, 0x10000};
    unsigned char first = in[0];
    int length, i;
    uint32_t value;
    if (first < 0x80) {
        *c = first;
        return 1;
    }
    if (first >= 0xc2 && first <= 0xdf)
        length = 2, value = first & 0x1f;
    else if (first >= 0xe0 && first <= 0xef)
        length = 3, value = first & 0x0f;
    else if (first >= 0xf0 && first <= 0xf4)
        length = 4, value = first & 0x07;
    else
        return DECODE_INVALID;
    for (i = 1; i < length; ++i) {
        if ((size_t)i >= left)
            return DECODE_INCOMPLETE;
        if ((in[i] & 0xc0) != 0x80)
            return DECODE_INVALID;
        value = value << 6 | (in[i] & 0x3f);
    }
    if (value < least[length] || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff))
        return DECODE_INVALID;
    *c = value;
    return length;
}

static unsigned read_unit(const unsigned char *in, int big)
{
    return big ? (unsigned)in[0] << 8 | in[1] : (unsigned)in[1] << 8 | in[0];
}

static int decode_utf16(const unsigned char *in, size_t left, int big, uint32_t *c)
{
    unsigned unit, low;
    if (left < 2)
        return DECODE_INCOMPLETE;
    unit = read_unit(in, big);
    if (unit < 0xd800 || unit > 0xdfff) {
        *c = unit;
        return 2;
    }
    if (unit > 0xdbff)
        return DECODE_INVALID;
    if (left < 4)
        return DECODE_INCOMPLETE;
    low = read_unit(in + 2, big);
    if (low < 0xdc00 || low > 0xdfff)
        return DECODE_INVALID;
    *c = 0x10000 + ((uint32_t)(unit - 0xd800) << 10 | (low - 0xdc00));
    return 4;
}

static int decode(const struct Charset *charset, const unsigned char *in, size_t left, uint32_t *c)
{
    switch (charset->form) {
    case FORM_CODE_PAGE: return decode_code_page(charset->table, in, left, c);
    case FORM_UTF8: return decode_utf8(in, left, c);
    case FORM_UTF16LE: return decode_utf16(in, left, 0, c);
    default: return decode_utf16(in, left, 1, c);
    }
}

static int encode_code_page(const struct CodePageTable *table, uint32_t c, unsigned char *out, size_t room)
{
    const struct Encoder *encoder;
    size_t low = 0, high;
    if (c < 0x80) {
        if (!room)
            return ENCODE_NO_ROOM;
        out[0] = (unsigned char)c;
        return 1;
    }
    if (c > 0xffff || !(encoder = encoder_for(table)))
        return ENCODE_UNMAPPED;
    high = encoder->count;
    while (low < high) {
        size_t middle = low + (high - low) / 2;
        uint32_t character = encoder->pairs[middle] >> 16;
        if (character == c) {
            unsigned code = encoder->pairs[middle] & 0xffff;
            if (code < 0x100) {
                if (!room)
                    return ENCODE_NO_ROOM;
                out[0] = (unsigned char)code;
                return 1;
            }
            if (room < 2)
                return ENCODE_NO_ROOM;
            out[0] = (unsigned char)(code >> 8);
            out[1] = (unsigned char)code;
            return 2;
        }
        if (character < c)
            low = middle + 1;
        else
            high = middle;
    }
    return ENCODE_UNMAPPED;
}

static int encode_utf8(uint32_t c, unsigned char *out, size_t room)
{
    int length = c < 0x80 ? 1 : c < 0x800 ? 2 : c < 0x10000 ? 3 : 4, i;
    if (room < (size_t)length)
        return ENCODE_NO_ROOM;
    if (length == 1) {
        out[0] = (unsigned char)c;
        return 1;
    }
    for (i = length - 1; i > 0; --i, c >>= 6)
        out[i] = (unsigned char)(0x80 | (c & 0x3f));
    out[0] = (unsigned char)((0xf00 >> length) | c);
    return length;
}

static void write_unit(unsigned char *out, unsigned unit, int big)
{
    out[big ? 0 : 1] = (unsigned char)(unit >> 8);
    out[big ? 1 : 0] = (unsigned char)unit;
}

static int encode_utf16(uint32_t c, unsigned char *out, size_t room, int big)
{
    if (c < 0x10000) {
        if (room < 2)
            return ENCODE_NO_ROOM;
        write_unit(out, c, big);
        return 2;
    }
    if (room < 4)
        return ENCODE_NO_ROOM;
    c -= 0x10000;
    write_unit(out, 0xd800 | (c >> 10), big);
    write_unit(out + 2, 0xdc00 | (c & 0x3ff), big);
    return 4;
}

static int encode(const struct Charset *charset, uint32_t c, unsigned char *out, size_t room)
{
    switch (charset->form) {
    case FORM_CODE_PAGE: return encode_code_page(charset->table, c, out, room);
    case FORM_UTF8: return encode_utf8(c, out, room);
    case FORM_UTF16LE: return encode_utf16(c, out, room, 0);
    default: return encode_utf16(c, out, room, 1);
    }
}

rage_iconv_t rage_iconv_open(const char *to, const char *from)
{
    struct CodeConverter *converter;
    struct Charset target, source;
    if (!to || !from || !parse_charset(to, &target) || !parse_charset(from, &source)) {
        errno = EINVAL;
        return (rage_iconv_t)-1;
    }
    converter = malloc(sizeof *converter);
    if (!converter) {
        errno = ENOMEM;
        return (rage_iconv_t)-1;
    }
    converter->to = target;
    converter->from = source;
    return converter;
}

/* As iconv: converts until the input ends, then returns 0, or stops at the first character that
   cannot be read (EILSEQ), is cut off (EINVAL), has no code in the target (EILSEQ) or does not
   fit (E2BIG), with the input left at it, and returns (size_t)-1. */
size_t rage_iconv(rage_iconv_t converter, char **in, size_t *in_left, char **out, size_t *out_left)
{
    if (!converter || converter == (rage_iconv_t)-1) {
        errno = EBADF;
        return (size_t)-1;
    }
    if (!in || !*in)
        return 0;
    while (*in_left) {
        uint32_t c;
        int read = decode(&converter->from, (const unsigned char *)*in, *in_left, &c), written;
        if (read < 0) {
            errno = read == DECODE_INCOMPLETE ? EINVAL : EILSEQ;
            return (size_t)-1;
        }
        written = encode(&converter->to, c, (unsigned char *)*out, *out_left);
        if (written < 0) {
            errno = written == ENCODE_NO_ROOM ? E2BIG : EILSEQ;
            return (size_t)-1;
        }
        *in += read;
        *in_left -= (size_t)read;
        *out += written;
        *out_left -= (size_t)written;
    }
    return 0;
}

int rage_iconv_close(rage_iconv_t converter)
{
    if (!converter || converter == (rage_iconv_t)-1) {
        errno = EBADF;
        return -1;
    }
    free(converter);
    return 0;
}
#endif
