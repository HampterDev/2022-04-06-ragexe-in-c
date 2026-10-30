#include "std/std.h"
#include "ui/window_mgr.h"
#include "ui/misc/snap_mgr.h"
#include "core/fatal.h"

static BYTE g_snapMgrEnabled = 1u;
static const int opposite_sides[4] = { 2, 3, 0, 1 };

#define SNAP_OBJECT(node) (*LIST_VALUE(node, struct SnapObject *))
#define SNAP_NODE_SIZE (sizeof(struct ListNode) + sizeof(struct SnapObject *))

struct SnapMgr g_snapMgr;

struct ListNode *snap_list_buy_node0(struct List *list, struct ListNode *next,
                                                struct ListNode *prev)
{
    struct ListNode *node = mem_alloc(SNAP_NODE_SIZE);

    if (!next) {
        node->next = node;
        node->prev = node;
    } else {
        node->next = next;
        node->prev = prev;
    }
    return node;
}

struct ListNode *snap_list_buy_node(struct List *list, struct ListNode *next,
                                               struct ListNode *prev, void *const *value)
{
    struct ListNode *node = snap_list_buy_node0(list, next, prev);

    *LIST_VALUE(node, void *) = *value;
    return node;
}

struct List *snap_list_construct(struct List *list)
{
    list->head = NULL;
    list->size = 0;
    list->head = snap_list_buy_node0(list, NULL, NULL);
    return list;
}

static void snap_list_clear(struct List *list)
{
    struct ListNode *node = list->head->next, *next;

    list->head->next = list->head;
    list->head->prev = list->head;
    list->size = 0;
    for (; node != list->head; node = next) {
        next = node->next;
        mem_free(node);
    }
}

void snap_list_destroy(struct List *list)
{
    snap_list_clear(list);
    mem_free(list->head);
}

void snap_list_push_back(struct List *list, void *const *value)
{
    struct ListNode *head = list->head, *prev = head->prev;
    struct ListNode *node = snap_list_buy_node(list, head, prev, value);

    if (SNAP_LIST_MAX - list->size < 1)
        fatal_error("list<T> too long");
    list->size++;
    head->prev = node;
    prev->next = node;
}

struct SnapMgr *snap_mgr_construct(struct SnapMgr *self)
{
    self->root = NULL;
    snap_list_construct(&self->objects);
    return self;
}

struct SnapObject *snap_object_construct(struct SnapObject *self)
{
    int i;

    self->wnd = NULL;
    for (i = 0; i < 4; i++)
        snap_list_construct(&self->sides[i]);
    self->visited = 0;
    return self;
}

void snap_object_destruct(struct SnapObject *self)
{
    int i;

    for (i = 3; i >= 0; i--)
        snap_list_destroy(&self->sides[i]);
}

static int spans_meet(int a_lo, int a_hi, int b_lo, int b_hi)
{
    if (a_hi >= b_lo && a_hi <= b_hi)
        return 1;
    if (b_hi >= a_lo && b_hi <= a_hi)
        return 1;
    if (b_lo <= a_lo) {
        if (a_lo <= b_hi)
            return 1;
        if (b_lo < a_lo)
            return 0;
    }
    return b_lo <= a_hi;
}

int snap_object_side_of(struct SnapObject *self, struct SnapObject *other)
{
    const struct UIWindow *a = self->wnd, *b = other->wnd;
    int a_right = a->x + a->width, a_bottom = a->y + a->height;
    int b_right = b->x + b->width, b_bottom = b->y + b->height;

    if (a->x == b_right && spans_meet(a->y, a_bottom, b->y, b_bottom))
        return 0;
    if (b->x == a_right && spans_meet(b->y, b_bottom, a->y, a_bottom))
        return 2;
    if (a->y == b_bottom && spans_meet(a->x, a_right, b->x, b_right))
        return 1;
    if (b->y == a_bottom && spans_meet(b->x, b_right, a->x, a_right))
        return 3;
    return -1;
}

void snap_object_unlink(struct SnapObject *self, struct SnapObject *other)
{
    struct List *list;
    struct ListNode *node;
    int i;

    if (!other)
        return;
    for (i = 0; i < 4; i++) {
        list = &self->sides[i];
        for (node = list->head->next; node != list->head; node = node->next) {
            if (*LIST_VALUE(node, void *) == other) {
                node->prev->next = node->next;
                node->next->prev = node->prev;
                list->size--;
                mem_free(node);
                break;
            }
        }
    }
}

void snap_object_sum_heights(struct SnapObject *self, int *sum, int side)
{
    struct ListNode *node;
    struct SnapObject *next;

    for (;;) {
        *sum += self->wnd->height;
        next = NULL;
        for (node = self->sides[side].head->next; node != self->sides[side].head; node = node->next)
            if (SNAP_OBJECT(node)->wnd->height > 0)
                next = SNAP_OBJECT(node);
        if (!next)
            return;
        self = next;
    }
}

BYTE snap_object_is_connected(struct SnapObject *self, struct SnapObject *target)
{
    struct ListNode *node;
    int i;

    if (self->visited == 1)
        return 0;
    if (target == self)
        return 1;
    self->visited = 1;
    for (i = 0; i < 4; i++)
        for (node = self->sides[i].head->next; node != self->sides[i].head; node = node->next)
            if (snap_object_is_connected(SNAP_OBJECT(node), target))
                return 1;
    return 0;
}

BYTE snap_object_is_connected_from(struct SnapObject *self, struct SnapObject *target)
{
    struct ListNode *node;
    int i;

    if (self->visited == 1 || target == self)
        return 0;
    self->visited = 1;
    for (i = 0; i < 4; i++) {
        if (!self->sides[i].size)
            continue;
        for (node = self->sides[i].head->next; node != self->sides[i].head; node = node->next)
            if (snap_object_is_connected(SNAP_OBJECT(node), target))
                return 1;
    }
    return 0;
}

BYTE snap_object_reaches_below(struct SnapObject *self, struct SnapObject *target)
{
    struct ListNode *node;

    if (self == target)
        return 1;
    for (node = self->sides[1].head->next; node != self->sides[1].head; node = node->next)
        if (snap_object_reaches_below(SNAP_OBJECT(node), target))
            return 1;
    return 0;
}

void snap_object_move_chain(struct SnapObject *self, int dx, int dy, int side)
{
    struct ListNode *node;

    if (side == -1 || self->visited == 1)
        return;
    self->visited = 1;
    if (self->wnd) {
        self->wnd->x += dx;
        self->wnd->y += dy;
        ui_window_mgr_update_snap_rect(&g_windowMgr, self->wnd);
    }
    for (node = self->sides[side].head->next; node != self->sides[side].head; node = node->next)
        snap_object_move_chain(SNAP_OBJECT(node), dx, dy, side);
}

static int screen_height(void)
{
    return g_renderer->height;
}

void snap_object_move_chain_checked(struct SnapObject *self, int dx, int dy, int side)
{
    struct SnapObject *root = g_snapMgr.root;
    struct ListNode *node, *inner;
    struct SnapObject *x;
    int below = 0, above = 0, at;

    if (side == -1 || self->visited == 1)
        return;
    self->visited = 1;
    snap_object_sum_heights(self, &below, 1);
    snap_object_sum_heights(self, &above, 3);
    at = self->wnd->y - dy + above;
    if (self->wnd->height - below + self->wnd->y == 0)
        goto chain;
    if (root && root == self && at != screen_height())
        goto chain;
    if (side == 3) {
        if (root != self) {
            for (node = root->sides[1].head->next; node != root->sides[1].head; node = node->next) {
                x = SNAP_OBJECT(node);
                if (x == self)
                    goto turn;
                for (inner = x->sides[1].head->next; inner != x->sides[1].head; inner = inner->next)
                    if (snap_object_reaches_below(SNAP_OBJECT(inner), self))
                        goto turn;
            }
        }
        if (at != screen_height())
            goto chain;
turn:
        dy = -dy;
        side = opposite_sides[3];
    }
    if (side == 1 && self->wnd) {
        self->wnd->x += dx;
        self->wnd->y += dy;
        ui_window_mgr_update_snap_rect(&g_windowMgr, self->wnd);
    }
chain:
    for (node = self->sides[side].head->next; node != self->sides[side].head; node = node->next)
        snap_object_move_chain(SNAP_OBJECT(node), dx, dy, side);
}

static void delete_objects(struct SnapMgr *self)
{
    struct ListNode *node;
    struct SnapObject *obj;

    for (node = self->objects.head->next; node != self->objects.head; node = node->next) {
        obj = SNAP_OBJECT(node);
        if (obj) {
            snap_object_destruct(obj);
            mem_free(obj);
        }
    }
    snap_list_clear(&self->objects);
}

void snap_mgr_destruct(struct SnapMgr *self)
{
    delete_objects(self);
    self->root = NULL;
    snap_list_destroy(&self->objects);
}

struct SnapObject *snap_mgr_find(struct SnapMgr *self, struct UIWindow *wnd)
{
    struct ListNode *node;

    for (node = self->objects.head->next; node != self->objects.head; node = node->next)
        if (SNAP_OBJECT(node)->wnd == wnd)
            return SNAP_OBJECT(node);
    return NULL;
}

static struct SnapObject *new_object(struct UIWindow *wnd)
{
    struct SnapObject *obj = mem_alloc(sizeof *obj);

    if (obj)
        snap_object_construct(obj);
    obj->wnd = wnd;
    return obj;
}

void snap_mgr_register_root(struct SnapMgr *self, struct UIWindow *wnd)
{
    struct SnapObject *obj;

    if (!g_snapMgrEnabled || snap_mgr_find(self, wnd) || self->root)
        return;
    obj = new_object(wnd);
    self->root = obj;
    snap_list_push_back(&self->objects, (void *const *)&obj);
}

void snap_mgr_register(struct SnapMgr *self, struct UIWindow *wnd)
{
    struct SnapObject *obj;

    if (!g_snapMgrEnabled)
        return;
    if (self->root && self->root->wnd == wnd)
        return;
    if (snap_mgr_find(self, wnd))
        return;
    obj = new_object(wnd);
    snap_mgr_relink(self, obj);
    snap_list_push_back(&self->objects, (void *const *)&obj);
}

void snap_mgr_clear_visited(struct SnapMgr *self)
{
    struct ListNode *node;

    for (node = self->objects.head->next; node != self->objects.head; node = node->next)
        SNAP_OBJECT(node)->visited = 0;
}

BYTE snap_mgr_in_root_group(struct SnapMgr *self, struct UIWindow *wnd)
{
    struct SnapObject *obj;

    if (!self->root || self->root->wnd == wnd)
        return 0;
    snap_mgr_clear_visited(self);
    obj = snap_mgr_find(self, wnd);
    if (!obj)
        return 0;
    return snap_object_is_connected_from(obj, self->root) ? 1 : 0;
}

void snap_mgr_save_group_positions(struct SnapMgr *self, struct UIWindow *wnd)
{
    struct ListNode *node;
    struct UIWindow *w;

    if (!self->root || !wnd || wnd != self->root->wnd)
        return;
    for (node = self->objects.head->next; node != self->objects.head; node = node->next) {
        w = SNAP_OBJECT(node)->wnd;
        if (w && snap_mgr_in_root_group(self, w))
            w->ops->save_pos(w);
    }
}

void snap_mgr_relink_all(struct SnapMgr *self, struct UIWindow *wnd)
{
    struct ListNode *node;

    if (!snap_mgr_find(self, wnd))
        return;
    for (node = self->objects.head->next; node != self->objects.head; node = node->next)
        snap_mgr_relink(self, SNAP_OBJECT(node));
}

static int larger_shift(int a, int b)
{
    return abs(a) < abs(b) ? b : a;
}

void snap_mgr_drag_group(struct SnapMgr *self, struct UIWindow *root, int *dx, int *dy)
{
    struct ListNode *node;
    struct UIWindow *w;
    int rx = 0, ry = 0, sx, sy, old_y;

    if (!self->root)
        return;
    if (root == self->root->wnd) {
        for (node = self->objects.head->next; node != self->objects.head; node = node->next) {
            w = SNAP_OBJECT(node)->wnd;
            if (!snap_mgr_in_root_group(self, w))
                continue;
            old_y = w->y;
            w->ops->move_delta(w, *dx, *dy);
            ui_window_mgr_get_snap_delta(&g_windowMgr, w, &sx, &sy, 1);
            w->x = old_y;
            rx = larger_shift(rx, sx);
            ry = larger_shift(ry, sy);
        }
    }
    *dx = rx;
    *dy = ry;
}

BYTE snap_mgr_are_connected(struct SnapMgr *self, struct UIWindow *a,
                                      struct UIWindow *b)
{
    struct SnapObject *obj_a, *obj_b;

    snap_mgr_clear_visited(self);
    obj_a = snap_mgr_find(self, a);
    if (!obj_a)
        return 0;
    obj_b = snap_mgr_find(self, b);
    if (!obj_b)
        return 0;
    return snap_object_is_connected_from(obj_a, obj_b);
}

BYTE snap_mgr_is_root_window(struct SnapMgr *self, struct UIWindow *wnd)
{
    return self->root ? self->root->wnd == wnd : 0;
}

BYTE snap_mgr_is_registered_to_snap(struct SnapMgr *self, struct UIWindow *wnd)
{
    return snap_mgr_find(self, wnd) != NULL;
}

void snap_mgr_move_chain(struct SnapMgr *self, struct UIWindow *wnd, int dx, int dy,
                                   int side)
{
    struct SnapObject *obj;
    struct ListNode *node;

    snap_mgr_clear_visited(self);
    obj = snap_mgr_find(self, wnd);
    if (!obj)
        return;
    snap_object_move_chain_checked(obj, dx, dy, side);
    if (!snap_mgr_find(self, wnd))
        return;
    for (node = self->objects.head->next; node != self->objects.head; node = node->next)
        snap_mgr_relink(self, SNAP_OBJECT(node));
}

void snap_mgr_move_group(struct SnapMgr *self, struct UIWindow *root, int dx, int dy)
{
    struct ListNode *node;
    struct UIWindow *w;

    if (!self->root || root != self->root->wnd)
        return;
    for (node = self->objects.head->next; node != self->objects.head; node = node->next) {
        w = SNAP_OBJECT(node)->wnd;
        if (snap_mgr_in_root_group(self, w)) {
            w->ops->move_delta(w, dx, dy);
            ui_window_mgr_update_snap_rect(&g_windowMgr, w);
        }
    }
}

void snap_mgr_relink(struct SnapMgr *self, struct SnapObject *obj)
{
    struct ListNode *node;
    struct SnapObject *other;
    int side, opposite;

    for (node = self->objects.head->next; node != self->objects.head; node = node->next) {
        other = SNAP_OBJECT(node);
        side = snap_object_side_of(obj, other);
        snap_object_unlink(obj, other);
        snap_object_unlink(other, obj);
        if (side == -1)
            continue;
        snap_list_push_back(&obj->sides[side], (void *const *)&other);
        opposite = opposite_sides[side];
        if (opposite != -1)
            snap_list_push_back(&other->sides[opposite], (void *const *)&obj);
    }
}

void snap_mgr_clear(struct SnapMgr *self)
{
    delete_objects(self);
    self->root = NULL;
}

void snap_mgr_unregister(struct SnapMgr *self, struct UIWindow *wnd)
{
    struct ListNode *node;
    struct SnapObject *obj;

    if (self->root && self->root->wnd == wnd)
        self->root = NULL;
    for (node = self->objects.head->next; node != self->objects.head; node = node->next)
        if (SNAP_OBJECT(node)->wnd == wnd)
            break;
    if (node == self->objects.head)
        return;
    obj = SNAP_OBJECT(node);
    if (obj) {
        snap_object_destruct(obj);
        mem_free(obj);
    }
    node->prev->next = node->next;
    node->next->prev = node->prev;
    self->objects.size--;
    mem_free(node);
    if (!obj)
        return;
    for (node = self->objects.head->next; node != self->objects.head; node = node->next)
        snap_object_unlink(SNAP_OBJECT(node), obj);
}
