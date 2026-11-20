#ifndef WORLD_ID_INDEX_H
#define WORLD_ID_INDEX_H

/* custom: an index from 32-bit ids to the entries of one list, as pure logic (the world's skill
   units by their aid, world/world.c): for an id, the first entry of the list that has it, in the
   list's order, in constant time, kept right as entries are appended to the list, leave it and
   change their id. The list grows only at its end, so the order its entries were appended in is
   its order; several entries may have one id.

   Each entry carries its link (struct IdLink), which also chains the entries of one id in the
   list's order. The caller owns the table of slots: before an entry is appended or given another
   id, it gives the index a table of the capacity id_index_capacity asks for (id_index_rehash).
   No allocation, no I/O. */

#include <stddef.h>
#include <stdint.h>

struct IdLink {
    struct IdLink *next, *prev; /* the entries of the same id, in the list's order */
    uint64_t order;             /* larger for an entry appended later */
    uint32_t id;
    unsigned char indexed;      /* the entry is in the index */
};

/* the entry of type `type` whose IdLink member `member` is `link` */
#define ID_INDEX_ENTRY(link, type, member) ((type *)((char *)(link) - offsetof(type, member)))

struct IdSlot {
    uint32_t id;
    struct IdLink *first, *last; /* first is NULL in a free slot */
};

struct IdIndex {
    struct IdSlot *slots;
    uint32_t capacity; /* 0 or a power of two */
    uint32_t ids;      /* slots in use */
    uint64_t appended; /* the order of the next entry appended */
};

void id_index_init(struct IdIndex *index);
/* The capacity the table needs so that one more id fits with the table at most half full: its own
   while that holds, otherwise the smallest power of two from 16 on that does; 0 past 2^31. */
uint32_t id_index_capacity(const struct IdIndex *index);
/* Moves the ids into `slots`, a table of `capacity` (a power of two that holds them at most half
   full) whose contents need not be set. The old table is the caller's to free. */
void id_index_rehash(struct IdIndex *index, struct IdSlot *slots, uint32_t capacity);
/* The entry of `link`, now the last of the list, with `id`. 0, and the entry left out, where its
   id is new and the table has not the capacity id_index_capacity asks for. */
int id_index_append(struct IdIndex *index, struct IdLink *link, uint32_t id);
/* The entry leaves the list; nothing for an entry that is not in the index. */
void id_index_remove(struct IdIndex *index, struct IdLink *link);
/* The entry has another id and keeps its place in the list; an entry not in the index only takes
   the id. 0, and nothing changed, where the id is new and the table has not the capacity
   id_index_capacity asks for. */
int id_index_set_id(struct IdIndex *index, struct IdLink *link, uint32_t id);
/* The first entry with `id`, in the list's order, or NULL. */
struct IdLink *id_index_first(const struct IdIndex *index, uint32_t id);
/* Forgets every entry without reading their links: only for when the list's entries are gone. */
void id_index_clear(struct IdIndex *index);

#endif
