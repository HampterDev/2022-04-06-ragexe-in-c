#include "core/text_markup.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

static void check(const char *input, size_t length, const char *expected,
                  size_t expected_length, const char *open, const char *close)
{
    unsigned char *allocation = malloc(length + 3);
    char *text;
    size_t result;
    assert(allocation);
    memset(allocation, 0xa5, length + 3);
    text = (char *)allocation + 1;
    memcpy(text, input, length);
    result = open ? text_unwrap_tag(text, length, open, close)
                  : text_strip_info(text, length);
    assert(result == expected_length);
    assert(memcmp(text, expected, result) == 0);
    assert(text[result] == '\0');
    assert(allocation[0] == 0xa5 && allocation[length + 2] == 0xa5);
    free(allocation);
}

static void fixtures(void)
{
#define UNWRAP(in, out) check(in, sizeof(in) - 1, out, sizeof(out) - 1, "<ITEM>", "</ITEM>")
#define INFO(in, out) check(in, sizeof(in) - 1, out, sizeof(out) - 1, NULL, NULL)
    UNWRAP("", "");
    UNWRAP("plain text", "plain text");
    UNWRAP("a<ITEM>b</ITEM>c<ITEM>d</ITEM>e", "abcde");
    UNWRAP("<ITEM></ITEM><ITEM>x</ITEM>", "x");
    UNWRAP("앞<ITEM>아이템</ITEM>뒤", "앞아이템뒤");
    UNWRAP("a\0<ITEM>b\0c</ITEM>d", "a\0b\0cd");
    UNWRAP("<item>x</item>", "<item>x</item>");
    UNWRAP("a<ITEM>incomplete", "a<ITEM>incomplete");
    UNWRAP("a</ITEM>b<ITEM>c</ITEM>d", "a</ITEM>bcd");
    UNWRAP("<ITEM>a</ITEM>b<ITEM>c", "ab<ITEM>c");
    UNWRAP("<ITEM>a<ITEM>b</ITEM>c</ITEM>", "a<ITEM>bc</ITEM>");
    INFO("", "");
    INFO("a<INFO>hidden</INFO>b<INFO>more</INFO>c", "abc");
    INFO("<INFO></INFO>", "");
    INFO("앞<INFO>숨김</INFO>뒤", "앞뒤");
    INFO("a\0<INFO>hidden\0data</INFO>b", "a\0b");
    INFO("before<INFO>incomplete", "before<INFO>incomplete");
    INFO("</INFO>x<INFO>hidden</INFO>y", "</INFO>xy");
    INFO("<INFO>a</INFO>b<INFO>c", "b<INFO>c");
    INFO("<INFO>a<INFO>b</INFO>c</INFO>", "c</INFO>");
    check("*a**b*", 6, "ab", 2, "*", "*");
    check("<ITEM>x</ITEM>", 14, "<ITEM>x</ITEM>", 14, "", "</ITEM>");
    check("<ITEM>x</ITEM>", 14, "<ITEM>x</ITEM>", 14, "<ITEM>", "");
#undef INFO
#undef UNWRAP
}

static void truncated_buffers(void)
{
    static const char input[] = "a<INFO>hidden</INFO>b<INFO>unfinished";
    size_t length;
#ifdef _WIN32
    SYSTEM_INFO info;
    unsigned char *pages;
    DWORD old_protection;
    GetSystemInfo(&info);
    pages = VirtualAlloc(NULL, info.dwPageSize * 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    assert(pages);
    assert(VirtualProtect(pages + info.dwPageSize, info.dwPageSize, PAGE_NOACCESS, &old_protection));
#endif
    for (length = 0; length < sizeof(input); ++length) {
        char local[sizeof(input) + 1];
        char *text = local;
        size_t result;
#ifdef _WIN32
        text = (char *)pages + info.dwPageSize - length - 1;
#endif
        memcpy(text, input, length);
        ((unsigned char *)text)[length] = 0xa5;
        result = text_strip_info(text, length);
        assert(result <= length && text[result] == '\0');
        memcpy(text, input, length);
        ((unsigned char *)text)[length] = 0xa5;
        result = text_unwrap_tag(text, length, "<INFO>", "</INFO>");
        assert(result <= length && text[result] == '\0');
    }
#ifdef _WIN32
    assert(VirtualFree(pages, 0, MEM_RELEASE));
#endif
}

static unsigned random_state = 0x7192a353;
static unsigned next_random(void)
{
    random_state = random_state * 1664525u + 1013904223u;
    return random_state;
}

static void generated_cases(int trace)
{
    static const char *const tags[] = { "ITEM", "NAVI", "QUEST", "TIPBOX", "URL", "INFO" };
    unsigned sample;
    for (sample = 0; sample < 5000; ++sample) {
        char input[4096], expected[4096], open[16], close[16];
        size_t size = 0, expected_size = 0, result, segment;
        unsigned tag = next_random() % 6;
        unsigned segments = next_random() % 32;
        size_t open_size = (size_t)sprintf(open, "<%s>", tags[tag]);
        size_t close_size = (size_t)sprintf(close, "</%s>", tags[tag]);
        for (segment = 0; segment < segments; ++segment) {
            unsigned wrapped = (next_random() >> 16) & 1;
            unsigned count = next_random() % 30, byte;
            if (wrapped) {
                memcpy(input + size, open, open_size);
                size += open_size;
            }
            for (byte = 0; byte < count; ++byte) {
                char value = (char)(next_random() >> 24);
                if (value == '<') value = 'x';
                input[size++] = value;
                if (!wrapped || tag != 5) expected[expected_size++] = value;
            }
            if (wrapped) {
                memcpy(input + size, close, close_size);
                size += close_size;
            }
        }
        input[size] = '\0';
        result = tag == 5 ? text_strip_info(input, size)
                          : text_unwrap_tag(input, size, open, close);
        assert(result == expected_size && memcmp(input, expected, result) == 0);
        assert(input[result] == '\0');
        if (trace) {
            size_t byte;
            printf("%u %lu ", sample, (unsigned long)result);
            for (byte = 0; byte < result; ++byte) printf("%02x", (unsigned char)input[byte]);
            putchar('\n');
        }
    }
}

int main(int argc, char **argv)
{
    int trace = argc == 2 && strcmp(argv[1], "--trace") == 0;
    if (!trace) {
        fixtures();
        truncated_buffers();
    }
    generated_cases(trace);
    if (!trace) puts("Text markup: in-place C buffers, Korean and counted bytes, malformed tags, protected-page truncation and 5000 generated cases passed.");
    return 0;
}
