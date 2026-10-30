#include "world/scene_graph.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t connect_nodes(struct SceneGraphNode *nodes, size_t index, unsigned depth)
{
    size_t next = index + 1;
    unsigned child;
    if (depth == RSW_TREE_DEPTH) return next;
    for (child = 0; child < 4; ++child) {
        nodes[index].child[child] = &nodes[next];
        nodes[next].parent = &nodes[index];
        next = connect_nodes(nodes, next, depth + 1);
    }
    return next;
}
int main(void)
{
    struct SceneGraphNode *nodes = calloc(RSW_TREE_NODE_COUNT, sizeof *nodes);
    struct RswBounds *bounds = calloc(RSW_TREE_NODE_COUNT, sizeof *bounds);
    size_t i;
    unsigned k;
    assert(nodes && bounds && connect_nodes(nodes, 0, 0) == RSW_TREE_NODE_COUNT);
    for (i = 0; i < RSW_TREE_NODE_COUNT; ++i) {
        nodes[i].needUpdate = 1;
        nodes[i].groundCell[0] = (int)i;
        for (k = 0; k < 3; ++k) {
            bounds[i].max[k] = (float)(i * 12 + k);
            bounds[i].min[k] = (float)(i * 12 + 3 + k);
            bounds[i].half[k] = (float)(i * 12 + 6 + k);
            bounds[i].centre[k] = (float)(i * 12 + 9 + k);
        }
    }
    scene_graph_apply_bounds(nodes, bounds);
    for (i = 0; i < RSW_TREE_NODE_COUNT; ++i) {
        assert(!nodes[i].needUpdate && nodes[i].groundCell[0] == (int)i);
        assert(!memcmp(nodes[i].max, bounds[i].max, sizeof bounds[i].max));
        assert(!memcmp(nodes[i].min, bounds[i].min, sizeof bounds[i].min));
        assert(!memcmp(nodes[i].half, bounds[i].half, sizeof bounds[i].half));
        assert(!memcmp(nodes[i].centre, bounds[i].centre, sizeof bounds[i].centre));
    }
    free(bounds); free(nodes);
    puts("scene bounds: all 1365 depth-first boxes applied without changing node topology or object state");
    return 0;
}
