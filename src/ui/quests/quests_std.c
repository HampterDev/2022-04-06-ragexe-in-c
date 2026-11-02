#include <errno.h>
#include "core/fatal.h"
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include "ui/ui_std.h"
#include "game/rodex_mail.h"
#include "ui/items/styling_shop_std.h"
#include "ui/quests/achievement.h"
#include "session/session_achievements.h"

void ach_pairs_clear(struct Vec *v)
{
    struct AchPair *pair;

    for (pair = v->first; pair != v->last; pair++)
        achievement_info_destruct(&pair->info);
    vec_clear(v);
}

void ach_pairs_free(struct Vec *v)
{
    ach_pairs_clear(v);
    vec_free(v);
}

struct AchPair *ach_pair_copy_construct(struct AchPair *dest, const int *id, const struct AchievementInfo *info)
{
    dest->id = *id;
    achievement_info_copy(&dest->info, info);
    return dest;
}

static int ach_sort_key(int id)
{
    struct SessionAchievementState state;

    return session_get_achievement(&g_session1, &state, id)->completed_at;
}

static int ach_less(int a, int b)
{
    return ach_sort_key(a) < ach_sort_key(b);
}

static void ach_swap(int *a, int *b)
{
    int t = *a;
    *a = *b;
    *b = t;
}

static void ach_med3(int *first, int *mid, int *last)
{
    if (ach_less(*mid, *first))
        ach_swap(mid, first);
    if (ach_less(*last, *mid)) {
        ach_swap(last, mid);
        if (ach_less(*mid, *first))
            ach_swap(mid, first);
    }
}

static void ach_partition(int *first, int *last, int **pfirst_out, int **plast_out)
{
    int *mid = first + ((last - first) >> 1);
    int *back = last - 1;
    int count = (int)(back - first);
    int *pfirst, *plast, *gfirst, *glast;

    if (count > 40) {
        int step = (count + 1) >> 3;
        int two_step = step << 1;
        ach_med3(first, first + step, first + two_step);
        ach_med3(mid - step, mid, mid + step);
        ach_med3(back - two_step, back - step, back);
        ach_med3(first + step, mid, back - step);
    } else {
        ach_med3(first, mid, back);
    }

    pfirst = mid;
    plast = pfirst + 1;
    while (first < pfirst && !ach_less(pfirst[-1], *pfirst) && !ach_less(*pfirst, pfirst[-1]))
        --pfirst;
    while (plast < last && !ach_less(*plast, *pfirst) && !ach_less(*pfirst, *plast))
        ++plast;

    gfirst = plast;
    glast = pfirst;
    for (;;) {
        for (; gfirst < last; ++gfirst) {
            if (ach_less(*pfirst, *gfirst))
                continue;
            if (ach_less(*gfirst, *pfirst))
                break;
            if (plast != gfirst)
                ach_swap(plast, gfirst);
            ++plast;
        }
        for (; first < glast; --glast) {
            if (ach_less(glast[-1], *pfirst))
                continue;
            if (ach_less(*pfirst, glast[-1]))
                break;
            if (--pfirst != glast - 1)
                ach_swap(pfirst, glast - 1);
        }
        if (glast == first && gfirst == last) {
            *pfirst_out = pfirst;
            *plast_out = plast;
            return;
        }
        if (glast == first) {
            if (plast != gfirst)
                ach_swap(pfirst, plast);
            ++plast;
            ach_swap(pfirst, gfirst);
            ++pfirst;
            ++gfirst;
        } else if (gfirst == last) {
            if (--glast != --pfirst)
                ach_swap(glast, pfirst);
            ach_swap(pfirst, --plast);
        } else {
            ach_swap(gfirst, --glast);
            ++gfirst;
        }
    }
}

static void ach_pop_heap_hole(int *first, int hole, int bottom, int val)
{
    int top = hole;
    int idx = hole;
    int max_non_leaf = (bottom - 1) >> 1;

    while (idx < max_non_leaf) {
        idx = 2 * idx + 2;
        if (ach_less(first[idx], first[idx - 1]))
            --idx;
        first[hole] = first[idx];
        hole = idx;
    }
    if (idx == max_non_leaf && bottom % 2 == 0) {
        first[hole] = first[bottom - 1];
        hole = bottom - 1;
    }
    for (idx = (hole - 1) >> 1; top < hole && ach_less(first[idx], val); idx = (hole - 1) >> 1) {
        first[hole] = first[idx];
        hole = idx;
    }
    first[hole] = val;
}

static void ach_sort_unchecked(int *first, int *last, int ideal)
{
    int *pfirst, *plast, *mid, *hole, *prev;
    int count, val, i;

    for (;;) {
        count = (int)(last - first);
        if (count <= 32)
            break;
        if (ideal <= 0) {
            for (i = count >> 1; i > 0;) {
                --i;
                ach_pop_heap_hole(first, i, count, first[i]);
            }
            for (; last - first >= 2; --last) {
                val = last[-1];
                last[-1] = *first;
                ach_pop_heap_hole(first, 0, (int)(last - 1 - first), val);
            }
            return;
        }
        ach_partition(first, last, &pfirst, &plast);
        ideal = (ideal >> 1) + (ideal >> 2);
        if (pfirst - first < last - plast) {
            ach_sort_unchecked(first, pfirst, ideal);
            first = plast;
        } else {
            ach_sort_unchecked(plast, last, ideal);
            last = pfirst;
        }
    }
    if (count < 2 || first == last)
        return;
    for (mid = first + 1; mid != last; ++mid) {
        val = *mid;
        if (ach_less(val, *first)) {
            memmove(first + 1, first, (char *)mid - (char *)first);
            *first = val;
        } else {
            for (hole = mid, prev = mid - 1; ach_less(val, *prev); hole = prev, --prev)
                *hole = *prev;
            *hole = val;
        }
    }
}

void ach_sort_ids(int *first, int *last)
{
    ach_sort_unchecked(first, last, (int)(last - first));
}
