#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "ui/draw.h"
#include "iteminfo/item_api.h"

unsigned char g_useCharsetTag;
static unsigned item_names, item_destroys;
static int item_id;
const char *language_char_next(const char *text) { return CharNextExA(949, text, 0); }
struct ItemInfo *item_info_construct(struct ItemInfo *item)
{ memset(item, 0, sizeof *item); return item; }
void item_info_set_item_id(struct ItemInfo *item, int id) { assert(item); item_id = id; }
const char *item_info_get_name(const struct ItemInfo *item)
{ assert(item && item_id == 5); ++item_names; return "아이템"; }
void string_free(struct String *text) { assert(!text->size); ++item_destroys; }
static void check(const struct TextArray *lines, size_t index, const char *text)
{ assert(index < lines->size && !strcmp(text_array_at(lines, index).data, text)); }
static void roundtrip(const char *text, int width, int rich)
{
    struct TextArray lines = {0};
    char joined[1024] = "";
    size_t i;
    if (rich) text_wrap_bytes(text, &lines, width, 0);
    else text_split_lines(text, &lines, width);
    for (i = 0; i < lines.size; ++i) {
        const char *row = text_array_at(&lines, i).data;
        strcat_s(joined, sizeof joined, row);
        while (*row) {
            if (IsDBCSLeadByteEx(949, (BYTE)*row)) assert(row[1]);
            row = CharNextExA(949, row, 0);
        }
    }
    assert(!strcmp(joined, text));
    text_array_clear(&lines);
}
int main(void)
{
    struct TextArray lines = {0};
    SYSTEM_INFO info;
    char *guard, *end;
    unsigned i, j, seed = 1234567;
    static const char *tokens[] = {"가", "나다", "[ab]", "[", "]", " ", "a", "^123456"};
    text_split_lines(NULL, &lines, 3); text_wrap_bytes("", &lines, 3, 1); assert(!lines.size);
    text_split_lines("ab cd ef", &lines, 4);
    assert(lines.size == 3); check(&lines, 0, "ab "); check(&lines, 1, "cd "); check(&lines, 2, "ef");
    text_array_clear(&lines);
    text_split_lines("ab\ncd", &lines, 9);
    assert(lines.size == 2); check(&lines, 0, "ab"); check(&lines, 1, "\ncd"); text_array_clear(&lines);
    text_wrap_bytes("ab\ncd", &lines, 9, 0);
    assert(lines.size == 2); check(&lines, 0, "ab\n"); check(&lines, 1, "cd"); text_array_clear(&lines);
    text_wrap_bytes("a\r\nb", &lines, 9, 1);
    assert(lines.size == 2); check(&lines, 0, "a"); check(&lines, 1, "b"); text_array_clear(&lines);
    text_wrap_bytes("x^nItemID^5z", &lines, 99, 0);
    assert(lines.size == 3); check(&lines, 0, "x"); check(&lines, 1, "아이템"); check(&lines, 2, "z");
    assert(item_names == 1 && item_destroys == 2); text_array_clear(&lines);
    g_useCharsetTag = 1;
    assert(text_array_append(&lines, text_view_cstr("existing")));
    assert(text_array_append(&lines, text_view_cstr("existing two")));
    text_split_lines("|c0abcdef", &lines, 4);
    assert(lines.size == 5); check(&lines, 0, "existing"); check(&lines, 1, "existing two");
    check(&lines, 2, "|c0a"); check(&lines, 3, "|c0bcde"); check(&lines, 4, "|c0f");
    text_array_clear(&lines);
    assert(text_array_append(&lines, text_view_cstr("|c0abcdef")));
    text_split_lines(text_array_at(&lines, 0).data, &lines, 4);
    assert(lines.size == 4); check(&lines, 0, "|c0abcdef"); check(&lines, 3, "|c0f");
    text_array_clear(&lines); g_useCharsetTag = 0;
    text_wrap_bytes("한국bca[abcd]", &lines, 13, 0);
    assert(lines.size == 2); check(&lines, 0, "한국bc"); check(&lines, 1, "a[abcd]");
    text_array_clear(&lines);
    text_wrap_bytes("한국^nItemID^5[가]^nItemID^5", &lines, 5, 0);
    assert(lines.size == 4); check(&lines, 0, "한국"); check(&lines, 1, "아이템");
    check(&lines, 2, "[가]"); check(&lines, 3, "아이템"); text_array_clear(&lines);
    text_wrap_bytes("bcabc한국[abcd]", &lines, 12, 0);
    assert(lines.size == 2); check(&lines, 0, "bcabc한"); check(&lines, 1, "국[abcd]");
    text_array_clear(&lines);
    text_wrap_bytes("bb[bc", &lines, 2, 0);
    assert(lines.size == 2); check(&lines, 0, "bb["); check(&lines, 1, "bc");
    text_array_clear(&lines);
    text_wrap_bytes("[ab]][]de", &lines, 3, 0);
    assert(lines.size == 3); check(&lines, 0, "[ab]]"); check(&lines, 1, "[]d"); check(&lines, 2, "e");
    text_array_clear(&lines);
    roundtrip("a[b]", 4, 1); roundtrip("가[ab]", 5, 1); roundtrip("ab[broken", 4, 1);
    for (i = 0; i < 1000; ++i) {
        char text[256] = "";
        int width;
        for (j = 0; j < 1 + i % 10; ++j) {
            seed = seed * 1664525u + 1013904223u;
            strcat_s(text, sizeof text, tokens[seed % 8]);
        }
        seed = seed * 1664525u + 1013904223u;
        width = i % 25 == 0 ? INT_MIN : (int)(seed % 16) - 1;
        roundtrip(text, width, 0); roundtrip(text, width, 1);
    }
    GetSystemInfo(&info);
    guard = VirtualAlloc(NULL, info.dwPageSize * 2, MEM_RESERVE, PAGE_NOACCESS); assert(guard);
    assert(VirtualAlloc(guard, info.dwPageSize, MEM_COMMIT, PAGE_READWRITE));
    end = guard + info.dwPageSize;
    memcpy(end - 8, "가[ab]\n", 8);
    roundtrip(end - 8, 4, 1);
    memcpy(end - 2, "[", 2); roundtrip(end - 2, INT_MAX, 1);
    assert(VirtualFree(guard, 0, MEM_RELEASE));
    puts("Byte lines: owned append, charset carry, item names, newline behavior, Korean boundaries, malformed groups and 2000 generated roundtrips passed.");
    return 0;
}
