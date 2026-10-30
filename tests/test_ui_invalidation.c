#include "ui/window_types.h"
#include "std/list.h"
#include "session/state.h"
#include "ui_invalidation_fixture.h"
#include "diag/perf.h"
#include "game/context.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ui_window_invalidate_base(struct UIWindow *self);
void ui_window_invalidate_children(struct UIWindow *self);
void ui_ondraw(struct UIWindow *self);
void ui_soft_next(int on);

struct perf_counters g_perf;
struct Session g_session1;
static DWORD ticks = 10000;
static int in_game = 1;
static unsigned draw_calls;
static struct UIWindowOps plain, shortcut, shop, bitmap;
static struct UIWindow window, parent, child, hidden, grandchild;
static struct ListNode heads[5];
static volatile unsigned call_barrier;

DWORD ui_fixture_time(void) { return ticks; }
struct GameMode *game_current_mode(void)
{
    return in_game ? (struct GameMode *)&in_game : NULL;
}

const char *object_type_name(const void *table)
{
    if (table == &shortcut) return "UIShortCutWnd";
    if (table == &shop) return "UIMerchantShopTitle";
    if (table == &bitmap) return "UIBitmapButton";
    assert(table == &plain);
    return "UIWindow";
}

static void draw(struct UIWindow *self)
{
    assert(self->dirty);
    ++draw_calls;
}

static void initialize_window(struct UIWindow *w, struct ListNode *head, unsigned id)
{
    memset(w, 0, sizeof *w);
    head->next = head->prev = head;
    w->ops = &plain;
    w->visible = 1;
    w->id = id;
    w->children.head = head;
}

static void render(struct UIWindow *w)
{
    unsigned before = draw_calls;
    w->dirty = 1;
    ui_ondraw(w);
    assert(!w->dirty && draw_calls == before + 1);
}

static void invalidate_soft(struct UIWindow *w)
{
    ui_soft_next(1);
    ui_window_invalidate_base(w);
}

static void check_floor(struct UIWindowOps *table, DWORD floor)
{
    window.ops = table;
    render(&window);
    ticks += floor - 1;
    invalidate_soft(&window);
    assert(!window.dirty);
    ++ticks;
    invalidate_soft(&window);
    assert(window.dirty);
}

static void check_invalidation(void)
{
    unsigned before;
    struct SkillCooldownNode cooldown_entry = {0};
    window.parent = &parent;
    parent.composites = 1;
    before = g_perf.invalidates;
    ui_window_invalidate_base(&window);
    assert(window.dirty && parent.dirty && g_perf.invalidates == before + 2);
    parent.composites = 0;
    parent.dirty = 0;
    ui_window_invalidate_base(&window);
    assert(!parent.dirty);

    parent.composites = 1;
    render(&window);
    render(&parent);
    invalidate_soft(&window);
    assert(!window.dirty && !parent.dirty);
    ticks += 50;
    render(&parent);
    invalidate_soft(&window);
    assert(window.dirty && parent.dirty);
    window.parent = NULL;

    check_floor(&plain, 50);
    check_floor(&shortcut, 100);
    check_floor(&bitmap, 100);
    check_floor(&shop, 250);
    window.ops = &shortcut;
    render(&window);
    g_session1.skillDelays.head = &cooldown_entry;
    invalidate_soft(&window);
    assert(window.dirty);
    render(&window);
    g_session1.skillDelays.head = NULL;
    invalidate_soft(&window);
    assert(!window.dirty);
    g_session1.skillDelays.head = NULL;

    in_game = 0;
    invalidate_soft(&window);
    assert(window.dirty);
    in_game = 1;
    render(&window);
    ui_window_invalidate_base(&window);
    assert(window.dirty);

    window.ops = &plain;
    ticks = 0xfffffff0;
    check_floor(&plain, 50);
}

static void check_children(void)
{
    struct { struct ListNode links; struct UIWindow *window; } nodes[3];
    unsigned before = g_perf.invalidates;
    window.dirty = child.dirty = hidden.dirty = grandchild.dirty = 0;
    hidden.visible = 0;
    nodes[0].window = &child;
    nodes[1].window = &hidden;
    nodes[2].window = &grandchild;
    heads[0].next = &nodes[0].links; heads[0].prev = &nodes[1].links;
    nodes[0].links.next = &nodes[1].links; nodes[0].links.prev = &heads[0];
    nodes[1].links.next = &heads[0]; nodes[1].links.prev = &nodes[0].links;
    heads[2].next = heads[2].prev = &nodes[2].links;
    nodes[2].links.next = nodes[2].links.prev = &heads[2];
    ui_window_invalidate_children(&window);
    assert(window.dirty && child.dirty && grandchild.dirty && !hidden.dirty);
    assert(g_perf.invalidates == before + 3);
    heads[0].next = heads[0].prev = &heads[0];
    heads[2].next = heads[2].prev = &heads[2];
}

int main(void)
{
    size_t i;
    struct UIWindowOps *tables[] = { &plain, &shortcut, &shop, &bitmap };
    assert(SetEnvironmentVariableA("RAGEXE_UI_FLOOR_MS", NULL));
    for (i = 0; i < sizeof tables / sizeof tables[0]; ++i) {
        tables[i]->on_draw = draw;
        tables[i]->invalidate = ui_window_invalidate_base;
    }
    initialize_window(&window, &heads[0], 7);
    initialize_window(&parent, &heads[1], 8);
    initialize_window(&child, &heads[2], 9);
    initialize_window(&hidden, &heads[3], 10);
    initialize_window(&grandchild, &heads[4], 11);
    check_invalidation();
    check_children();
    puts("PASS: UI parent/child invalidation, redraw floors, cooldowns, clock wrap");
    return 0;
}
