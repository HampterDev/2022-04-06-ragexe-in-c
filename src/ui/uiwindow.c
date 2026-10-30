#include "std/list.h"
#include "diag/perf.h"
#include "ui/window_types.h"
#include "ui/window_access.h"
#include "ui/window_manager_state.h"

void ui_ondraw(struct UIWindow *self)
{
    g_perf.ondraws++;
    self->ops->on_draw(self);
    self->dirty = 0;
}

static void redraw_root(struct UIWindow *self)
{
    struct UIWindow *root = self->parent;

    if (!root)
        return;
    while (root->parent)
        root = root->parent;
    if (root->dirty)
        return;
    if (root->redraws_tree || root == ui_window_mgr_focus_root(&g_windowMgr))
        ui_window_invalidate(root);
}

void ui_window_invalidate_base(struct UIWindow *self)
{
    struct UIWindow *parent = self->parent;

    g_perf.invalidates++;
    self->dirty = 1;
    if (parent && parent->composites == 1)
        parent->ops->invalidate(parent);
    else
        redraw_root(self);
}

void ui_window_invalidate_children(struct UIWindow *self)
{
    struct ListNode *head, *node;
    g_perf.invalidates++;
    self->dirty = 1;
    redraw_root(self);
    head = self->children.head;
    for (node = head->next; node != head; node = node->next) {
        struct UIWindow *child = *LIST_VALUE(node, struct UIWindow *);

        if (child->visible)
            ui_window_invalidate_children(child);
    }
}
