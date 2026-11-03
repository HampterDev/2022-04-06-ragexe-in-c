#define ENCODING_FIXTURE_IMPLEMENTATION
#include "encoding_fixture.h"
#include "ui/controls/static.h"
#include "utf8_text.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned string_blocks, invalidations;

void *mem_alloc(size_t bytes)
{
    void *p = malloc(bytes);

    assert(p);
    ++string_blocks;
    return p;
}
void mem_free(void *p)
{
    assert(string_blocks);
    --string_blocks;
    free(p);
}
void fatal_error(const char *message) { (void)message; abort(); }

int ui_window_text_width(struct UIWindow *self, const char *text, int unused, int face,
                       int height, int arg, int arg2)
{
    int count = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    (void)self; (void)unused; (void)face; (void)height; (void)arg; (void)arg2;
    assert(count);
    return (count - 1) * 7;
}
static void invalidate(struct UIWindow *self)
{
    (void)self;
    ++invalidations;
}

static void ellipsis(const char *text, int width, const char *expected)
{
    struct UIWindow wnd = {0};
    struct String result;
    encoding_test_reset();
    ui_window_ellipsize_text(&wnd, &result, width, text, 3, 12, 0);
    assert(!strcmp(string_cstr(&result), expected));
    assert(!encoding_live && !encoding_calls);
    string_free(&result);
    assert(!string_blocks);
}

int main(void)
{
    struct UIWindowOps methods = {0};
    struct UIStaticText label = {0};
    ellipsis("", 0, "");
    ellipsis("abcdef", 42, "abcdef");
    ellipsis("abcdef", 35, "ab...");
    ellipsis("abcdef", 0, "a...");
    ellipsis("a", -1, "a...");
    ellipsis(U8_GA U8_NA U8_DA, 0, U8_GA "...");
    ellipsis(U8_GA U8_NA U8_DA, 21, U8_GA U8_NA U8_DA);
    ellipsis(U8_GA U8_NA U8_DA U8_HAN U8_GUK U8_EO, 35, U8_GA U8_NA "...");
    ellipsis(U8_ZHE U8_GRIN "abcd", 42, U8_ZHE U8_GRIN "...");
    ellipsis(U8_ZHE U8_GRIN "abcd", 41, U8_ZHE "...");
    ellipsis("ab\xe2\x82" "cdef", 35, "ab...");
    ellipsis("abcdefghijklmnopqrstuv", 42, "abc...");
    methods.invalidate = invalidate;
    label.base.ops = &methods;
    label.base.width = 35;
    label.font_height = 12;
    string_init(&label.text);
    string_init(&label.tooltip);
    encoding_test_reset();
    ui_static_text_set_text_ellipsis_wide(&label.base, "abcdef");
    assert(!strcmp(string_cstr(&label.text), "abcdef") && !label.has_tooltip);
    label.base.canvas = (void *)1;
    ui_static_text_set_text_ellipsis_wide(&label.base, "abcdef");
    assert(!strcmp(string_cstr(&label.text), "ab...") && label.has_tooltip);
    assert(!strcmp(string_cstr(&label.tooltip), "abcdef"));
    ui_static_text_set_text_ellipsis_wide(&label.base, U8_HAN U8_GUK U8_EO U8_GA U8_NA U8_DA);
    assert(!strcmp(string_cstr(&label.text), U8_HAN U8_GUK "...") && label.has_tooltip);
    assert(invalidations == 3 && !encoding_live);
    string_free(&label.text);
    string_free(&label.tooltip);
    assert(!string_blocks && !encoding_live);
    puts("PASS: ASCII and UTF-8 ellipsis by whole characters, static labels and allocation cleanup");
    return 0;
}
