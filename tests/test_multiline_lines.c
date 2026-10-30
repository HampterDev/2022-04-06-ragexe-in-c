#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui/controls/edit.h"

const char *language_char_next(const char *text) { return CharNextExA(949, text, 0); }
int ui_window_text_width(struct UIWindow *self, const char *text, int len, int face, int height, int a, int b)
{
    int width = 0, i;
    assert(self && len >= 0 && !face && height == 12 && !a && !b);
    if (!len) len = (int)strlen(text);
    for (i = 0; i < len; ++i) if (text[i] != '\r' && text[i] != '\n') ++width;
    return width;
}
void ui_window_move_base(struct UIWindow *self, int x, int y) { (void)self; (void)x; (void)y; abort(); }
static void check(struct TextArray *lines, size_t index, const char *expected)
{
    assert(index < lines->size && !strcmp(text_array_at(lines, index).data, expected));
}
int main(void)
{
    struct UIEditCtrl3 edit = {0};
    struct TextArray lines = {0};
    struct ByteArray flags = {0};
    SYSTEM_INFO info;
    char *guard, *end;
    size_t i;
    static const char text[] = "ab\r\ncde";

    ui_window_break_coloured_lines(&edit.edit.base, NULL, &lines, &flags, 2);
    ui_window_break_coloured_lines(&edit.edit.base, "", &lines, &flags, 2);
    assert(!lines.size && !flags.size);
    ui_window_break_coloured_lines(&edit.edit.base, text, &lines, &flags, 2);
    assert(lines.size == 3 && flags.size == 3);
    check(&lines, 0, "ab\r\n"); check(&lines, 1, "cd"); check(&lines, 2, "e");
    assert(flags.items[0] == 1 && !flags.items[1] && !flags.items[2]);
    text_array_clear(&lines); byte_array_clear(&flags);
    ui_window_break_coloured_lines(&edit.edit.base, "가나다x", &lines, &flags, 1);
    assert(lines.size == 4 && flags.size == 4);
    check(&lines, 0, "가"); check(&lines, 1, "나"); check(&lines, 2, "다"); check(&lines, 3, "x");
    text_array_clear(&lines); byte_array_clear(&flags);
    for (i = 0; i < 100; ++i) {
        assert(text_array_resize(&lines, 0) && byte_array_resize(&flags, 0));
        ui_window_break_coloured_lines(&edit.edit.base, "^123456a", &lines, &flags, -1);
        assert(lines.size == 8 && flags.size == 8);
        check(&lines, 0, "^"); check(&lines, 7, "a");
    }
    text_array_clear(&lines); byte_array_clear(&flags);
    assert(text_array_append(&lines, text_view_cstr("prefix")));
    ui_window_break_coloured_lines(&edit.edit.base, "ab\rcd\ne", &lines, NULL, 99);
    assert(lines.size == 3); check(&lines, 0, "prefix"); check(&lines, 1, "ab\rc"); check(&lines, 2, "d\ne");
    text_array_clear(&lines);

    GetSystemInfo(&info);
    guard = VirtualAlloc(NULL, info.dwPageSize * 2, MEM_RESERVE, PAGE_NOACCESS); assert(guard);
    assert(VirtualAlloc(guard, info.dwPageSize, MEM_COMMIT, PAGE_READWRITE));
    end = guard + info.dwPageSize;
    memcpy(end - 3, "a\r", 3);
    ui_window_break_coloured_lines(&edit.edit.base, end - 3, &lines, &flags, 1);
    check(&lines, 0, "a\r"); assert(lines.size == 1 && flags.items[0]);
    text_array_clear(&lines); byte_array_clear(&flags);
    memcpy(end - 3, "가", 3);
    ui_window_break_coloured_lines(&edit.edit.base, end - 3, &lines, &flags, 0);
    check(&lines, 0, "가"); assert(lines.size == 1);
    assert(VirtualFree(guard, 0, MEM_RELEASE));
    text_array_clear(&lines); byte_array_clear(&flags);

    edit.edit.base.width = 12;
    assert(text_array_append(&lines, text_view_cstr("abc<de")));
    assert(text_array_append(&lines, text_view_cstr("f>ghi")));
    ui_edit_ctrl3_rewrap(&edit, &lines);
    assert(lines.size == 3); check(&lines, 0, "abc"); check(&lines, 1, "<def>g"); check(&lines, 2, "hi");
    text_array_clear(&lines);
    puts("Multiline lines: preserved breaks, counted Korean characters, narrow/negative widths, append, guarded endings and link rewrap passed.");
    return 0;
}
