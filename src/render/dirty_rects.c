#include "render/dirty_rects.h"

#include <stdint.h>

static int touch(const RECT *a, const RECT *b)
{
    return a->left <= b->right && b->left <= a->right && a->top <= b->bottom && b->top <= a->bottom;
}

static void take_in(RECT *into, const RECT *r)
{
    if (r->left < into->left) into->left = r->left;
    if (r->top < into->top) into->top = r->top;
    if (r->right > into->right) into->right = r->right;
    if (r->bottom > into->bottom) into->bottom = r->bottom;
}

static int64_t area(const RECT *r)
{
    return (int64_t)(r->right - r->left) * (r->bottom - r->top);
}

/* The kept rectangle r should take in: the first it touches, or, with no room left, the one whose
   union with it wastes least; -1 for none. */
static int partner(const struct DirtyRects *d, const RECT *r)
{
    int i, best = -1;
    int64_t least = 0;
    for (i = 0; i < d->count; i++)
        if (touch(&d->rect[i], r))
            return i;
    if (d->count < DIRTY_RECTS_MOST)
        return -1;
    for (i = 0; i < d->count; i++) {
        RECT u = *r;
        int64_t waste;
        take_in(&u, &d->rect[i]);
        waste = area(&u) - area(&d->rect[i]) - area(r);
        if (best < 0 || waste < least) {
            least = waste;
            best = i;
        }
    }
    return best;
}

void dirty_rects_add(struct DirtyRects *d, const RECT *r)
{
    RECT add;
    int i;
    if (r->right <= r->left || r->bottom <= r->top)
        return;
    add = *r;
    while ((i = partner(d, &add)) >= 0) {
        take_in(&add, &d->rect[i]);
        d->rect[i] = d->rect[--d->count];
    }
    d->rect[d->count++] = add;
}
