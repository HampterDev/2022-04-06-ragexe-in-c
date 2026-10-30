#ifndef WORLD_SCENE_TYPES_H
#define WORLD_SCENE_TYPES_H

#include <stddef.h>
#include "resources/world_records.h"

#define SCENE_GRAPH_MAX_LEAVES (1u << (2 * RSW_TREE_DEPTH))

struct C3dActor;

struct SceneObjects {
    struct C3dActor **data;
    size_t count, capacity;
};

struct C3dGround15;
struct C3dAttr;

struct SceneGraphNode {
    struct SceneGraphNode *parent;
    struct SceneGraphNode *child[4];
    float min[3];
    float max[3];
    float centre[3];
    float half[3];
    int needUpdate;
    struct SceneObjects objects;
    struct C3dGround15 *ground;
    int groundCell[4];
    struct C3dAttr *attr;
    int attrCell[4];
};

struct RayPicker {
    struct SceneGraphNode *hits[SCENE_GRAPH_MAX_LEAVES];
    size_t count;
    float origin[3];
    float dir[3];
    float maxT;
};

#endif
