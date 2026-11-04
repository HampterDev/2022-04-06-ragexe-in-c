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
#include "../src/navi/navi_ui_hooks.c"
#undef free
#undef realloc
#undef malloc
#ifdef LINK_BUTTONS_REFERENCE
#include "link_buttons_reference.c"
#endif

void *mem_alloc(size_t size) { return checked_malloc(size); }
void mem_free(void *p)
{
    checked_free(p);
}
__declspec(noreturn) void fatal_error(const char *text) { (void)text; abort(); }

struct UIWindowMgr g_windowMgr;
static struct UIWindow parent, item_window;
static struct UIWindow *focused;
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
intptr_t ui_window_send_msg_base(struct UIWindow *window, struct UIWindow *sender, int message, intptr_t a, intptr_t b, intptr_t c, intptr_t d)
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

struct UIWindow *ui_window_mgr_get_focus_edit(struct UIWindowMgr *manager)
{ assert(manager == &g_windowMgr); return focused; }
struct NaviManager g_navi;
struct UIHooks g_uiHooks;
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
int main(void)
{
    g_uiHooks.navi_navigate = navi_link_navigate;
    g_uiHooks.navi_search = navi_link_search;
    g_uiHooks.navi_show_map = navi_link_show_map;
    boundary_ops.invalidate = ui_window_invalidate_base; boundary_ops.send_msg = ui_window_send_msg_base;
    navigation_cases();
    return 0;
}
