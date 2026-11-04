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
#undef free
#undef realloc
#undef malloc
#ifdef TEXT_BUTTONS_REFERENCE
struct archived_UIButton {
    struct UIWindow base;
    struct String text, tooltip;
    BYTE disabled, skinned, padding[2];
    int cmd_value;
};
static struct archived_UIButton *archived_button_construct(struct archived_UIButton *button);
static void archived_button_destruct(struct archived_UIButton *button);
static void archived_button_hover(struct archived_UIButton *button, int x, int y);
#define UIButton archived_UIButton
#define ui_button_construct archived_button_construct
#define ui_button_destruct archived_button_destruct
#define ui_button_on_mouse_hover archived_button_hover
#include "text_buttons_reference.c"
#undef ui_button_on_mouse_hover
#undef ui_button_destruct
#undef ui_button_construct
#undef UIButton
#endif

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
intptr_t ui_window_send_msg_base(struct UIWindow *window, struct UIWindow *sender, int message, intptr_t a, intptr_t b, intptr_t c, intptr_t d)
{ assert(window == &parent && sender); event(6, message, a, b, c, d); return 0; }
void ui_window_on_mouse_hover_base(struct UIWindow *window, int x, int y)
{ assert(window == &parent); event(7, x, y, 0, 0, 0); }
void ui_window_on_wheel_base(struct UIWindow *window, int delta)
{ assert(window == &parent); event(8, delta, 0, 0, 0, 0); }
void ui_button_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIButton *button = (struct UIButton *)window; assert(button); event(9, x, y, 0, 0, 0); }
#ifdef TEXT_BUTTONS_REFERENCE
static struct archived_UIButton *archived_button_construct(struct archived_UIButton *button)
{
    memset(button, 0, sizeof *button); button->base.ops = &boundary_ops;
    string_init(&button->text); string_init(&button->tooltip); return button;
}
static void archived_button_destruct(struct archived_UIButton *button)
{ string_free(&button->text); string_free(&button->tooltip); event(1, 0, 0, 0, 0, 0); }
static void archived_button_hover(struct archived_UIButton *button, int x, int y)
{ assert(button); event(9, x, y, 0, 0, 0); }
#endif
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
#include "text_buttons_unreached.h"

static void reset_fixture(void)
{
    assert(!outstanding); event_count = 0; memset(events, 0, sizeof events);
    memset(drawn, 0, sizeof drawn); memset(&parent, 0, sizeof parent);
    parent.ops = &boundary_ops; captured = NULL;
}
static void native_cases(void)
{
    struct UITextButton *button;
    struct TextView line;
    size_t i;
    static const char embedded[] = "map\0suffix";
    reset_fixture(); button = ui_text_button_create();
    ui_text_button_set_text(&button->button.base, "abcdef", 7, 0, 12, 0, 1);
    ui_text_button_set_text(&button->button.base, string_cstr(&button->button.text) + 2, 7, 0, 12, 0, 1);
    assert(!strcmp(string_cstr(&button->label), "cdef") && button->button.base.width == 36);
    ui_text_button_set_text(&button->button.base, "abcdefghijklmnop 한글", 7, 0, 12, 0, 1);
    ui_text_button_set_text(&button->button.base, string_cstr(&button->label) + 16, 7, 0, 12, 0, 1);
    assert(!strcmp(string_cstr(&button->label), " 한글"));
    assert(button->button.base.width == (int)strlen(" 한글") * 6 + 12);
    assert(!ui_text_button_line_count(button) && !ui_text_button_get_line(button, 99).size);
    ui_text_button_add_line(button, text_view_cstr(""));
    ui_text_button_add_line(button, text_view(embedded, sizeof embedded - 1));
    for (i = 0; i < 24; ++i) ui_text_button_add_line(button, ui_text_button_get_line(button, 1));
    assert(ui_text_button_line_count(button) == 26);
    for (i = 1; i < 26; ++i) {
        line = ui_text_button_get_line(button, i);
        assert(line.size == sizeof embedded - 1 && !memcmp(line.data, embedded, line.size) && !line.data[line.size]);
    }
    ui_text_button_destroy((struct UIWindow *)(button)); assert(!outstanding);
    puts("Text buttons: caption/label suffix aliases, Korean bytes, native factory, counted/empty arguments, moved storage and cleanup passed.");
}

struct Snapshot {
    struct Event events[128];
    unsigned event_count;
    int width, height, state, captured, value;
    DWORD colour;
    char caption[256], label[256], drawn[256];
};
static void text_case(struct Snapshot *out, int old, const char *text, int style, int width, int disabled)
{
    struct UITextButton *current = NULL;
    struct UIWindow *window;
    BYTE *disabled_field;
    const char *label;
#ifdef TEXT_BUTTONS_REFERENCE
    struct reference_UITextButton *saved = NULL;
#define CALL(name, ...) do { if (old) reference_##name(saved, __VA_ARGS__); else name(current, __VA_ARGS__); } while (0)
#define WINDOW_CALL(name, ...) do { if (old) reference_##name(saved, __VA_ARGS__); else name(window, __VA_ARGS__); } while (0)
#define CALL0(name) do { if (old) reference_##name(saved); else name(current); } while (0)
#define WINDOW_CALL0(name) do { if (old) reference_##name(saved); else name(window); } while (0)
    if (old) {
        reset_fixture(); saved = reference_UITextButton_Construct(mem_alloc(sizeof *saved));
        window = &saved->button.base; disabled_field = &saved->button.disabled;
        saved->hover_colour = 27; saved->value = 51;
    } else
#else
    (void)old;
#define CALL(name, ...) name(current, __VA_ARGS__)
#define WINDOW_CALL(name, ...) name(window, __VA_ARGS__)
#define CALL0(name) name(current)
#define WINDOW_CALL0(name) name(window)
#endif
    {
        reset_fixture(); current = ui_text_button_create();
        window = &current->button.base; disabled_field = &current->button.disabled;
        current->hover_colour = 27; current->value = 51;
    }
    memset(out, 0, sizeof *out);
    window->parent = &parent; window->x = 3; window->y = 4; window->id = 61;
    *disabled_field = (BYTE)disabled;
#ifdef TEXT_BUTTONS_REFERENCE
    if (old) reference_UITextButton_SetText(window, text, 17, 1, 12, (BYTE)(style & 1), (BYTE)(style >> 1));
    else
#endif
        ui_text_button_set_text(window, text, 17, 1, 12, (BYTE)(style & 1), (BYTE)(style >> 1));
    assert(window->width == (int)strlen(text) * 6 + (style & 1) * 4 + 12 && window->height == 14);
    WINDOW_CALL0(ui_text_button_on_draw);
    CALL(ui_text_button_fit_width, width, 19);
    WINDOW_CALL0(ui_text_button_on_draw);
    if (*text) {
        assert(window->width == ((int)strlen(text) * 7 + 2 > width ? width : (int)strlen(text) * 7 + 2));
        assert(!strcmp(drawn, (int)strlen(text) * 7 + 2 > width ? "cut..." : text));
    }
    CALL0(ui_text_button_fit_to_caption);
    WINDOW_CALL(ui_text_button_on_mouse_move, 1, 1);
    captured = window; WINDOW_CALL(ui_text_button_on_mouse_move, 1, 1);
    WINDOW_CALL(ui_text_button_on_mouse_move, -1, -1);
    WINDOW_CALL(ui_text_button_on_mouse_hover, 2, 5);
    WINDOW_CALL(ui_text_button_on_wheel, -120);
    WINDOW_CALL(ui_text_button_on_l_btn_up, 1, 1);
    WINDOW_CALL(ui_text_button_on_r_btn_down, 0, 0);
    captured = window; WINDOW_CALL(ui_text_button_on_r_btn_up, -1, -1);
    WINDOW_CALL0(ui_text_button_on_draw);
    assert(!strcmp(drawn, text));
    out->width = window->width; out->height = window->height; out->state = window->state;
    out->captured = captured == window;
    strcpy(out->drawn, drawn);
#ifdef TEXT_BUTTONS_REFERENCE
    if (old) { strcpy(out->caption, string_cstr(&saved->button.text)); label = string_cstr(&saved->label); out->colour = saved->text_colour; out->value = saved->value; }
    else
#endif
    { strcpy(out->caption, string_cstr(&current->button.text)); label = string_cstr(&current->label); out->colour = current->text_colour; out->value = current->value; }
    strcpy(out->label, label);
#ifdef TEXT_BUTTONS_REFERENCE
    if (old) reference_UITextButton_Destroy(saved, 1);
    else
#endif
        ui_text_button_destroy((struct UIWindow *)(current));
    out->event_count = event_count; memcpy(out->events, events, event_count * sizeof *events);
    assert(!outstanding);
#undef CALL
#undef WINDOW_CALL
#undef CALL0
#undef WINDOW_CALL0
}

static void behavior_cases(void)
{
    static const char *const inputs[] = {"", "x", "한글", "abcdef", "a longer label with spaces", "안녕하세요 세계"};
    static const int widths[] = {8, 60, 300};
    struct Snapshot actual;
    int input, style, width, disabled, text_cases = 0;
#ifdef TEXT_BUTTONS_REFERENCE
    struct Snapshot expected;
#endif
    for (input = 0; input < 6; ++input) for (style = 0; style < 4; ++style)
        for (width = 0; width < 3; ++width) for (disabled = 0; disabled < 2; ++disabled) {
            text_case(&actual, 0, inputs[input], style, widths[width], disabled);
#ifdef TEXT_BUTTONS_REFERENCE
            text_case(&expected, 1, inputs[input], style, widths[width], disabled);
            assert(!memcmp(&actual, &expected, sizeof actual));
#endif
            ++text_cases;
        }
#ifdef TEXT_BUTTONS_REFERENCE
    printf("Text buttons: %d archived text layout/draw/input cases passed.\n", text_cases);
#else
    printf("Text buttons: %d text layout/draw/input cases passed.\n", text_cases);
#endif
}

#ifdef TEXT_BUTTONS_REFERENCE
static void alias_regression(void)
{
    struct reference_UITextButton saved;
    reset_fixture();
    reference_UITextButton_Construct(&saved);
    reference_UITextButton_SetText(&saved.button.base, "abcdef", 7, 0, 12, 0, 1);
    reference_UITextButton_SetText(&saved.button.base, string_cstr(&saved.button.text) + 2, 7, 0, 12, 0, 1);
    assert(!strcmp(string_cstr(&saved.label), "cdef"));
    assert(saved.button.base.width == 24);
    reference_UITextButton_Destruct(&saved);
    assert(!outstanding);
    puts("Text buttons: archived suffix-alias width regression reproduced; native width is 36 for cdef.");
}
#endif

int main(void)
{
    boundary_ops.invalidate = ui_window_invalidate_base;
    boundary_ops.on_mouse_hover = ui_window_on_mouse_hover_base;
    boundary_ops.on_wheel = ui_window_on_wheel_base;
    boundary_ops.send_msg = ui_window_send_msg_base;
    native_cases(); behavior_cases();
#ifdef TEXT_BUTTONS_REFERENCE
    alias_regression();
#endif
    return 0;
}
