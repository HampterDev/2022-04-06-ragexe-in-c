#include "world/world.h"
#include "world/model.h"
#include "world/models.h"

static struct C3dModelRes *model_resource(const struct RswModel *m)
{
    struct C3dModelRes *res = res_mgr_get(g_resMgr(), m->fileName);

    if (!res)
        return NULL;
    if (!res->nodeCount)
        return NULL;
    return res;
}

static void place_actor(struct C3dActor *a, const struct RswModel *m)
{
    a->animType = m->animType;
    a->pos[0] = m->pos[0];
    a->pos[1] = m->pos[1];
    a->pos[2] = m->pos[2];
    a->dirty = 1;
    a->rot[0] = m->rot[0];
    a->rot[1] = m->rot[1];
    a->rot[2] = m->rot[2];
    a->dirty = 1;
    a->scale[0] = m->scale[0];
    a->scale[1] = m->scale[1];
    a->scale[2] = m->scale[2];
    a->dirty = 1;
    a->layer = m->flag;
}

struct C3dActor *world_model_create(const struct RswModel *record,
                                  const struct WorldLighting *lighting,
                                  int deferred)
{
    struct C3dModelRes *res = model_resource(record);
    struct C3dActor *actor;
    if (!res)
        return NULL;
    actor = model_actor_create(res->version);
    if (!actor)
        return NULL;
    if (!c3d_actor_add_volume_boxes(actor, res->blocks, res->blockCount)) goto fail;
    if (deferred)
        actor->pickable = 0;
    actor->ops->set_speed(actor, record->animSpeed);
    place_actor(actor, record);
    if (!actor->ops->create(actor, res)) goto fail;
    if (!deferred)
        actor->pickable = 0;
    actor->ops->light(actor, lighting->direction,
                                 lighting->diffuse, lighting->ambient);
    return actor;
fail:
    actor->ops->destroy(actor);
    return NULL;
}

void world_model_destroy(struct C3dActor *model)
{
    if (model)
        model->ops->destroy(model);
}
