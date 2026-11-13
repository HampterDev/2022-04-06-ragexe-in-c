#include "render/atlas_index.h"
#include <stdint.h>
#include <string.h>

static size_t home_of(const struct AtlasIndex *index, const void *image, const void *palette, int bucket)
{
    uint64_t hash = (uint64_t)(uintptr_t)image * 0x9e3779b97f4a7c15ull;

    hash ^= (uint64_t)(uintptr_t)palette * 0xc2b2ae3d27d4eb4full;
    hash ^= (uint64_t)(unsigned)bucket * 0x165667b19e3779f9ull;
    hash ^= hash >> 29;
    return (size_t)hash & (index->capacity - 1);
}

static int same_key(const struct AtlasIndexSlot *slot, const void *image, const void *palette, int bucket)
{
    return slot->image == image && slot->palette == palette && slot->bucket == bucket;
}

static size_t slot_of(const struct AtlasIndex *index, const void *image, const void *palette, int bucket)
{
    size_t mask = index->capacity - 1, slot = home_of(index, image, palette, bucket);

    while (index->slots[slot].image && !same_key(&index->slots[slot], image, palette, bucket))
        slot = (slot + 1) & mask;
    return slot;
}

static size_t power_of_two_floor(size_t value)
{
    size_t power = 1;

    if (!value)
        return 0;
    while (power <= value / 2)
        power *= 2;
    return power;
}

void atlas_index_init(struct AtlasIndex *index, struct AtlasIndexSlot *slots, size_t capacity)
{
    index->slots = slots;
    index->capacity = slots ? power_of_two_floor(capacity) : 0;
    index->count = 0;
    if (index->capacity)
        memset(slots, 0, index->capacity * sizeof *slots);
}

void *atlas_index_find(const struct AtlasIndex *index, const void *image, const void *palette, int bucket)
{
    const struct AtlasIndexSlot *slot;

    if (!index->capacity || !image)
        return NULL;
    slot = &index->slots[slot_of(index, image, palette, bucket)];
    return slot->image ? slot->cell : NULL;
}

int atlas_index_full(const struct AtlasIndex *index)
{
    return index->count + 1 > index->capacity / 4 * 3;
}

int atlas_index_insert(struct AtlasIndex *index, const void *image, const void *palette, int bucket,
                       void *cell)
{
    struct AtlasIndexSlot *slot;

    if (!index->capacity || !image)
        return 0;
    slot = &index->slots[slot_of(index, image, palette, bucket)];
    if (!slot->image) {
        if (atlas_index_full(index))
            return 0;
        slot->image = image;
        slot->palette = palette;
        slot->bucket = bucket;
        ++index->count;
    }
    slot->cell = cell;
    return 1;
}

void atlas_index_remove(struct AtlasIndex *index, const void *image, const void *palette, int bucket)
{
    size_t mask, hole, next;

    if (!index->capacity || !image)
        return;
    mask = index->capacity - 1;
    hole = slot_of(index, image, palette, bucket);
    if (!index->slots[hole].image)
        return;
    --index->count;
    for (next = (hole + 1) & mask; index->slots[next].image; next = (next + 1) & mask) {
        const struct AtlasIndexSlot *moving = &index->slots[next];
        size_t home = home_of(index, moving->image, moving->palette, moving->bucket);
        int stays = hole <= next ? hole < home && home <= next : hole < home || home <= next;

        if (!stays) {
            index->slots[hole] = *moving;
            hole = next;
        }
    }
    memset(&index->slots[hole], 0, sizeof index->slots[hole]);
}

void atlas_index_move(struct AtlasIndex *to, const struct AtlasIndex *from)
{
    size_t i;

    for (i = 0; i < from->capacity; ++i) {
        const struct AtlasIndexSlot *slot = &from->slots[i];

        if (slot->image) {
            to->slots[slot_of(to, slot->image, slot->palette, slot->bucket)] = *slot;
            ++to->count;
        }
    }
}
