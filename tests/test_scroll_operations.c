#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ui/controls/scroll.h"
#include "ui/ui_std.h"

#include "../src/ui/controls/scroll_bar_skins.c"
#include "../src/ui/controls/scroll.c"
#include "../src/ui/controls/scroll_wnd.c"

static char events[32];
static unsigned event_count;

static void event(char kind)
{
    assert(event_count + 1 < sizeof events);
    events[event_count++] = kind;
    events[event_count] = 0;
}

void ui_window_resize_base(struct UIWindow *window, int width, int height)
{
    window->width = width;
    window->height = height;
    event('R');
}

void ui_window_invalidate_base(struct UIWindow *window)
{
    (void)window;
    event('I');
}

void ui_window_set_show_base(struct UIWindow *window, int show)
{
    window->visible = (BYTE)show;
    event('S');
}

static const struct UIWindowOps normal_ops = { .resize = ui_scroll_bar_resize, .invalidate = ui_window_invalidate_base, .set_show = ui_window_set_show_base };
static void test_thumb_geometry(void)
{
    struct UIScrollBar bar = {0};
    int axis;

    bar.base.ops = &ui_scroll_bar_ops;
    bar.button_size = 10;
    bar.max_pos = 10;
    bar.pos = 5;
    for (axis = UI_SCROLL_HORZ; axis <= UI_SCROLL_VERT; axis++) {
        bar.orientation = axis;
        bar.base.width = axis == UI_SCROLL_HORZ ? 100 : 13;
        bar.base.height = axis == UI_SCROLL_VERT ? 100 : 13;
        bar.total = 100;
        bar.page = 25;
        assert(ui_scroll_bar_get_thumb_size(&bar) == 20);
        assert(ui_scroll_bar_thumb_travel(&bar) == 60);
        assert(ui_scroll_bar_thumb_pos(&bar) == 30);
        assert(ui_scroll_bar_hit_part(&bar, 0, 0) == UI_SCROLL_ARROW_BACK);
        assert(ui_scroll_bar_hit_part(&bar, axis ? 0 : 39, axis ? 39 : 0) == UI_SCROLL_TRACK_BACK);
        assert(ui_scroll_bar_hit_part(&bar, axis ? 0 : 40, axis ? 40 : 0) == UI_SCROLL_THUMB);
        assert(ui_scroll_bar_hit_part(&bar, axis ? 0 : 59, axis ? 59 : 0) == UI_SCROLL_THUMB);
        assert(ui_scroll_bar_hit_part(&bar, axis ? 0 : 60, axis ? 60 : 0) == UI_SCROLL_TRACK_FWD);
        assert(ui_scroll_bar_hit_part(&bar, axis ? 0 : 90, axis ? 90 : 0) == UI_SCROLL_ARROW_FWD);
        assert(ui_scroll_bar_hit_part(&bar, -1, 0) == UI_SCROLL_OUTSIDE);
        bar.page = 1;
        bar.min_thumb = 1;
        assert(ui_scroll_bar_get_thumb_size(&bar) == UI_SCROLL_MIN_THUMB);
        bar.min_thumb = 0;
        assert(ui_scroll_bar_get_thumb_size(&bar) == 0);
        bar.total = 0;
        assert(ui_scroll_bar_get_thumb_size(&bar) == 80);
    }
    bar.max_pos = 0;
    assert(ui_scroll_bar_hit_part(&bar, 0, 0) == UI_SCROLL_OUTSIDE);
}

struct ChildNode {
    struct TreeNode node;
    struct ChildPosition position;
};

static void test_ranges(void)
{
    static const struct {
        int width, height, visible, h_on, v_on, view_width, view_height, content_width, content_height;
        const char *events;
    } cases[] = {
        {50, 50, 1, 0, 0, 100, 100, 100, 100, "SS"},
        {120, 50, 1, 1, 0, 100, 87, 120, 87, "IRRIISS"},
        {50, 120, 1, 0, 1, 87, 100, 87, 120, "IRIISS"},
        {120, 120, 1, 1, 1, 87, 87, 120, 120, "IRRIIIRIISS"},
        {95, 120, 1, 1, 1, 87, 87, 95, 120, "IRRIIIRIISS"},
        {120, 95, 1, 1, 1, 87, 87, 120, 95, "IRRIIIRIISS"},
        {200, 200, 0, 0, 0, 100, 100, 100, 100, "SS"},
    };
    unsigned i;
    for (i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        struct UIScrollWnd scroll = {0};
        struct UIScrollBar horizontal = {0}, vertical = {0};
        struct UIWindow child = {0};
        struct TreeNode head = {0};
        struct ChildNode node = {0};

        head.is_head = 1;
        head.left = head.right = head.parent = &node.node;
        node.node.left = node.node.right = node.node.parent = &head;
        node.position.child = &child;
        child.width = cases[i].width;
        child.height = cases[i].height;
        child.visible = (BYTE)cases[i].visible;
        scroll.child_positions.head = &head;
        scroll.child_positions.size = 1;
        scroll.base.width = scroll.base.height = 100;
        scroll.hbar_height = scroll.vbar_width = 13;
        scroll.hbar = &horizontal;
        scroll.vbar = &vertical;
        horizontal.base.ops = vertical.base.ops = &normal_ops;
        horizontal.orientation = UI_SCROLL_HORZ;
        vertical.orientation = UI_SCROLL_VERT;
        horizontal.thickness = vertical.thickness = 13;
        scroll.hpos = 500;
        scroll.vpos = -10;
        horizontal.pos = 500;
        vertical.pos = -10;
        event_count = 0;
        events[0] = 0;
        ui_scroll_wnd_update_bars(&scroll);
        assert(scroll.hscroll_on == cases[i].h_on && scroll.vscroll_on == cases[i].v_on);
        assert(scroll.view_width == cases[i].view_width && scroll.view_height == cases[i].view_height);
        assert(scroll.content_width == cases[i].content_width && scroll.content_height == cases[i].content_height);
        assert(horizontal.base.visible == cases[i].h_on && vertical.base.visible == cases[i].v_on);
        assert(!strcmp(events, cases[i].events));
        assert(scroll.hpos == (cases[i].h_on ? cases[i].content_width - cases[i].view_width : 0));
        assert(scroll.vpos == 0);
        if (cases[i].h_on) {
            assert(horizontal.max_pos == scroll.content_width - scroll.view_width);
            assert(horizontal.page == scroll.view_width && horizontal.total == scroll.content_width);
            assert(horizontal.pos == scroll.hpos);
        }
        if (cases[i].v_on) {
            assert(vertical.max_pos == scroll.content_height - scroll.view_height);
            assert(vertical.page == scroll.view_height && vertical.total == scroll.content_height);
            assert(vertical.pos == 0);
        }
        scroll.hbar = NULL;
        event_count = 0;
        ui_scroll_wnd_update_bars(&scroll);
        assert(event_count == 0);
    }
}

int main(void)
{
    test_thumb_geometry();
    test_ranges();
    puts("Scroll operations: both thumb implementations, hit boundaries, coupled ranges, hidden children, clamping and event order passed.");
    return 0;
}

#include "scroll_operations_unreached.h"
