#include "actor/object_ops.h"
#include "render/texture_cache.h"
#include "world/ground_api.h"
#include "session/state.h"
#include "world/world.h"
#include "core/fatal.h"
#include "world/mode.h"

#include <string.h>
#include "actor/types.h"

struct World *world_construct(struct World *self)
{
    list_init(&self->gameObjects);
    actor_list_generation++;
    list_init(&self->actors);
    list_init(&self->items);
    list_init(&self->skills);
    memset(&self->models, 0, sizeof self->models);
    self->modelLoader = NULL;
    scene_graph_node_construct(&self->root);
    list_init(&self->pendingSkills);
    tree_init(&self->idsByCell);
    list_init(&self->idsNowhere);
    self->prebuilt = NULL;
    self->mode = NULL;
    self->attr = NULL;
    self->ground = NULL;
    self->player = NULL;
    self->is_pvp = self->is_siege = self->is_battleground = 0;
    g_session1.battlegroundPoints[0] = -1;
    g_session1.battlegroundPoints[1] = -1;
    self->is_event_pvp = 0;
    self->nearFlag = 0;
    self->nearRange = 2;
    return self;
}

void world_clear_map(struct World *self)
{
    world_models_wait(self);
    world_free_scene_graph(self);
    world_clear_objects(self);
    world_models_clear(self);
    if (self->ground)
        ground_free(self->ground);
    self->ground = NULL;
    cache_purge(g_resMgr());
    texcache_evict(&g_textureCache);
}
