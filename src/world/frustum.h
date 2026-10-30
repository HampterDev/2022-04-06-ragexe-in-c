#ifndef WORLD_FRUSTUM_H
#define WORLD_FRUSTUM_H

#include "world/scene_types.h"

struct FrustumObb {
    float extent[3], centre[3], axis[3][3];
};

struct ViewFrustum {
    struct SceneGraphNode *inside[SCENE_GRAPH_MAX_LEAVES];
    struct SceneGraphNode *crossing[SCENE_GRAPH_MAX_LEAVES];
    size_t inside_count, crossing_count;
    float plane[6][4];
    float normal[6][3];
};

enum { FRUSTUM_OUTSIDE = 0, FRUSTUM_INSIDE = 1, FRUSTUM_CROSSING = 2 };

struct ViewFrustum *view_frustum_construct(struct ViewFrustum *self);
void view_frustum_build(struct ViewFrustum *self, float tanX, float tanY,
                       const float *viewMatrix, struct SceneGraphNode *root);
int view_frustum_test_obb(struct ViewFrustum *self, const struct FrustumObb *box);
void view_frustum_clear(struct ViewFrustum *self);

#define VIEW_NEAR_D 10.0f
#define VIEW_FAR_D 1500.0f

#endif
