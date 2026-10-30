#include <string.h>
#include "std/std.h"

#define SORT_MAX_ELEM 0x400

struct sort_ctx {
    size_t size;
    StdSortLessFn less;
    void *pred;
};

#define AT(base, i) ((char *)(base) + (ptrdiff_t)(i) * (ptrdiff_t)c->size)
#define LESS(a, b) (c->less((a), (b), c->pred))

static void elem_swap(const struct sort_ctx *c, char *a, char *b)
{
    char saved[SORT_MAX_ELEM];

    memcpy(saved, a, c->size);
    memcpy(a, b, c->size);
    memcpy(b, saved, c->size);
}

static ptrdiff_t span(const struct sort_ctx *c, const char *first, const char *last)
{
    return (last - first) / (ptrdiff_t)c->size;
}

static void med3(const struct sort_ctx *c, char *first, char *mid, char *last)
{
    if (LESS(mid, first))
        elem_swap(c, mid, first);
    if (LESS(last, mid)) {
        elem_swap(c, last, mid);
        if (LESS(mid, first))
            elem_swap(c, mid, first);
    }
}

static void guess_median(const struct sort_ctx *c, char *first, char *mid, char *last)
{
    ptrdiff_t count = span(c, first, last);

    if (count > 40) {
        ptrdiff_t step = (count + 1) >> 3;
        ptrdiff_t two_step = step << 1;

        med3(c, first, AT(first, step), AT(first, two_step));
        med3(c, AT(mid, -step), mid, AT(mid, step));
        med3(c, AT(last, -two_step), AT(last, -step), last);
        med3(c, AT(first, step), mid, AT(last, -step));
    } else {
        med3(c, first, mid, last);
    }
}

static void partition(const struct sort_ctx *c, char *first, char *last, char **out_first, char **out_last)
{
    char *mid = AT(first, span(c, first, last) >> 1);
    char *pfirst, *plast, *gfirst, *glast;

    guess_median(c, first, mid, AT(last, -1));
    pfirst = mid;
    plast = AT(pfirst, 1);
    while (first < pfirst && !LESS(AT(pfirst, -1), pfirst) && !LESS(pfirst, AT(pfirst, -1)))
        pfirst = AT(pfirst, -1);
    while (plast < last && !LESS(plast, pfirst) && !LESS(pfirst, plast))
        plast = AT(plast, 1);

    gfirst = plast;
    glast = pfirst;
    for (;;) {
        for (; gfirst < last; gfirst = AT(gfirst, 1)) {
            if (LESS(pfirst, gfirst))
                continue;
            if (LESS(gfirst, pfirst))
                break;
            if (plast != gfirst)
                elem_swap(c, plast, gfirst);
            plast = AT(plast, 1);
        }
        for (; first < glast; glast = AT(glast, -1)) {
            if (LESS(AT(glast, -1), pfirst))
                continue;
            if (LESS(pfirst, AT(glast, -1)))
                break;
            pfirst = AT(pfirst, -1);
            if (pfirst != AT(glast, -1))
                elem_swap(c, pfirst, AT(glast, -1));
        }
        if (glast == first && gfirst == last) {
            *out_first = pfirst;
            *out_last = plast;
            return;
        }
        if (glast == first) {
            if (plast != gfirst)
                elem_swap(c, pfirst, plast);
            plast = AT(plast, 1);
            elem_swap(c, pfirst, gfirst);
            pfirst = AT(pfirst, 1);
            gfirst = AT(gfirst, 1);
        } else if (gfirst == last) {
            glast = AT(glast, -1);
            pfirst = AT(pfirst, -1);
            if (glast != pfirst)
                elem_swap(c, glast, pfirst);
            plast = AT(plast, -1);
            elem_swap(c, pfirst, plast);
        } else {
            glast = AT(glast, -1);
            elem_swap(c, gfirst, glast);
            gfirst = AT(gfirst, 1);
        }
    }
}

static void heap_adjust(const struct sort_ctx *c, char *first, ptrdiff_t hole, ptrdiff_t bottom, const char *val)
{
    ptrdiff_t top = hole, idx = hole;
    ptrdiff_t max_non_leaf = (bottom - 1) >> 1;

    while (idx < max_non_leaf) {
        idx = 2 * idx + 2;
        if (LESS(AT(first, idx), AT(first, idx - 1)))
            idx--;
        memcpy(AT(first, hole), AT(first, idx), c->size);
        hole = idx;
    }
    if (idx == max_non_leaf && bottom % 2 == 0) {
        memcpy(AT(first, hole), AT(first, bottom - 1), c->size);
        hole = bottom - 1;
    }
    for (idx = (hole - 1) >> 1; top < hole && LESS(AT(first, idx), val); idx = (hole - 1) >> 1) {
        memcpy(AT(first, hole), AT(first, idx), c->size);
        hole = idx;
    }
    memcpy(AT(first, hole), val, c->size);
}

static void heap_sort(const struct sort_ctx *c, char *first, char *last)
{
    char val[SORT_MAX_ELEM];
    ptrdiff_t bottom = span(c, first, last), hole;

    for (hole = bottom >> 1; hole > 0;) {
        hole--;
        memcpy(val, AT(first, hole), c->size);
        heap_adjust(c, first, hole, bottom, val);
    }
    for (; span(c, first, last) >= 2; last = AT(last, -1)) {
        char *back = AT(last, -1);

        memcpy(val, back, c->size);
        memcpy(back, first, c->size);
        heap_adjust(c, first, 0, span(c, first, back), val);
    }
}

static void insertion_sort(const struct sort_ctx *c, char *first, char *last)
{
    char val[SORT_MAX_ELEM];
    char *mid, *hole, *prev;

    if (first == last)
        return;
    for (mid = AT(first, 1); mid != last; mid = AT(mid, 1)) {
        memcpy(val, mid, c->size);
        if (LESS(val, first)) {
            memmove(AT(first, 1), first, mid - first);
            memcpy(first, val, c->size);
            continue;
        }
        for (hole = mid, prev = AT(hole, -1); LESS(val, prev); hole = prev, prev = AT(prev, -1))
            memcpy(hole, prev, c->size);
        memcpy(hole, val, c->size);
    }
}

static void sort_unchecked(const struct sort_ctx *c, char *first, char *last, ptrdiff_t ideal)
{
    for (;;) {
        char *mid_first, *mid_last;

        if (span(c, first, last) <= 32) {
            insertion_sort(c, first, last);
            return;
        }
        if (ideal <= 0) {
            heap_sort(c, first, last);
            return;
        }
        partition(c, first, last, &mid_first, &mid_last);
        ideal = (ideal >> 1) + (ideal >> 2);
        if (span(c, first, mid_first) < span(c, mid_last, last)) {
            sort_unchecked(c, first, mid_first, ideal);
            first = mid_last;
        } else {
            sort_unchecked(c, mid_last, last, ideal);
            last = mid_first;
        }
    }
}

void std_sort(void *first, void *last, size_t elem_size, StdSortLessFn less, void *pred)
{
    struct sort_ctx c;

    c.size = elem_size;
    c.less = less;
    c.pred = pred;
    sort_unchecked(&c, (char *)first, (char *)last, span(&c, (char *)first, (char *)last));
}

static char *merge_move(const struct sort_ctx *c, char *first, char *mid, char *last, char *dest)
{
    char *a = first, *b = mid;

    while (a != mid && b != last) {
        if (LESS(b, a)) {
            memcpy(dest, b, c->size);
            b = AT(b, 1);
        } else {
            memcpy(dest, a, c->size);
            a = AT(a, 1);
        }
        dest = AT(dest, 1);
    }
    memcpy(dest, a, mid - a);
    dest += mid - a;
    memcpy(dest, b, last - b);
    return dest + (last - b);
}

static void chunked_merge(const struct sort_ctx *c, char *first, char *last, char *dest,
                          ptrdiff_t chunk, ptrdiff_t count)
{
    while (chunk < count) {
        char *mid1, *mid2;
        ptrdiff_t chunk2;

        count -= chunk;
        mid1 = AT(first, chunk);
        chunk2 = chunk < count ? chunk : count;
        count -= chunk2;
        mid2 = AT(mid1, chunk2);
        dest = merge_move(c, first, mid1, mid2, dest);
        first = mid2;
    }
    memcpy(dest, first, last - first);
}

static void buffered_merge_sort(const struct sort_ctx *c, char *first, char *last, ptrdiff_t count, char *temp)
{
    ptrdiff_t chunk, left;
    char *run;

    for (run = first, left = count; left > 32; left -= 32, run = AT(run, 32))
        insertion_sort(c, run, AT(run, 32));
    insertion_sort(c, run, last);
    for (chunk = 32; chunk < count; chunk *= 2) {
        chunked_merge(c, first, last, temp, chunk, count);
        chunk *= 2;
        if (count <= chunk) {
            memcpy(first, temp, count * c->size);
            break;
        }
        chunked_merge(c, temp, AT(temp, count), first, chunk, count);
    }
}

void std_stable_sort(void *first, void *last, size_t elem_size, StdSortLessFn less, void *pred)
{
    struct sort_ctx ctx, *c = &ctx;
    char stack_buf[0x1000];
    ptrdiff_t count, half, ceil_half;
    char *lo = (char *)first, *hi = (char *)last, *mid, *temp, *a, *b, *dest, *temp_end;

    ctx.size = elem_size;
    ctx.less = less;
    ctx.pred = pred;
    count = span(c, lo, hi);
    if (count <= 32) {
        if (count > 1)
            insertion_sort(c, lo, hi);
        return;
    }
    half = count >> 1;
    ceil_half = count - half;
    temp = ceil_half * (ptrdiff_t)elem_size <= (ptrdiff_t)sizeof stack_buf
         ? stack_buf : (char *)mem_alloc(ceil_half * elem_size);
    mid = AT(lo, ceil_half);
    buffered_merge_sort(c, lo, mid, ceil_half, temp);
    buffered_merge_sort(c, mid, hi, half, temp);
    memcpy(temp, lo, mid - lo);
    temp_end = temp + (mid - lo);
    for (a = temp, b = mid, dest = lo; a != temp_end && b != hi; dest = AT(dest, 1)) {
        if (LESS(b, a)) {
            memcpy(dest, b, elem_size);
            b = AT(b, 1);
        } else {
            memcpy(dest, a, elem_size);
            a = AT(a, 1);
        }
    }
    memcpy(dest, a, temp_end - a);
    if (temp != stack_buf)
        mem_free(temp);
}
