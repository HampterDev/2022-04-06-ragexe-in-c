#include "world/aid_index.h"
#include "actors/types.h"
#include "core/memory.h"
#include "diag/perf.h"
#include "std/list.h"
#include "world/actor_lookup.h"
#include "world/state.h"
#include <stdlib.h>

void world_index_room(struct IdIndex *index)
{
    uint32_t wanted = id_index_capacity(index);
    struct IdSlot *old = index->slots, *slots;

    if (wanted == index->capacity)
        return;
    if (!wanted)
        abort(); /* 2^30 aids: each holds a node of a list, so the memory ran out long before */
    slots = mem_alloc((size_t)wanted * sizeof *slots);
    id_index_rehash(index, slots, wanted);
    if (old)
        mem_free(old);
}

static void index_free(struct IdIndex *index)
{
    if (index->slots)
        mem_free(index->slots);
    id_index_init(index);
}

void world_init_aid_lists(struct World *self)
{
    list_init(&self->actors);
    id_index_init(&self->actorIndex);
    list_init(&self->skills);
    id_index_init(&self->skillIndex);
    list_init(&self->pendingSkills);
}

void world_clear_aid_lists(struct World *self)
{
    list_clear(&self->actors, NULL);
    id_index_clear(&self->actorIndex);
    list_clear(&self->skills, NULL);
    id_index_clear(&self->skillIndex);
    list_clear(&self->pendingSkills, NULL);
}

void world_free_aid_lists(struct World *self)
{
    list_free(&self->pendingSkills, NULL);
    list_free(&self->skills, NULL);
    index_free(&self->skillIndex);
    list_free(&self->actors, NULL);
    index_free(&self->actorIndex);
}

struct GameActor *world_get_game_actor_by_aid(struct World *self, unsigned aid)
{
    struct IdLink *link;

    g_perf.actorLookups++;
    if (!self)
        return NULL;
    link = id_index_first(&self->actorIndex, (uint32_t)aid);
    return link ? ID_INDEX_ENTRY(link, struct ActorEntry, aidLink)->actor : NULL;
}

void world_set_actor_aid(struct World *self, struct GameActor *actor, int aid)
{
    if (actor->actor_node) {
        world_index_room(&self->actorIndex);
        id_index_set_id(&self->actorIndex, &LIST_VALUE(actor->actor_node, struct ActorEntry)->aidLink,
                        (uint32_t)aid);
    }
    actor->aid = aid;
}
