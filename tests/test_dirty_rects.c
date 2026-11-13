/* render/dirty_rects.c: rectangles written far apart stay apart, those that touch become one, a
   ninth far from the rest joins the one it wastes least with, a rectangle that bridges two takes
   in both; and fuzzed sequences of writes on a small texture against a map of the texels written:
   every texel written lies within a kept rectangle, at most eight are kept and no two touch, none
   reaches past the box of what was written, and writes that never touch are kept as written. */
#include "render/dirty_rects.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIDE 64

static RECT rect(LONG left, LONG top, LONG right, LONG bottom)
{
    RECT r;
    r.left = left;
    r.top = top;
    r.right = right;
    r.bottom = bottom;
    return r;
}

static int same(const RECT *a, const RECT *b)
{
    return a->left == b->left && a->top == b->top && a->right == b->right && a->bottom == b->bottom;
}

/* whether d keeps a rectangle equal to r */
static int keeps(const struct DirtyRects *d, RECT r)
{
    int i;
    for (i = 0; i < d->count; i++)
        if (same(&d->rect[i], &r))
            return 1;
    return 0;
}

static void add(struct DirtyRects *d, LONG left, LONG top, LONG right, LONG bottom)
{
    RECT r = rect(left, top, right, bottom);
    dirty_rects_add(d, &r);
}

static void check_cases(void)
{
    struct DirtyRects d;
    int i;

    /* empty rectangles change nothing */
    memset(&d, 0, sizeof d);
    add(&d, 4, 4, 4, 8);
    add(&d, 4, 4, 8, 4);
    add(&d, 8, 8, 4, 12);
    assert(d.count == 0);

    /* apart, sharing an edge, sharing a corner, overlapping */
    add(&d, 0, 0, 4, 4);
    add(&d, 10, 0, 14, 4);
    assert(d.count == 2 && keeps(&d, rect(0, 0, 4, 4)) && keeps(&d, rect(10, 0, 14, 4)));
    add(&d, 4, 0, 6, 2);
    assert(d.count == 2 && keeps(&d, rect(0, 0, 6, 4)));
    add(&d, 14, 4, 16, 6);
    assert(d.count == 2 && keeps(&d, rect(10, 0, 16, 6)));
    add(&d, 12, 2, 20, 3);
    assert(d.count == 2 && keeps(&d, rect(10, 0, 20, 6)));

    /* a rectangle that bridges two takes in both, and the union what it then touches */
    memset(&d, 0, sizeof d);
    add(&d, 0, 0, 2, 2);
    add(&d, 6, 0, 8, 2);
    add(&d, 0, 5, 8, 7);
    add(&d, 20, 20, 22, 22);
    assert(d.count == 4);
    add(&d, 2, 0, 6, 1);
    assert(d.count == 3 && keeps(&d, rect(0, 0, 8, 2)));
    add(&d, 3, 2, 4, 5);
    assert(d.count == 2 && keeps(&d, rect(0, 0, 8, 7)) && keeps(&d, rect(20, 20, 22, 22)));

    /* eight cells of an atlas page kept apart; a ninth far from all of them joins the one whose
       union with it wastes least: its neighbour in the row, not one across the page */
    memset(&d, 0, sizeof d);
    for (i = 0; i < 8; i++)
        add(&d, i * 32, i % 2 * 128, i * 32 + 16, i % 2 * 128 + 16);
    assert(d.count == DIRTY_RECTS_MOST);
    add(&d, 7 * 32, 160, 7 * 32 + 16, 176);
    assert(d.count == DIRTY_RECTS_MOST && keeps(&d, rect(7 * 32, 128, 7 * 32 + 16, 176)));
    for (i = 0; i < 7; i++)
        assert(keeps(&d, rect(i * 32, i % 2 * 128, i * 32 + 16, i % 2 * 128 + 16)));
}

static unsigned next(unsigned *seed)
{
    *seed = *seed * 1103515245u + 12345u;
    return *seed >> 16 & 0x7fff;
}

/* A random rectangle within the texture, from a texel to most of it */
static RECT random_rect(unsigned *seed)
{
    int big = next(seed) % 8 == 0;
    LONG w = 1 + (LONG)(next(seed) % (big ? SIDE / 2 : 6)), h = 1 + (LONG)(next(seed) % (big ? SIDE / 2 : 6));
    LONG x = (LONG)(next(seed) % (SIDE - w + 1)), y = (LONG)(next(seed) % (SIDE - h + 1));
    return rect(x, y, x + w, y + h);
}

static int touch(const RECT *a, const RECT *b)
{
    return a->left <= b->right && b->left <= a->right && a->top <= b->bottom && b->top <= a->bottom;
}

static void check_fuzz(unsigned seed, int sequences)
{
    static unsigned char written[SIDE][SIDE];
    long merged = 0, total = 0;
    int s;
    for (s = 0; s < sequences; s++) {
        struct DirtyRects d;
        RECT given[DIRTY_RECTS_MOST + 1], hull = rect(SIDE, SIDE, 0, 0);
        int n = 1 + (int)(next(&seed) % 24), i, j, x, y, apart = 1;
        memset(&d, 0, sizeof d);
        memset(written, 0, sizeof written);
        for (i = 0; i < n; i++) {
            RECT r = random_rect(&seed);
            dirty_rects_add(&d, &r);
            for (y = r.top; y < r.bottom; y++)
                for (x = r.left; x < r.right; x++)
                    written[y][x] = 1;
            if (r.left < hull.left) hull.left = r.left;
            if (r.top < hull.top) hull.top = r.top;
            if (r.right > hull.right) hull.right = r.right;
            if (r.bottom > hull.bottom) hull.bottom = r.bottom;
            if (i <= DIRTY_RECTS_MOST)
                given[i] = r;
            for (j = 0; j < i && j <= DIRTY_RECTS_MOST && apart; j++)
                apart = !touch(&given[j], &r);
            if (i >= DIRTY_RECTS_MOST)
                apart = 0;
            /* writes that never touch, no more than there is room for, are kept as written */
            if (apart) {
                assert(d.count == i + 1);
                for (j = 0; j <= i; j++)
                    assert(keeps(&d, given[j]));
            }
            assert(d.count >= 1 && d.count <= DIRTY_RECTS_MOST);
            for (j = 0; j < d.count; j++) {
                int k;
                const RECT *k0 = &d.rect[j];
                assert(k0->left < k0->right && k0->top < k0->bottom);
                assert(k0->left >= hull.left && k0->top >= hull.top && k0->right <= hull.right
                       && k0->bottom <= hull.bottom);
                for (k = j + 1; k < d.count; k++)
                    assert(!touch(k0, &d.rect[k]));
            }
        }
        for (y = 0; y < SIDE; y++)
            for (x = 0; x < SIDE; x++) {
                int in = 0;
                for (i = 0; i < d.count && !in; i++)
                    in = x >= d.rect[i].left && x < d.rect[i].right && y >= d.rect[i].top && y < d.rect[i].bottom;
                assert(!written[y][x] || in);
                total += written[y][x];
                merged += in && !written[y][x];
            }
    }
    printf("dirty rects: %d fuzzed sequences of up to 24 writes on %d by %d texels: every texel written kept, "
           "%ld texels written, %ld more kept that were not\n", sequences, SIDE, SIDE, total, merged);
}

int main(void)
{
    check_cases();
    check_fuzz(20261113u, 200000);
    return 0;
}
