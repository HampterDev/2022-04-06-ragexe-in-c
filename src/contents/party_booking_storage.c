#include "contents/party_booking_storage.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

void party_search_row_clear(struct PartySearchRow *row)
{
    owned_text_clear(&row->jobs);
    owned_text_clear(&row->place);
    owned_text_clear(&row->levels);
    owned_text_clear(&row->leader);
    owned_text_clear(&row->time);
    row->is_more_row = 0;
}

int party_search_row_copy(struct PartySearchRow *out, const struct PartySearchRow *source)
{
    struct PartySearchRow copy = {0};
    if (out == source)
        return 1;
    if (!owned_text_assign(&copy.time, owned_text_view(&source->time)) ||
        !owned_text_assign(&copy.leader, owned_text_view(&source->leader)) ||
        !owned_text_assign(&copy.levels, owned_text_view(&source->levels)) ||
        !owned_text_assign(&copy.place, owned_text_view(&source->place)) ||
        !owned_text_assign(&copy.jobs, owned_text_view(&source->jobs))) {
        party_search_row_clear(&copy);
        return 0;
    }
    copy.is_more_row = source->is_more_row;
    party_search_row_clear(out);
    *out = copy;
    return 1;
}

static unsigned row_lower_bound(const struct PartySearchRows *rows, int key)
{
    unsigned first = 0, last = rows->size;
    while (first < last) {
        unsigned at = first + (last - first) / 2;
        if (rows->items[at].key < key)
            first = at + 1;
        else
            last = at;
    }
    return first;
}

void party_search_rows_clear(struct PartySearchRows *rows)
{
    unsigned i;
    for (i = 0; i < rows->size; ++i)
        party_search_row_clear(&rows->items[i].row);
    free(rows->items);
    *rows = (struct PartySearchRows){0};
}

int party_search_rows_insert(struct PartySearchRows *rows, int key, const struct PartySearchRow *row)
{
    unsigned at = row_lower_bound(rows, key);
    struct PartySearchRow copy = {0};
    struct PartySearchEntry *items;
    if (at < rows->size && rows->items[at].key == key)
        return 1;
    if (rows->size >= INT_MAX || (size_t)rows->size + 1 > (size_t)-1 / sizeof *items)
        return 0;
    if (!party_search_row_copy(&copy, row))
        return 0;
    items = malloc(((size_t)rows->size + 1) * sizeof *items);
    if (!items) {
        party_search_row_clear(&copy);
        return 0;
    }
    if (at)
        memcpy(items, rows->items, at * sizeof *items);
    if (at < rows->size)
        memcpy(items + at + 1, rows->items + at, (rows->size - at) * sizeof *items);
    items[at].key = key;
    items[at].row = copy;
    free(rows->items);
    rows->items = items;
    ++rows->size;
    return 1;
}

int party_search_rows_remove(struct PartySearchRows *rows, int key)
{
    unsigned at = row_lower_bound(rows, key);
    if (at == rows->size || rows->items[at].key != key)
        return 0;
    party_search_row_clear(&rows->items[at].row);
    if (at + 1 < rows->size)
        memmove(rows->items + at, rows->items + at + 1, (rows->size - at - 1) * sizeof *rows->items);
    --rows->size;
    if (!rows->size) {
        free(rows->items);
        rows->items = NULL;
    }
    return 1;
}

struct PartySearchEntry *party_search_rows_find(struct PartySearchRows *rows, int key)
{
    unsigned at = row_lower_bound(rows, key);
    return at < rows->size && rows->items[at].key == key ? &rows->items[at] : NULL;
}

const struct PartySearchEntry *party_search_rows_at(const struct PartySearchRows *rows, unsigned index)
{
    return index < rows->size ? &rows->items[index] : NULL;
}

const struct PartySearchEntry *party_search_rows_after(const struct PartySearchRows *rows, int key)
{
    unsigned at = row_lower_bound(rows, key);
    if (at < rows->size && rows->items[at].key == key)
        ++at;
    return party_search_rows_at(rows, at);
}

void party_search_maps_clear(struct PartySearchMaps *maps)
{
    unsigned i;
    for (i = 0; i < maps->size; ++i)
        owned_text_clear(&maps->items[i].name);
    free(maps->items);
    *maps = (struct PartySearchMaps){0};
}

int party_search_maps_copy(struct PartySearchMaps *out, const struct PartySearchMaps *source)
{
    struct PartySearchMaps copy = {0};
    unsigned i;
    if (out == source)
        return 1;
    if (source->size) {
        if ((size_t)source->size > (size_t)-1 / sizeof *copy.items)
            return 0;
        copy.items = calloc(source->size, sizeof *copy.items);
        if (!copy.items)
            return 0;
        copy.size = source->size;
        for (i = 0; i < source->size; ++i) {
            if (!owned_text_assign(&copy.items[i].name, owned_text_view(&source->items[i].name))) {
                party_search_maps_clear(&copy);
                return 0;
            }
            copy.items[i].color = source->items[i].color;
        }
    }
    party_search_maps_clear(out);
    *out = copy;
    return 1;
}

int party_search_maps_append(struct PartySearchMaps *maps, struct TextView name, unsigned color)
{
    struct OwnedText copy = {0};
    struct PartySearchMap *items;
    if (maps->size >= INT_MAX || (size_t)maps->size + 1 > (size_t)-1 / sizeof *items)
        return 0;
    if (!owned_text_assign(&copy, name))
        return 0;
    items = malloc(((size_t)maps->size + 1) * sizeof *items);
    if (!items) {
        owned_text_clear(&copy);
        return 0;
    }
    if (maps->size)
        memcpy(items, maps->items, maps->size * sizeof *items);
    items[maps->size].name = copy;
    items[maps->size].color = color;
    free(maps->items);
    maps->items = items;
    ++maps->size;
    return 1;
}

static unsigned region_lower_bound(const struct PartySearchRegions *regions, int key)
{
    unsigned first = 0, last = regions->size;
    while (first < last) {
        unsigned at = first + (last - first) / 2;
        if (regions->items[at].key < key)
            first = at + 1;
        else
            last = at;
    }
    return first;
}

void party_search_regions_clear(struct PartySearchRegions *regions)
{
    unsigned i;
    for (i = 0; i < regions->size; ++i)
        party_search_maps_clear(&regions->items[i].maps);
    free(regions->items);
    *regions = (struct PartySearchRegions){0};
}

int party_search_regions_insert(struct PartySearchRegions *regions, int key, const struct PartySearchMaps *maps)
{
    unsigned at = region_lower_bound(regions, key);
    struct PartySearchMaps copy = {0};
    struct PartySearchRegion *items;
    if (at < regions->size && regions->items[at].key == key)
        return 1;
    if (regions->size >= INT_MAX || (size_t)regions->size + 1 > (size_t)-1 / sizeof *items)
        return 0;
    if (!party_search_maps_copy(&copy, maps))
        return 0;
    items = malloc(((size_t)regions->size + 1) * sizeof *items);
    if (!items) {
        party_search_maps_clear(&copy);
        return 0;
    }
    if (at)
        memcpy(items, regions->items, at * sizeof *items);
    if (at < regions->size)
        memcpy(items + at + 1, regions->items + at, (regions->size - at) * sizeof *items);
    items[at].key = key;
    items[at].maps = copy;
    free(regions->items);
    regions->items = items;
    ++regions->size;
    return 1;
}

const struct PartySearchMaps *party_search_region_maps(const struct PartySearchRegions *regions, int key)
{
    unsigned at = region_lower_bound(regions, key);
    return at < regions->size && regions->items[at].key == key ? &regions->items[at].maps : NULL;
}

const struct PartySearchMap *party_search_map_at(const struct PartySearchMaps *maps, unsigned index)
{
    return maps && index < maps->size ? &maps->items[index] : NULL;
}
