#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui/controls/buttons.h"
#include "ui/window_mgr.h"
#include "ui/window_manager_state.h"
#include "core/fatal.h"
#include "std/std.h"
#include "core/markup_tags.h"
#include "ui/chat/chat_history.h"
static void *navigation_malloc(size_t size);
#include "owned_storage_test_allocator.h"

static void *moving_realloc(void *old, size_t size)
{
    void *next = checked_malloc(size);
    assert(next);
    if (old) {
        size_t before = ((struct Allocation *)old - 1)->size;
        memcpy(next, old, before < size ? before : size); checked_free(old);
    }
    return next;
}
#define malloc checked_malloc
#define realloc moving_realloc
#define free checked_free
#include "../src/core/owned_text.c"
#include "../src/core/text_array.c"
#include "../src/ui/controls/text_button.c"
#undef malloc
#define malloc navigation_malloc
#include "../src/ui/controls/navi_link_button.c"
#undef malloc
#define malloc checked_malloc
#include "../src/std/list.c"
#include "../src/ui/chat/chat_history.c"
#include "../src/ui/chat/chat_link_lines.c"
#undef free
#undef realloc
#undef malloc

void *mem_alloc(size_t size) { return checked_malloc(size); }
void mem_free(void *p)
{
    checked_free(p);
}
__declspec(noreturn) void fatal_error(const char *text) { (void)text; abort(); }

struct UIWindowMgr g_windowMgr;
static struct UIWindow parent;
static struct UIWindow *captured;
static struct UIWindowOps boundary_ops;
static unsigned event_count;
struct Event { int kind, a, b, c, d, e; };
static struct Event events[128];
static char drawn[256];
static void event(int kind, int a, int b, int c, int d, int e)
{
    struct Event *out;
    assert(event_count < 128); out = &events[event_count++];
    out->kind = kind; out->a = a; out->b = b; out->c = c; out->d = d; out->e = e;
}
struct UIButton *ui_button_construct(struct UIButton *button)
{
    memset(button, 0, sizeof *button); button->base.ops = &boundary_ops;
    string_init(&button->text); string_init(&button->tooltip); return button;
}
void ui_button_destruct(struct UIButton *button)
{ string_free(&button->text); string_free(&button->tooltip); event(1, 0, 0, 0, 0, 0); }
void ui_window_resize_base(struct UIWindow *window, int width, int height)
{ window->width = width; window->height = height; event(2, width, height, 0, 0, 0); }
void ui_window_invalidate_base(struct UIWindow *window) { event(3, window == &parent, 0, 0, 0, 0); }
void ui_window_on_create_base(struct UIWindow *window, int width, int height)
{ (void)window; event(4, width, height, 0, 0, 0); }
int ui_window_send_msg_base(struct UIWindow *window, struct UIWindow *sender, int message, int a, int b, int c, int d)
{ assert(window == &parent && sender); event(6, message, a, b, c, d); return 0; }
void ui_window_on_mouse_hover_base(struct UIWindow *window, int x, int y)
{ assert(window == &parent); event(7, x, y, 0, 0, 0); }
void ui_window_on_wheel_base(struct UIWindow *window, int delta)
{ assert(window == &parent); event(8, delta, 0, 0, 0, 0); }
void ui_button_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIButton *button = (struct UIButton *)window; assert(button); event(9, x, y, 0, 0, 0); }
struct UIWindow *ui_window_mgr_get_capture(struct UIWindowMgr *manager)
{ assert(manager == &g_windowMgr); return captured; }
void ui_window_mgr_release_capture(struct UIWindowMgr *manager)
{ assert(manager == &g_windowMgr); captured = NULL; event(10, 0, 0, 0, 0, 0); }
struct Mode *client_current_mode(void) { return (struct Mode *)&parent; }
void mode_set_cursor_action(struct Mode *mode, int cursor)
{ assert(mode == (struct Mode *)&parent); event(11, cursor, 0, 0, 0, 0); }
void ui_window_on_mouse_shape_base(struct UIWindow *window, int x, int y)
{ assert(window); event(12, x, y, 0, 0, 0); }
void ui_window_clear_canvas(struct UIWindow *window, DWORD colour)
{ assert(window && colour == UI_TRANSPARENT); event(13, 0, 0, 0, 0, 0); }
void ui_window_fill_rect(struct UIWindow *window, int x, int y, int width, int height, DWORD colour)
{ assert(window); event(14, x, y, width, height, (int)colour); }
void ui_window_draw_round_frame(struct UIWindow *window, int x, int y, int width, int height, DWORD colour)
{ assert(window); event(15, x, y, width, height, (int)colour); }
int ui_window_text_width(struct UIWindow *window, const char *text, int len, int face, int height, int bold, int italic)
{ assert(window && !len); event(16, face, height, bold, italic, (int)strlen(text)); return (int)strlen(text) * 7; }
void ui_window_text_out(struct UIWindow *window, int x, int y, const char *text, int len,
                     int face, int height, COLORREF colour, int bold, int italic)
{
    assert(window && !len && strlen(text) < sizeof drawn); strcpy(drawn, text);
    event(17, x, y, face, height, (int)colour); event(18, bold, italic, 0, 0, 0);
}
void ui_window_text_out_outlined(struct UIWindow *window, int x, int y, const char *text, int len,
                             COLORREF colour, COLORREF outline, int face, int height, int bold, int italic)
{
    ui_window_text_out(window, x, y, text, len, face, height, colour, bold, italic);
    event(19, (int)outline, 0, 0, 0, 0);
}
const char *ui_window_shorten_text(struct UIWindow *window, struct String *out, const char *text,
                               int width, int face, int height, int outlined)
{
    assert(window); event(20, width, face, height, outlined, (int)strlen(text));
    string_assign_cstr(out, "cut..."); return string_cstr(out);
}
#include "chat_navigation_unreached.h"

static void reset_fixture(void)
{
    assert(!outstanding); event_count = 0; memset(events, 0, sizeof events);
    memset(drawn, 0, sizeof drawn); memset(&parent, 0, sizeof parent);
    parent.ops = &boundary_ops; captured = NULL;
}

static int fail_navigation;
static void *navigation_malloc(size_t size)
{ assert(size == sizeof(struct UINaviLinkButton)); return fail_navigation ? NULL : checked_malloc(size); }
static struct UIWindow *children[8];
static unsigned child_count, removed_count, move_count, tooltip_count;
static void reset_children(void)
{ memset(children, 0, sizeof children); child_count = removed_count = move_count = tooltip_count = 0; fail_navigation = 0; }
void ui_window_move_base(struct UIWindow *window, int x, int y)
{ ++move_count; window->x = x; window->y = y; }
void ui_window_add_child(struct UIWindow *window, struct UIWindow *child)
{ assert(window && child && child_count < 8); children[child_count++] = child; child->parent = window; }
void ui_window_remove_child(struct UIWindow *window, struct UIWindow *child, BYTE destroy)
{
    unsigned i; assert(window && child && destroy && child->parent == window);
    for (i = 0; i < child_count; ++i) if (children[i] == child) break;
    assert(i < child_count); children[i] = NULL; ++removed_count;
    ui_navi_link_button_destroy(child);
}
void ui_bitmap_button_set_tool_tip(struct UIBitmapButton *button, const char *text)
{ assert(button && text); ++tooltip_count; }
struct String *ui_window_fit_plain_text(struct UIWindow *window, struct String *out, struct TextView text, int width)
{ assert(window && width >= 0); string_assign(out, text.data, text.size); return out; }
struct UIHooks g_uiHooks;
static void init_box(struct UIChatHisBox *box, struct ListNode *head, struct String *rows, size_t count)
{
    memset(box, 0, sizeof *box); memset(head, 0, sizeof *head);
    head->next = head->prev = head; box->navi_links.head = head;
    box->list.items.first = rows; box->list.items.last = box->list.items.end = rows + count;
    box->list.base.width = 400; box->list.base.height = 40; box->list.line_height = 10; box->list.scroll_x = 3;
}
static void free_links(struct UIChatHisBox *box)
{
    struct ListNode *head = box->navi_links.head;
    unsigned i;
    while (head->next != head) list_erase(&box->navi_links, head->next, destroy_windows);
    for (i = 0; i < child_count; ++i) if (children[i]) ui_window_remove_child(&box->list.base, children[i], 1);
}
static void create_cases(void)
{
    int failed;
    for (failed = 0; failed < 2; ++failed) {
        struct UIChatHisBox box; struct ListNode head; struct String rows[4], input, output;
        reset_fixture(); reset_children(); init_box(&box, &head, rows, 2); fail_navigation = failed;
        string_init(&input);
        string_assign_cstr(&input, "prefix<NAVIL>prontera0102</NAVIL>tail");
        build_navi_line(&box, &output, "prefix<prontera 1,2>tail", &input);
        assert(!strcmp(string_cstr(&output), "prefix<prontera 1,2>tail"));
        assert(child_count == (unsigned)!failed && box.navi_links.size == (unsigned)!failed);
        if (!failed) {
            struct UINaviLinkButton *button = (struct UINaviLinkButton *)children[0];
            assert(button->line == 2 && button->line_x == 39);
            assert(!strcmp(string_cstr(&button->link), "<NAVIL>prontera0102</NAVIL>"));
            assert(!strcmp(string_cstr(&button->text.button.text), "<prontera 1,2>"));
        }
        string_free(&input); string_free(&output); free_links(&box); assert(!outstanding);
    }
    puts("Chat navigation: readable text, complete native control allocation, typed line/position fields and failed child allocation passed.");
}
static struct UINaviLinkButton *append_child(struct UIChatHisBox *box, int line, int x, int id)
{
    struct UINaviLinkButton *button = ui_navi_link_button_create();
    void *ptr = button; struct Vec values = {&ptr, &ptr + 1, &ptr + 1}, copy;
    vec_copy(&copy, &values, sizeof ptr);
    list_push_back(&box->navi_links, &copy, sizeof copy);
    button->line = line; button->line_x = x; button->text.button.base.id = id;
    button->text.button.base.x = 77; button->text.button.base.y = 88;
    string_assign_cstr(&button->link, "<NAVIL>한글0102</NAVIL>");
    ui_window_add_child(&box->list.base, &button->text.button.base); return button;
}
static void placement_and_trim(void)
{
    struct UIChatHisBox box; struct ListNode head; struct String rows[8];
    struct UINaviLinkButton *low, *shown, *high, *other;
    reset_fixture(); reset_children(); init_box(&box, &head, rows, 8); box.list.top = 3;
    low = append_child(&box, 1, 11, UI_ID_NAVI_LINK);
    shown = append_child(&box, 4, 21, UI_ID_NAVI_LINK);
    high = append_child(&box, 7, 31, UI_ID_NAVI_LINK);
    other = append_child(&box, 6, 41, 999);
    place_navi_links(&box, 4);
    assert(move_count == 3 && low->text.button.base.x == -100 && high->text.button.base.y == -100);
    assert(shown->text.button.base.x == 22 && shown->text.button.base.y == 12);
    assert(other->text.button.base.x == 77 && other->line == 6);
    drop_navi_links_first_half(&box);
    assert(removed_count == 1 && !children[0] && box.navi_links.size == 3);
    assert(shown->line == 0 && high->line == 3 && other->line == 6);
    assert(!strcmp(string_cstr(&shown->link), "<NAVIL>한글0102</NAVIL>"));
    box.list.items.last = rows + 4; box.list.top = 0;
    place_navi_links(&box, 0);
    assert(shown->text.button.base.x == 22 && shown->text.button.base.y == 2);
    free_links(&box); assert(!outstanding);
    puts("Chat navigation: scrolling, hidden rows, retained coordinates, half-history trimming, child removal and guarded allocation bounds passed.");
}
int main(void)
{
    boundary_ops.invalidate = ui_window_invalidate_base;
    create_cases(); placement_and_trim(); return 0;
}

DWORD *std_vector_dword_emplace_realloc_c(struct Vec *v, DWORD *where, const DWORD *value)
{
    size_t size = v->first ? (DWORD *)v->last - (DWORD *)v->first : 0;
    DWORD *next = mem_alloc((size + 1) * sizeof *next);
    DWORD copy = *value;
    assert(where == v->last);
    if (size) memcpy(next, v->first, size * sizeof *next);
    if (v->first) mem_free(v->first);
    next[size] = copy; v->first = next; v->last = v->end = next + size + 1;
    return next + size;
}
