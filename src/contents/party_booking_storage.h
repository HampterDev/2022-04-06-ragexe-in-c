#ifndef CONTENTS_PARTY_BOOKING_STORAGE_H
#define CONTENTS_PARTY_BOOKING_STORAGE_H

#include "core/owned_text.h"

struct PartySearchRow {
    struct OwnedText time;
    struct OwnedText leader;
    struct OwnedText levels;
    struct OwnedText place;
    struct OwnedText jobs;
    int is_more_row;
};

struct PartySearchEntry {
    int key;
    struct PartySearchRow row;
};

struct PartySearchRows {
    struct PartySearchEntry *items;
    unsigned size;
};

void party_search_row_clear(struct PartySearchRow *row);
int party_search_row_copy(struct PartySearchRow *out, const struct PartySearchRow *source);
void party_search_rows_clear(struct PartySearchRows *rows);
int party_search_rows_insert(struct PartySearchRows *rows, int key, const struct PartySearchRow *row);
int party_search_rows_remove(struct PartySearchRows *rows, int key);
struct PartySearchEntry *party_search_rows_find(struct PartySearchRows *rows, int key);
const struct PartySearchEntry *party_search_rows_at(const struct PartySearchRows *rows, unsigned index);
const struct PartySearchEntry *party_search_rows_after(const struct PartySearchRows *rows, int key);

struct PartySearchMap {
    struct OwnedText name;
    unsigned color;
};

struct PartySearchMaps {
    struct PartySearchMap *items;
    unsigned size;
};

struct PartySearchRegion {
    int key;
    struct PartySearchMaps maps;
};

struct PartySearchRegions {
    struct PartySearchRegion *items;
    unsigned size;
};

void party_search_maps_clear(struct PartySearchMaps *maps);
int party_search_maps_copy(struct PartySearchMaps *out, const struct PartySearchMaps *source);
int party_search_maps_append(struct PartySearchMaps *maps, struct TextView name, unsigned color);
void party_search_regions_clear(struct PartySearchRegions *regions);
int party_search_regions_insert(struct PartySearchRegions *regions, int key, const struct PartySearchMaps *maps);
const struct PartySearchMaps *party_search_region_maps(const struct PartySearchRegions *regions, int key);
const struct PartySearchMap *party_search_map_at(const struct PartySearchMaps *maps, unsigned index);

#endif
