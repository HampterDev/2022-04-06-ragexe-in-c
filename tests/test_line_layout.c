#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mbctype.h>
#include "ui/controls/line_info.h"
#include "ui/controls/buttons.h"
#include "ui/window.h"
#include "ui/draw.h"
#include "iteminfo/item_api.h"
#include "core/fatal.h"
#include "core/text_markup.h"
#include "core/owned_text.h"
#include "core/hex.h"
#include "std/std.h"
#include "owned_storage_test_allocator.h"

static unsigned fail_window, window_allocations, measured, moves, created, invalidated;
static struct UIWindow *created_windows[1024];
static uint32_t move_hash;
static void *window_malloc(size_t size)
{
    if (++window_allocations == fail_window) return NULL;
    return checked_malloc(size);
}
static void *moving_realloc(void *old, size_t size)
{
    void *next = checked_malloc(size);
    if (!next) return NULL;
    if (old) {
        size_t before = ((struct Allocation *)old - 1)->size;
        memcpy(next, old, before < size ? before : size);
        checked_free(old);
    }
    return next;
}
#define realloc moving_realloc
#define free checked_free
#define malloc checked_malloc
#include "../src/core/owned_text.c"
#include "../src/core/text_array.c"
#undef malloc
#define malloc window_malloc
#include "../src/ui/controls/line_info.c"
#undef malloc
#undef free
#undef realloc

unsigned g_codePage = 949;
void *mem_alloc(size_t size)
{
    if ((size == sizeof(struct LineCutterText) || size == sizeof(struct UITextButton)) &&
        ++window_allocations == fail_window) return NULL;
    return checked_malloc(size);
}
void mem_free(void *p)
{
    checked_free(p);
}
__declspec(noreturn) void fatal_error(const char *message) { (void)message; abort(); }

void ui_text_button_add_line(struct UITextButton *button, struct TextView value)
{ assert(text_array_append(&button->lines, value)); }
struct TextView ui_text_button_get_line(const struct UITextButton *button, size_t i)
{ return text_array_at(&button->lines, i); }
size_t ui_text_button_line_count(const struct UITextButton *button) { return button->lines.size; }

static struct UIWindowOps button_ops;
struct UIWindow *ui_window_construct(struct UIWindow *window, int composites)
{
    assert(!composites && created < 1024);
    memset(window, 0, sizeof *window);
    window->ops = &button_ops;
    created_windows[created++] = window;
    return window;
}
void ui_window_destruct(struct UIWindow *window) { assert(window); }
void ui_window_create(struct UIWindow *window, int width, int height)
{ window->width = width; window->height = height; }
void ui_window_resize_base(struct UIWindow *window, int width, int height)
{ window->width = width; window->height = height; }
void ui_window_move_base(struct UIWindow *window, int x, int y)
{
    unsigned i;
    for (i = 0; i < created; ++i) if (created_windows[i] == window) break;
    assert(i < created);
    move_hash = ((move_hash * 33 + i) * 33 + (unsigned)x) * 33 + (unsigned)y;
    ++moves;
    window->x = x; window->y = y;
}
void ui_window_invalidate_base(struct UIWindow *window) { assert(window); ++invalidated; }

static int width_of(const char *text, size_t length, int height, int bold, int italic)
{
    int width = 0;
    size_t i = 0;
    while (i < length) {
        int dbcs = IsDBCSLeadByteEx((UINT)g_codePage, (BYTE)text[i]) && i + 1 < length;
        width += (dbcs ? 11 : text[i] == 'W' ? 9 : 5) + height / 6 + !!bold + !!italic;
        i += dbcs ? 2 : 1;
    }
    return width;
}
SIZE *ui_window_text_extent(struct UIWindow *window, SIZE *out, const char *text, int len,
                         int face, int height, int bold, int italic)
{
    assert(window && text && len >= 0 && !face);
    ++measured;
    out->cx = width_of(text, (size_t)len, height, bold, italic); out->cy = height;
    return out;
}
int ui_window_text_width(struct UIWindow *window, const char *text, int len, int face, int height, int bold, int italic)
{
    assert(window && !face);
    ++measured;
    return width_of(text, len ? (size_t)len : strlen(text), height, bold, italic);
}

static struct String drawn;
static int draw_x, draw_y, draw_height, draw_bold, draw_italic, draw_outlined;
static COLORREF draw_colour, draw_outline;
void ui_window_clear_canvas(struct UIWindow *window, DWORD colour) { assert(window && colour == UI_TRANSPARENT); }
void ui_window_text_out(struct UIWindow *window, int x, int y, const char *text, int len,
                     int face, int height, COLORREF colour, int bold, int italic)
{
    assert(window && !face && len >= 0);
    string_assign(&drawn, text, (size_t)len);
    draw_x = x; draw_y = y; draw_height = height; draw_colour = colour;
    draw_bold = bold; draw_italic = italic; draw_outlined = 0; draw_outline = 0;
}
void ui_window_text_out_outlined(struct UIWindow *window, int x, int y, const char *text, int len,
                             COLORREF colour, COLORREF outline, int face, int height, int bold, int italic)
{
    ui_window_text_out(window, x, y, text, len, face, height, colour, bold, italic);
    draw_outlined = 1; draw_outline = outline;
}

struct UITextButton *ui_text_button_construct(struct UITextButton *button)
{
    memset(button, 0, sizeof *button);
    ui_window_construct(&button->button.base, 0);
    string_init(&button->label);
    return button;
}
void ui_text_button_set_text(struct UIWindow *window, const char *text, DWORD colour,
                         int face, int height, BYTE outlined, BYTE underlined)
{
    struct UITextButton *button = (struct UITextButton *)window;
    assert(!face && height == 12 && !outlined && underlined);
    string_assign_cstr(&button->label, text);
    button->normal_colour = colour; button->font_height = height;
    button->underlined = underlined; window->height = 14;
}
void ui_text_button_fit_width(struct UITextButton *button, int width, int height)
{ button->button.base.width = width; button->button.base.height = height; }
const char *item_info_get_name(const struct ItemInfo *item)
{
    int id = atoi(string_cstr(&item->id_text));
    return id == 62 ? "Sword" : id == 63 ? "한글 갑옷" : "Unknown";
}
char *msg_str(int id) { return id == 1 ? "Message %s %d" : "알림"; }
#include "line_layout_unreached.h"

struct WindowRecord {
    int x, y, width, height, kind, font_height, bold, italic, outlined;
    COLORREF colour, hover, outline;
    unsigned args;
    char text[2048], arguments[2048];
};
struct LayoutRecord {
    int height;
    unsigned count, measured, moves, invalidated;
    uint32_t move_hash;
    struct WindowRecord windows[256];
};
static void reset_counters(void)
{
    created = measured = moves = invalidated = window_allocations = 0;
    move_hash = 0;
}
static void capture_window(struct WindowRecord *out, struct UIWindow *window, int plain)
{
    out->x = window->x; out->y = window->y; out->width = window->width; out->height = window->height;
    out->kind = plain ? 0 : window->id;
    if (plain) {
        ui_window_on_draw(UI_WINDOW(window));
        assert(drawn.size < sizeof out->text);
        memcpy(out->text, string_cstr(&drawn), drawn.size);
        out->colour = draw_colour; out->outline = draw_outline;
        out->font_height = draw_height; out->bold = draw_bold;
        out->italic = draw_italic; out->outlined = draw_outlined;
        assert(draw_x == draw_outlined && draw_y == draw_outlined);
        string_free(&drawn);
    } else {
        struct UITextButton *button = (struct UITextButton *)window;
        size_t i;
        size_t at = 0;
        assert(button->label.size < sizeof out->text);
        memcpy(out->text, string_cstr(&button->label), button->label.size);
        out->colour = button->normal_colour; out->hover = button->hover_colour;
        out->font_height = button->font_height;
        for (i = 0; i < button->lines.size; ++i) {
            struct TextView arg = ui_text_button_get_line(button, i);
            assert(at + arg.size + 1 < sizeof out->arguments);
            memcpy(out->arguments + at, arg.data, arg.size);
            at += arg.size; out->arguments[at++] = '|'; ++out->args;
        }
    }
}
static void free_button(struct UIWindow *window)
{
    struct UITextButton *button = (struct UITextButton *)window;
    string_free(&button->label); text_array_clear(&button->lines);
    mem_free(button);
}
static void run_layout(struct LayoutRecord *out, const char *text, int width, int spacing,
                       struct LineStyle style, BYTE outlined, BYTE align)
{
    struct CreateLineInfo layout;
    struct UIWindow parent = {0};
    size_t i;
    reset_counters(); memset(out, 0, sizeof *out);
    create_line_info_construct(&layout, &parent, text, width, spacing, style, outlined, 0xabcdef, align);
    assert(layout.count < 256);
    out->count = (unsigned)layout.count; out->height = layout.height;
    out->measured = measured; out->moves = moves; out->invalidated = invalidated; out->move_hash = move_hash;
    for (i = 0; i < layout.count; ++i) {
        struct UIWindow *window = layout.windows[i];
        int plain = window->ops == &line_cutter_text_ops;
        capture_window(&out->windows[i], window, plain);
        if (plain) line_cutter_text_destroy(window);
        else free_button(window);
    }
    create_line_info_destruct(&layout); create_line_info_destruct(&layout);
    assert(!outstanding);
}

#ifdef LINE_LAYOUT_REFERENCE
#include "line_layout_reference.c"
static void run_reference(struct LayoutRecord *out, const char *text, int width, int spacing,
                          struct LineStyle style, BYTE outlined, BYTE align)
{
    struct reference_CCreateLineInfo layout;
    struct reference_LineStyle old_style = {style.colour, style.font_height, style.bold, style.italic, {0}};
    struct UIWindow parent = {0};
    struct Vec *row;
    reset_counters(); memset(out, 0, sizeof *out);
    reference_create_line_info_construct(&layout, &parent, text, width, spacing, old_style, outlined, 0xabcdef, align);
    out->height = layout.height; out->measured = measured; out->moves = moves;
    out->invalidated = invalidated; out->move_hash = move_hash;
    for (row = layout.rows.first; row != layout.rows.last; ++row) {
        struct UIWindow **window;
        for (window = row->first; window != row->last; ++window) {
            int plain = (*window)->ops == (struct UIWindowOps *)reference_LineCutterText_ops;
            assert(out->count < 256);
            capture_window(&out->windows[out->count++], *window, plain);
            if (plain) reference_LineCutterText_Destroy((struct reference_LineCutterText *)*window, 1);
            else free_button(*window);
        }
    }
    reference_create_line_info_destruct(&layout);
    assert(!outstanding);
}
#endif

static struct LayoutRecord actual;
#ifdef LINE_LAYOUT_REFERENCE
static struct LayoutRecord expected;
#endif
static void expected_cases(void)
{
    struct LineStyle style = {0, 12, 0, 0};
    unsigned align;
    for (align = 0; align < 3; ++align) {
        int start = align == 0 ? 0 : align == 1 ? 78 : 39;
        run_layout(&actual, "a<B>b</B>c", 100, 4, style, 0, (BYTE)align);
        assert(actual.count == 3 && actual.height == 12 && actual.moves == 3);
        assert(!strcmp(actual.windows[0].text, "a") && actual.windows[0].x == start);
        assert(!strcmp(actual.windows[1].text, "b") && actual.windows[1].x == start + 7);
        assert(!strcmp(actual.windows[2].text, "c") && actual.windows[2].x == start + 15);
        assert(!actual.windows[0].bold && actual.windows[1].bold && !actual.windows[2].bold);
    }
    run_layout(&actual, "<URL>go<INFO>a,,b,</INFO></URL>", 100, 4, style, 0, 0);
    assert(actual.count == 1 && actual.height == 14 && actual.windows[0].width == 15);
    assert(actual.windows[0].kind == 0x1ad && !strcmp(actual.windows[0].text, "go"));
    assert(actual.windows[0].args == 4 && !strcmp(actual.windows[0].arguments, "a||b||"));
    assert(actual.windows[0].colour == 0xe6a300 && actual.windows[0].hover == 0xefb700);
    run_layout(&actual, "^112233^nItemID^62", 100, 4, style, 1, 0);
    assert(actual.count == 1 && !strcmp(actual.windows[0].text, "Sword"));
    assert(actual.windows[0].colour == 0x332211 && actual.windows[0].outlined);
    assert(actual.windows[0].outline == 0xabcdef && actual.windows[0].height == 14);
    run_layout(&actual, "a\nb", 100, 4, style, 0, 0);
    assert(actual.count == 2 && actual.height == 28 && actual.windows[1].y == 16);
    run_layout(&actual, "<FONT SIZE=20 COLOR=123456>a<FONT SIZE=8>b</FONT>c</FONT>d", 1000, 4, style, 0, 0);
    assert(actual.count == 4 && actual.height == 20);
    assert(actual.windows[0].font_height == 20 && actual.windows[1].font_height == 8);
    assert(actual.windows[2].font_height == 20 && actual.windows[3].font_height == 12);
    assert(actual.windows[1].y == 12 && actual.windows[3].y == 8);
    run_layout(&actual, "<TIPBOX>Guide label<INFO>42</INFO></TIPBOX>", 1000, 4, style, 0, 0);
    assert(actual.count == 1 && !actual.windows[0].kind && !actual.windows[0].args);
    assert(!strcmp(actual.windows[0].text, "Guide label") && actual.windows[0].colour == style.colour);
    run_layout(&actual, "<URL>Site<INFO>https://example.test</INFO></URL><TIPBOX>한글<INFO>42</INFO></TIPBOX><QUEST>Quest<INFO>7</INFO></QUEST>", 1000, 4, style, 0, 0);
    assert(actual.count == 3 && actual.windows[0].kind == 0x1ad && actual.windows[2].kind == 0x213);
    assert(!actual.windows[1].kind && !actual.windows[1].args && !strcmp(actual.windows[1].text, "한글"));
    assert(!strcmp(actual.windows[0].arguments, "https://example.test|") && !strcmp(actual.windows[2].arguments, "7|"));
    run_layout(&actual, "<TIPBOX>plain label</TIPBOX>", 1000, 4, style, 0, 0);
    assert(actual.count == 1 && !actual.windows[0].kind && !strcmp(actual.windows[0].text, "plain label"));
    puts("Line layout: text, fonts, colours, outlines, active links, retired guide labels, alignment and line spacing passed.");
}
static void compare_cases(void)
{
    static const char *const texts[] = {
        "", "a", "abc def ghi", "WWWWWWWWWW WWWWWWWWWWW", " 한글 갑옷 설명입니다 ",
        "\n", "\n\n", "a\r\nb\n c\r d", "a\\n b\\r c\\\"d\\t", "한글<B>굵은 글씨</B>끝",
        "a<B>b<B>c</B>d</B>e<I>f</I>g", "<b>lowercase</b><i>italic</i>",
        "^123456one^ffffff two^invalid end", "<FONT SIZE=20 COLOR=123456>a<FONT SIZE=8>b</FONT>c</FONT>d",
        "<FONT size=16 color=00ff00>hello world</font>", "<FONT SIZE=0>x</FONT>y",
        "<FONT SIZE=bad>x</FONT>", "<FONT COLOR=123456>x</FONT>z", "<unknown>text</unknown>",
        "<URL>Link<INFO>https://example.test,a,,c,</INFO></URL> tail",
        "<ITEM>한글 무기<INFO>62</INFO></ITEM>", "<NAVI>A long route name<INFO>prontera,1,2,3</INFO></NAVI>",
        "<QUEST>Quest<INFO>1</INFO></QUEST>",
        "<ITEML>ignored</ITEML><NAVIL>ignored</NAVIL>", "<URL>broken</URL>tail", "<URL>unclosed",
        "<URL><INFO>1</INFO></URL>", "<URL>empty<INFO></INFO></URL>",
        "<URL>^123456coloured link<INFO>a,b</INFO></URL>", "^nItemID^62 tail ^nItemID^63 끝",
        "^nItemID^12abc ^nItemID^bad", "<MSG>1,word,7</MSG>", "x<MSG>2</MSG> y",
        "<FONT SIZE=18>test ^010203red</FONT>default", "<FONTISH SIZE=12>prefix</FONT>tail"
    };
    static const int widths[] = {24, 53, 120, 1000};
    unsigned t, w, a, outline, s, comparisons = 0;
    for (t = 0; t < sizeof texts / sizeof texts[0]; ++t)
    for (w = 0; w < sizeof widths / sizeof widths[0]; ++w)
    for (a = 0; a < 4; ++a)
    for (outline = 0; outline < 2; ++outline)
    for (s = 0; s < 2; ++s) {
        struct LineStyle style = {0x112233, 12, (BYTE)s, (BYTE)s};
        run_layout(&actual, texts[t], widths[w], s ? 6 : 0, style, (BYTE)outline, (BYTE)a);
#ifdef LINE_LAYOUT_REFERENCE
        run_reference(&expected, texts[t], widths[w], s ? 6 : 0, style, (BYTE)outline, (BYTE)a);
        if (memcmp(&actual, &expected, sizeof actual)) {
            fprintf(stderr, "Layout mismatch: text %u width %d align %u outline %u style %u\n", t, widths[w], a, outline, s);
            fprintf(stderr, "counts %u/%u height %d/%d measurements %u/%u moves %u/%u\n", actual.count, expected.count, actual.height, expected.height, actual.measured, expected.measured, actual.moves, expected.moves);
            abort();
        }
#endif
        ++comparisons;
    }
    printf("Line layout: %u %slayout/style/link/placement cases passed.\n", comparisons,
#ifdef LINE_LAYOUT_REFERENCE
           "archived "
#else
           ""
#endif
    );
}

static void safety_cases(void)
{
    struct LineStyle style = {0x112233, 12, 1, 0};
    struct LineCutterText run;
    struct CreateLineInfo empty;
    int width;
    for (width = -2; width < 14; ++width) {
        run_layout(&actual, "W한글<URL>WW한글<INFO>a</INFO></URL>", width, 4, style, 0, 0);
        assert(actual.count && actual.count < 20);
    }
    run_layout(&actual, "</FONT>plain<FONT COLOR=bad>x</FONT>end", 1000, 4, style, 0, 0);
    assert(actual.count == 1 && !strcmp(actual.windows[0].text, "plainxend"));
    create_line_info_construct(&empty, NULL, "text", 10, 4, style, 0, 0, 0);
    assert(!empty.count && !empty.height && !empty.windows); create_line_info_destruct(&empty);
    run_layout(&actual, NULL, 10, 4, style, 0, 0); assert(!actual.count && !actual.height);
    for (fail_window = 1; fail_window <= 8; ++fail_window)
        run_layout(&actual, "first <URL>link<INFO>arg</INFO></URL> last", 53, 4, style, 0, 0);
    fail_window = 0;
    reset_counters(); line_cutter_text_construct(&run);
    line_cutter_text_set_text(&run, text_view("abcd", 4), style, 1, 0x123456);
    line_cutter_text_set_text(&run, text_view(run.text.data + 1, 2), style, 1, 0x123456);
    assert(!strcmp(string_cstr(&run.text), "bc"));
    {
        SYSTEM_INFO system;
        char *pages, *edge;
        DWORD prior;
        GetSystemInfo(&system);
        pages = VirtualAlloc(NULL, system.dwPageSize * 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        assert(pages && VirtualProtect(pages + system.dwPageSize, system.dwPageSize, PAGE_NOACCESS, &prior));
        edge = pages + system.dwPageSize - 4;
        memcpy(edge, "A\0BC", 4);
        line_cutter_text_set_text(&run, text_view(edge, 4), style, 0, 0);
        line_cutter_text_on_draw((struct UIWindow *)(&run));
        assert(drawn.size == 4 && !memcmp(drawn.data, edge, 4));
        string_free(&drawn);
        assert(VirtualFree(pages, 0, MEM_RELEASE));
    }
    line_cutter_text_destruct(&run); assert(!outstanding);
    puts("Line layout: narrow widths, malformed fonts, failed child allocations, aliased/counted text and protected pages passed.");
}

static void escape_cases(void)
{
    static const char alphabet[] = "\\rn\"x";
    unsigned n, code;
    for (n = 0; n <= 7; ++n) {
        unsigned max = 1, i;
        for (i = 0; i < n; ++i) max *= sizeof alphabet - 1;
        for (code = 0; code < max; ++code) {
            char text[16] = {0}, original[16] = {0};
            unsigned value = code;
            size_t length;
            for (i = 0; i < n; ++i) { original[i] = alphabet[value % 5]; value /= 5; }
            memcpy(text, original, sizeof text);
            length = text_unescape(text, n);
            assert(length <= n && !text[length]);
#ifdef LINE_LAYOUT_REFERENCE
            {
                struct String prior;
                string_init(&prior); string_assign(&prior, original, n);
                reference_text_unescape(&prior);
                assert(length == prior.size && !memcmp(text, string_cstr(&prior), length));
                string_free(&prior); assert(!outstanding);
            }
#endif
        }
    }
    puts("Line layout: 97656 escape combinations passed.");
}

int main(void)
{
    assert(!_setmbcp(949));
    button_ops.move = ui_window_move_base; button_ops.resize = ui_window_resize_base;
    expected_cases(); compare_cases(); safety_cases(); escape_cases();
    assert(!outstanding);
    return 0;
}
