#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game/drag_info.h"
#include "ui/window.h"
#include "ui/wndmgr/wndmgr.h"
#include "world/mode_dispatch.h"
#include "world/mode_manager.h"
#include "session/session_items.h"
#include "session/state.h"

typedef void (*DropCase)(int, int, const struct DragInfo *, int);
void test_grade_drop(int, int, const struct DragInfo *, int);
void test_refine_drop(int, int, const struct DragInfo *, int);
void test_bitmap_tab_drop(int, int, const struct DragInfo *, int);
void test_tab_drop(int, int, const struct DragInfo *, int);
void test_cart_drop(int, int, const struct DragInfo *, int);
void test_goods_drop(int, int, const struct DragInfo *, int);
void test_button_drop(int, int, const struct DragInfo *, int);
void test_window_drop(int, int, const struct DragInfo *, int);
void test_item_base_drop(int, int, const struct DragInfo *, int);
void test_result_drop(int, int, const struct DragInfo *, int);

struct UIWindowMgr g_windowMgr;
static struct UIWindow *active, parent_window;
static struct Mode *current_mode = (struct Mode *)&parent_window;
static const struct DragInfo *expected_drag;
static int expected_x, expected_y, messages, completions, forwards;

intptr_t game_send_message(int message, intptr_t a, intptr_t b, intptr_t c, intptr_t d)
{
    assert(message == 0x12 && !a && !b && !c && !d);
    ++completions;
    return 0;
}
struct Mode *client_current_mode(void) { return current_mode; }
intptr_t mode_send_message(struct Mode *mode, int message, intptr_t a, intptr_t b, intptr_t c, intptr_t d)
{
    assert(mode == current_mode);
    return game_send_message(message, a, b, c, d);
}
static intptr_t on_message(struct UIWindow *window, struct UIWindow *sender, int message,
                      intptr_t a, intptr_t b, intptr_t c, intptr_t d)
{
    assert(window == active && sender == active);
    assert(message == 0x17 && a == expected_drag->index && !b && !c && !d);
    assert(!completions && !forwards);
    ++messages;
    return 0;
}
struct ItemInfo *session_get_inventory_item(struct Session *session, struct ItemInfo *out, int index)
{
    assert(session == &g_session1 && active);
    assert(index == expected_drag->index);
    assert(!completions && !forwards);
    ++messages;
    memset(out, 0, sizeof *out);
    return out;
}
int session_is_in_switch_sets(struct Session *session, int index)
{
    assert(session == &g_session1 && !index);
    return 0;
}
void string_free(struct String *text) { assert(text && !text->data); }
intptr_t ui_frame_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int message, intptr_t a, intptr_t b, intptr_t c, intptr_t d)
{
    assert(window == active && sender == active && message == 0x17 && a == expected_drag->index && !b && !c && !d);
    return 0;
}
intptr_t ui_refining_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int message, intptr_t a, intptr_t b, intptr_t c, intptr_t d)
{
    return on_message(window, sender, message, a, b, c, d);
}
static void on_parent_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    assert(window == &parent_window && drag == expected_drag);
    assert(x == expected_x && y == expected_y);
    assert(!messages && !completions);
    ++forwards;
}

void test_drop_invoke(struct UIWindow *window, size_t size,
                     void (*callback)(struct UIWindow *, int, int, const struct DragInfo *),
                     int x, int y, const struct DragInfo *drag, int parent)
{
    struct UIWindowOps operations = {.drag_and_drop = callback, .send_msg = on_message};
    struct UIWindowOps parent_operations = {.drag_and_drop = on_parent_drop};
    unsigned char *saved = malloc(size);
    struct DragInfo original = *drag;
    assert(saved);
    parent_window.ops = &parent_operations;
    window->ops = &operations;
    window->parent = parent ? &parent_window : NULL;
    window->x = 17; window->y = -23;
    parent_window.x = 200; parent_window.y = 300;
    memcpy(saved, window, size);
    active = window; expected_drag = drag; expected_x = x; expected_y = y;
    messages = completions = forwards = 0;
    ui_window_drag_and_drop(UI_WINDOW(window), x, y, drag);
    assert(!memcmp(window, saved, size));
    assert(!memcmp(drag, &original, sizeof original));
    free(saved);
    active = NULL;
}

static void item_box(DropCase drop, int left, int top)
{
    static const int points[][2] = {{0,0},{46,46},{23,23},{-1,0},{47,0},{0,-1},{0,47}};
    struct DragInfo drag = {0};
    size_t i;
    int kind;
    drag.index = 0x12345678;
    drag.drop_count = 7; drag.count = 19; drag.shop_index = 42;
    for (kind = 0; kind < 3; ++kind) {
        drag.source = kind;
        for (i = 0; i < sizeof points / sizeof points[0]; ++i) {
            drop(left + points[i][0], top + points[i][1], &drag, 0);
            assert(messages == (kind == 1 && i < 3));
            assert(completions == 1 && !forwards);
        }
    }
}

int main(void)
{
    struct DragInfo drag = {0};
    static const DropCase forwarding[] = {test_bitmap_tab_drop, test_tab_drop,
        test_cart_drop, test_goods_drop, test_button_drop};
    size_t i;
    int parent;
    item_box(test_grade_drop, 53, 114);
    item_box(test_refine_drop, 109, 201);
    for (i = 0; i < sizeof forwarding / sizeof forwarding[0]; ++i)
        for (parent = 0; parent < 2; ++parent) {
            forwarding[i](-400, 600, &drag, parent);
            assert(forwards == parent && !messages && !completions);
        }
    g_windowMgr.exchange_wnd = &parent_window;
    for (parent = 0; parent < 2; ++parent) {
        test_button_drop(12, -7, &drag, parent);
        assert(!forwards && !messages && completions == 1);
    }
    g_windowMgr.exchange_wnd = NULL;
    test_window_drop(12, -7, &drag, 0);
    assert(!forwards && !messages && completions == 1);
    test_item_base_drop(12, -7, &drag, 1);
    assert(!forwards && !messages && completions == 1);
    test_result_drop(12, -7, &drag, 1);
    assert(!forwards && !messages && !completions);
    puts("UI drops: typed payloads, item-box boundaries, parent forwarding, blocked exchange, "
         "completion order and unchanged owners passed.");
    return 0;
}
