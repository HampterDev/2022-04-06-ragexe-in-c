#include <string.h>
#include "std/std.h"
#include "game/guild_storage.h"

static void copy_plain_fields(struct GuildStorageLogEntry *to, const struct GuildStorageLogEntry *from)
{
    to->id = from->id;
    to->amount = from->amount;
    to->action = from->action;
    memcpy(to->name, from->name, sizeof to->name);
    memcpy(to->time, from->time, sizeof to->time);
}

static void copy_tail_fields(struct GuildStorageLogEntry *to, const struct GuildStorageLogEntry *from)
{
    to->identified = from->identified;
    to->item_type = from->item_type;
    to->refine = from->refine;
    memcpy(to->cards, from->cards, sizeof to->cards);
    to->attribute = from->attribute;
    to->grade = from->grade;
}

struct GuildStorageLogEntry *guild_log_entry_construct(struct GuildStorageLogEntry *entry)
{
    entry->id = 0;
    entry->amount = 0;
    entry->action = 0;
    string_init(&entry->item_name);
    entry->grade = 0;
    entry->identified = 0;
    entry->item_type = 0;
    entry->refine = 0;
    entry->attribute = 0;
    memset(entry->name, 0, sizeof entry->name);
    memset(entry->time, 0, sizeof entry->time);
    memset(entry->cards, 0, sizeof entry->cards);
    return entry;
}

void guild_log_entry_construct_copy(struct GuildStorageLogEntry *where,
                                         const struct GuildStorageLogEntry *from)
{
    copy_plain_fields(where, from);
    string_copy(&where->item_name, &from->item_name);
    copy_tail_fields(where, from);
}

struct GuildStorageLogEntry *guild_log_entry_construct_move(struct GuildStorageLogEntry *entry,
                                                                    struct GuildStorageLogEntry *from)
{
    copy_plain_fields(entry, from);
    string_move(&entry->item_name, &from->item_name);
    copy_tail_fields(entry, from);
    return entry;
}

typedef struct GuildStorageLogEntry LogEntry;

#define LOG_ISORT_MAX 32
#define LOG_NINTHER_MIN 40

static int log_newer(const LogEntry *a, const LogEntry *b)
{
    return strcmp(a->time, b->time) > 0;
}

static void log_move_assign(LogEntry *to, LogEntry *from)
{
    copy_plain_fields(to, from);
    string_free(&to->item_name);
    string_move(&to->item_name, &from->item_name);
    copy_tail_fields(to, from);
}

static void log_swap(LogEntry *a, LogEntry *b)
{
    LogEntry tmp;

    guild_log_entry_construct_move(&tmp, a);
    log_move_assign(a, b);
    log_move_assign(b, &tmp);
    string_free(&tmp.item_name);
}

static void log_insertion_sort(LogEntry *first, LogEntry *last)
{
    LogEntry *mid, *hole, *prev, val;

    if (first == last)
        return;
    for (mid = first + 1; mid != last; mid++) {
        guild_log_entry_construct_move(&val, mid);
        if (log_newer(&val, first)) {
            for (hole = mid; hole != first; hole--)
                log_move_assign(hole, hole - 1);
            log_move_assign(first, &val);
        } else {
            for (hole = mid, prev = hole - 1; log_newer(&val, prev); hole = prev, prev--)
                log_move_assign(hole, prev);
            log_move_assign(hole, &val);
        }
        string_free(&val.item_name);
    }
}

static void log_med3(LogEntry *first, LogEntry *mid, LogEntry *last)
{
    if (log_newer(mid, first))
        log_swap(mid, first);
    if (log_newer(last, mid)) {
        log_swap(last, mid);
        if (log_newer(mid, first))
            log_swap(mid, first);
    }
}

static void log_partition(LogEntry *first, LogEntry *last, LogEntry **out_first, LogEntry **out_last)
{
    LogEntry *mid = first + (last - first) / 2;
    LogEntry *pfirst, *plast, *gfirst, *glast;
    int count = (int)(last - 1 - first);

    if (count > LOG_NINTHER_MIN) {
        int step = (count + 1) >> 3, two_step = step << 1;

        log_med3(first, first + step, first + two_step);
        log_med3(mid - step, mid, mid + step);
        log_med3(last - 1 - two_step, last - 1 - step, last - 1);
        log_med3(first + step, mid, last - 1 - step);
    } else {
        log_med3(first, mid, last - 1);
    }

    pfirst = mid;
    plast = pfirst + 1;
    while (first < pfirst && !log_newer(pfirst - 1, pfirst) && !log_newer(pfirst, pfirst - 1))
        pfirst--;
    while (plast < last && !log_newer(plast, pfirst) && !log_newer(pfirst, plast))
        plast++;
    gfirst = plast;
    glast = pfirst;

    for (;;) {
        for (; gfirst < last; gfirst++) {
            if (log_newer(pfirst, gfirst))
                continue;
            if (log_newer(gfirst, pfirst))
                break;
            if (plast != gfirst)
                log_swap(plast, gfirst);
            plast++;
        }
        for (; first < glast; glast--) {
            if (log_newer(glast - 1, pfirst))
                continue;
            if (log_newer(pfirst, glast - 1))
                break;
            if (--pfirst != glast - 1)
                log_swap(pfirst, glast - 1);
        }
        if (glast == first && gfirst == last) {
            *out_first = pfirst;
            *out_last = plast;
            return;
        }
        if (glast == first) {
            if (plast != gfirst)
                log_swap(pfirst, plast);
            plast++;
            log_swap(pfirst, gfirst);
            pfirst++;
            gfirst++;
        } else if (gfirst == last) {
            if (--glast != --pfirst)
                log_swap(glast, pfirst);
            log_swap(pfirst, --plast);
        } else {
            log_swap(gfirst, --glast);
            gfirst++;
        }
    }
}

static void log_heap_hole(LogEntry *first, int hole, int bottom, LogEntry *val)
{
    int top = hole, idx = hole;
    int max_non_leaf = (bottom - 1) >> 1;

    while (idx < max_non_leaf) {
        idx = 2 * idx + 2;
        if (log_newer(first + idx, first + idx - 1))
            idx--;
        log_move_assign(first + hole, first + idx);
        hole = idx;
    }
    if (idx == max_non_leaf && bottom % 2 == 0) {
        log_move_assign(first + hole, first + bottom - 1);
        hole = bottom - 1;
    }
    for (idx = (hole - 1) >> 1; top < hole && log_newer(first + idx, val); idx = (hole - 1) >> 1) {
        log_move_assign(first + hole, first + idx);
        hole = idx;
    }
    log_move_assign(first + hole, val);
}

void guild_log_sort(LogEntry *first, LogEntry *last, int ideal, void *pred)
{
    LogEntry *mid_first, *mid_last, val;
    int count, hole;

    while ((count = (int)(last - first)) > LOG_ISORT_MAX && ideal > 0) {
        log_partition(first, last, &mid_first, &mid_last);
        ideal = (ideal >> 1) + (ideal >> 2);
        if (mid_first - first < last - mid_last) {
            guild_log_sort(first, mid_first, ideal, pred);
            first = mid_last;
        } else {
            guild_log_sort(mid_last, last, ideal, pred);
            last = mid_first;
        }
    }
    if (count > LOG_ISORT_MAX) {
        for (hole = count >> 1; hole > 0;) {
            hole--;
            guild_log_entry_construct_move(&val, first + hole);
            log_heap_hole(first, hole, count, &val);
            string_free(&val.item_name);
        }
        for (; last - first >= 2; last--) {
            guild_log_entry_construct_move(&val, last - 1);
            log_move_assign(last - 1, first);
            log_heap_hole(first, 0, (int)(last - 1 - first), &val);
            string_free(&val.item_name);
        }
    } else if (count >= 2) {
        log_insertion_sort(first, last);
    }
}

void **std_make_move_iterator(void **out, void *in)
{
    *out = in;
    return out;
}
