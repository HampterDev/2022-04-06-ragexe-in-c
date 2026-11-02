#include "world/models.h"
#include "world/state.h"
#include "world/scene_objects.h"
#include "resources/world_records.h"
#include "platform/thread.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct DeferredModel {
    struct RswModel record;
    float distance;
    size_t source_index;
};

struct WorldLoader {
    struct World *world;
    struct WorldLighting lighting;
    struct DeferredModel *queue;
    size_t count, capacity;
    struct PlatformThread *thread;
    int started;
};

int world_models_prepare(struct World *world, size_t maximum,
                         const struct WorldLighting *lighting)
{
    struct WorldLoader *loader;
    struct C3dActor **data = NULL;
    if (!lighting || world->modelLoader || world->models.data ||
        world->models.count || world->models.pending || world->models.inserted ||
        maximum > INT32_MAX || maximum > SIZE_MAX / sizeof *data)
        return 0;
    loader = calloc(1, sizeof *loader);
    if (!loader)
        return 0;
    if (maximum) {
        data = malloc(maximum * sizeof *data);
        if (!data) {
            free(loader);
            return 0;
        }
    }
    loader->world = world;
    loader->lighting = *lighting;
    world->modelLoader = loader;
    world->models.data = data;
    world->models.capacity = maximum;
    return 1;
}

int world_models_add(struct World *world, struct C3dActor *model)
{
    struct WorldLoader *loader = world->modelLoader;
    struct WorldModels *models = &world->models;
    if (!model || !loader || loader->started ||
        models->count + loader->count >= models->capacity)
        return 0;
    models->data[models->count++] = model;
    return 1;
}

int world_models_queue(struct World *world, const struct RswModel *model,
                       float distance, size_t source_index)
{
    struct WorldLoader *loader = world->modelLoader;
    struct DeferredModel *queue;
    size_t capacity;
    if (!model || !loader || loader->started || distance != distance ||
        world->models.count + loader->count >= world->models.capacity)
        return 0;
    if (loader->count == loader->capacity) {
        capacity = loader->capacity ? loader->capacity * 2 : 16;
        if (capacity < loader->capacity || capacity > SIZE_MAX / sizeof *queue)
            return 0;
        if (capacity > world->models.capacity)
            capacity = world->models.capacity;
        queue = realloc(loader->queue, capacity * sizeof *queue);
        if (!queue)
            return 0;
        loader->queue = queue;
        loader->capacity = capacity;
    }
    loader->queue[loader->count].record = *model;
    loader->queue[loader->count].distance = distance;
    loader->queue[loader->count].source_index = source_index;
    ++loader->count;
    return 1;
}

static int deferred_order(const void *left, const void *right)
{
    const struct DeferredModel *a = left, *b = right;
    if (a->distance < b->distance) return -1;
    if (a->distance > b->distance) return 1;
    return (a->source_index > b->source_index) - (a->source_index < b->source_index);
}

static void load_deferred(struct WorldLoader *loader)
{
    struct WorldModels *models = &loader->world->models;
    size_t i;
    qsort(loader->queue, loader->count, sizeof *loader->queue, deferred_order);
    for (i = 0; i < loader->count; ++i) {
        struct C3dActor *model = world_model_create(&loader->queue[i].record,
                                                   &loader->lighting, 1);
        if (!model)
            continue;
        models->data[models->count++] = model;
        platform_atomic_increment(&models->pending);
        platform_thread_yield();
    }
}

static void loader_thread(void *context)
{
    platform_thread_raise_priority();
    load_deferred(context);
}

void world_models_start(struct World *world)
{
    struct WorldLoader *loader = world->modelLoader;
    if (!loader || loader->started)
        return;
    loader->started = 1;
    world->models.inserted = world->models.count;
    if (!loader->count) {
        world_models_wait(world);
        return;
    }
    loader->thread = platform_thread_create(loader_thread, loader);
    if (!loader->thread) {
        load_deferred(loader);
        world_models_wait(world);
    }
}

void world_models_process(struct World *world)
{
    struct WorldModels *models = &world->models;
    int32_t pending = platform_atomic_load(&models->pending);
    while (pending-- > 0) {
        if (!scene_graph_node_insert(&world->root, models->data[models->inserted], 0))
            break;
        ++models->inserted;
        platform_atomic_decrement(&models->pending);
    }
}

void world_models_wait(struct World *world)
{
    struct WorldLoader *loader = world->modelLoader;
    if (!loader)
        return;
    if (loader->thread)
        platform_thread_join(loader->thread);
    free(loader->queue);
    free(loader);
    world->modelLoader = NULL;
}

void world_models_clear(struct World *world)
{
    size_t i;
    world_models_wait(world);
    for (i = 0; i < world->models.count; ++i)
        world_model_destroy(world->models.data[i]);
    free(world->models.data);
    memset(&world->models, 0, sizeof world->models);
}
