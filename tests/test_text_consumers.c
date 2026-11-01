#define ENCODING_FIXTURE_IMPLEMENTATION
#include "encoding_fixture.h"
#include "ui/controls/static.h"
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
    int count = MultiByteToWideChar(CP_THREAD_ACP, 0, text, -1, NULL, 0);
    (void)self; (void)unused; (void)face; (void)height; (void)arg; (void)arg2;
    assert(count);
    return (count - 1) * 7;
}
static void invalidate(struct UIWindow *self)
{
    (void)self;
    ++invalidations;
}

static unsigned ellipsis(const char *text, int width, const char *expected, unsigned fail)
{
    struct UIWindow wnd = {0};
    struct String result;
    unsigned allocations;
    encoding_test_reset();
    encoding_fail_allocation = fail;
    ui_window_ellipsize_text(&wnd, &result, width, text, 3, 12, 0);
    if (expected)
        assert(!strcmp(string_cstr(&result), expected));
    assert(!encoding_live);
    allocations = encoding_allocations;
    string_free(&result);
    assert(!string_blocks);
    return allocations;
}

int main(void)
{
    LCID previous = GetThreadLocale();
    unsigned count, fail;
    struct UIWindowOps methods = {0};
    struct UIStaticText label = {0};
    assert(SetThreadLocale(MAKELCID(MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT), SORT_DEFAULT)));
    ellipsis("", 0, "", 0);
    ellipsis("abcdef", 42, "abcdef", 0);
    ellipsis("abcdef", 35, "ab...", 0);
    ellipsis("abcdef", 0, "a...", 0);
    ellipsis("a", -1, "a...", 0);
    ellipsis("\xb0\xa1\xb3\xaa\xb4\xd9", 0, "\xb0\xa1...", 0);
    count = ellipsis("abcdefghijklmnopqrstuv", 42, "abc...", 0);
    for (fail = 1; fail <= count; ++fail)
        ellipsis("abcdefghijklmnopqrstuv", 42, fail == 1 || fail == 2 ? "abcdefghijklmnopqrstuv" : NULL, fail);
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
    encoding_test_reset(); encoding_fail_allocation = 1;
    ui_static_text_set_text_ellipsis_wide(&label.base, "abcdef");
    assert(!strcmp(string_cstr(&label.text), "abcdef") && label.has_tooltip);
    assert(invalidations == 3 && !encoding_live);
    string_free(&label.text);
    string_free(&label.tooltip);
    assert(!string_blocks && !encoding_live);
    assert(SetThreadLocale(previous));
    puts("PASS: ASCII/Korean ellipsis, static labels, conversion-failure fallback and allocation cleanup");
    return 0;
}
