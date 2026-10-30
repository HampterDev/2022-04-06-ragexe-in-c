#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contents/party_booking_storage.h"
#include "owned_storage_test_allocator.h"

static void *checked_calloc(size_t count, size_t size)
{
    void *result;
    assert(!size || count <= (size_t)-1 / size);
    result = checked_malloc(count * size);
    if (result) memset(result, 0, count * size);
    return result;
}

#define malloc checked_malloc
#define calloc checked_calloc
#define free checked_free
#include "../src/core/owned_text.c"
#include "../src/contents/party_booking_storage.c"
#undef free
#undef calloc
#undef malloc

static struct PartySearchRow make_row(void)
{
    struct PartySearchRow row = {0};
    const char name[] = "한글\0이름";
    assert(owned_text_assign(&row.time, text_view_cstr(" 11 : 03 ")));
    assert(owned_text_assign(&row.leader, text_view(name, sizeof name - 1)));
    assert(owned_text_assign(&row.levels, text_view_cstr("12~27")));
    assert(owned_text_assign(&row.place, text_view_cstr("지역 지도")));
    assert(owned_text_assign(&row.jobs, text_view_cstr("검사,마법사")));
    return row;
}

static void check_text(const struct OwnedText *a, const struct OwnedText *b)
{
    assert(a->size == b->size);
    assert(!memcmp(owned_text_cstr(a), owned_text_cstr(b), a->size + 1));
    if (a != b && a->size) assert(a->data != b->data);
}

static void check_row(const struct PartySearchRow *a, const struct PartySearchRow *b)
{
    check_text(&a->time, &b->time); check_text(&a->leader, &b->leader);
    check_text(&a->levels, &b->levels); check_text(&a->place, &b->place);
    check_text(&a->jobs, &b->jobs);
    assert(a->is_more_row == b->is_more_row);
}

static void check_rows(void)
{
    struct PartySearchRows rows = {0};
    struct PartySearchRow value = make_row(), copy = {0};
    struct PartySearchEntry before[4], *storage;
    int keys[] = {INT_MAX, 0, INT_MIN, -2};
    int sorted[] = {INT_MIN, -2, 0, INT_MAX};
    unsigned i, live, calls;
    assert(sizeof rows == 8 && sizeof value == 44);
    assert(!party_search_rows_at(&rows, 0) && !party_search_rows_after(&rows, INT_MIN));
    for (i = 0; i < 4; ++i) assert(party_search_rows_insert(&rows, keys[i], &value));
    for (i = 0; i < 4; ++i) {
        assert(party_search_rows_at(&rows, i)->key == sorted[i]);
        check_row(&party_search_rows_find(&rows, sorted[i])->row, &value);
    }
    assert(!party_search_rows_at(&rows, UINT_MAX));
    assert(!party_search_rows_after(&rows, INT_MAX));
    memcpy(before, rows.items, sizeof before); storage = rows.items; live = outstanding;
    for (i = 1; i <= 6; ++i) {
        fail_at = allocation_calls + i;
        assert(!party_search_rows_insert(&rows, 3, &rows.items[0].row));
        fail_at = 0;
        assert(rows.items == storage && rows.size == 4 && outstanding == live);
        assert(!memcmp(before, rows.items, sizeof before));
    }
    calls = allocation_calls;
    value.is_more_row = 1;
    fail_at = calls + 1;
    assert(party_search_rows_insert(&rows, 0, &value));
    assert(allocation_calls == calls && !party_search_rows_find(&rows, 0)->row.is_more_row);
    fail_at = 0;
    assert(party_search_row_copy(&copy, &value));
    live = outstanding;
    for (i = 1; i <= 5; ++i) {
        fail_at = allocation_calls + i;
        assert(!party_search_row_copy(&copy, &rows.items[0].row));
        fail_at = 0;
        assert(outstanding == live); check_row(&copy, &value);
    }
    calls = allocation_calls;
    assert(party_search_row_copy(&copy, &copy) && allocation_calls == calls);
    assert(party_search_rows_insert(&rows, 3, &rows.items[0].row));
    check_row(&party_search_rows_find(&rows, 3)->row, &party_search_rows_find(&rows, INT_MIN)->row);
    assert(party_search_rows_remove(&rows, -2));
    assert(!party_search_rows_remove(&rows, -2));
    assert(party_search_rows_after(&rows, -2)->key == 0);
    party_search_row_clear(&copy); party_search_row_clear(&copy);
    party_search_rows_clear(&rows); party_search_rows_clear(&rows);
    party_search_row_clear(&value);
    assert(outstanding == 0);
}

static void check_maps(void)
{
    struct PartySearchMaps maps = {0}, copy = {0};
    struct PartySearchRegions regions = {0};
    struct PartySearchMap before[2], *storage;
    const struct PartySearchMaps *view;
    struct PartySearchRegion region_before;
    unsigned i, calls, live;
    const char name[] = "지도\0한글";
    assert(sizeof maps == 8 && sizeof regions == 8 && sizeof(struct PartySearchMap) == 12);
    assert(!party_search_map_at(&maps, 0) && !party_search_region_maps(&regions, 2));
    assert(party_search_maps_append(&maps, text_view(name, sizeof name - 1), 0x123456));
    assert(party_search_maps_append(&maps, owned_text_view(&maps.items[0].name), 0xaabbcc));
    assert(maps.items[0].name.size == sizeof name - 1);
    assert(!memcmp(maps.items[0].name.data, name, sizeof name));
    check_text(&maps.items[0].name, &maps.items[1].name);
    assert(!party_search_map_at(&maps, 2) && !party_search_map_at(&maps, UINT_MAX));
    memcpy(before, maps.items, sizeof before); storage = maps.items; live = outstanding;
    for (i = 1; i <= 2; ++i) {
        fail_at = allocation_calls + i;
        assert(!party_search_maps_append(&maps, owned_text_view(&maps.items[0].name), 9));
        fail_at = 0;
        assert(maps.items == storage && maps.size == 2 && outstanding == live);
        assert(!memcmp(before, maps.items, sizeof before));
    }
    assert(party_search_maps_copy(&copy, &maps));
    live = outstanding; storage = copy.items;
    for (i = 1; i <= 3; ++i) {
        fail_at = allocation_calls + i;
        assert(!party_search_maps_copy(&copy, &maps));
        fail_at = 0;
        assert(outstanding == live && copy.items == storage && copy.size == 2);
        check_text(&copy.items[0].name, &maps.items[0].name);
        assert(copy.items[0].color == 0x123456 && copy.items[1].color == 0xaabbcc);
    }
    calls = allocation_calls;
    assert(party_search_maps_copy(&copy, &copy) && allocation_calls == calls);
    assert(party_search_regions_insert(&regions, 2, &maps));
    region_before = regions.items[0]; live = outstanding;
    for (i = 1; i <= 4; ++i) {
        fail_at = allocation_calls + i;
        assert(!party_search_regions_insert(&regions, -3, &regions.items[0].maps));
        fail_at = 0;
        assert(outstanding == live && regions.size == 1);
        assert(!memcmp(&region_before, regions.items, sizeof region_before));
    }
    calls = allocation_calls;
    assert(party_search_regions_insert(&regions, 2, &copy) && allocation_calls == calls);
    assert(party_search_regions_insert(&regions, -3, &regions.items[0].maps));
    view = party_search_region_maps(&regions, -3);
    assert(view && regions.items[0].key == -3 && regions.items[1].key == 2);
    check_text(&view->items[0].name, &maps.items[0].name);
    assert(!party_search_region_maps(&regions, 0));
    party_search_maps_clear(&copy); party_search_maps_clear(&copy);
    party_search_maps_clear(&maps);
    party_search_regions_clear(&regions); party_search_regions_clear(&regions);
    assert(outstanding == 0);
}

static void check_edits(void)
{
    struct PartySearchRows rows = {0};
    struct PartySearchRow value = {0};
    int present[81] = {0};
    char expected[81][16] = {{0}};
    unsigned seed = 123, i;
    for (i = 0; i < 4000; ++i) {
        int at, j, count = 0;
        char text[16];
        seed = seed * 1664525u + 1013904223u;
        at = (seed >> 8) % 81;
        if (seed & 1) {
            snprintf(text, sizeof text, "%u", i);
            assert(owned_text_assign(&value.time, text_view_cstr(text)));
            assert(party_search_rows_insert(&rows, at - 40, &value));
            if (!present[at]) strcpy(expected[at], text);
            present[at] = 1;
        } else {
            assert(party_search_rows_remove(&rows, at - 40) == present[at]);
            present[at] = 0;
        }
        for (j = 0; j < 81; ++j) {
            struct PartySearchEntry *entry = party_search_rows_find(&rows, j - 40);
            assert(!!entry == present[j]);
            if (present[j]) {
                assert(rows.items[count++].key == j - 40);
                assert(!strcmp(owned_text_cstr(&entry->row.time), expected[j]));
            }
        }
        assert(rows.size == (unsigned)count);
    }
    party_search_row_clear(&value);
    party_search_rows_clear(&rows);
    assert(outstanding == 0);
}

int main(void)
{
    check_rows(); check_maps(); check_edits();
    puts("Party-search storage: counted Korean text, signed row order, duplicates, aliased copies, all allocation failures, map colors, bounds and 4000 edit sequences passed");
    return 0;
}
