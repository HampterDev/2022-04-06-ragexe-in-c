#include "effects/rag/rag_effect.h"
#include "world/state.h"
#include "std/list.h"

struct EffectPrim *rag_launch_prim(struct RagEffect *self, int kind, float dx, float dy, float dz)
{
    struct World *world = game_current_world();
    struct EffectPrim *prim;
    Vec3 offset = { dx, dy, dz };

    /* nothing is made for a world whose lists are gone (effect_add_world_object would refuse it) */
    if (!world || !world->gameObjects.head) {
        self->last_prim = NULL;
        return NULL;
    }
    prim = mem_alloc(sizeof *prim);
    if (!prim) {
        self->last_prim = NULL;
        return NULL;
    }
    effect_prim_construct(prim);
    effect_prim_init(prim, self, kind, offset);
    if (!effect_add_world_object(world, &prim->base)) {
        self->last_prim = NULL;
        return NULL;
    }
    list_push_back(&self->prims, &prim, sizeof prim);
    return prim;
}
