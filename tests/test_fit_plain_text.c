#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui/draw.h"
#include "utf8_text.h"

static int resized;
#include "../src/ui/controls/window_fit_plain.c"

int ui_window_text_width(struct UIWindow *window, const char *text, int length,
                        int face, int height, int a, int b)
{
    assert(window && window->canvas && length >= 0 && !face && height == 12 && !a && !b);
    return length ? length : (int)strlen(text);
}
static void resize(struct UIWindow *window, int width, int height)
{
    assert(height == 15);
    window->width = width; window->height = height;
    window->canvas = (struct DcBitmap *)window;
    ++resized;
}
static struct UIWindowOps ops = { .resize = resize };
static void check(struct UIWindow *window, struct String *out, const char *text,
                  int width, const char *expected)
{
    assert(ui_window_fit_plain_text(window, out, text_view_cstr(text), width) == out);
    assert(!strcmp(string_cstr(out), expected));
}
int main(void)
{
    struct UIWindow window = {0};
    struct String out = {0};
    SYSTEM_INFO info;
    char *pages, *edge;
    unsigned i;
    window.ops = &ops;
    check(&window, &out, "", 10, ""); assert(resized == 1);
    check(&window, &out, "abcdef", 6, "abcdef");
    check(&window, &out, "abcdef", 5, "ab...");
    check(&window, &out, "abcdef", 0, "a...");
    check(&window, &out, "a", -1, "a...");
    check(&window, &out, U8_HAN U8_GUK, 3, U8_HAN "...");
    check(&window, &out, U8_NA U8_DA, 3, U8_NA "...");
    check(&window, &out, U8_GA U8_NA U8_DA, 4, U8_GA "...");
    check(&window, &out, U8_GA U8_NA U8_DA, 9, U8_GA U8_NA U8_DA);
    check(&window, &out, U8_GA U8_NA U8_DA, 8, U8_GA "...");
    check(&window, &out, U8_ZHE U8_GRIN "a", 5, U8_ZHE "...");
    check(&window, &out, U8_ZHE U8_GRIN "a", 9, U8_ZHE U8_GRIN "a");
    check(&window, &out, U8_GRIN U8_GRIN, 7, U8_GRIN "...");
    check(&window, &out, "a\xe2\x82", 2, "a...");
    check(&window, &out, "ab\xe2\x82", 4, "ab\xe2\x82");
    check(&window, &out, "ab\xe2\x82" "c", 4, "a...");
    assert(resized == 1);
    string_assign_cstr(&out, "prefix abcdef suffix");
    ui_window_fit_plain_text(&window, &out, text_view(out.data + 7, 6), 5);
    assert(!strcmp(string_cstr(&out), "ab..."));
    ui_window_fit_plain_text(&window, &out, text_view("a\0b", 3), 100);
    assert(out.size == 3 && !memcmp(out.data, "a\0b", 4));
    GetSystemInfo(&info);
    pages = VirtualAlloc(NULL, info.dwPageSize * 2, MEM_RESERVE, PAGE_NOACCESS); assert(pages);
    assert(VirtualAlloc(pages, info.dwPageSize, MEM_COMMIT, PAGE_READWRITE));
    edge = pages + info.dwPageSize;
    memcpy(edge - 6, U8_HAN U8_GUK, 6);
    ui_window_fit_plain_text(&window, &out, text_view(edge - 6, 6), 20);
    assert(!strcmp(string_cstr(&out), U8_HAN U8_GUK));
    ui_window_fit_plain_text(&window, &out, text_view(edge - 6, 6), 4);
    assert(!strcmp(string_cstr(&out), U8_HAN "..."));
    assert(VirtualFree(pages, 0, MEM_RELEASE));
    for (i = 0; i < 200; ++i) check(&window, &out, "abcdef", 5, "ab...");
    string_free(&out);
    puts("Plain text fitting: whole UTF-8 characters (2-4 bytes and invalid bytes), ellipsis cuts, aliasing, guarded bounds and repeated reuse passed.");
    return 0;
}
