#include "world/id_index.h"

/* An open-addressed table of the ids with linear probing, at most half full, from which a removed
   id's followers move back so that no probe ever passes a hole; each slot holds the chain of its
   id's entries, which the entries link themselves. */

/* lowbias32 (Chris Wellons): the server's ids, which come in runs, spread over the table */
static uint32_t hash(uint32_t id)
{
    id ^= id >> 16;
    id *= 0x7feb352du;
    id ^= id >> 15;
    id *= 0x846ca68bu;
    id ^= id >> 16;
    return id;
}

static int has_room(const struct IdIndex *index)
{
    return index->capacity && (uint64_t)(index->ids + 1) * 2 <= index->capacity;
}

static struct IdSlot *find(const struct IdIndex *index, uint32_t id)
{
    uint32_t mask, i;

    if (!index->capacity)
        return NULL;
    mask = index->capacity - 1;
    for (i = hash(id) & mask; index->slots[i].first; i = (i + 1) & mask)
        if (index->slots[i].id == id)
            return &index->slots[i];
    return NULL;
}

/* the slot of an id that has none yet: free until its chain is set */
static struct IdSlot *claim(struct IdIndex *index, uint32_t id)
{
    uint32_t mask = index->capacity - 1, i;

    for (i = hash(id) & mask; index->slots[i].first; i = (i + 1) & mask)
        ;
    index->slots[i].id = id;
    index->slots[i].last = NULL;
    index->ids++;
    return &index->slots[i];
}

/* frees a slot whose chain is empty; the slots after it that may stand nearer their own start
   move back into the hole */
static void release(struct IdIndex *index, struct IdSlot *slot)
{
    uint32_t mask = index->capacity - 1, hole = (uint32_t)(slot - index->slots), i = hole;

    index->ids--;
    for (;;) {
        uint32_t home;

        i = (i + 1) & mask;
        if (!index->slots[i].first)
            break;
        home = hash(index->slots[i].id) & mask;
        /* the slot stays where its start lies cyclically after the hole and not after the slot */
        if (hole <= i ? home > hole && home <= i : home > hole || home <= i)
            continue;
        index->slots[hole] = index->slots[i];
        hole = i;
    }
    index->slots[hole].first = NULL;
    index->slots[hole].last = NULL;
}

/* takes the link out of its id's chain, the slot too where that empties it */
static void unlink_entry(struct IdIndex *index, struct IdSlot *slot, struct IdLink *link)
{
    if (link->prev)
        link->prev->next = link->next;
    else
        slot->first = link->next;
    if (link->next)
        link->next->prev = link->prev;
    else
        slot->last = link->prev;
    link->next = link->prev = NULL;
    if (!slot->first)
        release(index, slot);
}

/* puts the link into its id's chain by its order: after the last entry appended before it */
static void link_entry(struct IdSlot *slot, struct IdLink *link)
{
    struct IdLink *after = slot->last;

    while (after && after->order > link->order)
        after = after->prev;
    link->prev = after;
    link->next = after ? after->next : slot->first;
    if (after)
        after->next = link;
    else
        slot->first = link;
    if (link->next)
        link->next->prev = link;
    else
        slot->last = link;
}

void id_index_init(struct IdIndex *index)
{
    index->slots = NULL;
    index->capacity = 0;
    index->ids = 0;
    index->appended = 0;
}

uint32_t id_index_capacity(const struct IdIndex *index)
{
    uint64_t capacity = index->capacity ? index->capacity : 16;

    while ((uint64_t)(index->ids + 1) * 2 > capacity)
        capacity *= 2;
    return capacity > 0x80000000u ? 0 : (uint32_t)capacity;
}

void id_index_rehash(struct IdIndex *index, struct IdSlot *slots, uint32_t capacity)
{
    const struct IdSlot *old = index->slots;
    uint32_t mask = capacity - 1, i;

    for (i = 0; i < capacity; i++) {
        slots[i].id = 0;
        slots[i].first = slots[i].last = NULL;
    }
    for (i = 0; i < index->capacity; i++) {
        uint32_t j;

        if (!old[i].first)
            continue;
        for (j = hash(old[i].id) & mask; slots[j].first; j = (j + 1) & mask)
            ;
        slots[j] = old[i];
    }
    index->slots = slots;
    index->capacity = capacity;
}

int id_index_append(struct IdIndex *index, struct IdLink *link, uint32_t id)
{
    struct IdSlot *slot = find(index, id);

    if (!slot) {
        if (!has_room(index))
            return 0;
        slot = claim(index, id);
    }
    link->id = id;
    link->order = index->appended++;
    link->next = NULL;
    link->prev = slot->last;
    if (slot->last)
        slot->last->next = link;
    else
        slot->first = link;
    slot->last = link;
    link->indexed = 1;
    return 1;
}

void id_index_remove(struct IdIndex *index, struct IdLink *link)
{
    struct IdSlot *slot;

    if (!link->indexed)
        return;
    link->indexed = 0;
    slot = find(index, link->id);
    if (slot)
        unlink_entry(index, slot, link);
}

int id_index_set_id(struct IdIndex *index, struct IdLink *link, uint32_t id)
{
    struct IdSlot *slot;

    if (!link->indexed || link->id == id) {
        link->id = id;
        return 1;
    }
    if (!find(index, id) && !has_room(index))
        return 0;
    slot = find(index, link->id);
    if (slot)
        unlink_entry(index, slot, link);
    link->id = id;
    slot = find(index, id);
    if (!slot)
        slot = claim(index, id);
    link_entry(slot, link);
    return 1;
}

struct IdLink *id_index_first(const struct IdIndex *index, uint32_t id)
{
    const struct IdSlot *slot = find(index, id);

    return slot ? slot->first : NULL;
}

void id_index_clear(struct IdIndex *index)
{
    uint32_t i;

    for (i = 0; i < index->capacity; i++)
        index->slots[i].first = index->slots[i].last = NULL;
    index->ids = 0;
}
