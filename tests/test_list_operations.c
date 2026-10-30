#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui/controls/listbox.h"
#include "ui/controls/combo_box.h"
#include "core/object_type_tables.h"
#include "ui/ui_std.h"
#include "std/tree.h"
#include "core/fatal.h"

static unsigned allocations, refreshes, invalidations;
static int combo_case;
static char events[8];
static unsigned event_count;
void *mem_alloc(size_t size) { void *p = malloc(size); assert(p); ++allocations; return p; }
void mem_free(void *p) { if (p) { assert(allocations); --allocations; free(p); } }
__declspec(noreturn) void fatal_error(const char *message) { (void)message; abort(); }
int ui_window_text_width(struct UIWindow *window, const char *text, int len, int face, int height, int bold, int italic)
{
    assert(window && !len && !face && height == 12 && !bold && !italic);
    return (int)strlen(text) * 7;
}
void ui_window_resize_base(struct UIWindow *window, int width, int height)
{
    window->width = width; window->height = height; events[event_count++] = 'R';
}
void ui_window_move_base(struct UIWindow *window, int x, int y)
{ window->x = x; window->y = y; events[event_count++] = 'M'; }
static void create(struct UIWindow *window, int width, int height)
{ assert(window->width == width && window->height == height); events[event_count++] = 'C'; }
static void refresh(struct UIListBoxEx *list) { assert(list); ++refreshes; if (combo_case) events[event_count++] = 'S'; }
void ui_window_invalidate_base(struct UIWindow *window) { assert(window); ++invalidations; if (combo_case) events[event_count++] = 'I'; }

static const struct UIListBoxExOps colour_ops = {
    .base = { .move = ui_window_move_base, .on_create = create, .resize = ui_window_resize_base, .invalidate = ui_window_invalidate_base },
    .clear_items = ui_list_box_ex_clear_items, .update_scroll_bar = refresh,
};
static const struct UIListBoxExOps id_ops = {
    .base = { .move = ui_window_move_base, .on_create = create, .resize = ui_window_resize_base, .invalidate = ui_window_invalidate_base },
    .clear_items = ui_list_box_id_clear_items, .update_scroll_bar = refresh,
};

static void test_payloads(void)
{
    static const int ids[] = {0, -1, INT_MIN, INT_MAX, 42};
    static const COLORREF colours[] = {0, 0xffffffffUL, 0x80000000UL, 0x7fffffffUL, 0x123456UL};
    struct UIListBoxEx colours_list = {0};
    struct UIListBoxID ids_list = {0};
    unsigned i;
    colours_list.base.ops = &colour_ops.base;
    tree_init(&colours_list.rows);
    ids_list.list.base.ops = &id_ops.base;
    tree_init(&ids_list.list.rows);
    for (i = 0; i < sizeof ids / sizeof ids[0]; ++i) {
        int key = (int)i;
        struct UIListBoxExRow *colour;
        struct UIListBoxIDRow *id;
        ui_list_box_ex_add_item(&colours_list, "한글 and a long retained string", colours[i]);
        ui_list_box_id_add_item(&ids_list, "한글 and a long retained string", ids[i]);
        colour = list_box_ex_entry_at(&colours_list.rows, &key);
        id = ui_list_box_id_row_at(&ids_list.list.rows, &key);
        assert(colour->colour == colours[i] && id->id == ids[i]);
        assert(!strcmp(string_cstr(&colour->text), "한글 and a long retained string"));
        assert(!strcmp(string_cstr(&id->text), string_cstr(&colour->text)));
        assert(colours_list.rows.size == i + 1 && ids_list.list.rows.size == i + 1);
        assert(refreshes == 2 * (i + 1));
    }
    assert(colours_list.widest == ids_list.list.widest && colours_list.widest > 0);
    colours_list.top = colours_list.scroll_x = colours_list.extra_lines = 9;
    ids_list.list.top = ids_list.list.scroll_x = ids_list.list.extra_lines = 9;
    ui_list_box_ex_clear(&colours_list);
    ui_list_box_ex_clear(&ids_list.list);
    assert(!colours_list.rows.size && !ids_list.list.rows.size);
    assert(colours_list.selected == -1 && ids_list.list.selected == -1);
    assert(!colours_list.top && !colours_list.scroll_x && !colours_list.extra_lines && !colours_list.widest);
    assert(!ids_list.list.top && !ids_list.list.scroll_x && !ids_list.list.extra_lines && !ids_list.list.widest);
    assert(refreshes == 10);
    tree_free(&colours_list.rows, NULL); tree_free(&ids_list.list.rows, NULL);
    assert(!allocations);
}

static void test_creation_and_scroll(void)
{
    struct UIListBoxEx list = {0};
    struct UIScrollBar bar = {0};
    static const struct UIWindowOps bar_ops = { .invalidate = ui_window_invalidate_base };
    unsigned flag;
    list.base.ops = &colour_ops.base;
    for (flag = 0; flag <= 255; ++flag) {
        event_count = 0; memset(events, 0, sizeof events);
        ui_list_box_ex_create(&list, -7, 9, 101, 37, (BYTE)flag);
        assert(!strcmp(events, "RMC"));
        assert(list.transparent == (int)flag && list.base.x == -7 && list.base.y == 9);
    }
    bar.base.ops = &bar_ops; list.vbar = &bar; list.line_height = 10;
    list.base.height = 30; list.rows.size = 8; list.extra_lines = 2; bar.pos = 99;
    ui_list_box_ex_update_scroll_bar(&list);
    assert(list.has_vscroll && bar.max_pos == 7 && bar.page == 3 && bar.total == 8 && bar.pos == 7);
    list.rows.size = 1; ui_list_box_ex_update_scroll_bar(&list);
    assert(!list.has_vscroll && bar.max_pos == 0 && bar.pos == 0 && bar.total == 1);
    assert(invalidations == 4);
}
static void refresh_plain(struct UIListBox *list)
{ assert(list); events[event_count++] = 'S'; }
static const struct UIListBoxOps plain_ops = {
    .base = { .resize = ui_window_resize_base, .invalidate = ui_window_invalidate_base },
    .update_scroll_bar = refresh_plain,
};
struct UIComboBoxID *ui_combo_box_id_construct(struct UIComboBoxID *list)
{ memset(list, 0, sizeof *list); list->list.list.base.ops = &id_ops.base; return list; }
void ui_window_add_child(struct UIWindow *parent, struct UIWindow *child)
{ assert(parent && child); child->parent = parent; events[event_count++] = 'B'; }
static void test_combo_windows(void)
{
    struct UIComboBoxWnd owner = {0};
    struct UIListBox plain = {0};
    struct UIListBoxID ids = {0};
    combo_case = 1;
    plain.base.ops = &plain_ops.base;
    ids.list.base.ops = &id_ops.base;
    owner.list = &plain.base;
    event_count = 0; memset(events, 0, sizeof events);
    ui_combo_box_wnd_ops.resize(&owner.frame.base, 123, 45);
    assert(!strcmp(events, "RRSI") && plain.base.width == 123 && plain.base.height == 45);
    owner.list = &ids.list.base;
    event_count = 0; memset(events, 0, sizeof events);
    ui_combo_box_id_wnd_ops.resize(&owner.frame.base, 234, 56);
    assert(!strcmp(events, "RRSI") && ids.list.base.width == 234 && ids.list.base.height == 56);
    event_count = 0; memset(events, 0, sizeof events);
    ui_combo_box_id_wnd_ops.on_create(&owner.frame.base, 80, 90);
    assert(!strcmp(events, "RMCB"));
    assert(owner.list->ops == &id_ops.base && owner.list->parent == &owner.frame.base);
    assert(((struct UIListBoxEx *)owner.list)->transparent == 1);
    mem_free(owner.list); assert(!allocations); combo_case = 0;
}

int main(void)
{
    test_payloads(); test_creation_and_scroll(); test_combo_windows();
    puts("List operations: signed IDs, full colour bits, Korean text ownership, clearing, all creation flags, scroll bounds and plain/ID dropdown contracts passed.");
    return 0;
}

#include "list_operations_unreached.h"
