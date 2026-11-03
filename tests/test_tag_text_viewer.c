#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <shellapi.h>
#include "ui/quests/tag_text_viewer.h"
#include "ui/controls/buttons.h"
#include "ui/controls/scroll.h"
#include "ui/window.h"
#include "ui/window_mgr.h"
#include "ui/draw.h"
#include "navi/navi_manager.h"
#include "game/context.h"
#include "iteminfo/item_api.h"
#include "core/fatal.h"
#include "std/std.h"
#include "owned_storage_test_allocator.h"
#ifdef TAG_TEXT_VIEWER_REFERENCE
#include "ui/quests/quests.h"
static void UIState_AppendRecord(struct Vec *out, int id, const void *data, unsigned len)
{
    struct Vec record;
    vec_init(&record);
    vec_resize(&record, 6 + len, 1);
    *(WORD *)record.first = (WORD)id;
    *(DWORD *)((BYTE *)record.first + 2) = len;
    if (len)
        memcpy((BYTE *)record.first + 6, data, len);
    std_vector_bytes_insert_range(out, out->last, record.first, record.last);
    vec_free(&record);
}
#endif

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
static HINSTANCE test_open_url(HWND window, const char *operation, const char *file,
                               const char *parameters, const char *directory, int show);
#define realloc moving_realloc
#define malloc checked_malloc
#define free checked_free
#define malloc checked_malloc
#include "../src/core/owned_text.c"
#include "../src/core/text_array.c"
#undef malloc
#define malloc checked_malloc
#define ShellExecuteA test_open_url
#include "../src/ui/controls/url_link.c"
#include "../src/ui/quests/tag_text_viewer.c"
#ifdef TAG_TEXT_VIEWER_REFERENCE
static struct String *reference_button_get_line(struct UITextButton *link, struct String *out, unsigned i);
#include "tag_text_viewer_reference.c"
#endif
#undef ShellExecuteA
#undef free
#undef malloc
#undef realloc

void *mem_alloc(size_t size) { return checked_malloc(size); }
void mem_free(void *p)
{
    checked_free(p);
}
__declspec(noreturn) void fatal_error(const char *message) { (void)message; abort(); }

enum { EV_MOVE, EV_ADD, EV_REMOVE, EV_SHOW, EV_RANGE, EV_POS, EV_INVALIDATE, EV_CLEAR, EV_LAYOUT, EV_ACTION };
struct Event { int kind, id, a, b, c; };
static struct Event events[8192];
static size_t event_count;
static void record(int kind, int id, int a, int b, int c)
{
    struct Event *event;
    assert(event_count < 8192);
    event = &events[event_count++];
    event->kind = kind; event->id = id; event->a = a; event->b = b; event->c = c;
}
void ui_text_button_add_line(struct UITextButton *button, struct TextView value)
{ assert(text_array_append(&button->lines, value)); }
struct TextView ui_text_button_get_line(const struct UITextButton *button, size_t i)
{ return text_array_at(&button->lines, i); }
size_t ui_text_button_line_count(const struct UITextButton *button) { return button->lines.size; }

static struct UIWindowOps child_ops;
static struct UIWindow pool[64], parent, action_window;
static struct UIWindow *children[128];
static size_t child_count;
static int shown, range_max, range_page, range_total, bar_pos;
static struct String layout_text, action_text;
static struct LineStyle layout_style;
static int layout_width, layout_spacing, layout_outline, layout_align;
static COLORREF layout_outline_colour;

struct UIWindow *ui_window_construct(struct UIWindow *window, int composites)
{
    assert(!composites); memset(window, 0, sizeof *window); window->ops = &child_ops; return window;
}
void ui_window_create(struct UIWindow *window, int width, int height)
{
    window->width = width; window->height = height;
    ui_window_on_create(UI_WINDOW(window), width, height);
}
void ui_window_resize_base(struct UIWindow *window, int width, int height)
{ window->width = width; window->height = height; }
void ui_window_move_base(struct UIWindow *window, int x, int y)
{ record(EV_MOVE, window->id, x, y, 0); window->x = x; window->y = y; }
void ui_window_invalidate_base(struct UIWindow *window)
{ record(EV_INVALIDATE, window->id, 0, 0, 0); }
void ui_window_set_show_base(struct UIWindow *window, int visible)
{ assert(window->id == -1); shown = visible; record(EV_SHOW, -1, visible, 0, 0); }
void ui_window_add_child(struct UIWindow *window, struct UIWindow *child)
{
    assert(child_count < 128); children[child_count++] = child; child->parent = window;
    record(EV_ADD, child->id, 0, 0, 0);
}
void ui_window_remove_child(struct UIWindow *window, struct UIWindow *child, BYTE destroy)
{
    size_t i;
    assert(child->parent == window && destroy);
    for (i = 0; i < child_count && children[i] != child; ++i) {}
    assert(i < child_count);
    memmove(children + i, children + i + 1, (--child_count - i) * sizeof *children);
    record(EV_REMOVE, child->id, 0, 0, 0);
    child->parent = NULL;
    if (child->id == -1) mem_free(child);
}
void ui_window_destruct(struct UIWindow *window)
{
    while (child_count) ui_window_remove_child(window, children[0], 1);
}
int ui_window_send_msg_base(struct UIWindow *window, struct UIWindow *sender, int message, int a, int b, int c, int d)
{
    (void)window; (void)sender; (void)b; (void)c; (void)d;
    if (window == &action_window) {
        const struct ItemInfo *item = (const struct ItemInfo *)a;
        assert(message == 0x18);
        string_assign(&action_text, string_cstr(&item->id_text), item->id_text.size);
        record(EV_ACTION, 0x18, item->type, item->identified, 0);
    }
    return 0;
}
void ui_window_clear_canvas(struct UIWindow *window, DWORD colour)
{ record(EV_CLEAR, window->id, (int)colour, 0, 0); }
struct UIScrollBar *ui_scroll_bar_construct(struct UIScrollBar *bar)
{ memset(bar, 0, sizeof *bar); ui_window_construct(&bar->base, 0); bar->base.id = -1; return bar; }
void ui_scroll_bar_create(struct UIScrollBar *bar, int x, int y, int orientation, int length)
{ assert(orientation == 1); bar->base.x = x; bar->base.y = y; bar->base.height = length; }
void ui_scroll_bar_set_range(struct UIScrollBar *bar, int maximum, int page, int total)
{ assert(bar->base.id == -1); range_max = maximum; range_page = page; range_total = total; record(EV_RANGE, -1, maximum, page, total); }
void ui_scroll_bar_set_pos(struct UIScrollBar *bar, int pos)
{ assert(bar->base.id == -1); bar_pos = pos; record(EV_POS, -1, pos, 0, 0); }

struct CreateLineInfo *create_line_info_construct(struct CreateLineInfo *out,
    struct UIWindow *window, const char *text, int width, int spacing,
    struct LineStyle style, BYTE outlined, COLORREF outline_colour, BYTE align)
{
    size_t i, count = *text ? strlen(text) % 23 + 1 : 0;
    (void)window;
    memset(out, 0, sizeof *out);
    string_assign_cstr(&layout_text, text);
    layout_style = style; layout_width = width; layout_spacing = spacing;
    layout_outline = outlined; layout_outline_colour = outline_colour; layout_align = align;
    out->windows = count ? checked_malloc(count * sizeof *out->windows) : NULL;
    out->count = count;
    record(EV_LAYOUT, 0, (int)count, width, spacing);
    for (i = 0; i < count; ++i) {
        struct UIWindow *child = &pool[(i * 17) % 64];
        ui_window_construct(child, 0);
        child->id = 100 + (int)i; child->x = (int)(i % 4) * 11; child->y = (int)(i / 4) * 13;
        child->width = 4 + (int)i * 2; child->height = 7 + (int)i % 5;
        out->windows[i] = child;
        if (child->y + child->height > out->height) out->height = child->y + child->height;
    }
    return out;
}
void create_line_info_destruct(struct CreateLineInfo *out)
{ checked_free(out->windows); memset(out, 0, sizeof *out); }

HWND g_hMainWnd;
struct NaviManager g_navi;
static HINSTANCE test_open_url(HWND window, const char *operation, const char *file,
                               const char *parameters, const char *directory, int show)
{
    assert(window == g_hMainWnd && !strcmp(operation, "open") && !parameters && !directory && show == SW_SHOWNORMAL);
    string_assign_cstr(&action_text, file); record(EV_ACTION, 0x1ad, 0, 0, 0);
    return (HINSTANCE)33;
}
struct UIWindow *ui_window_open(int id)
{ record(EV_ACTION, id, 0, 0, 0); return &action_window; }
struct GameMode *game_current_mode(void) { return (struct GameMode *)&parent; }
void game_mode_ui_open_v2(struct GameMode *mode, unsigned char kind, int value)
{ assert(mode == (void *)&parent && kind == 6); record(EV_ACTION, 0x213, value, 0, 0); }
int navi_open_search(struct NaviManager *manager, const char *text, size_t length, int kind, unsigned char zero)
{
    assert(manager == &g_navi && !zero); string_assign(&action_text, text, length);
    record(EV_ACTION, -0xde, kind, 0, 0);
    return 1;
}
int navi_navigate(struct NaviManager *manager, const char *map, int kind, int flags, unsigned char audible, int x, int y, int id)
{
    assert(manager == &g_navi); string_assign_cstr(&action_text, map);
    record(EV_ACTION, 0x1ae, kind, flags, audible); record(EV_ACTION, id, x, y, 0);
    return 1;
}
#include "tag_text_viewer_unreached.h"

#ifdef TAG_TEXT_VIEWER_REFERENCE
void ui_window_on_draw_base(struct UIWindow *window) { (void)window; abort(); }
static struct String *reference_button_get_line(struct UITextButton *link, struct String *out, unsigned i)
{
    struct TextView view = ui_text_button_get_line(link, i);
    string_init(out);
    string_assign(out, view.data, view.size); return out;
}

#endif

static void reset_fixture(void)
{
    assert(!outstanding && !child_count);
    event_count = 0; shown = range_max = range_page = range_total = bar_pos = 0;
    memset(pool, 0, sizeof pool); memset(&parent, 0, sizeof parent); parent.id = -2;
    parent.ops = &child_ops; action_window.ops = &child_ops;
}
static void release_fixture(void)
{
    string_free(&layout_text); string_free(&action_text);
    assert(!outstanding && !child_count);
}
static void expected_text(struct UITagTextViewer *viewer, const char *text)
{
    assert(viewer->has_text && !strcmp(string_cstr(&viewer->text), text));
    assert(!strcmp(string_cstr(&layout_text), text));
}
static void expect_sorted_removal(size_t begin, size_t count)
{
    uintptr_t previous = 0;
    size_t i, n = 0;
    for (i = begin; i < event_count; ++i) if (events[i].kind == EV_REMOVE && events[i].id >= 100) {
        uintptr_t address = (uintptr_t)&pool[((events[i].id - 100) * 17) % 64];
        assert(address > previous); previous = address; ++n;
    }
    assert(n == count);
}
static void expected_cases(void)
{
    struct UITagTextViewer *viewer;
    size_t begin, count;
    reset_fixture(); viewer = ui_tag_text_viewer_create(100, 20); viewer->base.parent = &parent;
    assert(!viewer->has_text && viewer->align_h == 0 && viewer->align_v == 3 && !shown);
    ui_tag_text_viewer_append_text(viewer, NULL); assert(!viewer->has_text);
    ui_tag_text_viewer_append_text(viewer, ""); expected_text(viewer, ""); assert(!viewer->item_count);
    ui_tag_text_viewer_append_text(viewer, "한글"); expected_text(viewer, "\n한글");
    assert(viewer->wrap_width == 84 && layout_width == 84 && layout_spacing == 6);
    assert(layout_style.font_height == 12 && !layout_style.bold && !layout_style.italic);
    assert(layout_align == 0 && !layout_outline && layout_outline_colour == 0xffffff);
    ui_tag_text_viewer_append_text(viewer, string_cstr(&viewer->text) + 1);
    expected_text(viewer, "\n한글\n한글");
    ui_tag_text_viewer_remove_all(viewer); assert(!viewer->has_text && !viewer->item_count && !viewer->scroll_pos && !shown);
    ui_tag_text_viewer_append_text(viewer, "abcdefghijklmnopqrstuv"); assert(viewer->item_count == 23 && shown);
    assert(range_page == 20 && range_total == ui_tag_text_viewer_content_height(viewer));
    assert(range_max == range_total - 20);
    assert(ui_tag_text_viewer_send_msg((struct UIWindow *)(viewer), NULL, UI_MSG_SCROLL_V, 999, 0, 0, 0) == range_max);
    assert(viewer->scroll_pos == range_max && bar_pos == range_max);
    assert(ui_tag_text_viewer_send_msg((struct UIWindow *)(viewer), NULL, UI_MSG_SCROLL_V, -999, 0, 0, 0) == -range_max);
    assert(!viewer->scroll_pos && !bar_pos);
    count = viewer->item_count; begin = event_count; ui_tag_text_viewer_remove_all(viewer); expect_sorted_removal(begin, count);
    ui_tag_text_viewer_set_align(viewer, 2); ui_tag_text_viewer_set_align(viewer, 5); ui_tag_text_viewer_set_align(viewer, 255);
    assert(viewer->align_h == 2 && viewer->align_v == 5);
    viewer->base.height = 100; ui_tag_text_viewer_append_text(viewer, "x");
    assert(pool[0].y == 100 - 8 - viewer->margin_y);
    viewer->bg_argb = 0xff00ff; ui_tag_text_viewer_on_draw((struct UIWindow *)(viewer));
    ui_tag_text_viewer_destroy((struct UIWindow *)(viewer)); release_fixture();
    puts("Tagged text viewer: empty/null/aliased lines, Korean text, typed layout settings, scroll clamping, alignment and sorted removal passed.");
}

static void link_init(struct UITextButton *link, const char *const *args, size_t count)
{
    size_t i;
    memset(link, 0, sizeof *link);
    for (i = 0; i < count; ++i) ui_text_button_add_line(link, text_view_cstr(args[i]));
}
static void link_cases(void)
{
    struct UITextButton link;
    static const char *const args[] = {"한글_map", "12", "34", "7", "111", "0", "42"};
    static const char *const search[] = {"prontera", "0", "0", "2", "-222"};
    static const char *const item[] = {"62", "4"};
    static const char *const url[] = {"https://example.com/"};
    reset_fixture(); link_init(&link, args, 7);
    ui_tag_text_viewer_on_navi_link(NULL, &link);
    assert(!strcmp(string_cstr(&action_text), "한글_map"));
    assert(events[0].a == 7 && events[0].b == 7 && events[0].c == 1);
    assert(events[1].id == 42 && events[1].a == 12 && events[1].b == 34);
    text_array_clear(&link.lines); link_init(&link, search, 5); event_count = 0;
    ui_tag_text_viewer_on_navi_link(NULL, &link); assert(!strcmp(string_cstr(&action_text), "$$DB prontera"));
    assert(events[0].id == -222 && events[0].a == 2);
    text_array_clear(&link.lines); link_init(&link, item, 2); event_count = 0;
    ui_tag_text_viewer_on_item_link(NULL, &link); assert(!strcmp(string_cstr(&action_text), "62"));
    assert(events[1].a == 4 && events[1].b == 1);
    event_count = 0;
    ui_tag_text_viewer_on_url_link(NULL, &link); assert(event_count == 0);
    text_array_clear(&link.lines); link_init(&link, url, 1);
    ui_tag_text_viewer_on_url_link(NULL, &link);
    assert(!strcmp(string_cstr(&action_text), url[0]) && event_count == 1);
    text_array_clear(&link.lines); link_init(&link, item, 2);
    ui_tag_text_viewer_on_quest_link(NULL, &link);
    text_array_clear(&link.lines); release_fixture();
    puts("Tagged text viewer: item, URL, navigation, search and quest actions passed.");
}

#ifdef TAG_TEXT_VIEWER_REFERENCE
struct Snapshot {
    struct Event events[8192];
    size_t event_count, save_size;
    unsigned char save[4096];
    char text[1024];
    int scroll, has_text, height, count;
    int width, spacing, align, outlined, font_height, bold, italic;
    COLORREF colour, outline_colour;
};
static void snapshot_save(struct Snapshot *snapshot, int old, void *viewer, int clear)
{
    struct Vec bytes;
    size_t size;
    if (old) {
        struct reference_UISayDialogWnd dialog = {0};
        dialog.text = viewer; dialog.clear_on_text = clear;
        reference_UISayDialogWnd_SaveState(&dialog, &bytes);
    } else {
        struct current_UISayDialogWnd dialog = {0};
        dialog.text = viewer; dialog.clear_on_text = clear;
        current_UISayDialogWnd_SaveState(&dialog, &bytes);
    }
    size = (char *)bytes.last - (char *)bytes.first;
    assert(snapshot->save_size + size < sizeof snapshot->save);
    memcpy(snapshot->save + snapshot->save_size, bytes.first, size);
    snapshot->save_size += size;
    vec_free(&bytes);
}

static void comparison_run(struct Snapshot *out, int old, int size, int align,
                           int margin, const char *input)
{
    struct UITagTextViewer *current = NULL;
    struct reference_UITagTextViewer *saved = NULL;
    struct UIWindow *base;
    const char *text;
    int i;
    static const int messages[] = {UI_MSG_SCROLL_V, UI_MSG_PAGE_UP, UI_MSG_PAGE_DOWN,
        UI_MSG_BAR_PAGE_DOWN, UI_MSG_BAR_PAGE_UP, UI_MSG_SCROLL_V};
    static const int values[] = {999, 0, 0, 0, 0, -999};
#define CALL(name, ...) (old ? reference_##name(saved, __VA_ARGS__) : name(current, __VA_ARGS__))
#define WINDOW_CALL(name, ...) (old ? reference_##name(saved, __VA_ARGS__) : name(base, __VA_ARGS__))
#define CALL0(name) (old ? reference_##name(saved) : name(current))
#define WINDOW_CALL0(name) (old ? reference_##name(saved) : name(base))
    memset(out, 0, sizeof *out); reset_fixture();
    if (old) {
        saved = reference_UITagTextViewer_Construct(mem_alloc(sizeof *saved));
        ui_window_create(&saved->base, 60 + size * 20, size * 31);
        saved->margin_x = margin; saved->margin_y = margin;
        saved->text_colour = 0x123456; saved->font_height = 18;
        saved->style_flags = 0x101; saved->outlined = 1; saved->outline_colour = 0x765432;
        base = &saved->base;
    } else {
        current = ui_tag_text_viewer_create(60 + size * 20, size * 31);
        current->margin_x = margin; current->margin_y = margin;
        current->style.colour = 0x123456; current->style.font_height = 18;
        current->style.bold = current->style.italic = 1;
        current->outlined = 1; current->outline_colour = 0x765432;
        base = &current->base;
    }
    base->parent = &parent;
    CALL(ui_tag_text_viewer_set_align, (BYTE)(align % 3));
    CALL(ui_tag_text_viewer_set_align, (BYTE)(3 + align / 3));
    snapshot_save(out, old, base, 0);
    WINDOW_CALL(ui_tag_text_viewer_send_msg, NULL, UI_MSG_TAG_ADD_LINE, 0, 0, 0, 0);
    snapshot_save(out, old, base, 1);
    WINDOW_CALL(ui_tag_text_viewer_send_msg, NULL, UI_MSG_TAG_ADD_LINE, (int)"", 0, 0, 0);
    snapshot_save(out, old, base, 0);
    WINDOW_CALL(ui_tag_text_viewer_send_msg, NULL, UI_MSG_TAG_ADD_LINE, (int)input, 0, 0, 0);
    text = old ? string_cstr((struct String *)saved->lines.first) : string_cstr(&current->text);
    WINDOW_CALL(ui_tag_text_viewer_send_msg, NULL, UI_MSG_TAG_ADD_LINE, (int)text, 0, 0, 0);
    snapshot_save(out, old, base, 123);
    for (i = 0; i < 6; ++i) {
        int delta = WINDOW_CALL(ui_tag_text_viewer_send_msg, NULL, messages[i], values[i], 0, 0, 0);
        record(EV_ACTION, messages[i], delta, 0, 0);
    }
    WINDOW_CALL(tag_text_viewer_ui_scroll_on_wheel, -1); WINDOW_CALL(tag_text_viewer_ui_scroll_on_wheel, 1);
    CALL0(ui_tag_text_viewer_layout);
    snapshot_save(out, old, base, -1);
    CALL0(ui_tag_text_viewer_remove_all);
    snapshot_save(out, old, base, 0);
    WINDOW_CALL(ui_tag_text_viewer_send_msg, NULL, UI_MSG_TAG_ADD_LINE, (int)input, 0, 0, 0);
    snapshot_save(out, old, base, 1);
    out->height = CALL0(ui_tag_text_viewer_content_height);
    if (old) {
        out->scroll = saved->scroll_pos; out->count = saved->item_count;
        out->has_text = saved->lines.first != saved->lines.last;
        text = string_cstr((struct String *)saved->lines.first);
    } else {
        struct current_UISayDialogWnd dialog = {0};
        out->scroll = current->scroll_pos; out->count = (int)current->item_count;
        out->has_text = current->has_text; text = string_cstr(&current->text);
        ui_say_dialog_wnd_set_text_align(NULL, 2); ui_say_dialog_wnd_set_text_align(&dialog, 2);
        dialog.text = current; ui_say_dialog_wnd_set_text_align(&dialog, 2);
        assert(current->align_h == 2);
    }
    assert(strlen(text) < sizeof out->text); strcpy(out->text, text);
    out->width = layout_width; out->spacing = layout_spacing; out->align = layout_align;
    out->outlined = layout_outline; out->outline_colour = layout_outline_colour;
    out->font_height = layout_style.font_height; out->colour = layout_style.colour;
    out->bold = layout_style.bold; out->italic = layout_style.italic;
    if (old) reference_UITagTextViewer_Destroy(saved, 1);
    else ui_tag_text_viewer_destroy((struct UIWindow *)(current));
    out->event_count = event_count; memcpy(out->events, events, event_count * sizeof *events);
    release_fixture();
#undef CALL
#undef WINDOW_CALL
#undef CALL0
#undef WINDOW_CALL0
}
static void reference_cases(void)
{
    static struct Snapshot expected, actual;
    static const char *const inputs[] = {"", "x", "한글", "one\ntwo\n", "abcdefghijklmnopqrstuv", "<B>안녕하세요</B>"};
    static const int margins[] = {-4, 0, 7};
    int size, align, margin, input, count = 0;
    for (size = 0; size < 4; ++size) for (align = 0; align < 9; ++align)
        for (margin = 0; margin < 3; ++margin) for (input = 0; input < 6; ++input) {
            comparison_run(&expected, 1, size, align, margins[margin], inputs[input]);
            comparison_run(&actual, 0, size, align, margins[margin], inputs[input]);
            assert(!memcmp(&expected, &actual, sizeof actual)); ++count;
        }
    printf("Tagged text viewer: %d archived layout/scroll/ownership cases and %d saved-state byte comparisons passed.\n", count, count * 7);
}
static void action_run(struct Snapshot *out, int old, int action, unsigned count, int variant)
{
    struct UITextButton link;
    const char *args[] = {"안녕하세요_prontera", "-12", "34", "7", "111", "0", "42"};
    static const char *const kinds[] = {"-222", "0", "1", "10", "100", "999"};
    memset(out, 0, sizeof *out); reset_fixture(); args[4] = kinds[variant];
    args[5] = variant % 2 ? "1" : "0";
    link_init(&link, args, count);
    if (count && variant == 5) {
        static const char embedded[] = "map\0suffix";
        assert(owned_text_assign(link.lines.items, text_view(embedded, sizeof embedded - 1)));
    }
#define ACTION(name) do { if (old) reference_##name(NULL, &link); else name(NULL, &link); } while (0)
    if (old >= 2) {
        static const int ids[] = {0x1ae, 0x1c8, 0x1ad, 0x213};
        if (old == 3) saved_link_on_link(&link, ids[action]);
        else current_link_on_link(&link, ids[action]);
    } else switch (action) {
    case 0: ACTION(ui_tag_text_viewer_on_navi_link); break;
    case 1: ACTION(ui_tag_text_viewer_on_item_link); break;
    case 2: ACTION(ui_tag_text_viewer_on_url_link); break;
    case 3: ACTION(ui_tag_text_viewer_on_quest_link); break;
    }
#undef ACTION
    assert(action_text.size < sizeof out->text);
    out->save_size = action_text.size;
    memcpy(out->text, string_cstr(&action_text), action_text.size);
    out->event_count = event_count; memcpy(out->events, events, event_count * sizeof *events);
    text_array_clear(&link.lines); release_fixture();
}
static void reference_actions(void)
{
    static struct Snapshot expected, actual;
    int family, action, count, variant, cases = 0;
    for (family = 0; family < 2; ++family)
    for (action = 0; action < 4; ++action) for (count = 0; count <= 7; ++count)
        for (variant = 0; variant < 6; ++variant) {
            action_run(&expected, family * 2 + 1, action, count, variant);
            action_run(&actual, family * 2, action, count, variant);
            assert(!memcmp(&expected, &actual, sizeof actual)); ++cases;
        }
    printf("Tagged text viewer: %d archived link-action cases passed.\n", cases);
}
static void split_arguments(void)
{
    static const char *const inputs[] = {"", ",", "a,", ",a", "a,,b", "한글,12,34", "one", "a,b,c,d,e,f,g,h,i"};
    size_t i, j;
    reset_fixture();
    for (i = 0; i < sizeof inputs / sizeof *inputs; ++i) {
        struct UITextButton old = {0}, current = {0};
        struct String input;
        string_init(&input); string_assign_cstr(&input, inputs[i]);
        saved_link_link_args_split(&old, &input); current_link_link_args_split(&current, &input);
        assert(ui_text_button_line_count(&old) == ui_text_button_line_count(&current));
        for (j = 0; j < ui_text_button_line_count(&old); ++j) {
            struct TextView a = ui_text_button_get_line(&old, j), b = ui_text_button_get_line(&current, j);
            assert(a.size == b.size && !memcmp(a.data, b.data, a.size));
        }
        string_free(&input); text_array_clear(&old.lines); text_array_clear(&current.lines);
    }
    release_fixture();
    puts("Text viewer: archived comma splitting preserves empty, repeated, trailing and Korean arguments.");
}
#endif

int main(void)
{
    child_ops.move = ui_window_move_base; child_ops.resize = ui_window_resize_base;
    child_ops.set_show = ui_window_set_show_base; child_ops.invalidate = ui_window_invalidate_base;
    child_ops.send_msg = ui_window_send_msg_base;
    expected_cases(); link_cases();
#ifdef TAG_TEXT_VIEWER_REFERENCE
    reference_cases();
    reference_actions();
    split_arguments();
#endif
    return 0;
}
