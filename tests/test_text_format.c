#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "core/text_markup.h"

static int live_blocks, allocation_calls, fail_allocation;
struct Allocation { size_t size; unsigned guard; };
static void *checked_malloc(size_t size)
{
    struct Allocation *p;
    ++allocation_calls;
    if (fail_allocation) return NULL;
    p = malloc(sizeof(*p) + size + 16);
    assert(p);
    p->size = size; p->guard = 0x71923acdu;
    memset(p + 1, 0xa5, size + 16);
    ++live_blocks;
    return p + 1;
}
static void checked_free(void *block)
{
    struct Allocation *p;
    size_t i;
    if (!block) return;
    p = (struct Allocation *)block - 1;
    assert(p->guard == 0x71923acdu && live_blocks > 0);
    for (i = 0; i < 16; ++i) assert(((unsigned char *)block)[p->size + i] == 0xa5);
    memset(block, 0xdd, p->size);
    p->guard = 0;
    --live_blocks;
    free(p);
}
#define malloc checked_malloc
#define free checked_free
#include "../src/core/owned_text.c"
#include "../src/core/text_format.c"
#undef malloc
#undef free

static void plain(const char *input, size_t length, const char *expected, size_t expected_size)
{
    char *buffer = malloc(length + 2);
    size_t result;
    assert(buffer);
    memcpy(buffer, input, length);
    buffer[length + 1] = '!';
    result = text_strip_colour_codes(buffer, length, 949);
    assert(result == expected_size && memcmp(buffer, expected, result) == 0);
    assert(buffer[result] == 0 && buffer[length + 1] == '!');
    free(buffer);
}

static void fixtures(void)
{
    struct OwnedText out = {0};
    unsigned long colour = 0xdeadbeef;
    const char *s = "^123AbCtail";
    char incomplete[] = "^12345";
    unsigned byte, position;
    int allocations = allocation_calls;
#define PLAIN(in, out) plain(in, sizeof(in)-1, out, sizeof(out)-1)
    PLAIN("", "");
    PLAIN("hello", "hello");
    PLAIN("^000000a^12aBcFb^FFFFFF", "ab");
    PLAIN("앞^00ff00가나다^FFFFFF뒤", "앞가나다뒤");
    PLAIN("a\0^123456b\0c", "a\0b\0c");
    PLAIN("^12345", "^12345");
    PLAIN("^GGGGGGx^000000y", "^GGGGGGxy");
    PLAIN("abc^1", "abc^1");
#undef PLAIN
    assert(text_read_colour_code(s, s + strlen(s), &colour) == s + 7);
    assert(colour == 0xbc3a12);
    assert(text_read_colour_code(incomplete, incomplete + 6, &colour) == incomplete);
    assert(colour == 0xbc3a12);
    for (position = 1; position < 7; ++position) {
        char code[] = "^123456";
        for (byte = 0; byte < 256; ++byte) {
            int valid = (byte >= '0' && byte <= '9') || (byte >= 'a' && byte <= 'f') ||
                        (byte >= 'A' && byte <= 'F');
            code[position] = (char)byte;
            assert(text_is_colour_code(code, code + 7) == valid);
        }
    }
    assert(text_find_charset_tag("a|c0b", 5, 1) == 1);
    assert(text_find_charset_tag("a|c0b", 5, 0) == 5);
    assert(text_find_charset_tag("a|c0b", 3, 1) == 3);
    assert(text_find_charset_tag("a\0|c0", 5, 1) == 5);
    assert(text_prefix_length("가나다", sizeof("가나다")-1, 1, 949) == 0);
    assert(text_prefix_length("가나다", sizeof("가나다")-1, 3, 949) == 2);
    assert(text_prefix_length("abc", 3, 0, 949) == 0);
    assert(text_prefix_length("abc", 3, 20, 949) == 3);
    assert(!text_has_markup_tags("", 0));
    assert(!text_has_markup_tags("<INFO>x</INFO>", 14));
    assert(!text_has_markup_tags("<ITEM>", 5));
    assert(text_has_markup_tags("x\0<ITEM>", 8));
    assert(text_has_markup_tags("<ITEML>", 7));
    assert(!text_has_markup_tags("<item>", 6));
    assert(allocation_calls == allocations);

    assert(text_copy_without_colours(&out, text_view_cstr("|c0abc"), 949, 1));
    assert(strcmp(owned_text_cstr(&out), "|c0|c0abc") == 0);
    assert(text_copy_without_colours(&out, text_view_cstr("a|c0b^123456c^abcdefd"), 949, 1));
    assert(strcmp(owned_text_cstr(&out), "|c0a|c0b|c0c|c0d") == 0);
    assert(text_copy_without_colours(&out, text_view_cstr("^123456^abcdef|c0x"), 949, 1));
    assert(strcmp(owned_text_cstr(&out), "|c0|c0x") == 0);
    assert(text_copy_without_colours(&out, text_view_cstr("|c0a^123456b"), 949, 0));
    assert(strcmp(owned_text_cstr(&out), "|c0ab") == 0);
    assert(text_copy_without_colours(&out, text_view("a\0^123456b", 10), 949, 0));
    assert(out.size == 3 && memcmp(out.data, "a\0b", 3) == 0);
    owned_text_clear(&out);
    assert(live_blocks == 0);
}

static void ownership(void)
{
    struct OwnedText out = {0};
    char *prior;
    size_t size;
    int calls;
    assert(owned_text_assign(&out, text_view_cstr("keep |c0a^123456b")));
    prior = out.data; size = out.size;
    fail_allocation = 1;
    assert(!text_copy_without_colours(&out, text_view(out.data + 5, out.size - 5), 949, 1));
    assert(out.data == prior && out.size == size && strcmp(out.data, "keep |c0a^123456b") == 0);
    calls = allocation_calls;
    assert(!text_copy_without_colours(&out, text_view("", (size_t)-1), 949, 1));
    assert(allocation_calls == calls && out.data == prior && out.size == size);
    fail_allocation = 0;
    calls = allocation_calls;
    assert(text_copy_without_colours(&out, text_view(out.data + 5, out.size - 5), 949, 1));
    assert(strcmp(out.data, "|c0|c0a|c0b") == 0 && live_blocks == 1);
    assert(allocation_calls == calls + 1);
    assert(text_copy_without_colours(&out, text_view("", 0), 949, 1));
    assert(out.size == 0 && !out.data && live_blocks == 0);
    owned_text_clear(&out);
    owned_text_clear(&out);
}

static void guarded_inputs(void)
{
    static const char *const inputs[] = { "^123456|c0tail", "a^12345", "|c", "가^12ab56나", "a\0^123456b" };
    static const size_t sizes[] = { 13, 7, 2, sizeof("가^12ab56나")-1, 10 };
    SYSTEM_INFO info;
    unsigned char *pages;
    DWORD old;
    size_t i, length;
    GetSystemInfo(&info);
    pages = VirtualAlloc(NULL, info.dwPageSize * 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    assert(pages && VirtualProtect(pages + info.dwPageSize, info.dwPageSize, PAGE_NOACCESS, &old));
    for (i = 0; i < sizeof(inputs)/sizeof(inputs[0]); ++i) {
        for (length = 0; length <= sizes[i]; ++length) {
            const char *start = (char *)pages + info.dwPageSize - length;
            const char *end = start + length, *found;
            struct OwnedText out = {0};
            unsigned long colour = 123;
            size_t limit;
            memcpy((char *)start, inputs[i], length);
            found = text_find_colour_code(start, end, 949);
            assert(found >= start && found <= end);
            assert(text_read_colour_code(found, end, &colour) <= end);
            assert(text_find_charset_tag(start, length, 1) <= length);
            (void)text_has_markup_tags(start, length);
            for (limit = 0; limit <= length + 1; ++limit)
                assert(text_prefix_length(start, length, limit, 949) <= limit);
            assert(text_copy_without_colours(&out, text_view(start, length), 949, 1));
            owned_text_clear(&out);
            start--;
            memcpy((char *)start, inputs[i], length);
            assert(text_strip_colour_codes((char *)start, length, 949) <= length);
        }
    }
    assert(VirtualFree(pages, 0, MEM_RELEASE));
    assert(live_blocks == 0);
}

static void code_page_steps(void)
{
    static const unsigned pages[] = { 1252, 874, 932, 936, 949, 950, 1361 };
    size_t cp;
    unsigned first, second;
    for (cp = 0; cp < sizeof(pages)/sizeof(pages[0]); ++cp)
        for (first = 1; first < 256; ++first)
            for (second = 1; second < 256; ++second) {
                char text[3] = { (char)first, (char)second, 0 };
                size_t step = (size_t)(CharNextExA((WORD)pages[cp], text, 0) - text);
                assert(text_prefix_length(text, 2, 1, pages[cp]) == (step == 1 ? 1u : 0u));
            }
}

static unsigned random_state = 0x19a846dfu;
static unsigned random_value(void)
{
    random_state = random_state * 1664525u + 1013904223u;
    return random_state;
}
static void hex_trace(const char *text, size_t length)
{
    size_t i;
    for (i = 0; i < length; ++i) printf("%02x", (unsigned char)text[i]);
}
static void generated_cases(int trace)
{
    static const unsigned code_pages[] = {1252, 874, 932, 936, 949, 950, 1361};
    unsigned sample;
    for (sample = 0; sample < 5000; ++sample) {
        char input[2048] = "A", plain_text[2048];
        size_t size = 1, plain_size, piece, limit;
        unsigned code_page = code_pages[random_value()%7];
        int enabled = (random_value() >> 16) & 1;
        unsigned pieces = random_value()%20;
        struct OwnedText out = {0};
        for (piece = 0; piece < pieces; ++piece) {
            unsigned count = random_value()%20, i;
            if (random_value() & 0x10000) { memcpy(input + size, "^12aB3c", 7); size += 7; }
            if (random_value() & 0x10000) { memcpy(input + size, "|c0", 3); size += 3; }
            if (random_value() & 0x10000) { memcpy(input + size, "<ITEM>", 6); size += 6; }
            for (i = 0; i < count; ++i) {
                unsigned char byte = (unsigned char)(random_value() >> 24);
                if (!byte || byte == '^' || byte == '|') byte = 'x';
                input[size++] = (char)byte;
            }
        }
        input[size] = 0;
        memcpy(plain_text, input, size + 1);
        plain_size = text_strip_colour_codes(plain_text, size, code_page);
        assert(text_copy_without_colours(&out, text_view(input, size), code_page, enabled));
        assert(plain_size <= size && out.size <= size + 3);
        if (trace) {
            printf("%u %u %d %lu %d ", sample, code_page, enabled,
                   (unsigned long)text_find_charset_tag(input, size, enabled), text_has_markup_tags(input, size));
            hex_trace(plain_text, plain_size); putchar(' ');
            hex_trace(owned_text_cstr(&out), out.size); putchar(' ');
            for (limit = 1; limit <= size + 1; ++limit)
                printf("%lu,", (unsigned long)text_prefix_length(input, size, limit, code_page));
            putchar('\n');
        }
        owned_text_clear(&out);
    }
    assert(live_blocks == 0);
}

int main(int argc, char **argv)
{
    int trace = argc == 2 && strcmp(argv[1], "--trace") == 0;
    if (!trace) {
        fixtures(); ownership(); guarded_inputs(); code_page_steps();
    }
    generated_cases(trace);
    if (!trace) puts("Text format: bounded colour/charset parsing, C ownership, alias/OOM safety, protected pages, 455175 SDK character pairs and 5000 generated cases passed.");
    return 0;
}
