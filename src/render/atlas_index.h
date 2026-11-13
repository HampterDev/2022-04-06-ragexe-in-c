#ifndef RENDER_ATLAS_INDEX_H
#define RENDER_ATLAS_INDEX_H

#include <stddef.h>

/* The sprite atlas's cells by what they hold: a sprite frame's image, the palette it was drawn
   with and the size bucket it was placed in, to the cell holding them, found in constant time. An
   open-addressed table (linear probing, removal by shifting back) in storage the caller gives; it
   holds at most three quarters of its slots, so an insert into a full index is refused and the
   caller grows it (atlas_index_move into larger storage). Pure: no allocation, no clock. */

struct AtlasIndexSlot {
    const void *image; /* NULL: a free slot */
    const void *palette;
    void *cell;
    int bucket;
};

struct AtlasIndex {
    struct AtlasIndexSlot *slots;
    size_t capacity; /* 0, or a power of two */
    size_t count;
};

/* capacity is rounded down to a power of two; slots may be NULL with capacity 0. */
void atlas_index_init(struct AtlasIndex *index, struct AtlasIndexSlot *slots, size_t capacity);
void *atlas_index_find(const struct AtlasIndex *index, const void *image, const void *palette, int bucket);
int atlas_index_full(const struct AtlasIndex *index);
/* 1 when the cell is kept for the key (a key already kept takes the new cell); 0 when image is
   NULL or the index is full. */
int atlas_index_insert(struct AtlasIndex *index, const void *image, const void *palette, int bucket,
                       void *cell);
void atlas_index_remove(struct AtlasIndex *index, const void *image, const void *palette, int bucket);
/* Every key of from into to, which must be empty and hold them; from is left as it was. */
void atlas_index_move(struct AtlasIndex *to, const struct AtlasIndex *from);

#endif
