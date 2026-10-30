#include "ui/window_access.h"
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
#include "ui/quests/tag_list_box.h"
static SHORT fixture_key_state(int key);
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
#include "../src/ui/controls/navi_link_button.c"
#define GetAsyncKeyState fixture_key_state
#include "../src/ui/quests/tag_button.c"
#include "../src/ui/quests/tag_list_box.c"
#undef free
#undef realloc
#undef malloc
#ifdef LINK_BUTTONS_REFERENCE
#include "link_buttons_reference.c"
#endif
#undef GetAsyncKeyState

void *mem_alloc(size_t size) { return checked_malloc(size); }
void mem_free(void *p)
{
    checked_free(p);
}
__declspec(noreturn) void fatal_error(const char *text) { (void)text; abort(); }

struct UIWindowMgr g_windowMgr;
static struct UIWindow parent, item_window;
static struct UIWindow *focused;
static int shift_pressed, window_available = 1;
static char item_name[256];
static int item_type;
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
{
    assert(window == &parent || window == focused || window == &item_window);
    if (message == 0x18 || message == 0x94) {
        const struct ItemInfo *item = (const struct ItemInfo *)a;
        item_type = item->type; strcpy(item_name, string_cstr(&item->id_text)); a = item_type;
    } else assert(sender);
    event(6, message, a, b, c, d); return 0;
}
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
#include "link_buttons_unreached.h"

static void reset_fixture(void)
{
    assert(!outstanding); event_count = 0; memset(events, 0, sizeof events);
    memset(drawn, 0, sizeof drawn); memset(&parent, 0, sizeof parent);
    parent.ops = &boundary_ops; captured = NULL;
}

static SHORT fixture_key_state(int key) { assert(key == VK_SHIFT); return shift_pressed ? (SHORT)0x8000 : 0; }
struct UIWindow *ui_window_mgr_get_focus_edit(struct UIWindowMgr *manager)
{ assert(manager == &g_windowMgr); return focused; }
struct UIWindow *ui_window_open(int id)
{ assert(id == WND_ITEM_DESCRIBE); event(21, id, 0, 0, 0, 0); item_window.ops = &boundary_ops; return window_available ? &item_window : NULL; }
struct NaviManager g_navi;
struct Action { char text[256]; size_t size; int kind, filter, flags, silent, x, y, id; };
static struct Action action;
static void action_text(const char *text, size_t size)
{ assert(size < sizeof action.text); memcpy(action.text, text, size); action.text[size] = 0; action.size = size; }
int navi_show_map(struct NaviManager *manager, const char *map, int x, int y)
{ assert(manager == &g_navi); action.kind = 1; action.x = x; action.y = y; action_text(map, strlen(map)); return 1; }
int navi_navigate(struct NaviManager *manager, const char *map, int kind, int flags, BYTE silent, int x, int y, int id)
{ assert(manager == &g_navi); action.kind = 2; action.filter = kind; action.flags = flags; action.silent = silent; action.x = x; action.y = y; action.id = id; action_text(map, strlen(map)); return 1; }
int navi_open_search(struct NaviManager *manager, const char *text, size_t size, int kind, BYTE silent)
{ assert(manager == &g_navi); action.kind = 3; action.filter = kind; action.silent = silent; action_text(text, size); return 1; }
void tag_item_link_decode(const struct String *code, struct Vec *values)
{ assert(!values->first && !values->last && !values->end); event(22, (int)code->size, 0, 0, 0, 0); }
BYTE tag_item_link_fill_item(const struct Vec *values, struct ItemInfo *item)
{ assert(!values->first); item->type = 501; item->identified = 1; event(23, 0, 0, 0, 0, 0); return 1; }

static struct Action activate_navigation(struct TextView text, int old)
{
    struct UINaviLinkButton *button;
    memset(&action, 0, sizeof action); reset_fixture();
#ifdef LINK_BUTTONS_REFERENCE
    if (old) {
        struct reference_UINaviLinkButton *saved = reference_UINaviLinkButton_Construct(mem_alloc(sizeof *saved));
        string_assign(&saved->link, text.data, text.size);
        reference_UINaviLinkButton_Activate(saved); reference_UINaviLinkButton_Destroy(saved, 1);
    } else
#else
    (void)old;
#endif
    {
        button = ui_navi_link_button_create(); assert(button);
        string_assign(&button->link, text.data, text.size);
        ui_navi_link_button_activate(button); ui_navi_link_button_destroy((struct UIWindow *)(button));
    }
    assert(!outstanding); return action;
}
static void navigation_cases(void)
{
    static const char *const inputs[] = {"", "x", "</NAVIL>", "abc</NAVIL>", "0000</NAVIL>",
        "<NAVIL>prontera0102</NAVIL>", "<NAVIL>한글0a0Z</NAVIL>", "prontera0102</NAVIL>",
        "prefix<NAVIL>map0102</NAVIL>", "<NAVIL>mapzzZZ</NAVIL>tail", "<NAVIL>map??!!</NAVIL>",
        "<NAVIL>map0102", "<navil>map0102</NAVIL>", "<NAVIL>0000</NAVIL>"};
    struct Action result;
    size_t i;
    for (i = 0; i < sizeof inputs / sizeof *inputs; ++i) {
        result = activate_navigation(text_view_cstr(inputs[i]), 0);
#ifdef LINK_BUTTONS_REFERENCE
        { struct Action saved = activate_navigation(text_view_cstr(inputs[i]), 1); assert(!memcmp(&result, &saved, sizeof result)); }
#endif
        if (i < 5 || i == 11 || i == 13) assert(!result.kind);
        if (i == 5) assert(result.kind == 1 && !strcmp(result.text, "prontera") && result.x == 1 && result.y == 2);
        if (i == 6) assert(!strcmp(result.text, "한글") && result.x == 10 && result.y == 61);
    }
    {
        static const char embedded[] = "<NAVIL>map\0suffix0102</NAVIL>";
        result = activate_navigation(text_view(embedded, sizeof embedded - 1), 0);
        assert(result.kind == 1 && !strcmp(result.text, "map"));
#ifdef LINK_BUTTONS_REFERENCE
        { struct Action saved = activate_navigation(text_view(embedded, sizeof embedded - 1), 1); assert(!memcmp(&result, &saved, sizeof result)); }
#endif
    }
    fail_at = allocation_calls + 1; assert(!ui_navi_link_button_create()); fail_at = 0;
    assert(!outstanding);
    puts("Link buttons: 15 navigation activation cases, malformed markers, counted bytes and failed control allocation passed.");
}
static void argument_cases(void)
{
    static const char *const inputs[] = {"", ",", "a,", ",a", "a,,b", "한글,501", "one", "a,b,c,d,e,f,g,h,i"};
    size_t i, kind;
    for (i = 0; i < sizeof inputs / sizeof *inputs; ++i) for (kind = 0; kind < 2; ++kind) {
        struct UITagButton *button = ui_tag_button_create();
        struct String input; size_t expected = *inputs[i] ? 1 : 0, j;
        string_init(&input);
        string_assign_cstr(&input, inputs[i]);
        for (j = 0; j < input.size; ++j) if (inputs[i][j] == ',') ++expected;
        ui_tag_list_box_set_button_args(button, &input, kind ? TAG_KIND_NAVI : TAG_KIND_ITEM);
        assert(ui_text_button_line_count(&button->button) == expected);
        if (input.size && !kind) {
            struct TextView name = ui_text_button_get_line(&button->button, 0);
            assert(button->item.identified && button->item.refine == 0 && button->item.type == atoi(ui_text_button_get_line(&button->button, 1).data));
            assert(button->item.id_text.size == name.size && !memcmp(string_cstr(&button->item.id_text), name.data, name.size));
        }
#ifdef LINK_BUTTONS_REFERENCE
        {
            struct reference_UITagButton saved;
            reference_UITagButton_Construct(&saved); reference_UITagListBox_SetButtonArgs(&saved, &input, kind ? TAG_KIND_NAVI : TAG_KIND_ITEM);
            assert(reference_UITagButton_ArgCount(&saved) == expected && !ui_text_button_line_count(&saved.button));
            for (j = 0; j < expected; ++j) {
                struct String old;
                struct TextView current = ui_text_button_get_line(&button->button, j);
                reference_UITagButton_GetArg(&saved, &old, j);
                assert(old.size == current.size && !memcmp(string_cstr(&old), current.data, current.size)); string_free(&old);
            }
            reference_UITagButton_Destroy(&saved, 0);
        }
#endif
        string_free(&input); ui_tag_button_destroy((struct UIWindow *)(button)); assert(!outstanding);
    }
    {
        struct UITagButton *button = ui_tag_button_create(); struct String encoded;
        string_init(&encoded);
        string_assign_cstr(&encoded, "encoded item");
        ui_tag_list_box_set_button_args(button, &encoded, TAG_KIND_ITEML);
        assert(button->item.type == 501 && !ui_text_button_line_count(&button->button));
        string_free(&encoded);
        ui_tag_button_split_args(button, text_view("", 0)); assert(ui_text_button_line_count(&button->button) == 1);
        ui_text_button_add_line(&button->button, text_view("map\0suffix", 10));
        for (i = 0; i < 32; ++i) ui_text_button_add_line(&button->button, ui_text_button_get_line(&button->button, 1));
        for (i = 1; i < 34; ++i) assert(ui_text_button_get_line(&button->button, i).size == 10);
        ui_tag_button_destroy((struct UIWindow *)(button)); assert(!outstanding);
    }
    puts("Link buttons: shared argument ownership, empty/trailing fields, Korean names, ITEML delegation, aliases and cleanup passed.");
}
static struct Action run_action(int old, int count, const char *flags)
{
    const char *args[] = {"한글_map", "-12", "34", "7", flags, "0", "42"};
    int i;
    reset_fixture(); memset(&action, 0, sizeof action);
#ifdef LINK_BUTTONS_REFERENCE
    if (old) {
        struct reference_UITagButton saved; reference_UITagButton_Construct(&saved);
        for (i = 0; i < count; ++i) { struct String s; string_init(&s); string_assign_cstr(&s, args[i]); reference_UITagButton_PushArg(&saved, &s); string_free(&s); }
        reference_UITagListBox_OnLinkClick(NULL, TAG_KIND_NAVI, &saved); reference_UITagButton_Destroy(&saved, 0);
    } else
#else
    (void)old;
#endif
    {
        struct UITagButton *button = ui_tag_button_create();
        for (i = 0; i < count; ++i) ui_text_button_add_line(&button->button, text_view_cstr(args[i]));
        ui_tag_list_box_on_link_click(NULL, TAG_KIND_NAVI, button); ui_tag_button_destroy((struct UIWindow *)(button));
    }
    assert(!outstanding); return action;
}
static void action_cases(void)
{
    static const char *const flags[] = {"-222", "0", "1", "10", "100", "111", "999", "-1"};
    int count; size_t variant; struct Action result;
    for (count = 0; count <= 7; ++count) for (variant = 0; variant < sizeof flags / sizeof *flags; ++variant) {
        result = run_action(0, count, flags[variant]);
#ifdef LINK_BUTTONS_REFERENCE
        { struct Action old = run_action(1, count, flags[variant]); assert(!memcmp(&result, &old, sizeof result)); }
#endif
        if (!count) assert(!result.kind);
        else {
            assert(result.kind == (count > 4 && !variant ? 3 : 2));
            if (count <= 3) assert(result.filter == 2 && result.flags == 5 && result.id == 1002);
        }
    }
    puts("Link buttons: 64 navigation flag/default/search action cases passed.");
}
static void mouse_cases(void)
{
    unsigned variant;
    for (variant = 0; variant < 64; ++variant) {
        struct UITagButton *button;
        reset_fixture(); button = ui_tag_button_create();
        button->button.button.base.parent = &parent; button->button.button.base.id = 61;
        button->button.button.base.width = 30; button->button.button.base.height = 20;
        button->button.value = 51; button->item.type = 501;
        button->button.button.disabled = variant & 1;
        shift_pressed = !!(variant & 2); focused = variant & 4 ? &parent : NULL;
        button->item.identified = !!(variant & 8);
        captured = variant & 16 ? &button->button.button.base : NULL;
        ui_tag_button_on_l_btn_up((struct UIWindow *)(button), variant & 32 ? 40 : 1, 1);
        if (!(variant & 1) && shift_pressed && focused && (variant & 8)) assert(event_count == 1 && events[0].a == 0x94 && item_type == 501);
        else if (!(variant & 1) && !shift_pressed && (variant & 16)) assert(captured == NULL);
        else assert(!event_count);
        ui_tag_button_destroy((struct UIWindow *)(button)); assert(!outstanding);
    }
    focused = NULL; shift_pressed = 0;
    puts("Link buttons: 64 disabled, Shift, focus, item, capture and release combinations passed.");
}
static void item_action_cases(void)
{
    int available, kind;
    for (available = 0; available < 2; ++available) for (kind = 0; kind < 2; ++kind) {
        struct UITagButton *button;
        reset_fixture(); button = ui_tag_button_create(); window_available = available;
        button->item.type = 501; string_assign_cstr(&button->item.id_text, "한글 item");
        item_type = 0; item_name[0] = 0;
        ui_tag_list_box_on_link_click(NULL, kind ? TAG_KIND_ITEML : TAG_KIND_ITEM, button);
        assert(event_count == (unsigned)(available ? 2 : 1));
        if (available) assert(item_type == 501 && !strcmp(item_name, "한글 item"));
        ui_tag_button_destroy((struct UIWindow *)(button)); assert(!outstanding);
    }
    window_available = 1;
    puts("Link buttons: ITEM/ITEML descriptions and unavailable windows passed.");
}
int main(void)
{
    boundary_ops.invalidate = ui_window_invalidate_base; boundary_ops.send_msg = ui_window_send_msg_base;
    navigation_cases(); argument_cases(); action_cases(); mouse_cases(); item_action_cases();
    return 0;
}
