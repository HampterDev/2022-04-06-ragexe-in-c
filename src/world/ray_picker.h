#ifndef WORLD_RAY_PICKER_H
#define WORLD_RAY_PICKER_H

#include "render/math3d.h"
#include "world/scene_types.h"

struct RayPicker *ray_picker_construct(struct RayPicker *self);
void ray_picker_pick_ray(struct RayPicker *self, const struct Ray3 *ray,
                        struct SceneGraphNode *root);

#endif
