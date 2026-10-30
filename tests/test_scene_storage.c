#include "world/model_types.h"
#include "world/scene_graph.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Guard { size_t size; unsigned magic, reserved[2]; };
static size_t live, array_calls, fail_at;
static struct SceneGraphNode *leaves[SCENE_GRAPH_MAX_LEAVES];
static size_t leaf_count;

static void *allocate(size_t size)
{
    struct Guard *g = malloc(sizeof *g + size + 16);
    assert(g);
    g->size = size; g->magic = 0x73544f52;
    g->reserved[0] = g->reserved[1] = 0xa5a5a5a5;
    memset(g + 1, 0, size); memset((char *)(g + 1) + size, 0x5a, 16);
    ++live;
    return g + 1;
}
static struct Guard *checked(void *memory)
{
    struct Guard *g = (struct Guard *)memory - 1;
    size_t i;
    assert(g->magic == 0x73544f52 && g->reserved[0] == 0xa5a5a5a5 && g->reserved[1] == 0xa5a5a5a5);
    for (i = 0; i < 16; ++i) assert(((unsigned char *)memory)[g->size + i] == 0x5a);
    return g;
}
static void observed_free(void *memory)
{
    if (memory) { struct Guard *g = checked(memory); assert(live); --live; g->magic = 0; free(g); }
}
static void *observed_calloc(size_t count, size_t size)
{ assert(count && size && count <= SIZE_MAX / size); return allocate(count * size); }
static void *observed_realloc(void *memory, size_t size)
{
    void *data;
    size_t previous = memory ? checked(memory)->size : 0;
    if (++array_calls == fail_at) return NULL;
    data = allocate(size);
    if (memory) { memcpy(data, memory, previous < size ? previous : size); observed_free(memory); }
    return data;
}
#define calloc observed_calloc
#define realloc observed_realloc
#define free observed_free
#include "../src/world/scene_lifecycle.c"
#include "../src/world/scene_objects.c"
#undef free
#undef realloc
#undef calloc

struct Model { struct C3dActor actor; int alive; };
static struct Model a, b, outside;

static void model(struct Model *m, float x, float z, float width)
{
    memset(m, 0, sizeof *m);
    m->actor.obb.extent[0] = m->actor.obb.extent[2] = width; m->actor.obb.extent[1] = 2000000;
    m->actor.obb.centre[0] = x; m->actor.obb.centre[2] = z;
    m->actor.obb.axis[0][0] = m->actor.obb.axis[1][1] = m->actor.obb.axis[2][2] = 1;
    m->alive = 123;
}
static void collect_leaves(struct SceneGraphNode *n, int depth)
{
    int i;
    n->needUpdate = 0;
    if (depth == SCENE_GRAPH_DEPTH) { leaves[leaf_count++] = n; return; }
    for (i = 0; i < 4; ++i) collect_leaves(n->child[i], depth + 1);
}
static void fresh_arrays(struct SceneGraphNode *root)
{
    size_t i;
    leaf_count = 0; collect_leaves(root, 0);
    assert(leaf_count == SCENE_GRAPH_MAX_LEAVES);
    for (i = 0; i < leaf_count; ++i) {
        observed_free(leaves[i]->objects.data);
        memset(&leaves[i]->objects, 0, sizeof leaves[i]->objects);
    }
    array_calls = fail_at = 0;
}
static void check_objects(size_t count, struct C3dActor *first, struct C3dActor *last, int dirty)
{
    size_t i;
    for (i = 0; i < leaf_count; ++i) {
        const struct SceneObjects *o = &leaves[i]->objects;
        assert(o->count == count && leaves[i]->needUpdate == dirty);
        if (count) assert(o->data[0] == first && o->data[count - 1] == last);
    }
}
static void all_dirty(struct SceneGraphNode *node, int depth, int value)
{
    int i;
    assert(node->needUpdate == value);
    if (depth < SCENE_GRAPH_DEPTH)
        for (i = 0; i < 4; ++i) all_dirty(node->child[i], depth + 1, value);
}
static void insertion(struct SceneGraphNode *root)
{
    size_t failure, i;
    fresh_arrays(root);
    assert(!scene_graph_node_insert(root, NULL, 0));
    assert(!scene_graph_node_insert(NULL, &a.actor, 0));
    assert(!scene_graph_node_insert(root, &a.actor, -1));
    assert(!scene_graph_node_insert(root, &a.actor, SCENE_GRAPH_DEPTH + 1));
    assert(scene_graph_node_insert(root, &outside.actor, 0));
    assert(!array_calls); check_objects(0, NULL, NULL, 0);
    for (failure = 1; failure <= SCENE_GRAPH_MAX_LEAVES; ++failure) {
        fresh_arrays(root); fail_at = failure;
        assert(!scene_graph_node_insert(root, &a.actor, 0));
        assert(array_calls == failure); check_objects(0, NULL, NULL, 0); all_dirty(root, 0, 0);
        fail_at = 0;
        assert(scene_graph_node_insert(root, &a.actor, 0));
        check_objects(1, &a.actor, &a.actor, 1); all_dirty(root, 0, 1);
    }
    fresh_arrays(root);
    for (i = 0; i < 8; ++i) assert(scene_graph_node_insert(root, &a.actor, 0));
    leaf_count = 0; collect_leaves(root, 0);
    array_calls = 0; fail_at = SCENE_GRAPH_MAX_LEAVES;
    assert(!scene_graph_node_insert(root, &b.actor, 0));
    check_objects(8, &a.actor, &a.actor, 0); all_dirty(root, 0, 0);
    fail_at = 0;
    assert(scene_graph_node_insert(root, &b.actor, 0)); check_objects(9, &a.actor, &b.actor, 1);
    scene_graph_node_remove(root, &a.actor, 0); check_objects(1, &b.actor, &b.actor, 1);
    assert(a.alive == 123 && b.alive == 123);
    leaf_count = 0; collect_leaves(root, 0);
    scene_graph_node_remove(root, &outside.actor, 0); check_objects(1, &b.actor, &b.actor, 0);
    scene_graph_node_remove(root, &b.actor, 0); check_objects(0, NULL, NULL, 1);

    fresh_arrays(root);
    assert(scene_graph_node_insert(root->child[2], &b.actor, 1));
    for (i = 0; i < leaf_count; ++i)
        assert(leaves[i]->objects.count == (i >= 512 && i < 768 ? 1u : 0u));
    scene_graph_node_remove(root, &b.actor, 0);
    fresh_arrays(root);
    model(&outside, leaves[700]->centre[0], leaves[700]->centre[2], 0.01f);
    assert(scene_graph_node_insert(root, &outside.actor, 0));
    for (i = 0; i < leaf_count; ++i)
        assert(leaves[i]->objects.count == (i == 700 ? 1u : 0u));
    scene_graph_node_remove(root, &outside.actor, 0);
    fresh_arrays(root);
}

static void uniform_boxes(struct SceneGraphNode *node, int depth, float x0, float x1)
{
    int i;
    node->min[0] = x0; node->max[0] = x1;
    node->min[1] = node->min[2] = -1;
    node->max[1] = node->max[2] = 1;
    if (depth < SCENE_GRAPH_DEPTH)
        for (i = 0; i < 4; ++i) uniform_boxes(node->child[i], depth + 1, x0, x1);
}
static void rays(struct SceneGraphNode *root)
{
    struct { unsigned before[4]; struct RayPicker picker; unsigned after[4]; } guarded;
    struct RayPicker *p = &guarded.picker;
    struct SceneGraphNode empty_root;
    struct Ray3 ray = {{-2, 0, 0}, {1, 0, 0}, 0.5f};
    size_t i, previous_live = live;
    memset(&guarded, 0xa5, sizeof guarded);
    ray_picker_construct(p);
    assert(!p->count && !p->maxT);
    uniform_boxes(root, 0, -1, 1);
    ray_picker_pick_ray(p, &ray, root);
    assert(p->count == SCENE_GRAPH_MAX_LEAVES);
    for (i = 0; i < p->count; ++i) assert(p->hits[i] == leaves[i]);
    ray_picker_pick_ray(p, &ray, root); assert(p->count == SCENE_GRAPH_MAX_LEAVES);
    ray.origin.y = 0.8f; ray_picker_pick_ray(p, &ray, root); assert(!p->count);
    ray.origin.y = 0;
    ray_picker_pick_ray(p, &ray, NULL); assert(!p->count);
    scene_graph_node_construct(&empty_root);
    ray_picker_pick_ray(p, &ray, &empty_root); assert(!p->count);
    for (i = 0; i < 4; ++i) assert(guarded.before[i] == 0xa5a5a5a5 && guarded.after[i] == 0xa5a5a5a5);
    assert(live == previous_live);
}
int main(void)
{
    struct SceneGraphNode root;
    model(&a, 0, 0, 10000); model(&b, 0, 0, 10000); model(&outside, 100000, 100000, 1);
    scene_graph_node_construct(&root);
    root.min[0] = -640; root.max[0] = 639; root.min[2] = -960; root.max[2] = 959;
    assert(scene_graph_node_split(&root, 0));
    insertion(&root); rays(&root);
    scene_graph_node_destruct(&root);
    assert(!live && a.alive == 123 && b.alive == 123 && outside.alive == 123);
    puts("scene storage: atomic insertion at all 1024 failure points, growth, ordered removal, borrowed models and bounded ray results passed");
    return 0;
}
