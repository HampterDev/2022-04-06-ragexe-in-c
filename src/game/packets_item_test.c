#include "game/packet_handlers.h"
#include "std/tree.h"
#include "platform/win_types.h"
#include <stdio.h>
#include <string.h>
#include "core/client_path.h"
#include "platform/filesystem.h"
#include "game/gamemode.h"
#include "game/item_info.h"
#include "core/unique_file.h"
#include "game/box_report.h"
#include <errno.h>
#include "ui/items/styling_shop.h"
#include "ui/ui_std.h"
#include "core/app.h"
#include "platform/clock.h"

#pragma pack(push, 1)
struct PackageItemTestEntry {
    int item_id;
    unsigned short amount;
    int count;
};
struct PACKET_ZC_RESULT_PACKAGE_ITEM_TEST {
    unsigned short packet_type;
    short packet_length;
    int package_item_id;
    struct PackageItemTestEntry records[];
};
#pragma pack(pop)
SIZE_ASSERT(PackageItemTestEntry, 10);
OFFSET_ASSERT(PACKET_ZC_RESULT_PACKAGE_ITEM_TEST, records, 8);

void game_mode_zc_result_package_item_test(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_RESULT_PACKAGE_ITEM_TEST *packet = (const struct PACKET_ZC_RESULT_PACKAGE_ITEM_TEST *)pkt;
    const char *package_name;
    char path[0x800];
    struct ItemInfo item;
    struct PlatformDateTime now;
    FILE *f;
    const struct PackageItemTestEntry *entry = packet->records;
    int n = 0, records, total, written;

    (void)self;

    item_info_construct(&item);
    item_info_set_item_id(&item, packet->package_item_id);
    package_name = item_info_get_name(&item);
    if (!client_path(path, sizeof path, "PackageItemTest")) goto done;
    if (!platform_path_exists(path) && platform_directory_create(path))
        goto done;
    platform_date_time_local(&now);
    if (!client_path(path, sizeof path, "PackageItemTest\\%s(%d_%d_%d).txt", package_name, now.year, now.month,
                     now.day))
        goto done;
    if (platform_path_exists(path)) {
        do {
            n++;
            if (!client_path(path, sizeof path, "PackageItemTest\\%s(%d_%d_%d)_%d.txt", package_name, now.year,
                             now.month, now.day, n))
                goto done;
        } while (platform_path_exists(path));
    }
    f = fopen(path, "w");
    if (!f)
        goto done;
    records = (int)((unsigned)(packet->packet_length - 8) / 10);
    total = 0;
    fprintf(f, "============================START============================\n");
    for (; records > 0; records--, entry++) {
        const char *name;
        struct PackageItemTestEntry e = *entry;

        item_info_set_item_id(&item, e.item_id);
        name = item_info_get_name(&item);
        fprintf(f, "%s\t\t\t%d\t%c%d%c\n", name, e.count, '(', e.amount, ')');
        total += e.count;
    }
    fprintf(f, "============================END============================\n");
    fprintf(f, "Unpacked %d Packages\n", total);
    fclose(f);
done:
    item_info_destruct(&item);
}

struct BoxTestRecord {
    int item_id;
    int count;
};
SIZE_ASSERT(BoxTestRecord, 8);

#pragma pack(push, 1)
struct PACKET_ZC_DDUKDDAK_BOX_TEST {
    unsigned short packet_type;
    short packet_length;
    int box_item_id;
    struct BoxTestRecord records[];
};
#pragma pack(pop)
OFFSET_ASSERT(PACKET_ZC_DDUKDDAK_BOX_TEST, records, 8);

static int box_less(const struct BoxTestRecord *a, const struct BoxTestRecord *b)
{
    if (a->count != b->count)
        return a->count > b->count;
    return a->item_id < b->item_id;
}

static void box_swap(struct BoxTestRecord *a, struct BoxTestRecord *b)
{
    struct BoxTestRecord t = *a;

    *a = *b;
    *b = t;
}

static void box_med3(struct BoxTestRecord *first, struct BoxTestRecord *mid, struct BoxTestRecord *last)
{
    if (box_less(mid, first))
        box_swap(mid, first);
    if (box_less(last, mid)) {
        box_swap(last, mid);
        if (box_less(mid, first))
            box_swap(mid, first);
    }
}

static void box_partition(struct BoxTestRecord *first, struct BoxTestRecord *last,
                          struct BoxTestRecord **out_first, struct BoxTestRecord **out_last)
{
    struct BoxTestRecord *mid = first + (last - first) / 2, *last1 = last - 1;
    struct BoxTestRecord *pfirst, *plast, *gfirst, *glast;
    int count = (int)(last1 - first);

    if (count > 40) {
        int step = (count + 1) >> 3, two_step = step * 2;

        box_med3(first, first + step, first + two_step);
        box_med3(mid - step, mid, mid + step);
        box_med3(last1 - two_step, last1 - step, last1);
        box_med3(first + step, mid, last1 - step);
    } else {
        box_med3(first, mid, last1);
    }

    pfirst = mid;
    plast = pfirst + 1;
    while (first < pfirst && !box_less(pfirst - 1, pfirst) && !box_less(pfirst, pfirst - 1))
        --pfirst;
    while (plast < last && !box_less(plast, pfirst) && !box_less(pfirst, plast))
        ++plast;
    gfirst = plast;
    glast = pfirst;

    for (;;) {
        for (; gfirst < last; ++gfirst) {
            if (box_less(pfirst, gfirst))
                continue;
            if (box_less(gfirst, pfirst))
                break;
            if (plast != gfirst)
                box_swap(plast, gfirst);
            ++plast;
        }
        for (; first < glast; --glast) {
            if (box_less(glast - 1, pfirst))
                continue;
            if (box_less(pfirst, glast - 1))
                break;
            if (--pfirst != glast - 1)
                box_swap(pfirst, glast - 1);
        }
        if (glast == first && gfirst == last) {
            *out_first = pfirst;
            *out_last = plast;
            return;
        }
        if (glast == first) {
            if (plast != gfirst)
                box_swap(pfirst, plast);
            ++plast;
            box_swap(pfirst, gfirst);
            ++pfirst;
            ++gfirst;
        } else if (gfirst == last) {
            if (--glast != --pfirst)
                box_swap(glast, pfirst);
            box_swap(pfirst, --plast);
        } else {
            box_swap(gfirst, --glast);
            ++gfirst;
        }
    }
}

static void box_pop_heap_hole(struct BoxTestRecord *first, int hole, int bottom, const struct BoxTestRecord *value)
{
    int top = hole, idx = hole, max_non_leaf = (bottom - 1) >> 1;

    while (idx < max_non_leaf) {
        idx = 2 * idx + 2;
        if (box_less(first + idx, first + idx - 1))
            --idx;
        first[hole] = first[idx];
        hole = idx;
    }
    if (idx == max_non_leaf && bottom % 2 == 0) {
        first[hole] = first[bottom - 1];
        hole = bottom - 1;
    }
    while (top < hole) {
        idx = (hole - 1) >> 1;
        if (!box_less(first + idx, value))
            break;
        first[hole] = first[idx];
        hole = idx;
    }
    first[hole] = *value;
}

static void box_insertion_sort(struct BoxTestRecord *first, struct BoxTestRecord *last)
{
    struct BoxTestRecord *next, *hole, value;

    if (last - first < 2)
        return;
    for (next = first + 1; next != last; ++next) {
        value = *next;
        if (box_less(&value, first)) {
            memmove(first + 1, first, (char *)next - (char *)first);
            hole = first;
        } else {
            for (hole = next; box_less(&value, hole - 1); --hole)
                hole[0] = hole[-1];
        }
        *hole = value;
    }
}

static void box_heap_sort(struct BoxTestRecord *first, struct BoxTestRecord *last)
{
    int bottom = (int)(last - first), hole;
    struct BoxTestRecord value, *last1;

    for (hole = bottom >> 1; hole > 0;) {
        --hole;
        value = first[hole];
        box_pop_heap_hole(first, hole, bottom, &value);
    }
    for (last1 = last - 1; last1 - first + 1 >= 2; --last1) {
        value = *last1;
        *last1 = *first;
        box_pop_heap_hole(first, 0, (int)(last1 - first), &value);
    }
}

static void box_sort(struct BoxTestRecord *first, struct BoxTestRecord *last, int ideal)
{
    struct BoxTestRecord *mid_first, *mid_last;

    while (last - first > 32) {
        if (ideal <= 0) {
            box_heap_sort(first, last);
            return;
        }
        box_partition(first, last, &mid_first, &mid_last);
        ideal = (ideal >> 1) + (ideal >> 2);
        if (mid_first - first < last - mid_last) {
            box_sort(first, mid_first, ideal);
            first = mid_last;
        } else {
            box_sort(mid_last, last, ideal);
            last = mid_first;
        }
    }
    box_insertion_sort(first, last);
}

static void box_counts_from_map(struct Vec *v, const struct Tree *counts)
{
    struct TreeNode *it;

    vec_init(v);
    vec_reserve(v, counts->size, sizeof(struct BoxTestRecord));
    for (it = tree_first(counts); it != counts->head; it = tree_next(it))
        vec_push(v, TREE_VALUE(it, struct BoxTestRecord), sizeof(struct BoxTestRecord));
}

static int *box_count_of(struct Tree *counts, struct BoxTestRecord *record)
{
    struct BoxTestRecord *entry = tree_get(counts, &record->item_id, sizeof record->item_id, sizeof *entry,
                                           tree_less_int);

    return &entry->count;
}

void game_mode_zc_ddukddak_box_test(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_DDUKDDAK_BOX_TEST *packet = (const struct PACKET_ZC_DDUKDDAK_BOX_TEST *)pkt;
    const struct BoxTestRecord *records = packet->records;
    struct Tree counts;
    struct Vec sorted;
    struct BoxTestRecord *it, *first, *last;
    struct ItemInfo box, item;
    FILE *file;
    char folder[0x800], path[0x800];
    int written, ok;
    const char *box_name, *item_name;
    struct PlatformDateTime now;
    unsigned n;
    int total = 0;

    (void)self;
    tree_init(&counts);
    n = (unsigned)(packet->packet_length - 8) >> 3;
    for (; n; n--, records++) {
        struct BoxTestRecord record = *records;

        *box_count_of(&counts, &record) += record.count;
        total += record.count;
    }

    box_counts_from_map(&sorted, &counts);
    first = sorted.first;
    last = sorted.last;
    if (first != last)
        box_sort(first, last, (int)(last - first));

    item_info_construct(&box);
    item_info_set_item_id(&box, packet->box_item_id);
    box_name = item_info_get_name(&box);
    platform_date_time_local(&now);
    if (!client_path(folder, sizeof folder, "DdukddakBoxTest"))
        goto cleanup;
    if (platform_directory_create(folder) != 0 && errno != EEXIST)
        goto cleanup;
    if (!client_path(path, sizeof path, "DdukddakBoxTest\\%d%02d%02d_(%s).txt",
                     now.year, now.month, now.day, box_name))
        goto cleanup;
    file = file_open_unique(path);
    if (!file)
        goto cleanup;

    ok = box_report_begin(file);
    for (it = first; ok && it != last; it++) {
        item_info_construct(&item);
        item_info_set_item_id(&item, it->item_id);
        item_name = item_info_get_name(&item);
        ok = box_report_item(file, it->item_id, item_name, it->count, total);
        item_info_destruct(&item);
    }
    if (ok)
        box_report_end(file, total);
    fclose(file);
cleanup:
    item_info_destruct(&box);
    vec_free(&sorted);
    tree_free(&counts, NULL);
}
