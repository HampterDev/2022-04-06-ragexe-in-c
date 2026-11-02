#include "platform/win_types.h"
#include "base.h"
#include "ui/misc/snap_mgr.h"

#define SNAP_RANGE 10

SIZE_ASSERT(SnapInfo, 0x30);

void snap_info_set(struct SnapInfo *self, int x, int y, int w, int h)
{
    self->left.pos = x;
    self->left.from = y;
    self->left.to = y + h;
    self->right.pos = x + w;
    self->right.from = y;
    self->right.to = y + h;
    self->top.pos = y;
    self->top.from = x;
    self->top.to = x + w;
    self->bottom.pos = y + h;
    self->bottom.from = x;
    self->bottom.to = x + w;
}

void snap_info_flip(struct SnapInfo *self)
{
    struct SnapEdge t;

    t = self->left;
    self->left = self->right;
    self->right = t;
    t = self->top;
    self->top = self->bottom;
    self->bottom = t;
}

BYTE snap_spans_overlap(int a, int b, int c, int d)
{
    return (c >= a && c <= b) || (d >= a && d <= b) || (a >= c && a <= d) || (b >= c && b <= d);
}

static int snap_near(int v, int at)
{
    return v >= at - SNAP_RANGE && v <= at + SNAP_RANGE;
}

void snap_info_get_delta_snap(struct SnapInfo *self, const struct SnapInfo *other,
                                       int *dx, int *dy)
{
    *dx = 0;
    *dy = 0;
    if (snap_spans_overlap(self->left.from, self->left.to, other->right.from, other->right.to)) {
        int snapped = 1;

        if (snap_near(other->right.pos, self->left.pos))
            *dx = self->left.pos - other->right.pos;
        else if (snap_near(other->left.pos, self->right.pos))
            *dx = self->right.pos - other->left.pos;
        else
            snapped = 0;
        if (snapped) {
            if (snap_near(other->top.pos, self->top.pos))
                *dy = self->top.pos - other->top.pos;
            else if (snap_near(other->bottom.pos, self->bottom.pos))
                *dy = self->bottom.pos - other->bottom.pos;
        }
    }
    if (snap_spans_overlap(self->top.from, self->top.to, other->bottom.from, other->bottom.to)) {
        if (snap_near(other->bottom.pos, self->top.pos))
            *dy = self->top.pos - other->bottom.pos;
        else if (snap_near(other->top.pos, self->bottom.pos))
            *dy = self->bottom.pos - other->top.pos;
        else
            return;
        if (snap_near(other->left.pos, self->left.pos))
            *dx = self->left.pos - other->left.pos;
        else if (snap_near(other->right.pos, self->right.pos))
            *dx = self->right.pos - other->right.pos;
    }
}
