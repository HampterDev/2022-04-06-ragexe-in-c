#include "ui/controls/listbox.h"
#include "ui_string_test.h"
#include <stdint.h>
#include "owned_storage_test_allocator.h"
#include "../src/ui/controls/text_view_ex.c"
#include "core/fatal.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "std/string.h"
unsigned int g_codePage = 949;

#ifdef UI_STRING_REFERENCE
typedef struct UITextViewWord TestWord;
#else
typedef struct UIString TestWord;
#endif

static uint32_t digest = 2166136261u;
static unsigned char trace[65536];
static size_t trace_size;
static void record_byte(unsigned char value)
{
    assert(trace_size < sizeof trace);
    trace[trace_size++] = value;
    digest = (digest ^ value) * 16777619u;
}
void test_ui_string_number(int value)
{
    unsigned i;
    for (i = 0; i < 4; ++i) record_byte((unsigned char)((uint32_t)value >> (8 * i)));
}
void test_ui_string_record(const struct String *text)
{
    size_t i;
    test_ui_string_number((int)text->size);
    for (i = 0; i < text->size; ++i) record_byte((unsigned char)string_cstr(text)[i]);
}
void test_ui_string_no_allocations(void) { assert(!outstanding); }
void *mem_alloc(size_t size) { void *p = checked_malloc(size); assert(p); return p; }
void mem_free(void *p) { checked_free(p); }
__declspec(noreturn) void fatal_error(const char *message) { fputs(message, stderr); abort(); }

static struct UIWindow *children[1024];
static size_t child_count;
static struct UIWindowOps scrollbar_ops;
#ifdef UI_STRING_REFERENCE
const void *ui_text_viewer_ops[52];
#else
const struct UIListBoxOps ui_text_viewer_ops;
#endif

struct UIWindow *ui_window_construct(struct UIWindow *window, int composites)
{ assert(!composites); memset(window, 0, sizeof *window); return window; }
void ui_window_resize_base(struct UIWindow *window, int width, int height)
{ window->width = width; window->height = height; test_ui_string_number(width); test_ui_string_number(height); }
void ui_window_create(struct UIWindow *window, int width, int height)
{ ui_window_resize_base(window, width, height); ui_window_on_create(UI_WINDOW(window), width, height); }
void ui_window_on_create_base(struct UIWindow *window, int width, int height)
{ assert(window->width == width && window->height == height); }
void ui_window_move_base(struct UIWindow *window, int x, int y)
{ window->x = x; window->y = y; test_ui_string_number(x); test_ui_string_number(y); }
void ui_window_set_show_base(struct UIWindow *window, int show)
{ (void)window; test_ui_string_number(show); }
void ui_window_invalidate_base(struct UIWindow *window) { (void)window; test_ui_string_number(0x123); }
void ui_window_add_child(struct UIWindow *window, struct UIWindow *child)
{ assert(child_count < 1024); child->parent = window; children[child_count++] = child; }
void ui_window_remove_child(struct UIWindow *window, struct UIWindow *child, BYTE destroy)
{
    size_t i;
    assert(child->parent == window && destroy);
    for (i = 0; i < child_count && children[i] != child; ++i) {}
    assert(i < child_count);
    memmove(children + i, children + i + 1, (--child_count - i) * sizeof *children);
    child->parent = NULL;
    ui_window_destroy(UI_WINDOW(child));
}
void ui_window_destruct(struct UIWindow *window)
{
    size_t i = 0;
    while (i < child_count) {
        if (children[i]->parent == window) ui_window_remove_child(window, children[i], 1);
        else ++i;
    }
}
void ui_list_box_destruct(struct UIListBox *list) { ui_window_destruct(&list->base); }
void ui_list_box_clear_items(struct UIListBox *list) { (void)list; }
void ui_window_clear_canvas(struct UIWindow *window, DWORD colour)
{ (void)window; test_ui_string_number((int)colour); }
int ui_window_text_width(struct UIWindow *window, const char *text, int len, int bold, int height, int font, int charset)
{ (void)window; (void)text; assert(!bold && height == 12 && !font && !charset); return len * 3; }
int ui_window_text_height(struct UIWindow *window, const char *text, int len, int bold, int height, int font, int charset)
{ (void)window; (void)text; (void)len; assert(!bold && height == 12 && !font && !charset); return 12; }

static void set_view_text(struct UITextViewEX *view, struct TextView input)
{
#ifdef UI_STRING_REFERENCE
    struct String copy;
    string_init(&copy);
    string_assign(&copy, input.data, input.size);
    ui_text_view_ex_set_text(view, copy);
#else
    ui_text_view_ex_set_text(view, input);
#endif
}

static void record_view(struct UITextViewEX *view)
{
    TestWord **it;
    struct Vec *row;
    test_ui_string_number((int)WORD_COUNT(&view->words));
    test_ui_string_number((int)ROW_COUNT(view));
    for (it = view->words.first; it != view->words.last; ++it) {
        test_ui_string_record(&(*it)->text);
        test_ui_string_record(&(*it)->image);
        test_ui_string_number((*it)->joined);
        test_ui_string_number((*it)->r); test_ui_string_number((*it)->g); test_ui_string_number((*it)->b);
        test_ui_string_number((*it)->base.width); test_ui_string_number((*it)->base.height);
    }
    for (row = view->rows.first; row != view->rows.last; ++row)
        test_ui_string_number((int)WORD_COUNT(row));
    ui_text_view_ex_on_draw(&view->viewer.list.base);
    view->viewer.list.top = 1;
    ui_text_view_ex_on_draw(&view->viewer.list.base);
    view->viewer.list.top = 0;
}

static void view_cases(void)
{
    static const char *const texts[] = {
        "", "one two three", "  one\t two\n\r\nthree  ", "한글 단어 긴 문장 테스트",
        "one^ff0000red words^000000tail",
        "before <=image=\"501\" >item<\\end> after",
        "<=link=\"poring\" >monster<\\end>and more words\nlast",
        "a very long word followed by several more words to exercise vector growth and wrapping"
    };
    static const int widths[] = {35, 80, 300};
    unsigned t, w;
    scrollbar_ops.invalidate = ui_window_invalidate_base;
    for (w = 0; w < sizeof widths / sizeof widths[0]; ++w) {
        for (t = 0; t < sizeof texts / sizeof texts[0]; ++t) {
            struct UITextViewEX view = {0};
            struct UIScrollBar bar = {0};
            TestWord **words;
            const char *alias;
#ifdef UI_STRING_REFERENCE
            view.viewer.list.base.ops = (struct UIWindowOps *)ui_text_view_ex_ops;
#else
            view.viewer.list.base.ops = &ui_text_view_ex_ops.base;
#endif
            view.viewer.list.base.width = widths[w];
            view.viewer.list.base.height = 63;
            view.viewer.list.line_height = 21;
            view.viewer.list.vbar = &bar;
            bar.base.ops = &scrollbar_ops;
            set_view_text(&view, text_view_cstr(texts[t]));
            if (t == 1) assert(WORD_COUNT(&view.words) == 3);
            record_view(&view);
            words = view.words.first;
            alias = words != view.words.last ? string_cstr(&words[0]->text) : "";
            ui_text_view_ex_send_msg(&view.viewer.list.base, NULL, UI_MSG_ADD_TEXT, (int)alias, 0, 0, 0);
            record_view(&view);
#ifdef UI_STRING_REFERENCE
            ui_text_view_ex_clear(&view);
#else
            ui_text_view_ex_clear(&view.viewer.list);
#endif
            assert(!child_count && view.words.first == view.words.last && view.rows.first == view.rows.last);
            text_view_ex_destruct(&view);
            test_ui_string_no_allocations();
        }
    }
}

#include "ui_string_unreached.h"

int main(int argc, char **argv)
{
    test_ui_string_owner();
    view_cases();
    assert(!outstanding && !child_count);
    assert(digest == 0x668f2b06u);
    if (argc > 1) {
        FILE *file = fopen(argv[1], "wb");
        assert(file && fwrite(trace, 1, trace_size, file) == trace_size);
        assert(!fclose(file));
    }
    printf("UI string parser and layout digest: %08x\n", (unsigned)digest);
    return 0;
}
