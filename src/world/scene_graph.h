#include "world/ground_api.h"
#ifndef WORLD_SCENE_GRAPH_H
#define WORLD_SCENE_GRAPH_H

#include "world/world_imports.h"
#include "resources/attr.h"
#include "resources/world_records.h"

#define SCENE_GRAPH_DEPTH       RSW_TREE_DEPTH
#define SCENE_HEIGHT_HUGE       999999.0f

#include "world/scene_types.h"
#include "world/ray_picker.h"

BYTE obb_overlaps_box(const void *obb, const float *box);

struct SceneGraphNode *scene_graph_node_construct(struct SceneGraphNode *self);
void scene_graph_node_destruct(struct SceneGraphNode *self);
void scene_graph_apply_bounds(struct SceneGraphNode *root, const struct RswBounds *bounds);
int scene_graph_node_split(struct SceneGraphNode *self, int level);
#include "world/scene_objects.h"

#endif
