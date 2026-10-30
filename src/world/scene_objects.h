#ifndef WORLD_SCENE_OBJECTS_H
#define WORLD_SCENE_OBJECTS_H

struct SceneGraphNode;
struct C3dActor;

int scene_graph_node_insert(struct SceneGraphNode *root, struct C3dActor *actor, int level);
void scene_graph_node_remove(struct SceneGraphNode *root, struct C3dActor *actor, int level);

#endif
