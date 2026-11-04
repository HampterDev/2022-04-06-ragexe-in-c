#ifndef WORLD_MODELS_H
#define WORLD_MODELS_H

#include <stddef.h>
#include <stdint.h>

struct World;
struct C3dActor;
struct RswModel;
struct WorldLoader;

struct WorldLighting {
    float direction[3], diffuse[3], ambient[3];
};

struct WorldModels {
    struct C3dActor **data;
    size_t count, capacity, inserted;
    volatile int32_t pending;
};

int world_models_prepare(struct World *world, size_t maximum,
                         const struct WorldLighting *lighting);
int world_models_add(struct World *world, struct C3dActor *model);
int world_models_queue(struct World *world, const struct RswModel *model,
                       float distance, size_t source_index);
void world_models_start(struct World *world);
void world_models_process(struct World *world);
void world_models_wait(struct World *world);
void world_models_clear(struct World *world);

struct C3dActor *world_model_create(const struct RswModel *record,
                                  const struct WorldLighting *lighting,
                                  int deferred);
void world_model_destroy(struct C3dActor *model);

#endif
