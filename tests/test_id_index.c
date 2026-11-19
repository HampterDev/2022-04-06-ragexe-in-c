/* world/id_index.c against the walk it replaces: a list of entries kept beside the index, appended
   at its end, removed anywhere and given other ids, where for every id the index's first entry is
   the one a walk of the list from its start finds first. Seeded runs of up to a million steps over few
   ids (many entries share one) and over many (the table grows, probes wrap and removed ids'
   followers move back), with the table grown as the index asks; then the edges: an empty index, a
   table without room, an entry not in the index, ids that share a start in the table, and the
   capacity past 2^31. */
#include "world/id_index.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Entry {
    struct IdLink link;
    uint32_t id;
    int listed;
};

/* the list: entries in the order they were appended; removed ones stay as holes until the list is
   copied */
static struct Entry *entries;
static size_t count, capacity, listed;
static struct IdIndex index_;
static uint64_t rng;

static uint32_t next_random(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 7;
    rng ^= rng << 17;
    return (uint32_t)(rng >> 32);
}

static void room(void)
{
    uint32_t wanted = id_index_capacity(&index_);
    struct IdSlot *old = index_.slots, *slots;

    assert(wanted);
    if (wanted == index_.capacity)
        return;
    slots = malloc(wanted * sizeof *slots);
    assert(slots);
    memset(slots, 0xcd, wanted * sizeof *slots); /* the index sets them */
    id_index_rehash(&index_, slots, wanted);
    free(old);
}

static struct Entry *walk(uint32_t id)
{
    size_t i;
    for (i = 0; i < count; i++)
        if (entries[i].listed && entries[i].id == id)
            return &entries[i];
    return NULL;
}

static void check_id(uint32_t id)
{
    struct IdLink *link = id_index_first(&index_, id);
    assert((link ? ID_INDEX_ENTRY(link, struct Entry, link) : NULL) == walk(id));
}

/* the chains hold exactly the listed entries, each chain in the list's order, and the index counts
   the ids that have entries */
static void check_all(void)
{
    uint32_t i, ids = 0;
    size_t chained = 0;

    for (i = 0; i < index_.capacity; i++) {
        const struct IdSlot *slot = &index_.slots[i];
        const struct IdLink *link, *prev = NULL;

        if (!slot->first)
            continue;
        ids++;
        assert(id_index_first(&index_, slot->id) == slot->first);
        for (link = slot->first; link; prev = link, link = link->next) {
            const struct Entry *e = ID_INDEX_ENTRY(link, const struct Entry, link);
            assert(link->indexed && e->listed && e->id == slot->id && link->id == slot->id);
            assert(link->prev == prev);
            assert(!prev || (prev->order < link->order && ID_INDEX_ENTRY(prev, const struct Entry, link) < e));
            chained++;
        }
        assert(slot->last == prev);
    }
    assert(ids == index_.ids);
    assert(chained == listed);
    assert(!index_.capacity || (uint64_t)index_.ids * 2 <= index_.capacity);
}

static void append(uint32_t id)
{
    struct Entry *e;

    if (count == capacity) {
        /* the links point into the entries, so a new list is filled in order and the index made
           again from it, as a list that is copied */
        struct Entry *grown;
        size_t i, kept = 0;

        capacity = listed * 2 > 1024 ? listed * 2 : 1024;
        grown = calloc(capacity, sizeof *grown);
        assert(grown);
        id_index_clear(&index_);
        for (i = 0; i < count; i++)
            if (entries[i].listed) {
                grown[kept] = entries[i];
                room();
                assert(id_index_append(&index_, &grown[kept].link, grown[kept].id));
                kept++;
            }
        free(entries);
        entries = grown;
        count = kept;
    }
    e = &entries[count++];
    memset(e, 0x5a, sizeof *e); /* the index sets the link */
    e->id = id;
    e->listed = 1;
    listed++;
    room();
    assert(id_index_append(&index_, &e->link, id));
}

static struct Entry *any_listed(void)
{
    size_t tries;
    for (tries = 0; tries < 64 && count; tries++) {
        struct Entry *e = &entries[next_random() % count];
        if (e->listed)
            return e;
    }
    return NULL;
}

/* `steps` random steps over the ids below `ids`, the list held near `target` entries */
static void run(unsigned seed, uint32_t ids, size_t steps, size_t target)
{
    size_t step;

    rng = 0x9e3779b97f4a7c15u ^ ((uint64_t)seed << 32 | seed);
    free(entries);
    entries = NULL;
    count = capacity = listed = 0;
    free(index_.slots);
    id_index_init(&index_);
    for (step = 0; step < steps; step++) {
        uint32_t r = next_random() % 100, id = next_random() % ids;
        struct Entry *e;

        if (r < (listed < target ? 50u : 30u)) {
            append(id);
        } else if (r < 80) {
            if ((e = any_listed()) != NULL) {
                id_index_remove(&index_, &e->link);
                assert(!e->link.indexed);
                e->listed = 0;
                listed--;
                check_id(e->id);
            }
        } else if (r < 92) {
            if ((e = any_listed()) != NULL) {
                uint32_t old = e->id;
                room();
                assert(id_index_set_id(&index_, &e->link, id));
                e->id = id;
                check_id(old);
            }
        }
        check_id(id);
        if (step % 4 == 0)
            check_id((uint32_t)(next_random() % ((uint64_t)ids + ids / 4 + 1)));
        if (step % 4096 == 0)
            check_all();
        if (step % 250000 == 249999) {
            /* the list emptied at once (the world's objects cleared) */
            id_index_clear(&index_);
            count = listed = 0;
            check_all();
        }
    }
    check_all();
}

/* the slot an id lands in, in an empty table */
static uint32_t home_of(struct IdIndex *index, uint32_t id)
{
    struct IdLink probe;
    uint32_t i;

    id_index_clear(index);
    assert(id_index_append(index, &probe, id));
    for (i = 0; i < index->capacity && index->slots[i].first != &probe; i++)
        ;
    id_index_clear(index);
    return i;
}

static void edges(void)
{
    static struct Entry many[9];
    struct IdIndex index;
    struct IdSlot slots[16];
    struct Entry a, b, c, d;
    uint32_t i, start, shared[3];
    int found;

    id_index_init(&index);
    assert(!id_index_first(&index, 0) && !id_index_first(&index, 0xffffffffu));
    assert(id_index_capacity(&index) == 16);
    /* no table: nothing goes in */
    memset(&a, 0, sizeof a);
    assert(!id_index_append(&index, &a.link, 7) && !a.link.indexed);
    id_index_remove(&index, &a.link);
    assert(id_index_set_id(&index, &a.link, 9) && a.link.id == 9 && !a.link.indexed);
    id_index_rehash(&index, slots, 16);
    assert(index.capacity == 16 && index.ids == 0);
    /* ids 0 and 0xffffffff are ids like any other */
    assert(id_index_append(&index, &a.link, 0) && id_index_first(&index, 0) == &a.link);
    assert(id_index_append(&index, &b.link, 0xffffffffu) && id_index_first(&index, 0xffffffffu) == &b.link);
    /* one id: its first entry is the one appended first, also where an earlier one changes to it */
    assert(id_index_append(&index, &c.link, 5) && id_index_append(&index, &d.link, 6));
    assert(id_index_set_id(&index, &a.link, 6) && id_index_first(&index, 6) == &a.link);
    assert(a.link.next == &d.link && d.link.prev == &a.link && !id_index_first(&index, 0));
    assert(id_index_set_id(&index, &d.link, 5) && id_index_first(&index, 5) == &c.link);
    assert(c.link.next == &d.link && index.ids == 3);
    id_index_remove(&index, &c.link);
    assert(id_index_first(&index, 5) == &d.link && index.ids == 3);
    id_index_remove(&index, &d.link);
    assert(!id_index_first(&index, 5) && index.ids == 2);
    id_index_remove(&index, &d.link); /* twice: nothing */
    assert(index.ids == 2);
    /* the table half full: a new id is refused and nothing changes; a known one still goes in */
    id_index_clear(&index);
    for (i = 0; i < 8; i++)
        assert(id_index_append(&index, &many[i].link, 100 + i));
    assert(id_index_capacity(&index) == 32);
    assert(!id_index_append(&index, &many[8].link, 200) && !many[8].link.indexed);
    assert(id_index_append(&index, &many[8].link, 103) && id_index_first(&index, 103) == &many[3].link);
    assert(!id_index_set_id(&index, &many[0].link, 201) && many[0].link.id == 100);
    assert(id_index_first(&index, 100) == &many[0].link);
    assert(id_index_set_id(&index, &many[0].link, 107) && id_index_first(&index, 107) == &many[0].link);
    assert(!id_index_first(&index, 100) && index.ids == 7);
    /* three ids that start at one slot: removing the first moves the others back to its slot */
    start = home_of(&index, 0);
    shared[0] = 0;
    for (found = 1, i = 1; found < 3; i++)
        if (home_of(&index, i) == start)
            shared[found++] = i;
    assert(id_index_append(&index, &a.link, shared[0]) && id_index_append(&index, &b.link, shared[1]));
    assert(id_index_append(&index, &c.link, shared[2]));
    assert(index.slots[start].first == &a.link);
    id_index_remove(&index, &a.link);
    assert(id_index_first(&index, shared[1]) == &b.link && id_index_first(&index, shared[2]) == &c.link);
    assert(index.slots[start].first == &b.link);
    id_index_remove(&index, &b.link);
    assert(id_index_first(&index, shared[2]) == &c.link && index.slots[start].first == &c.link);
    assert(index.ids == 1);
    /* the capacity past 2^31 */
    index.capacity = 0x80000000u;
    index.ids = 0x3fffffffu;
    assert(id_index_capacity(&index) == 0x80000000u);
    index.ids = 0x40000000u;
    assert(id_index_capacity(&index) == 0);
}

int main(void)
{
    edges();
    run(1, 4, 1000000, 64);          /* few ids: long chains, ids changed into crowded ones */
    run(2, 64, 400000, 512);         /* ids that come and go */
    run(3, 4096, 300000, 1000);      /* many ids: the table grows, probes wrap, followers move */
    run(4, 0xffffffffu, 300000, 1000); /* ids over the whole range */
    free(entries);
    free(index_.slots);
    printf("id_index: ok\n");
    return 0;
}
