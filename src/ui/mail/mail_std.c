#include <string.h>
#include "ui/ui_std.h"
#include "game/rodex_mail.h"

static void swap_refs(struct MailRef *a, struct MailRef *b)
{
    struct MailRef t;

    if (a == b)
        return;
    t = *a;
    *a = *b;
    *b = t;
}

void std_insertion_sort_mailrefs(struct MailRef *first, struct MailRef *last)
{
    struct MailRef *mid, *hole, *prev, val;

    if (first == last)
        return;
    for (mid = first + 1; mid != last; ++mid) {
        val = *mid;
        if (rodex_inbox_mail_before(&val, first)) {
            for (hole = mid; hole != first; --hole)
                *hole = hole[-1];
            *first = val;
        } else {
            for (hole = mid, prev = hole - 1; rodex_inbox_mail_before(&val, prev); hole = prev, --prev)
                *hole = *prev;
            *hole = val;
        }
    }
}

void std_pop_heap_hole_mailrefs(struct MailRef *first, int hole, int bottom, const struct MailRef *value)
{
    int top = hole, idx = hole, last_parent = (bottom - 1) >> 1;

    while (idx < last_parent) {
        idx = 2 * idx + 2;
        if (rodex_inbox_mail_before(&first[idx], &first[idx - 1]))
            --idx;
        first[hole] = first[idx];
        hole = idx;
    }
    if (idx == last_parent && bottom % 2 == 0) {
        first[hole] = first[bottom - 1];
        hole = bottom - 1;
    }
    for (idx = (hole - 1) >> 1; top < hole && rodex_inbox_mail_before(&first[idx], value); idx = (hole - 1) >> 1) {
        first[hole] = first[idx];
        hole = idx;
    }
    first[hole] = *value;
}

void std_make_heap_mailrefs(struct MailRef *first, struct MailRef *last)
{
    int bottom = (int)(last - first), hole;
    struct MailRef val;

    for (hole = bottom >> 1; hole > 0;) {
        --hole;
        val = first[hole];
        std_pop_heap_hole_mailrefs(first, hole, bottom, &val);
    }
}

void std_med3_mailrefs(struct MailRef *first, struct MailRef *mid, struct MailRef *last)
{
    if (rodex_inbox_mail_before(mid, first))
        swap_refs(mid, first);
    if (rodex_inbox_mail_before(last, mid)) {
        swap_refs(last, mid);
        if (rodex_inbox_mail_before(mid, first))
            swap_refs(mid, first);
    }
}

struct MailRefRange *std_partition_mailrefs(struct MailRefRange *out, struct MailRef *first,
                                                    struct MailRef *last)
{
    struct MailRef *mid = first + ((last - first) >> 1);
    struct MailRef *top = last - 1;
    struct MailRef *pfirst, *plast, *gfirst, *glast;
    int count = (int)(top - first);

    if (count > 40) {
        int step = (count + 1) >> 3, two_step = step << 1;

        std_med3_mailrefs(first, first + step, first + two_step);
        std_med3_mailrefs(mid - step, mid, mid + step);
        std_med3_mailrefs(top - two_step, top - step, top);
        std_med3_mailrefs(first + step, mid, top - step);
    } else {
        std_med3_mailrefs(first, mid, top);
    }
    pfirst = mid;
    plast = pfirst + 1;
    while (first < pfirst && !rodex_inbox_mail_before(pfirst - 1, pfirst) && !rodex_inbox_mail_before(pfirst, pfirst - 1))
        --pfirst;
    while (plast < last && !rodex_inbox_mail_before(plast, pfirst) && !rodex_inbox_mail_before(pfirst, plast))
        ++plast;
    gfirst = plast;
    glast = pfirst;
    for (;;) {
        for (; gfirst < last; ++gfirst) {
            if (rodex_inbox_mail_before(pfirst, gfirst))
                continue;
            if (rodex_inbox_mail_before(gfirst, pfirst))
                break;
            if (plast != gfirst)
                swap_refs(plast, gfirst);
            ++plast;
        }
        for (; first < glast; --glast) {
            if (rodex_inbox_mail_before(glast - 1, pfirst))
                continue;
            if (rodex_inbox_mail_before(pfirst, glast - 1))
                break;
            if (--pfirst != glast - 1)
                swap_refs(pfirst, glast - 1);
        }
        if (glast == first && gfirst == last) {
            out->first = pfirst;
            out->last = plast;
            return out;
        }
        if (glast == first) {
            if (plast != gfirst)
                swap_refs(pfirst, plast);
            ++plast;
            swap_refs(pfirst, gfirst);
            ++pfirst;
            ++gfirst;
        } else if (gfirst == last) {
            if (--glast != --pfirst)
                swap_refs(glast, pfirst);
            swap_refs(pfirst, --plast);
        } else {
            swap_refs(gfirst, --glast);
            ++gfirst;
        }
    }
}

void std_sort_mailrefs(struct MailRef *first, struct MailRef *last, int ideal)
{
    struct MailRefRange mid;
    struct MailRef val;

    while (last - first > 32) {
        if (ideal <= 0) {
            std_make_heap_mailrefs(first, last);
            for (; last - first >= 2; --last) {
                val = last[-1];
                last[-1] = *first;
                std_pop_heap_hole_mailrefs(first, 0, (int)(last - 1 - first), &val);
            }
            return;
        }
        std_partition_mailrefs(&mid, first, last);
        ideal = (ideal >> 1) + (ideal >> 2);
        if (mid.first - first < last - mid.last) {
            std_sort_mailrefs(first, mid.first, ideal);
            first = mid.last;
        } else {
            std_sort_mailrefs(mid.last, last, ideal);
            last = mid.first;
        }
    }
    if (last - first >= 2)
        std_insertion_sort_mailrefs(first, last);
}
