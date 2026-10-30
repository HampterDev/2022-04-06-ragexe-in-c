#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui/draw.h"
#include "core/text_encoding.h"

static unsigned conversions, live, fail_wide, fail_narrow;
static int resized;
wchar_t *encoding_to_wide(unsigned page, const char *text)
{
    int count;
    wchar_t *wide;
    assert(page == CP_THREAD_ACP);
    if (fail_wide) return NULL;
    count = MultiByteToWideChar(949, 0, text, -1, NULL, 0); assert(count);
    wide = malloc(count * sizeof *wide); assert(wide); ++live;
    assert(MultiByteToWideChar(949, 0, text, -1, wide, count));
    return wide;
}
char *encoding_from_wide(unsigned page, const wchar_t *wide)
{
    int count;
    char *text;
    assert(page == CP_THREAD_ACP);
    if (++conversions == fail_narrow) return NULL;
    count = WideCharToMultiByte(949, 0, wide, -1, NULL, 0, NULL, NULL); assert(count);
    text = malloc(count); assert(text); ++live;
    assert(WideCharToMultiByte(949, 0, wide, -1, text, count, NULL, NULL));
    return text;
}
static void conversion_free(void *p) { assert(p && live); --live; free(p); }
#define free conversion_free
#include "../src/ui/controls/window_fit_plain.c"
#undef free

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
    conversions = 0;
    assert(ui_window_fit_plain_text(window, out, text_view_cstr(text), width) == out);
    assert(!strcmp(string_cstr(out), expected));
    assert(!live);
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
    check(&window, &out, "한국", 3, "한...");
    check(&window, &out, "나다", 3, "...");
    check(&window, &out, "가나다", 4, "가...");
    assert(resized == 1);
    fail_wide = 1; check(&window, &out, "abcdef", 1, "abcdef"); fail_wide = 0;
    fail_narrow = 1; check(&window, &out, "abcdef", 1, "abcdef");
    fail_narrow = 2; check(&window, &out, "abcdef", 1, "abcde..."); fail_narrow = 0;
    string_assign_cstr(&out, "prefix abcdef suffix");
    ui_window_fit_plain_text(&window, &out, text_view(out.data + 7, 6), 5);
    assert(!strcmp(string_cstr(&out), "ab...") && !live);
    ui_window_fit_plain_text(&window, &out, text_view("a\0b", 3), 100);
    assert(out.size == 3 && !memcmp(out.data, "a\0b", 4) && !live);
    GetSystemInfo(&info);
    pages = VirtualAlloc(NULL, info.dwPageSize * 2, MEM_RESERVE, PAGE_NOACCESS); assert(pages);
    assert(VirtualAlloc(pages, info.dwPageSize, MEM_COMMIT, PAGE_READWRITE));
    edge = pages + info.dwPageSize;
    memcpy(edge - 4, "한국", 4);
    ui_window_fit_plain_text(&window, &out, text_view(edge - 4, 4), 20);
    assert(!strcmp(string_cstr(&out), "한국") && !live);
    assert(VirtualFree(pages, 0, MEM_RELEASE));
    for (i = 0; i < 200; ++i) check(&window, &out, "abcdef", 5, "ab...");
    string_free(&out);
    puts("Plain text fitting: counted Korean input, ellipsis cuts, aliasing, guarded bounds, conversion failures and repeated reuse passed.");
    return 0;
}
