#include "world/model_types.h"
#include "world/world.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Guard { size_t size; unsigned magic, before[2]; };
static size_t live, calls, fail_at, frees;
static struct C3dActor borrowed[2] = {{.animType = 17}, {.animType = 29}};

static void *observed_calloc(size_t count, size_t size)
{
    struct Guard *guard;
    unsigned char *data;
    assert(count && size && count <= SIZE_MAX / size);
    size *= count;
    if (++calls == fail_at) return NULL;
    guard = malloc(sizeof *guard + size + 16);
    assert(guard);
    guard->size = size; guard->magic = 0x53434e45;
    guard->before[0] = guard->before[1] = 0xa5a5a5a5;
    data = (unsigned char *)(guard + 1);
    memset(data, 0, size); memset(data + size, 0x5a, 16);
    ++live;
    return data;
}

static void observed_free(void *memory)
{
    struct Guard *guard;
    size_t i;
    unsigned char *data = memory;
    if (!memory) return;
    guard = (struct Guard *)memory - 1;
    assert(live && guard->magic == 0x53434e45);
    assert(guard->before[0] == 0xa5a5a5a5 && guard->before[1] == 0xa5a5a5a5);
    for (i = 0; i < 16; ++i) assert(data[guard->size + i] == 0x5a);
    guard->magic = 0; --live; ++frees;
    free(guard);
}

#define calloc observed_calloc
#define free observed_free
#include "../src/world/scene_lifecycle.c"
#undef free
#undef calloc

static void add_borrowed_objects(struct SceneGraphNode *node)
{
    struct C3dActor **values = observed_calloc(2, sizeof *values);
    assert(values && !node->objects.data);
    values[0] = &borrowed[0]; values[1] = &borrowed[1];
    node->objects.data = values;
    node->objects.count = node->objects.capacity = 2;
}

static void empty(const struct SceneGraphNode *node)
{
    struct SceneGraphNode expected;
    memset(&expected, 0, sizeof expected); expected.needUpdate = 1;
    assert(!memcmp(node, &expected, sizeof expected));
}

static void prepare(struct SceneGraphNode *root)
{
    memset(root, 0xcc, sizeof *root);
    assert(scene_graph_node_construct(root) == root);
    empty(root);
    root->min[0] = -640; root->max[0] = 639;
    root->min[2] = -960; root->max[2] = 959;
}

static size_t verify(struct SceneGraphNode *node, struct SceneGraphNode *parent,
                     int depth, float x0, float x1, float z0, float z1)
{
    size_t count = 1;
    int i;
    float cx = (x0 + x1) * 0.5f, cz = (z0 + z1) * 0.5f;
    assert(node->parent == parent && node->needUpdate == 1);
    assert(node->min[0] == x0 && node->max[0] == x1 && node->centre[0] == cx);
    assert(node->min[2] == z0 && node->max[2] == z1 && node->centre[2] == cz);
    assert(node->min[1] == -SCENE_HEIGHT_HUGE && node->max[1] == SCENE_HEIGHT_HUGE);
    assert(node->centre[1] == 0 && !node->ground && !node->attr);
    assert(!node->objects.data && !node->objects.count && !node->objects.capacity);
    for (i = 0; i < 4; ++i) {
        assert(!node->groundCell[i] && !node->attrCell[i]);
        if (depth == SCENE_GRAPH_DEPTH) assert(!node->child[i]);
        else {
            assert(node->child[i]);
            count += verify(node->child[i], node, depth + 1,
                i & 1 ? cx : x0, i & 1 ? x1 : cx,
                i & 2 ? cz : z0, i & 2 ? z1 : cz);
        }
    }
    return count;
}

int main(void)
{
    struct World world = {0};
    struct SceneGraphNode root;
    size_t n;
    prepare(&root);
    calls = 0;
    assert(scene_graph_node_split(&root, 0));
    assert(calls == RSW_TREE_NODE_COUNT - 1 && live == calls);
    assert(verify(&root, NULL, 0, -640, 639, -960, 959) == RSW_TREE_NODE_COUNT);
    add_borrowed_objects(root.child[0]->child[0]->child[0]->child[0]->child[0]);
    add_borrowed_objects(root.child[3]->child[3]->child[3]->child[3]->child[3]);
    frees = 0;
    scene_graph_node_destruct(&root);
    assert(frees == RSW_TREE_NODE_COUNT + 1 && !live);
    empty(&root);
    scene_graph_node_destruct(&root); empty(&root);
    assert(!live && borrowed[0].animType == 17 && borrowed[1].animType == 29);

    for (n = 1; n < RSW_TREE_NODE_COUNT; ++n) {
        prepare(&root); calls = 0; fail_at = n;
        assert(!scene_graph_node_split(&root, 0));
        assert(calls == n && !live);
        empty(&root);
        scene_graph_node_destruct(&root); empty(&root);
    }
    fail_at = 0;
    world.nearFlag = 7; world.nearRange = 19;
    for (n = 0; n < 2; ++n) {
        prepare(&world.root); calls = 0;
        assert(scene_graph_node_split(&world.root, 0));
        add_borrowed_objects(&world.root);
        world_free_scene_graph(&world);
        empty(&world.root); assert(!live);
        assert(world.nearFlag == 7 && world.nearRange == 19);
    }
    assert(!scene_graph_node_split(&world.root, -1)); empty(&world.root);
    assert(!scene_graph_node_split(&world.root, SCENE_GRAPH_DEPTH + 1)); empty(&world.root);
    world_free_scene_graph(&world); empty(&world.root);
    puts("scene lifecycle: complete topology, initialized fields, all 1364 allocation failures, rebuilds and borrowed models passed");
    return 0;
}
