#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "render/atlas_index.h"

/* A reference index: the kept keys in a list, searched from the start. */
struct Kept { const void *image, *palette; int bucket; void *cell; };
static struct Kept kept[8192];
static size_t kept_count;

static struct Kept *kept_find(const void *image, const void *palette, int bucket)
{
    size_t i;
    for (i = 0; i < kept_count; ++i)
        if (kept[i].image == image && kept[i].palette == palette && kept[i].bucket == bucket) return &kept[i];
    return NULL;
}

static unsigned random_state;
static unsigned next_random(void)
{
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
}

static char images[64][48], palettes[4][1024], cells[16];

static void basics(void)
{
    static struct AtlasIndexSlot storage[8], larger[16];
    struct AtlasIndex index, grown, none;
    int i;

    atlas_index_init(&none, NULL, 8);
    assert(!none.capacity && !atlas_index_find(&none, images[0], palettes[0], 0));
    assert(!atlas_index_insert(&none, images[0], palettes[0], 0, cells));
    atlas_index_remove(&none, images[0], palettes[0], 0);
    atlas_index_init(&index, storage, 11);
    assert(index.capacity == 8 && !index.count);
    assert(!atlas_index_insert(&index, NULL, palettes[0], 0, cells) && !atlas_index_find(&index, NULL, palettes[0], 0));
    assert(atlas_index_insert(&index, images[0], palettes[0], 3, cells + 1));
    assert(atlas_index_find(&index, images[0], palettes[0], 3) == cells + 1);
    assert(!atlas_index_find(&index, images[0], palettes[0], 4));
    assert(!atlas_index_find(&index, images[0], palettes[1], 3));
    assert(!atlas_index_find(&index, images[1], palettes[0], 3));
    assert(atlas_index_insert(&index, images[0], palettes[0], 3, cells + 2) && index.count == 1);
    assert(atlas_index_find(&index, images[0], palettes[0], 3) == cells + 2);
    for (i = 1; index.count < 6; ++i) {
        assert(!atlas_index_full(&index));
        assert(atlas_index_insert(&index, images[i], palettes[i & 3], i % 25, cells + i));
    }
    assert(atlas_index_full(&index) && !atlas_index_insert(&index, images[40], palettes[0], 0, cells));
    atlas_index_remove(&index, images[40], palettes[0], 0);
    assert(index.count == 6);
    atlas_index_remove(&index, images[0], palettes[0], 3);
    assert(index.count == 5 && !atlas_index_find(&index, images[0], palettes[0], 3));
    for (i = 1; i < 6; ++i) assert(atlas_index_find(&index, images[i], palettes[i & 3], i % 25) == cells + i);
    atlas_index_init(&grown, larger, 16);
    atlas_index_move(&grown, &index);
    assert(grown.count == 5);
    for (i = 1; i < 6; ++i) assert(atlas_index_find(&grown, images[i], palettes[i & 3], i % 25) == cells + i);
}

/* Inserts, removals and growth against the reference, over few images, palettes and buckets in
   a small table, so that probe runs are long and removals shift entries back across the wrap. */
static void fuzz(unsigned seed, size_t first_capacity, unsigned image_count)
{
    static struct AtlasIndexSlot storage_a[8192], storage_b[8192];
    struct AtlasIndexSlot *storage = storage_a, *spare = storage_b;
    struct AtlasIndex index;
    unsigned step;

    random_state = seed;
    kept_count = 0;
    atlas_index_init(&index, storage, first_capacity);
    for (step = 0; step < 200000; ++step) {
        const void *image = images[next_random() % image_count];
        const void *palette = palettes[next_random() % 4];
        int bucket = (int)(next_random() % 3);
        void *cell = cells + next_random() % 16;
        unsigned op = next_random() % 100;
        struct Kept *reference = kept_find(image, palette, bucket);
        size_t i;

        if (op < 45) {
            int expected;
            while (!reference && atlas_index_full(&index) && index.capacity < 8192) {
                struct AtlasIndex grown;
                struct AtlasIndexSlot *swap = storage;
                atlas_index_init(&grown, spare, index.capacity * 2);
                atlas_index_move(&grown, &index);
                assert(grown.count == index.count);
                index = grown;
                storage = spare;
                spare = swap;
            }
            expected = reference || kept_count + 1 <= index.capacity / 4 * 3;
            assert(atlas_index_insert(&index, image, palette, bucket, cell) == expected);
            if (expected) {
                if (!reference) {
                    reference = &kept[kept_count++];
                    reference->image = image;
                    reference->palette = palette;
                    reference->bucket = bucket;
                }
                reference->cell = cell;
            }
        } else if (op < 90) {
            atlas_index_remove(&index, image, palette, bucket);
            if (reference) *reference = kept[--kept_count];
        }
        reference = kept_find(image, palette, bucket);
        assert(atlas_index_find(&index, image, palette, bucket) == (reference ? reference->cell : NULL));
        assert(index.count == kept_count && index.count <= index.capacity / 4 * 3);
        if (step % 1024 == 0 || op >= 45)
            for (i = 0; i < kept_count; ++i)
                assert(atlas_index_find(&index, kept[i].image, kept[i].palette, kept[i].bucket) == kept[i].cell);
    }
}

int main(void)
{
    basics();
    fuzz(0x2545f491u, 8, 8);
    fuzz(0x9e3779b9u, 1, 64);
    fuzz(0x87654321u, 64, 64);
    puts("Atlas index: cells by image, palette and bucket, a full index refused, removal by shifting back, "
         "growth by moving, and 600000 fuzzed steps against a list passed.");
    return 0;
}
