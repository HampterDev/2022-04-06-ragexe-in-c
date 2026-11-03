#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "ui/draw.h"
#include "utf8_text.h"

int ui_window_text_width(struct UIWindow *self, const char *text, int len,
                        int face, int height, int a, int b)
{
    int width = 0, i;
    assert(self && len >= 0 && !face && height == 12 && !a && !b);
    if (!len) len = (int)strlen(text);
    for (i = 0; i < len; ++i) if (text[i] != '\r' && text[i] != '\n') ++width;
    return width;
}
static void check(const struct TextArray *lines, size_t i, const char *expected)
{
    assert(i < lines->size && !strcmp(text_array_at(lines, i).data, expected));
}
int main(void)
{
    struct UIWindow window = {0};
    struct TextArray lines = {0};
    SYSTEM_INFO info;
    char *guard, *end;
    char long_text[1004];
    unsigned i;

    ui_window_break_lines(&window, NULL, &lines, 3);
    ui_window_break_lines(&window, "", &lines, 3);
    assert(!lines.size);
    ui_window_break_lines(&window, "abcdef", &lines, 2);
    assert(lines.size == 3); check(&lines, 0, "ab"); check(&lines, 1, "cd"); check(&lines, 2, "ef");
    text_array_clear(&lines);
    ui_window_break_lines(&window, U8_HAN U8_GUK U8_EO "a", &lines, 4);
    assert(lines.size == 3); check(&lines, 0, U8_HAN); check(&lines, 1, U8_GUK); check(&lines, 2, U8_EO "a");
    text_array_clear(&lines);
    ui_window_break_lines(&window, U8_ZHE U8_HAN U8_GRIN "a", &lines, 3);
    assert(lines.size == 4); check(&lines, 0, U8_ZHE); check(&lines, 1, U8_HAN);
    check(&lines, 2, U8_GRIN); check(&lines, 3, "a");
    text_array_clear(&lines);
    ui_window_break_lines(&window, "ab [cd]e", &lines, 5);
    assert(lines.size == 2); check(&lines, 0, "ab "); check(&lines, 1, "[cd]e");
    text_array_clear(&lines);
    ui_window_break_lines(&window, "[abcdef]", &lines, 3);
    assert(lines.size == 3); check(&lines, 0, "[ab"); check(&lines, 1, "cde"); check(&lines, 2, "f]");
    text_array_clear(&lines);
    ui_window_break_lines(&window, "^123456abcd", &lines, 2);
    assert(lines.size == 2); check(&lines, 0, "^123456ab"); check(&lines, 1, "cd");
    text_array_clear(&lines);
    ui_window_break_lines(&window, "ab[cd", &lines, 4);
    assert(lines.size == 2); check(&lines, 0, "ab[c"); check(&lines, 1, "d");
    text_array_clear(&lines);

    for (i = 0; i < 100; ++i) {
        ui_window_break_lines(&window, U8_GA U8_NA U8_DA "x", &lines, i % 3 == 0 ? INT_MIN : (int)(i % 3) - 1);
        assert(lines.size == 4);
        check(&lines, 0, U8_GA); check(&lines, 1, U8_NA); check(&lines, 2, U8_DA); check(&lines, 3, "x");
        assert(text_array_resize(&lines, 0));
    }
    ui_window_break_lines(&window, "\r\n\rX\nY\rZ\n", &lines, 40);
    assert(lines.size == 5);
    check(&lines, 0, " "); check(&lines, 1, " "); check(&lines, 2, "X");
    check(&lines, 3, "Y"); check(&lines, 4, "Z");
    text_array_clear(&lines);
    ui_window_break_lines(&window, "\n", &lines, 0);
    assert(lines.size == 1); check(&lines, 0, " ");
    text_array_clear(&lines);
    ui_window_break_lines(&window, U8_GA "\n" U8_NA "\r" U8_DA "\r\nx", &lines, 3);
    assert(lines.size == 4);
    check(&lines, 0, U8_GA); check(&lines, 1, U8_NA); check(&lines, 2, U8_DA); check(&lines, 3, "x");
    text_array_clear(&lines);

    assert(text_array_append(&lines, text_view_cstr("abcdef")));
    ui_window_break_lines(&window, text_array_at(&lines, 0).data, &lines, 1);
    assert(lines.size == 7); check(&lines, 0, "abcdef"); check(&lines, 1, "a"); check(&lines, 6, "f");
    text_array_clear(&lines);
    memset(long_text, 'x', sizeof long_text - 1); long_text[sizeof long_text - 1] = 0;
    ui_window_break_lines(&window, long_text, &lines, INT_MAX);
    assert(lines.size == 2 && lines.items[0].size == 1001 && lines.items[1].size == 2);
    text_array_clear(&lines);
    ui_window_break_lines(&window, long_text, &lines, 1);
    assert(lines.size == 1001); check(&lines, 1000, "x");
    text_array_clear(&lines);

    GetSystemInfo(&info);
    guard = VirtualAlloc(NULL, info.dwPageSize * 2, MEM_RESERVE, PAGE_NOACCESS); assert(guard);
    assert(VirtualAlloc(guard, info.dwPageSize, MEM_COMMIT, PAGE_READWRITE));
    end = guard + info.dwPageSize;
    memcpy(end - 3, "a\r", 3);
    ui_window_break_lines(&window, end - 3, &lines, 1);
    assert(lines.size == 1); check(&lines, 0, "a"); text_array_clear(&lines);
    memcpy(end - 3, "a\n", 3);
    ui_window_break_lines(&window, end - 3, &lines, 1);
    assert(lines.size == 1); check(&lines, 0, "a"); text_array_clear(&lines);
    memcpy(end - 4, U8_GA, 4);
    ui_window_break_lines(&window, end - 4, &lines, 0);
    assert(lines.size == 1); check(&lines, 0, U8_GA); text_array_clear(&lines);
    memcpy(end - 2, "\xe2", 2);
    ui_window_break_lines(&window, end - 2, &lines, 0);
    assert(lines.size == 1 && lines.items[0].size == 1);
    text_array_clear(&lines);
    assert(VirtualFree(guard, 0, MEM_RELEASE));
    puts("Plain lines: UTF-8 text, colour/bracket wrapping, narrow widths, CR/LF, guarded endings, owned append and limits passed.");
    return 0;
}
