#include "platform/iconv_port.h"
#include "platform/code_page_tables.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

static size_t convert(const char *to, const char *from, const char *in, size_t in_size, char *out, size_t out_size,
                      int *error)
{
    rage_iconv_t converter = rage_iconv_open(to, from);
    char *src = (char *)in, *dst = out;
    size_t left = in_size, room = out_size, result;
    assert(converter != (rage_iconv_t)-1);
    errno = 0;
    result = rage_iconv(converter, &src, &left, &dst, &room);
    *error = result == (size_t)-1 ? errno : 0;
    rage_iconv_close(converter);
    return out_size - room;
}

static void expect(const char *to, const char *from, const char *in, size_t in_size, const char *want, size_t want_size)
{
    char out[64];
    int error;
    size_t n = convert(to, from, in, in_size, out, sizeof out, &error);
    if (error || n != want_size || memcmp(out, want, n)) {
        fprintf(stderr, "%s -> %s: got %zu bytes, error %d\n", from, to, n, error);
        assert(0);
    }
}

static void expect_error(const char *to, const char *from, const char *in, size_t in_size, size_t room, int want)
{
    char out[64];
    int error;
    convert(to, from, in, in_size, out, room, &error);
    assert(error == want);
}

static unsigned code_of(size_t i)
{
    return (unsigned)(CODE_PAGE_LEAD_FIRST + i / CODE_PAGE_TRAILS) << 8 | (unsigned)(CODE_PAGE_TRAIL_FIRST + i % CODE_PAGE_TRAILS);
}

static int is_decode_only(const struct CodePageTable *table, unsigned code)
{
    size_t i;
    for (i = 0; i < table->decode_only_count; ++i)
        if (table->decode_only[i] == code)
            return 1;
    return 0;
}

/* Every code of every table decodes to its character as UTF-16 and, unless another code owns the
   character, encodes back to itself; on Windows both directions are also checked against the OS. */
static void check_table(const struct CodePageTable *table)
{
    char name[16];
    size_t i, pairs = table->double_byte ? 126 * CODE_PAGE_TRAILS : 0;
    snprintf(name, sizeof name, "CP%u", table->code_page);
    for (i = 0; i < 128 + pairs; ++i) {
        unsigned code = i < 128 ? (unsigned)(0x80 + i) : code_of(i - 128);
        unsigned character = i < 128 ? table->single[i] : table->double_byte[i - 128];
        unsigned char bytes[2], unit[2], back[2];
        int length = code < 0x100 ? 1 : 2, error;
        size_t n;
        if (length == 1)
            bytes[0] = (unsigned char)code;
        else
            bytes[0] = (unsigned char)(code >> 8), bytes[1] = (unsigned char)code;
        n = convert("UTF-16LE", name, (const char *)bytes, (size_t)length, (char *)unit, sizeof unit, &error);
#ifdef _WIN32
        {
            WCHAR wide[4];
            int windows = MultiByteToWideChar(table->code_page, MB_ERR_INVALID_CHARS, (LPCSTR)bytes, length, wide, 4);
            assert(character ? windows == 1 && wide[0] == character : windows != 1 || !wide[0]);
        }
#endif
        if (!character) {
            assert(error);
            continue;
        }
        assert(!error && n == 2 && (unsigned)(unit[0] | unit[1] << 8) == character);
        n = convert(name, "UTF-16LE", (const char *)unit, 2, (char *)back, sizeof back, &error);
        assert(!error);
        if (!is_decode_only(table, code))
            assert(n == (size_t)length && !memcmp(back, bytes, n));
#ifdef _WIN32
        {
            WCHAR wide = (WCHAR)character;
            char windows[4];
            BOOL used = FALSE;
            int w = WideCharToMultiByte(table->code_page, WC_NO_BEST_FIT_CHARS, &wide, 1, windows, sizeof windows,
                                        NULL, &used);
            assert(!used && w == (int)n && !memcmp(windows, back, n));
        }
#endif
    }
}

int main(void)
{
    size_t i;
    /* "한글 abc" */
    expect("UTF-8", "CP949", "\xc7\xd1\xb1\xdb abc", 8, "\xed\x95\x9c\xea\xb8\x80 abc", 10);
    expect("CP949", "UTF-8", "\xed\x95\x9c\xea\xb8\x80 abc", 10, "\xc7\xd1\xb1\xdb abc", 8);
    /* UHC's extension Hangul and a Hanja. */
    expect("UTF-8", "CP949", "\x81\x41\xca\xa1", 4, "\xea\xb0\x82\xe4\xbc\xbd", 6);
    /* Half-width katakana are single bytes in 932, the euro sign is 0x80 in 1252 and 936. */
    expect("UTF-16LE", "CP932", "\xb1\x82\xa0", 3, "\x71\xff\x42\x30", 4);
    expect("UTF-8", "CP1252", "\x80", 1, "\xe2\x82\xac", 3);
    expect("CP936", "UTF-8", "\xe2\x82\xac", 3, "\x80", 1);
    expect("UTF-16BE", "UTF-8", "\xf0\x9f\x98\x80", 4, "\xd8\x3d\xde\x00", 4);
    expect("UTF-8", "UTF-16LE", "\x3d\xd8\x00\xde", 4, "\xf0\x9f\x98\x80", 4);
    /* Unreadable, cut off, unmappable, no room. */
    expect_error("UTF-8", "CP949", "\xc7\x20", 2, 64, EILSEQ);
    expect_error("UTF-8", "CP949", "\xc7", 1, 64, EINVAL);
    expect_error("UTF-8", "UTF-8", "\xc0\x80", 2, 64, EILSEQ);
    expect_error("UTF-8", "UTF-8", "\xed\xa0\x80", 3, 64, EILSEQ);
    expect_error("CP949", "UTF-8", "\xe0\xb8\x81", 3, 64, EILSEQ);
    expect_error("CP949", "UTF-8", "\xed\x95\x9c", 3, 1, E2BIG);
    assert(rage_iconv_open("UTF-8", "CP1145") == (rage_iconv_t)-1 && errno == EINVAL);
    assert(rage_iconv_open("KOI8-R", "UTF-8") == (rage_iconv_t)-1);
    for (i = 0; i < code_page_table_count; ++i)
        check_table(&code_page_tables[i]);
    printf("code page iconv: %zu code pages, samples, errors and round trips passed\n", code_page_table_count);
    return 0;
}
