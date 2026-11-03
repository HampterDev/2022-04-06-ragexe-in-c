#include <assert.h>
#include "utf8_text.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui/draw.h"
#include "render/textdc.h"

static unsigned allocations, contexts, measurements;
static int expected_face, expected_height, expected_arg;
static int dots_width = 3;

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

int ui_window_text_width(struct UIWindow *self, const char *text, int length,
                        int face, int height, int arg, int extra)
{
    assert(self && self->canvas && text && length >= 0 && !extra);
    assert(face == expected_face && height == expected_height && arg == expected_arg);
    ++measurements;
    return length ? length : (int)strlen(text);
}
struct TextDC *text_dc_construct(struct TextDC *self, struct DcBitmap *canvas)
{
    assert(canvas); memset(self, 0, sizeof *self); ++contexts; return self;
}
void text_dc_select_font(struct TextDC *self, int face, int height, int bold, int italic)
{
    assert(self && contexts == 1 && !italic);
    assert(face == expected_face && height == expected_height && bold == expected_arg);
}
void text_dc_text_extent(struct TextDC *self, const char *text, int length, SIZE *out)
{
    assert(self && contexts == 1 && length == 3 && !memcmp(text, "...", 3));
    out->cx = dots_width; out->cy = expected_height;
}
void text_dc_destruct(struct TextDC *self) { assert(self && contexts == 1); --contexts; }

static const char *shorten(struct UIWindow *window, struct String *out, const char *text, int width)
{
    const char *result = ui_window_shorten_text(window, out, text, width,
                                             expected_face, expected_height, expected_arg);
    assert(!contexts);
    if (text) assert(result == string_cstr(out));
    else assert(!result && !out->size);
    return result;
}
static void check(struct UIWindow *window, struct String *out, const char *text, int width, const char *expected)
{
    assert(!strcmp(shorten(window, out, text, width), expected));
}
int main(void)
{
    struct UIWindow window = {0};
    struct String first = {0}, second = {0};
    const char *saved;
    char input[] = "unchanged";
    SYSTEM_INFO info;
    char *guard, *edge;
    unsigned i, before;
    window.canvas = (struct DcBitmap *)&window;
    expected_height = 12;
    check(&window, &first, "", 0, "");
    check(&window, &first, "abcdef", 6, "abcdef");
    check(&window, &first, "abcdef", 5, "ab...");
    check(&window, &first, "abcdef", 4, "a...");
    check(&window, &first, "abcdef", 3, "abcdef");
    check(&window, &first, "abcdef", 0, "abcdef");
    check(&window, &first, U8_HAN U8_GUK U8_EO, 6, U8_HAN "...");
    check(&window, &first, U8_HAN U8_GUK U8_EO, 5, U8_HAN U8_GUK U8_EO);
    check(&window, &first, U8_HAN U8_GUK U8_EO, INT_MIN, U8_HAN U8_GUK U8_EO);
    check(&window, &first, U8_HAN U8_GUK U8_EO, INT_MAX, U8_HAN U8_GUK U8_EO);
    check(&window, &first, U8_ZHE U8_GRIN "ab", 6, U8_ZHE "...");
    check(&window, &first, "a\xe2\x82" U8_GRIN, 6, "a\xe2\x82...");
    expected_face = 3; expected_height = 20; expected_arg = 1;
    check(&window, &first, "abc" U8_HAN U8_GUK, 6, "abc...");
    saved = string_cstr(&first);
    check(&window, &second, "different text", 7, "diff...");
    assert(!strcmp(saved, "abc..."));
    check(&window, &first, input, 100, "unchanged");
    input[0] = 'X'; assert(!strcmp(string_cstr(&first), "unchanged"));
    string_assign_cstr(&first, "prefixabcdef");
    check(&window, &first, first.data + 6, 5, "ab...");
    check(&window, &first, first.data, 4, "a...");
    before = measurements; shorten(&window, &first, NULL, 4); assert(measurements == before);
    dots_width = INT_MAX;
    check(&window, &first, "abcdef", INT_MIN, "abcdef");
    dots_width = 3;
    for (i = 0; i < 1000; ++i) {
        check(&window, &first, U8_GIN " " U8_I U8_REUM " " U8_HAN U8_GUK U8_EO, 7, U8_GIN " ...");
        check(&window, &second, "abcdefghijk", 8, "abcde...");
    }
    GetSystemInfo(&info);
    guard = VirtualAlloc(NULL, info.dwPageSize * 2, MEM_RESERVE, PAGE_NOACCESS); assert(guard);
    assert(VirtualAlloc(guard, info.dwPageSize, MEM_COMMIT, PAGE_READWRITE));
    edge = guard + info.dwPageSize;
    memcpy(edge - 10, U8_HAN U8_GUK U8_EO, 10);
    check(&window, &first, edge - 10, 6, U8_HAN "...");
    assert(VirtualFree(guard, 0, MEM_RELEASE));
    string_free(&first); string_free(&second);
    assert(!allocations && !contexts);
    puts("Shortened text: independent owners, copied input, aliasing, UTF-8 boundaries, narrow/extreme widths, font arguments, guarded endings and 1000 reuse cycles passed.");
    return 0;
}
