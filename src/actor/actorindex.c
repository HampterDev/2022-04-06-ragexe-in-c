#define WIN32_LEAN_AND_MEAN
#include "platform/win_types.h"
#include "diag/perf.h"
#include "world/actor_lookup.h"

#include "actor/types.h"
#include "std/list.h"
#include "world/state.h"

#define INDEX_SLOTS 4096

struct actor_entry {
    struct World *world;
    DWORD aid;
    const struct ListNode *node;
    struct GameActor *actor;
    DWORD generation;
};

static struct actor_entry entries[INDEX_SLOTS];
static unsigned entry_count;
DWORD actor_list_generation;

static unsigned slot_of(struct World *world, DWORD aid)
{
    return ((aid ^ ((DWORD)(ULONG_PTR)world >> 4)) * 2654435761u) >> 20;
}

static struct actor_entry *index_find(struct World *world, DWORD aid)
{
    unsigned h = slot_of(world, aid), i;
    for (i = 0; i < 8; i++) {
        struct actor_entry *e = &entries[(h + i) & (INDEX_SLOTS - 1)];
        if (!e->world)
            return NULL;
        if (e->world == world && e->aid == aid)
            return e;
    }
    return NULL;
}

static void index_store(struct World *world, DWORD aid, const struct ListNode *node, DWORD generation)
{
    unsigned h = slot_of(world, aid), i;
    if (entry_count >= INDEX_SLOTS / 2)
        return;
    for (i = 0; i < 8; i++) {
        struct actor_entry *e = &entries[(h + i) & (INDEX_SLOTS - 1)];
        if (!e->world) {
            e->world = world;
            e->aid = aid;
            e->node = node;
            e->actor = node ? *LIST_VALUE(node, struct GameActor *) : NULL;
            e->generation = generation;
            entry_count++;
            return;
        }
    }
}

static void index_drop(struct actor_entry *e)
{
    unsigned i = (unsigned)(e - entries), j;
    e->world = NULL;
    entry_count--;
    for (j = (i + 1) & (INDEX_SLOTS - 1); entries[j].world; j = (j + 1) & (INDEX_SLOTS - 1)) {
        struct actor_entry moved = entries[j];
        entries[j].world = NULL;
        entry_count--;
        index_store(moved.world, moved.aid, moved.node, moved.generation);
    }
}

struct GameActor *world_get_game_actor_by_aid(struct World *self, unsigned aid)
{
    struct actor_entry *e;
    struct ListNode *head, *node;

    g_perf.actorLookups++;
    e = index_find(self, aid);
    if (e) {
        if (!e->node) {
            if (e->generation == actor_list_generation) {
                g_perf.actorIndexHits++;
                return NULL;
            }
        } else if (e->actor->aid == aid) {
            g_perf.actorIndexHits++;
            return e->actor;
        }
        index_drop(e);
    }
    head = self->actors.head;
    for (node = head->next; node != head; node = node->next) {
        struct GameActor *actor = *LIST_VALUE(node, struct GameActor *);

        if (actor->aid == aid) {
            index_store(self, aid, node, actor_list_generation);
            return actor;
        }
    }
    index_store(self, aid, NULL, actor_list_generation);
    return NULL;
}

void actor_index_drop_node(const struct ListNode *node)
{
    unsigned i;
    if (entry_count)
        for (i = 0; i < INDEX_SLOTS; i++)
            if (entries[i].world && entries[i].node == node) {
                index_drop(&entries[i]);
                i = (unsigned)-1;
            }
}

void actor_index_clear(void)
{
    if (entry_count) {
        unsigned i;
        for (i = 0; i < INDEX_SLOTS; i++)
            entries[i].world = NULL;
        entry_count = 0;
    }
}
