#include "world/world.h"
#include "world/model.h"
#include "world/models.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct C3dModelRes resource;
static struct ModelResNode node_resource;
static struct ModelResNode *nodes[] = {&node_resource};
static struct RswModel record;
static struct WorldLighting lighting;
static int missing, fail_allocation, fail_volumes, fail_create, phase, deleted;

struct ResMgr *g_resMgr(void) { return (struct ResMgr *)&resource; }
void *res_mgr_get(struct ResMgr *mgr, const char *name)
{
    assert(mgr == (struct ResMgr *)&resource && strcmp(name, "test.rsm") == 0);
    return missing ? NULL : &resource;
}
void model_res_place_videos(const struct C3dModelRes *res, Vec3 pos)
{ assert(res == &resource && memcmp(&pos, record.pos, sizeof pos) == 0); }
static void actor_delete(struct C3dActor *actor)
{ ++deleted; free(actor); }
static void actor_speed(struct C3dActor *actor, float speed)
{
    (void)actor;
    assert(phase++ == 1 && speed == record.animSpeed);
}
static int actor_create(struct C3dActor *actor, struct C3dModelRes *res)
{
    struct C3dActor *head = actor;
    assert(phase++ == 2 && res == &resource);
    assert(head->animType == record.animType && head->layer == record.flag && head->dirty);
    assert(memcmp(head->pos, record.pos, sizeof record.pos) == 0);
    assert(memcmp(head->rot, record.rot, sizeof record.rot) == 0);
    assert(memcmp(head->scale, record.scale, sizeof record.scale) == 0);
    return !fail_create;
}
static void actor_light(struct C3dActor *actor, const float *dir, const float *diffuse, const float *ambient)
{
    (void)actor;
    assert(phase++ == 3);
    assert(memcmp(dir, lighting.direction, sizeof lighting.direction) == 0);
    assert(memcmp(diffuse, lighting.diffuse, sizeof lighting.diffuse) == 0);
    assert(memcmp(ambient, lighting.ambient, sizeof lighting.ambient) == 0);
}
static const struct ModelActorOps actor_ops_of = {
    .destroy = actor_delete, .create = actor_create, .light = actor_light, .set_speed = actor_speed
};
static void construct(void *actor)
{
    struct C3dActor *head = actor;
    assert(phase == 0);
    head->ops = &actor_ops_of;
}
struct C3dActor *model_actor_create(int version)
{
    struct C3dActor *actor;
    assert(version == resource.version);
    if (fail_allocation || (version != 1 && version != 2)) return NULL;
    actor = calloc(1, version == 1 ? sizeof(struct C3dActorV1) : sizeof(struct C3dActorV2));
    assert(actor); construct(actor); return actor;
}
int c3d_actor_add_volume_boxes(struct C3dActor *actor, const struct VolumeBox *boxes, size_t count)
{ assert(actor && boxes == resource.blocks && count == resource.blockCount && phase++ == 0); return !fail_volumes; }

int main(void)
{
    struct C3dActor *actor;
    int version, i;
    strcpy(record.fileName, "test.rsm"); strcpy(record.nodeName, "test node");
    record.animType = 7; record.animSpeed = 1.5f; record.flag = 3;
    for (i = 0; i < 3; ++i) {
        record.pos[i] = (float)i; record.rot[i] = (float)i + 4;
        record.scale[i] = (float)i + 1; lighting.direction[i] = (float)i;
        lighting.diffuse[i] = (float)i + 10; lighting.ambient[i] = (float)i + 20;
    }
    resource.nodes = nodes;
    missing = 1;
    assert(!world_model_create(&record, &lighting));
    missing = 0;
    assert(!world_model_create(&record, &lighting));
    resource.nodeCount = 1; resource.version = 9;
    assert(!world_model_create(&record, &lighting));
    resource.version = 1; fail_allocation = 1;
    assert(!world_model_create(&record, &lighting) && phase == 0);
    fail_allocation = 0;
    for (version = 1; version <= 2; ++version) {
        resource.version = version;
        phase = 0;
        actor = world_model_create(&record, &lighting);
        assert(actor && phase == 4); world_model_destroy(actor);
    }
    strcpy(record.nodeName, "unknown node");
    phase = 0;
    actor = world_model_create(&record, &lighting);
    assert(actor && phase == 4); world_model_destroy(actor);
    world_model_destroy(NULL); assert(deleted == 3);
    for (version = 1; version <= 2; ++version) {
        resource.version = version;
        phase = 0; fail_volumes = 1;
        assert(!world_model_create(&record, &lighting) && phase == 1);
        fail_volumes = 0; phase = 0; fail_create = 1;
        assert(!world_model_create(&record, &lighting) && phase == 3);
        fail_create = 0;
    }
    assert(deleted == 7);
    puts("model factory: resource skips, versions, allocation failure, transforms, lighting and creation order passed");
    return 0;
}
