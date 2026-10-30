#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "ui/chat/navi_text.h"

static unsigned allocations;
void *mem_alloc(size_t size)
{
    void *p = malloc(size);
    assert(p);
    ++allocations;
    return p;
}
void mem_free(void *p)
{
    assert(p && allocations);
    --allocations;
    free(p);
}
void fatal_error(const char *message)
{
    (void)message;
    abort();
}
#include "../src/std/string.c"

static void check_link(struct String *out, const char *input, int brackets, const char *expected)
{
    assert(navi_links_readable_link(out, text_view_cstr(input), brackets) == out);
    assert(out->size == strlen(expected));
    assert(!strcmp(string_cstr(out), expected));
}
static void check_text(struct String *out, const char *input, const char *expected)
{
    assert(navi_links_readable_links(out, text_view_cstr(input)) == out);
    assert(out->size == strlen(expected));
    assert(!strcmp(string_cstr(out), expected));
}

static void guarded_inputs(void)
{
    SYSTEM_INFO info;
    DWORD old;
    char *memory, *edge;
    struct String out = {0};
    static const char input[] = "prefix<NAVIL>한국0102</NAVIL>tail";
    size_t i, size = sizeof input - 1;

    GetSystemInfo(&info);
    memory = VirtualAlloc(NULL, info.dwPageSize * 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    assert(memory);
    assert(VirtualProtect(memory + info.dwPageSize, info.dwPageSize, PAGE_NOACCESS, &old));
    for (i = 0; i <= size; ++i) {
        edge = memory + info.dwPageSize - i;
        memcpy(edge, input, i);
        navi_links_readable_link(&out, text_view(edge, i), 1);
        navi_links_readable_links(&out, text_view(edge, i));
        navi_links_base62(text_view(edge, i));
    }
    assert(!strcmp(string_cstr(&out), "prefix<한국 1,2>tail"));
    string_free(&out);
    assert(VirtualFree(memory, 0, MEM_RELEASE));
}

int main(void)
{
    struct String first = {0}, second = {0};
    static const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    unsigned i;

    assert(navi_links_base62(text_view("", 0)) == 0);
    assert(navi_links_base62(text_view_cstr("1?")) == 62);
    assert(navi_links_base62(text_view_cstr("?1")) == 1);
    assert(navi_links_base62(text_view_cstr("1?1")) == 3845);
    assert(navi_links_base62(text_view("1\0z", 3)) == 3879);
    assert(navi_links_base62(text_view_cstr("2lkCB1")) == INT_MAX);
    assert(navi_links_base62(text_view_cstr("2lkCB2")) == INT_MIN);
    assert(navi_links_base62(text_view_cstr("4GFfc3")) == -1);
    for (i = 0; i < 62 * 62; ++i) {
        char pair[2] = {digits[i / 62], digits[i % 62]};
        assert(navi_links_base62(text_view(pair, sizeof pair)) == (int)i);
    }
    check_link(&first, "<NAVIL>prontera0102</NAVIL>", 0, "prontera 1,2");
    check_link(&first, "<NAVIL>prontera0102</NAVIL>", 1, "<prontera 1,2>");
    check_link(&first, "<NAVIL>한국0102</NAVIL>", 1, "<한국 1,2>");
    check_link(&first, "<NAVIL>mapg8g8</NAVIL>", 1, "<map>");
    check_link(&first, "<NAVIL>mapg7g8</NAVIL>", 0, "map 999,1000");
    check_link(&first, "<NAVIL>mapZZ00</NAVIL>", 1, "<map 3843,0>");
    check_link(&first, "<NAVIL>0102</NAVIL>", 1, "< 1,2>");
    check_link(&first, "0102</NAVIL>", 0, " 1,2");
    check_link(&first, "", 1, "< 0,0>");
    check_link(&first, "plain", 0, " 0,0");
    check_link(&first, "abc</NAVIL>", 1, "< 0,0>");
    check_link(&first, "</NAVIL>", 0, " 0,0");
    check_text(&first, "", "");
    check_text(&first, "plain 한국", "plain 한국");
    check_text(&first, "prefix<NAVIL>missing end", "");
    check_text(&first, "x<NAVIL>map0102</NAVIL>y<NAVIL>next0304</NAVIL>",
                       "x<map 1,2>y<NAVIL>next0304</NAVIL>");
    check_text(&first, "</NAVIL><NAVIL>map0102</NAVIL>", "</NAVIL><map 1,2>");
    {
        char input[] = "<NAVIL>map0102</NAVIL>";
        check_text(&first, input, "<map 1,2>");
        memset(input, 'x', sizeof input - 1);
        check_text(&second, "<NAVIL>other0304</NAVIL>", "<other 3,4>");
        assert(!strcmp(string_cstr(&first), "<map 1,2>"));
    }
    string_assign_cstr(&first, "xx<NAVIL>map0102</NAVIL>");
    navi_links_readable_link(&first, text_view(first.data + 2, first.size - 2), 1);
    assert(!strcmp(string_cstr(&first), "<map 1,2>"));
    string_assign_cstr(&first, "x<NAVIL>map0102</NAVIL>y");
    navi_links_readable_links(&first, string_view(&first));
    assert(!strcmp(string_cstr(&first), "x<map 1,2>y"));
    navi_links_readable_links(&first, text_view(first.data + 1, first.size - 2));
    assert(!strcmp(string_cstr(&first), "<map 1,2>"));
    string_assign_cstr(&first, "<NAVIL>unfinished");
    navi_links_readable_links(&first, string_view(&first));
    assert(first.size == 0);
    {
        char long_link[5020];
        memcpy(long_link, "<NAVIL>", 7);
        memset(long_link + 7, 'a', 5000);
        memcpy(long_link + 5007, "0102</NAVIL>", 12);
        navi_links_readable_link(&first, text_view(long_link, 5019), 1);
        assert(first.size == 5006 && first.data[0] == '<');
        for (i = 1; i <= 5000; ++i) assert(first.data[i] == 'a');
        assert(!strcmp(first.data + 5001, " 1,2>"));
    }
    {
        static const char embedded[] = "<NAVIL>a\0b0102</NAVIL>";
        static const char expected[] = "<a\0b 1,2>";
        navi_links_readable_links(&first, text_view(embedded, sizeof embedded - 1));
        assert(first.size == sizeof expected - 1);
        assert(!memcmp(first.data, expected, sizeof expected));
    }
    for (i = 0; i < 1000; ++i) {
        check_link(&first, "<NAVIL>한국0102</NAVIL>", 0, "한국 1,2");
        check_text(&second, "<NAVIL>한국0102</NAVIL>", "<한국 1,2>");
        check_text(&first, "<NAVIL>unfinished", "");
        string_free(&second);
    }
    guarded_inputs();
    string_free(&first);
    string_free(&second);
    assert(!allocations);
    puts("Navigation text: coordinates, long and counted labels, Korean bytes, aliases, malformed suffixes and ownership passed.");
    return 0;
}
