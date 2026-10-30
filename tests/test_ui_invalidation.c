#include "ui/window_types.h"
#include "ui/window_manager_state.h"
#include "std/list.h"
#include "ui_invalidation_fixture.h"
#include "diag/perf.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void ui_window_invalidate_base(struct UIWindow *self);
void ui_window_invalidate_children(struct UIWindow *self);
void ui_ondraw(struct UIWindow *self);

struct perf_counters g_perf;
struct UIWindowMgr g_windowMgr;
static struct UIWindow *focus_root;
static unsigned draw_calls, focus_queries;
static struct UIWindowOps ops;
static struct UIWindow root, parent, child, hidden, grandchild;
static struct ListNode heads[5];
static struct { struct ListNode links; struct UIWindow *window; } nodes[4];

DWORD ui_fixture_time(void) { return 0; }

struct UIWindow *ui_window_mgr_focus_root(struct UIWindowMgr *manager)
{
    assert(manager == &g_windowMgr);
    ++focus_queries;
    return focus_root;
}

static void draw(struct UIWindow *self)
{
    assert(self->dirty);
    ++draw_calls;
}

static void initialize_window(struct UIWindow *w, struct ListNode *head, struct UIWindow *up, unsigned id)
{
    memset(w, 0, sizeof *w);
    head->next = head->prev = head;
    w->ops = &ops;
    w->visible = 1;
    w->id = id;
    w->parent = up;
    w->children.head = head;
}

static void attach(struct UIWindow *up, int node, struct UIWindow *down)
{
    struct ListNode *head = up->children.head;

    nodes[node].window = down;
    nodes[node].links.prev = head->prev;
    nodes[node].links.next = head;
    head->prev->next = &nodes[node].links;
    head->prev = &nodes[node].links;
}

static void clean(void)
{
    root.dirty = parent.dirty = child.dirty = hidden.dirty = grandchild.dirty = 0;
}

static void build(void)
{
    initialize_window(&root, &heads[0], NULL, 7);
    initialize_window(&parent, &heads[1], &root, 8);
    initialize_window(&child, &heads[2], &parent, 9);
    initialize_window(&hidden, &heads[3], &parent, 10);
    initialize_window(&grandchild, &heads[4], &child, 11);
    hidden.visible = 0;
    attach(&root, 0, &parent);
    attach(&parent, 1, &child);
    attach(&parent, 2, &hidden);
    attach(&child, 3, &grandchild);
}

static void check_draw(void)
{
    unsigned before = g_perf.ondraws;

    root.dirty = 1;
    ui_ondraw(&root);
    assert(!root.dirty && draw_calls == 1 && g_perf.ondraws == before + 1);
}

static void check_plain_root(void)
{
    unsigned before = g_perf.invalidates;

    clean();
    ui_window_invalidate_base(&root);
    assert(root.dirty && g_perf.invalidates == before + 1);
    clean();
    before = g_perf.invalidates;
    ui_window_invalidate_base(&grandchild);
    assert(grandchild.dirty && !child.dirty && !parent.dirty && !root.dirty);
    assert(g_perf.invalidates == before + 1);
}

static void check_composited_parent(void)
{
    unsigned before;

    clean();
    parent.composites = 1;
    before = g_perf.invalidates;
    ui_window_invalidate_base(&child);
    assert(child.dirty && parent.dirty && !root.dirty && g_perf.invalidates == before + 2);
    clean();
    root.redraws_tree = 1;
    before = g_perf.invalidates;
    ui_window_invalidate_base(&child);
    assert(child.dirty && parent.dirty && root.dirty && g_perf.invalidates == before + 3);
    root.redraws_tree = 0;
    parent.composites = 0;
}

static void check_tree_root(void)
{
    unsigned before;

    clean();
    root.redraws_tree = 1;
    before = g_perf.invalidates;
    ui_window_invalidate_base(&grandchild);
    assert(grandchild.dirty && root.dirty && !child.dirty && !parent.dirty);
    assert(g_perf.invalidates == before + 2);
    before = g_perf.invalidates;
    ui_window_invalidate_base(&child);
    assert(child.dirty && g_perf.invalidates == before + 1);
    root.dirty = 1;
    ui_ondraw(&root);
    assert(!root.dirty);
    root.redraws_tree = 0;
}

static void check_focus_root(void)
{
    unsigned before, queries = focus_queries;

    clean();
    focus_root = &root;
    before = g_perf.invalidates;
    ui_window_invalidate_base(&child);
    assert(child.dirty && root.dirty && !parent.dirty && g_perf.invalidates == before + 2);
    assert(focus_queries == queries + 1);
    clean();
    focus_root = &parent;
    ui_window_invalidate_base(&child);
    assert(child.dirty && !root.dirty && !parent.dirty);
    focus_root = NULL;
}

static void check_children(void)
{
    unsigned before;

    clean();
    before = g_perf.invalidates;
    ui_window_invalidate_children(&parent);
    assert(parent.dirty && child.dirty && grandchild.dirty && !hidden.dirty && !root.dirty);
    assert(g_perf.invalidates == before + 3);
    clean();
    root.redraws_tree = 1;
    before = g_perf.invalidates;
    ui_window_invalidate_children(&child);
    assert(child.dirty && grandchild.dirty && root.dirty && !parent.dirty && !hidden.dirty);
    assert(g_perf.invalidates == before + 3);
    root.redraws_tree = 0;
}

int main(void)
{
    ops.on_draw = draw;
    ops.invalidate = ui_window_invalidate_base;
    build();
    check_draw();
    check_plain_root();
    check_composited_parent();
    check_tree_root();
    check_focus_root();
    check_children();
    puts("PASS: UI invalidation: redraw counting, composited parents, whole-tree roots, the focus root, subtrees");
    return 0;
}
