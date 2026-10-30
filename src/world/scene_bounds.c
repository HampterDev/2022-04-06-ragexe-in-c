#include "world/scene_graph.h"

static void scene_graph_node_copy_bounds(struct SceneGraphNode *self, int level,
                                          const struct RswBounds **next)
{
    const struct RswBounds *src = (*next)++;
    int i;

    self->needUpdate = 0;
    self->max[0] = src->max[0];
    self->max[1] = src->max[1];
    self->max[2] = src->max[2];
    self->min[0] = src->min[0];
    self->min[1] = src->min[1];
    self->min[2] = src->min[2];
    self->half[0] = src->half[0];
    self->half[1] = src->half[1];
    self->half[2] = src->half[2];
    self->centre[0] = src->centre[0];
    self->centre[1] = src->centre[1];
    self->centre[2] = src->centre[2];
    if (level == SCENE_GRAPH_DEPTH)
        return;
    level++;
    for (i = 0; i < 4; i++)
        scene_graph_node_copy_bounds(self->child[i], level, next);
}

void scene_graph_apply_bounds(struct SceneGraphNode *root, const struct RswBounds *bounds)
{
    const struct RswBounds *next = bounds;
    scene_graph_node_copy_bounds(root, 0, &next);
}
