#include "world/frustum.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static struct SceneGraphNode nodes[RSW_TREE_NODE_COUNT];
static struct SceneGraphNode *leaves[SCENE_GRAPH_MAX_LEAVES];
static size_t node_count, leaf_count;
static const float identity[12] = {1,0,0, 0,1,0, 0,0,1, 0,0,0};

struct GuardedFrustum {
    unsigned before[4];
    struct ViewFrustum value;
    unsigned after[4];
};
static struct GuardedFrustum guarded;

static void guards(void)
{
    size_t i;
    for (i = 0; i < 4; ++i)
        assert(guarded.before[i] == 0xa5a5a5a5 && guarded.after[i] == 0xa5a5a5a5);
}
static void bounds(struct SceneGraphNode *node, float z, float half)
{
    node->centre[0] = node->centre[1] = 0;
    node->centre[2] = z;
    node->half[0] = node->half[1] = node->half[2] = half;
}
static struct SceneGraphNode *tree(int depth)
{
    struct SceneGraphNode *node;
    int i;
    assert(node_count < RSW_TREE_NODE_COUNT);
    node = &nodes[node_count++];
    bounds(node, 100, 1);
    if (depth == RSW_TREE_DEPTH) {
        assert(leaf_count < SCENE_GRAPH_MAX_LEAVES);
        leaves[leaf_count++] = node;
    } else {
        for (i = 0; i < 4; ++i) {
            node->child[i] = tree(depth + 1);
            node->child[i]->parent = node;
        }
    }
    return node;
}
static void set_all(float z, float half)
{
    size_t i;
    for (i = 0; i < node_count; ++i) bounds(&nodes[i], z, half);
}
static void complete_tree(void)
{
    struct ViewFrustum *frustum = &guarded.value;
    size_t i;
    assert(tree(0) == nodes);
    assert(node_count == RSW_TREE_NODE_COUNT && leaf_count == SCENE_GRAPH_MAX_LEAVES);
    view_frustum_build(frustum, 1, 1, identity, nodes);
    assert(frustum->inside_count == SCENE_GRAPH_MAX_LEAVES && !frustum->crossing_count);
    for (i = 0; i < leaf_count; ++i) assert(frustum->inside[i] == leaves[i]);
    guards();
    set_all(10, 1);
    view_frustum_build(frustum, 1, 1, identity, nodes);
    assert(frustum->crossing_count == SCENE_GRAPH_MAX_LEAVES && !frustum->inside_count);
    for (i = 0; i < leaf_count; ++i) assert(frustum->crossing[i] == leaves[i]);
    guards();
    set_all(-100, 1);
    view_frustum_build(frustum, 1, 1, identity, nodes);
    assert(!frustum->inside_count && !frustum->crossing_count);
    guards();
}
static void mixed_tree(void)
{
    struct ViewFrustum *frustum = &guarded.value;
    size_t i, inside = 0, crossing = 0;
    set_all(10, 200);
    bounds(nodes[0].child[0], 100, 1);
    bounds(nodes[0].child[1], -100, 1);
    bounds(nodes[0].child[3], 500, 1);
    for (i = 0; i < leaf_count; ++i) {
        if (i / 256 == 2) bounds(leaves[i], i % 3 == 0 ? 100.0f : i % 3 == 1 ? 10.0f : -100.0f, 1);
        else bounds(leaves[i], i / 256 == 1 ? -100.0f : 100.0f, 1);
    }
    view_frustum_build(frustum, 1, 1, identity, nodes);
    for (i = 0; i < leaf_count; ++i) {
        if (i / 256 == 1) continue;
        if (i / 256 != 2 || i % 3 == 0) assert(frustum->inside[inside++] == leaves[i]);
        else if (i % 3 == 1) assert(frustum->crossing[crossing++] == leaves[i]);
    }
    assert(inside == frustum->inside_count && crossing == frustum->crossing_count);
    guards();
    view_frustum_clear(frustum); view_frustum_clear(frustum);
    assert(!frustum->inside_count && !frustum->crossing_count);
    assert(nodes[0].child[0] && node_count == RSW_TREE_NODE_COUNT);
    view_frustum_build(frustum, 1, 1, identity, NULL);
    assert(!frustum->inside_count && !frustum->crossing_count);
    {
        struct SceneGraphNode empty = {0};
        view_frustum_build(frustum, 1, 1, identity, &empty);
        assert(!frustum->inside_count && !frustum->crossing_count);
        empty.child[3] = nodes[0].child[3];
        view_frustum_build(frustum, 1, 1, identity, &empty);
        assert(frustum->inside_count == 256 && !frustum->crossing_count);
        for (i = 0; i < 256; ++i) assert(frustum->inside[i] == leaves[768+i]);
    }
    guards();
}
static int corners(const struct ViewFrustum *frustum, const struct FrustumObb *box)
{
    int p, c, axis, crossing = 0;
    for (p = 0; p < 6; ++p) {
        float low = 1e30f, high = -1e30f;
        for (c = 0; c < 8; ++c) {
            float point[3], distance;
            int coordinate;
            memcpy(point, box->centre, sizeof point);
            for (axis = 0; axis < 3; ++axis)
                for (coordinate = 0; coordinate < 3; ++coordinate)
                    point[coordinate] += ((c & (1 << axis)) ? 1 : -1) * box->extent[axis] * box->axis[axis][coordinate];
            distance = frustum->plane[p][3];
            for (coordinate = 0; coordinate < 3; ++coordinate) distance += frustum->plane[p][coordinate] * point[coordinate];
            if (distance < low) low = distance;
            if (distance > high) high = distance;
        }
        if (high <= 0) return FRUSTUM_OUTSIDE;
        if (low <= 0) crossing = 1;
    }
    return crossing ? FRUSTUM_CROSSING : FRUSTUM_INSIDE;
}
static void plane_tests(void)
{
    static const float expected[6][4] = {{-1,0,1,0},{1,0,1,0},{0,-1,1,0},{0,1,1,0},{0,0,1,-10},{0,0,-1,1500}};
    static const float depths[] = {-10,0,8,9,10,11,12,100,1498,1499,1500,1501,1502};
    struct ViewFrustum *frustum = &guarded.value;
    struct FrustumObb box = {{1,1,1},{0,0,100},{{1,0,0},{0,1,0},{0,0,1}}};
    size_t z;
    int x, y, p, permutation;
    view_frustum_build(frustum, 1, 1, identity, NULL);
    assert(memcmp(frustum->plane, expected, sizeof expected) == 0);
    for (p = 0; p < 6; ++p) assert(memcmp(frustum->normal[p], expected[p], 3*sizeof(float)) == 0);
    for (permutation = 0; permutation < 2; ++permutation) {
        if (permutation) {
            box.axis[0][0] = box.axis[1][1] = 0;
            box.axis[0][1] = 1; box.axis[1][0] = -1;
            box.extent[0] = 2;
        }
        for (z = 0; z < sizeof depths / sizeof *depths; ++z)
            for (x = -2; x <= 2; ++x)
                for (y = -2; y <= 2; ++y) {
                    box.centre[0] = (float)x * depths[z];
                    box.centre[1] = (float)y * depths[z];
                    box.centre[2] = depths[z];
                    assert(view_frustum_test_obb(frustum, &box) == corners(frustum, &box));
                }
    }
    guards();
}
int main(void)
{
    size_t i;
    const unsigned char *bytes = (const unsigned char *)&guarded.value;
    memset(&guarded, 0xa5, sizeof guarded);
    assert(view_frustum_construct(&guarded.value) == &guarded.value);
    for (i = 0; i < sizeof guarded.value; ++i) assert(bytes[i] == 0);
    guards(); plane_tests(); complete_tree(); mixed_tree();
    puts("frustum: six planes, independent corner classifications, all 1024 hits, depth-first ordering, repeat queries and empty trees passed");
    return 0;
}
