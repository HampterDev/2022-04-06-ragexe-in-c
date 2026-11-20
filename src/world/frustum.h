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

/* The screen a view is drawn on: a point at view x, y and depth z lands on the pixel
   (x * proj[0] / z + centre[0], y * proj[1] / z + centre[1]) of a screen size[0] by size[1]. */
struct FrustumScreen {
    float proj[2], centre[2], size[2];
};

struct ViewFrustum *view_frustum_construct(struct ViewFrustum *self);
/* The planes are the screen's four edges, from the projection itself, so that the frustum covers
   every pixel at any screen size (the centre of an odd size lies half a pixel off the middle), and
   the near and far depths. */
void view_frustum_build(struct ViewFrustum *self, const struct FrustumScreen *screen,
                       const float *viewMatrix, struct SceneGraphNode *root);
int view_frustum_test_obb(struct ViewFrustum *self, const struct FrustumObb *box);
void view_frustum_clear(struct ViewFrustum *self);

#endif
