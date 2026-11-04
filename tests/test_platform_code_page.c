#include "platform/code_page.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

unsigned int g_codePage = 949;

static size_t next_step(unsigned int code_page, const char *text)
{
    return (size_t)(platform_code_page_next(code_page, text) - text);
}

static void check_wide(unsigned int code_page, const char *text, const uint16_t *expected, int units)
{
    uint16_t wide[16];
    char narrow[16];
    int bytes = (int)strlen(text) + 1;
    assert(platform_code_page_is_valid(code_page, text));
    assert(platform_code_page_to_utf16(code_page, text, -1, NULL, 0) == units);
    assert(platform_code_page_to_utf16(code_page, text, -1, wide, 16) == units);
    assert(!memcmp(wide, expected, (size_t)units * sizeof *wide));
    assert(platform_code_page_from_utf16(code_page, wide, -1, NULL, 0) == bytes);
    assert(platform_code_page_from_utf16(code_page, wide, -1, narrow, 16) == bytes);
    assert(!strcmp(narrow, text));
}

static void check_lead_bytes(void)
{
    const char *korean = "\260\241A\260\260\260";
    assert(next_step(949, "\260\241") == 2 && next_step(949, "\260") == 1 && next_step(949, "") == 0);
    assert(next_step(949, "A\241") == 1 && next_step(949, "\200A") == 1 && next_step(949, "\376A") == 2);
    assert(next_step(932, "\202\240") == 2 && next_step(932, "\240A") == 1 && next_step(932, "\340A") == 2);
    assert(next_step(932, "\375A") == 1 && next_step(936, "\376A") == 2 && next_step(950, "\201A") == 2);
    assert(next_step(1252, "\260\241") == 1 && next_step(874, "\260\241") == 1 && next_step(65001, "\303\244") == 1);
    assert(platform_code_page_next(949, korean + 3) == korean + 5);
    assert(platform_code_page_next(949, korean + 5) == korean + 6);
    assert(platform_code_page_prev(949, korean, korean + 6) == korean + 5);
    assert(platform_code_page_prev(949, korean, korean + 5) == korean + 3);
    assert(platform_code_page_prev(949, korean, korean + 2) == korean);
    assert(platform_code_page_prev(949, korean, korean) == korean);
}

static void check_byte_types(void)
{
    const char *text = "a\260\241\001\200 \260 \377";
    const int expected[] = {
        CODE_PAGE_BYTE_SINGLE, CODE_PAGE_BYTE_LEAD, CODE_PAGE_BYTE_TRAIL, CODE_PAGE_BYTE_ILLEGAL, CODE_PAGE_BYTE_ILLEGAL,
        CODE_PAGE_BYTE_SINGLE, CODE_PAGE_BYTE_LEAD, CODE_PAGE_BYTE_ILLEGAL, CODE_PAGE_BYTE_ILLEGAL
    };
    size_t i;
    platform_code_page_set_multibyte(949);
    for (i = 0; i < sizeof expected / sizeof *expected; ++i)
        assert(platform_code_page_byte_type(text, i) == expected[i]);
    assert(platform_code_page_byte_type("\260[", 1) == CODE_PAGE_BYTE_TRAIL);
    platform_code_page_set_multibyte(932);
    assert(platform_code_page_byte_type("\261", 0) == CODE_PAGE_BYTE_SINGLE);
    assert(platform_code_page_byte_type("\201\240", 1) == CODE_PAGE_BYTE_TRAIL);
    assert(platform_code_page_byte_type("\201\177", 1) == CODE_PAGE_BYTE_ILLEGAL);
    platform_code_page_set_multibyte(1252);
    assert(platform_code_page_byte_type("\260\241", 1) == CODE_PAGE_BYTE_SINGLE);
}

static void check_find(void)
{
    const char *text = "\260<x<", *korean = "\260\241]a]";
    platform_code_page_set_multibyte(949);
    assert(platform_code_page_find(text, "<") == text + 3);
    assert(platform_code_page_find(text, "") == text);
    assert(!platform_code_page_find(text, "<x<<"));
    assert(platform_code_page_find(korean, "]") == korean + 2);
    assert(!platform_code_page_find(korean, "\241]"));
    assert(!platform_code_page_find("\260", "\260\241"));
    platform_code_page_set_multibyte(1252);
    assert(platform_code_page_find(text, "<") == text + 1);
    assert(platform_code_page_find(korean, "\241]") == korean + 1);
}

static void check_conversions(void)
{
    static const uint16_t korean[] = {0xac00, 0xb098, 0}, japanese[] = {0x3042, 0}, simplified[] = {0x4f60, 0};
    static const uint16_t western[] = {0x20ac, 0x00e4, 0}, cyrillic[] = {0x0416, 0}, utf8[] = {0x00e4, 0xac00, 0};
    uint16_t wide[8];
    char narrow[8];
    check_wide(949, "\260\241\263\252", korean, 3);
    check_wide(932, "\202\240", japanese, 2);
    check_wide(936, "\304\343", simplified, 2);
    check_wide(950, "\247\101", simplified, 2);
    check_wide(1252, "\200\344", western, 3);
    check_wide(1251, "\306", cyrillic, 2);
    check_wide(CODE_PAGE_UTF8, "\303\244\352\260\200", utf8, 3);
    assert(!platform_code_page_to_utf16(949, "abc", 0, NULL, 0));
    assert(platform_code_page_to_utf16(949, "a\260\241b", 3, NULL, 0) == 2);
    assert(platform_code_page_to_utf16(949, "a\260\241b", 3, wide, 8) == 2 && wide[0] == 'a' && wide[1] == 0xac00);
    memset(wide, 0x55, sizeof wide);
    assert(!platform_code_page_to_utf16(949, "[abc", -1, wide, 4));
    assert(wide[0] == '[' && wide[3] == 'c' && wide[4] == 0x5555);
    memset(narrow, 0x55, sizeof narrow);
    assert(!platform_code_page_from_utf16(949, korean, -1, narrow, 3));
    assert((unsigned char)narrow[0] == 0260 && (unsigned char)narrow[1] == 0241 && narrow[2] == 0x55);
    assert(!platform_code_page_is_valid(CODE_PAGE_UTF8, "\303"));
    assert(!platform_code_page_is_valid(CODE_PAGE_UTF8, "\377"));
}

static void check_case(void)
{
    static const uint16_t text[] = {'x', 'x', 'A', 'b', 'C', 0x00c4, 0x0416, 0x0401, 0xff21, 0x1ea0, 0x2160, 0};
    static const uint16_t latin[] = {'a', 'B', 'c', 0}, wide[] = {0x00e4, 0x0436, 0x0451, 0xff41, 0x1ea1, 0x2170, 0};
    static const uint16_t dotted[] = {0x0130, 0}, plain_i[] = {'i', 0}, empty[] = {0}, missing[] = {'y', 0};
    assert(platform_utf16_find_ignore_case(text, latin) == text + 2);
    assert(platform_utf16_find_ignore_case(text, wide) == text + 5);
    assert(!platform_utf16_find_ignore_case(text, empty));
    assert(!platform_utf16_find_ignore_case(text, missing));
    assert(!platform_utf16_find_ignore_case(dotted, plain_i));
}

#ifndef _WIN32
static void check_client_code_page(void)
{
    static const uint16_t korean[] = {0xac00, 0}, euro[] = {0x20ac, 0}, thai[] = {0x0e01, 0};
    const char *marked = "\260<<";
    uint16_t wide[4];
    char narrow[4];
    g_codePage = 949;
    platform_code_page_set_multibyte(CODE_PAGE_SYSTEM);
    assert(next_step(CODE_PAGE_SYSTEM, "\260\241") == 2 && next_step(CODE_PAGE_THREAD, "\260\241") == 2);
    assert(platform_code_page_is_system_lead_byte(0xb0) && !platform_code_page_is_system_lead_byte('A'));
    assert(platform_code_page_find(marked, "<") == marked + 2);
    check_wide(CODE_PAGE_THREAD, "\260\241", korean, 2);
    g_codePage = 0;
    assert(next_step(CODE_PAGE_SYSTEM, "\260\241") == 1 && !platform_code_page_is_system_lead_byte(0xb0));
    check_wide(CODE_PAGE_SYSTEM, "\200", euro, 2);
    g_codePage = 949;
    memset(wide, 0x55, sizeof wide);
    assert(!platform_code_page_to_utf16(949, "a\260 ", -1, wide, 4) && !wide[0]);
    assert(!platform_code_page_to_utf16(949, "a\260", 2, NULL, 0));
    memset(narrow, 0x55, sizeof narrow);
    assert(!platform_code_page_from_utf16(949, thai, -1, narrow, 4) && !narrow[0]);
    assert(!platform_code_page_to_utf16(12345, "a", -1, NULL, 0));
}
#endif

int main(void)
{
    check_lead_bytes();
    check_byte_types();
    check_find();
    check_conversions();
    check_case();
#ifndef _WIN32
    check_client_code_page();
#endif
    puts("Code pages: lead bytes, byte types, multibyte search, UTF-16 conversion and case folding passed");
    return 0;
}
