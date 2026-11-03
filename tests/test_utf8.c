#include "core/utf8.h"
#include <windows.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define S(text) text, sizeof text - 1

static unsigned char *guard_page;
static size_t page_size;

/* Copies bytes so they end right before an inaccessible page. */
static const char *at_page_end(const char *bytes, size_t length)
{
    char *at = (char *)guard_page + page_size - length;

    memcpy(at, bytes, length);
    return at;
}

static void expect_char(const char *text, size_t length, long code, size_t size)
{
    size_t got = 99;
    const char *guarded = at_page_end(text, length);

    assert(utf8_decode(guarded, length, &got) == code && got == size);
    assert(utf8_char_size(guarded, length) == size);
}

static void check_decode(void)
{
    expect_char(S("A"), 'A', 1);
    expect_char(S("\x00"), 0, 1);
    expect_char(S("\xc3\xa9"), 0xe9, 2);                 /* é */
    expect_char(S("\xd0\x96"), 0x416, 2);                /* Ж */
    expect_char(S("\xed\x95\x9c"), 0xd55c, 3);           /* 한 */
    expect_char(S("\xe2\x82\xac"), 0x20ac, 3);           /* € */
    expect_char(S("\xf0\x9f\x98\x80"), 0x1f600, 4);      /* 😀 */
    expect_char(S("\xf4\x8f\xbf\xbf"), 0x10ffff, 4);
    expect_char(S("\xef\xbf\xbf"), 0xffff, 3);
    expect_char(S("\x80"), UTF8_INVALID, 1);
    expect_char(S("\xbf\x80"), UTF8_INVALID, 1);
    expect_char(S("\xc0\x80"), UTF8_INVALID, 1);         /* overlong NUL */
    expect_char(S("\xc1\xbf"), UTF8_INVALID, 1);
    expect_char(S("\xe0\x80\x80"), UTF8_INVALID, 1);
    expect_char(S("\xe0\x9f\xbf"), UTF8_INVALID, 1);
    expect_char(S("\xf0\x80\x80\x80"), UTF8_INVALID, 1);
    expect_char(S("\xf0\x8f\xbf\xbf"), UTF8_INVALID, 1);
    expect_char(S("\xed\xa0\x80"), UTF8_INVALID, 1);     /* surrogate */
    expect_char(S("\xed\xbf\xbf"), UTF8_INVALID, 1);
    expect_char(S("\xf4\x90\x80\x80"), UTF8_INVALID, 1); /* above U+10FFFF */
    expect_char(S("\xf5\x80\x80\x80"), UTF8_INVALID, 1);
    expect_char(S("\xff"), UTF8_INVALID, 1);
    expect_char(S("\xe2\x82"), UTF8_INVALID, 1);         /* truncated at the end */
    expect_char(S("\xf0\x9f\x98"), UTF8_INVALID, 1);
    expect_char(S("\xe2\x82\x41"), UTF8_INVALID, 1);     /* truncated before ASCII */
    expect_char(S("\xc3\xc3\xa9"), UTF8_INVALID, 1);
    assert(utf8_char_size("\xc3\xa9", 1) == 1);
    assert(utf8_char_size("", 0) == 0 && utf8_decode("x", 0, NULL) == UTF8_INVALID);
}

/* Every byte sequence of up to three bytes, and four-byte ones over all lead and second
   bytes, must agree with Windows' strict UTF-8 decoder. */
static void check_against_windows(void)
{
    unsigned a, b, c, d;
    unsigned char s[4];
    WCHAR wide[4];

    for (a = 0; a < 256; a++) {
        for (b = 0; b < 256; b++) {
            size_t size;
            long code;
            int n;

            s[0] = (unsigned char)a;
            s[1] = (unsigned char)b;
            code = utf8_decode((const char *)s, 2, &size);
            n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (const char *)s, (int)size, wide, 4);
            assert((code != UTF8_INVALID) == (n > 0));
            if (a >= 0xe0 && a <= 0xef) {
                for (c = 0; c < 256; c += (b & 0xc0) == 0x80 ? 1 : 64) {
                    s[2] = (unsigned char)c;
                    code = utf8_decode((const char *)s, 3, &size);
                    n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (const char *)s, 3, wide, 4);
                    assert((code != UTF8_INVALID) == (size == 3));
                    assert((size == 3) == (n == 1));
                    if (n == 1)
                        assert((unsigned long)code == wide[0]);
                }
            }
            if (a >= 0xf0) {
                static const unsigned char tails[] = { 0x00, 0x7f, 0x80, 0x9f, 0xa0, 0xbf, 0xc0, 0xff };

                for (c = 0; c < sizeof tails; c++) {
                    for (d = 0; d < sizeof tails; d++) {
                        s[2] = tails[c];
                        s[3] = tails[d];
                        code = utf8_decode((const char *)s, 4, &size);
                        n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (const char *)s, 4, wide, 4);
                        assert((size == 4) == (n == 2));
                        if (n == 2)
                            assert((unsigned long)code
                                   == 0x10000 + ((unsigned long)(wide[0] - 0xd800) << 10) + (wide[1] - 0xdc00));
                    }
                }
            }
        }
    }
}

static void check_encode(void)
{
    unsigned long c;
    char out[UTF8_CHAR_MAX];
    size_t n, size;

    for (c = 0; c <= 0x10ffff; c++) {
        n = utf8_encode(c, out);
        if (c >= 0xd800 && c <= 0xdfff) {
            assert(n == 0);
            continue;
        }
        assert(n == (c < 0x80 ? 1u : c < 0x800 ? 2u : c < 0x10000 ? 3u : 4u));
        assert(utf8_decode(out, n, &size) == (long)c && size == n);
        assert(utf8_valid(out, n));
    }
    assert(utf8_encode(0x110000, out) == 0 && utf8_encode(0xffffffffUL, out) == 0);
}

/* Boundaries found stepping forward must equal those found stepping back. */
static void check_steps(const char *text, size_t length)
{
    const char *forward[600], *back[600];
    size_t nf = 0, nb = 0, i;
    const char *p, *end = text + length;

    for (p = text; p < end; p = utf8_step(p, end))
        forward[nf++] = p;
    assert(p == end && utf8_step(end, end) == end);
    for (p = end; p > text;) {
        p = utf8_prev(text, p);
        back[nb++] = p;
    }
    assert(nf == nb);
    for (i = 0; i < nf; i++)
        assert(forward[i] == back[nf - 1 - i]);
    assert(utf8_prev(text, text) == text);
}

static void check_navigation(void)
{
    static const char mixed[] = "a\xc3\xa9\xed\x95\x9c\xf0\x9f\x98\x80\x80\xe2\x82\x41\xff\xd0\x96";
    static const unsigned char pool[] = {
        'a', 0x80, 0x9f, 0xa0, 0xbf, 0xc2, 0xc3, 0xd0, 0xe0, 0xe2, 0xed, 0xef, 0xf0, 0xf4, 0xf5, 0xff
    };
    char text[512];
    const char *p, *guarded;
    uint32_t state = 0x12345678u;
    size_t trial, i;

    p = mixed;
    p = utf8_next(p); assert(p == mixed + 1);
    p = utf8_next(p); assert(p == mixed + 3);
    p = utf8_next(p); assert(p == mixed + 6);
    p = utf8_next(p); assert(p == mixed + 10);
    p = utf8_next(p); assert(p == mixed + 11);       /* stray continuation */
    p = utf8_next(p); assert(p == mixed + 12);       /* truncated lead keeps the 'A' */
    p = utf8_next(p); assert(p == mixed + 13);
    p = utf8_next(p); assert(p == mixed + 14);
    p = utf8_next(p); assert(p == mixed + 15);
    p = utf8_next(p); assert(p == mixed + 17 && !*p);
    assert(utf8_next(p) == p);
    check_steps(mixed, sizeof mixed - 1);

    /* A lead byte right before the terminator at the end of a page is not over-read. */
    guarded = at_page_end("\xf0\x9f\x98\x00", 4);
    assert(utf8_next(guarded) == guarded + 1 && utf8_next(guarded + 3) == guarded + 3);
    guarded = at_page_end("\xe2\x82", 2);
    assert(utf8_prev(guarded, guarded + 2) == guarded + 1);
    assert(utf8_prev(guarded, guarded + 1) == guarded);
    guarded = at_page_end("\x80\x80\x80\x80\x80", 5);
    assert(utf8_prev(guarded, guarded + 5) == guarded + 4);
    check_steps(guarded, 5);

    for (trial = 0; trial < 20000; trial++) {
        size_t length = trial % 500;

        for (i = 0; i < length; i++) {
            state = state * 1664525u + 1013904223u;
            text[i] = (char)(state >> 28 < 4 ? pool[(state >> 20) % sizeof pool] : (state >> 21) & 0xff);
        }
        guarded = at_page_end(text, length);
        check_steps(guarded, length);
        assert(utf8_prefix(guarded, length, length) == length);
        for (i = 0; i <= length && i < 40; i++) {
            size_t cut = utf8_prefix(guarded, length, i), at = 0;

            assert(cut <= i);
            while (at < cut)
                at += utf8_char_size(guarded + at, length - at);
            assert(at == cut);
            assert(cut == length || cut + utf8_char_size(guarded + cut, length - cut) > i);
        }
    }
}

static void check_prefix_columns_valid(void)
{
    static const char text[] = "a\xed\x95\x9c" "b";
    static const size_t expected[] = {0, 1, 1, 1, 4, 5, 5};
    size_t i;

    for (i = 0; i < 7; i++)
        assert(utf8_prefix(text, 5, i) == expected[i]);
    assert(utf8_prefix(S("\xf0\x9f\x98\x80"), 3) == 0);
    assert(utf8_prefix(S("\xff\xff\xff"), 2) == 2);
    assert(utf8_prefix(S("\xe2\x82"), 1) == 1);
    assert(utf8_columns(S("abc")) == 3);
    assert(utf8_columns(S("\xed\x95\x9c\xea\xb5\xad")) == 4);   /* 한국 */
    assert(utf8_columns(S("\xd0\x96\xd0\xb6")) == 2);           /* Жж */
    assert(utf8_columns(S("\xef\xbc\xa1")) == 2);               /* fullwidth A */
    assert(utf8_columns(S("\xe4\xb8\xad\xe3\x81\x82")) == 4);   /* 中あ */
    assert(utf8_columns(S("\xf0\x9f\x98\x80")) == 2);
    assert(utf8_columns(S("\xff\x80\xe2\x82")) == 4);
    assert(utf8_columns("", 0) == 0);
    assert(utf8_valid(S("plain \xc3\xa9 \xed\x95\x9c \xf0\x9f\x98\x80")));
    assert(utf8_valid("", 0));
    assert(!utf8_valid(S("ok\x80")) && !utf8_valid(S("\xed\xa0\x80")) && !utf8_valid(S("\xc0\xaf")));
    assert(!utf8_valid(S("\xe2\x82")) && !utf8_valid(S("\xb0\xa1")));   /* CP949 가 is not UTF-8 */
}

static void check_utf16(void)
{
    static const wchar_t sample[] = { 'A', 0xe9, 0xd55c, 0xd83d, 0xde00, 0x416 };
    static const wchar_t lone[] = { 0xd83d, 'x', 0xde00, 0xd83d };
    char out[32];
    unsigned long points[2];
    wchar_t pending = 0;
    size_t n, i;

    n = utf8_from_utf16(out, sizeof out, sample, 6);
    assert(n == 12 && !memcmp(out, "A\xc3\xa9\xed\x95\x9c\xf0\x9f\x98\x80\xd0\x96", 13));
    n = utf8_from_utf16(out, sizeof out, lone, 4);
    assert(n == 10 && !strcmp(out, "\xef\xbf\xbdx\xef\xbf\xbd\xef\xbf\xbd"));
    memset(out, 0x55, sizeof out);
    assert(utf8_from_utf16(out, 0, sample, 6) == 0 && out[0] == 0x55);
    assert(utf8_from_utf16(out, 1, sample, 6) == 0 && !out[0]);
    assert(utf8_from_utf16(out, 3, sample, 6) == 1 && !strcmp(out, "A"));
    assert(utf8_from_utf16(out, 4, sample, 6) == 3 && !strcmp(out, "A\xc3\xa9"));
    for (i = 0; i <= 13; i++) {
        memset(out, 0x55, sizeof out);
        n = utf8_from_utf16(out, i, sample, 6);
        if (i) {
            assert(n < i && !out[n] && utf8_valid(out, n));
            assert(!memcmp(out, "A\xc3\xa9\xed\x95\x9c\xf0\x9f\x98\x80\xd0\x96", n));
            assert(out[i] == 0x55 || i == sizeof out);
        }
    }
    assert(utf8_from_utf16(out, 4, sample + 3, 2) == 0 && !out[0]);  /* 😀 needs four bytes plus NUL */
    assert(utf8_from_utf16(out, 5, sample + 3, 2) == 4);

    assert(utf16_feed(&pending, 'A', points) == 1 && points[0] == 'A' && !pending);
    assert(utf16_feed(&pending, 0xd83d, points) == 0 && pending == 0xd83d);
    assert(utf16_feed(&pending, 0xde00, points) == 1 && points[0] == 0x1f600 && !pending);
    assert(utf16_feed(&pending, 0xde00, points) == 1 && points[0] == UTF8_REPLACEMENT);
    assert(utf16_feed(&pending, 0xd83d, points) == 0);
    assert(utf16_feed(&pending, 0xd55c, points) == 2 && points[0] == UTF8_REPLACEMENT && points[1] == 0xd55c);
    assert(utf16_feed(&pending, 0xd83d, points) == 0);
    assert(utf16_feed(&pending, 0xdbff, points) == 1 && points[0] == UTF8_REPLACEMENT && pending == 0xdbff);
    assert(utf16_feed(&pending, 0xdfff, points) == 1 && points[0] == 0x10ffff && !pending);
}

int main(void)
{
    SYSTEM_INFO info;
    DWORD old;

    GetSystemInfo(&info);
    page_size = info.dwPageSize;
    guard_page = VirtualAlloc(NULL, page_size * 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    assert(guard_page && VirtualProtect(guard_page + page_size, page_size, PAGE_NOACCESS, &old));
    check_decode();
    check_against_windows();
    check_encode();
    check_navigation();
    check_prefix_columns_valid();
    check_utf16();
    assert(VirtualFree(guard_page, 0, MEM_RELEASE));
    puts("UTF-8: decoding against Windows, every code point, guarded next/prev/prefix with invalid bytes, columns and UTF-16 pairs passed.");
    return 0;
}
