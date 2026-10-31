#include "world/model_types.h"
#include "world/scene_graph.h"
#include <stdint.h>
#include <stdlib.h>

static int reserve_object(struct SceneObjects *objects)
{
    size_t maximum = SIZE_MAX / sizeof *objects->data;
    size_t capacity;
    struct C3dActor **data;
    if (objects->count < objects->capacity)
        return 1;
    if (objects->count == maximum)
        return 0;
    capacity = !objects->capacity ? 8 : objects->capacity > maximum / 2
        ? maximum : objects->capacity * 2;
    data = realloc(objects->data, capacity * sizeof *data);
    if (!data)
        return 0;
    objects->data = data;
    objects->capacity = capacity;
    return 1;
}

static int prepare_insert(struct SceneGraphNode *node, const struct C3dActor *actor, int level,
                          struct SceneGraphNode **leaves, size_t *count)
{
    int i;
    if (level == SCENE_GRAPH_DEPTH) {
        if (!reserve_object(&node->objects)) return 0;
        leaves[(*count)++] = node;
        return 1;
    }
    for (i = 0; i < 4; ++i) {
        struct SceneGraphNode *child = node->child[i];
        if (!child) return 0;
        if (obb_overlaps_box(&actor->obb, child)
                && !prepare_insert(child, actor, level + 1, leaves, count))
            return 0;
    }
    return 1;
}

int scene_graph_node_insert(struct SceneGraphNode *root, struct C3dActor *actor, int level)
{
    struct SceneGraphNode *leaves[SCENE_GRAPH_MAX_LEAVES];
    size_t count = 0, i;
    if (!root || !actor || level < 0 || level > SCENE_GRAPH_DEPTH)
        return 0;
    if (!prepare_insert(root, actor, level, leaves, &count))
        return 0;
    for (i = 0; i < count; ++i) {
        struct SceneGraphNode *node = leaves[i];
        node->objects.data[node->objects.count++] = actor;
        for (; node; node = node->parent)
            node->needUpdate = 1;
    }
    return 1;
}

void scene_graph_node_remove(struct SceneGraphNode *node, struct C3dActor *actor, int level)
{
    int i;
    if (!node || level < 0 || level > SCENE_GRAPH_DEPTH)
        return;
    if (level == SCENE_GRAPH_DEPTH) {
        size_t read, write = 0, previous = node->objects.count;
        struct SceneGraphNode *parent;
        for (read = 0; read < previous; ++read)
            if (node->objects.data[read] != actor)
                node->objects.data[write++] = node->objects.data[read];
        node->objects.count = write;
        if (write != previous)
            for (parent = node; parent; parent = parent->parent)
                parent->needUpdate = 1;
        return;
    }
    for (i = 0; i < 4; ++i)
        scene_graph_node_remove(node->child[i], actor, level + 1);
}
