#include "world/world.h"
#include <stdlib.h>
#include <string.h>

struct SceneGraphNode *scene_graph_node_construct(struct SceneGraphNode *self)
{
    memset(self, 0, sizeof *self);
    self->needUpdate = 1;
    return self;
}

void scene_graph_node_destruct(struct SceneGraphNode *self)
{
    int i;
    for (i = 0; i < 4; ++i) {
        if (self->child[i]) {
            scene_graph_node_destruct(self->child[i]);
            free(self->child[i]);
        }
    }
    free(self->objects.data);
    scene_graph_node_construct(self);
}

static struct SceneGraphNode *node_new(void)
{
    struct SceneGraphNode *node = calloc(1, sizeof *node);
    return node ? scene_graph_node_construct(node) : NULL;
}

int scene_graph_node_split(struct SceneGraphNode *self, int level)
{
    struct SceneGraphNode *c;
    int i;

    if (level < 0 || level > SCENE_GRAPH_DEPTH)
        return 0;

    self->centre[1] = 0;
    self->min[1] = -SCENE_HEIGHT_HUGE;
    self->max[1] = SCENE_HEIGHT_HUGE;
    self->centre[0] = (self->min[0] + self->max[0]) * 0.5f;
    self->centre[2] = (self->min[2] + self->max[2]) * 0.5f;
    if (level == SCENE_GRAPH_DEPTH)
        return 1;

    c = self->child[0] = node_new();
    if (!c) goto fail;
    c->parent = self;
    c->min[0] = self->min[0];
    c->max[0] = self->centre[0];
    c->min[2] = self->min[2];
    c->max[2] = self->centre[2];

    c = self->child[1] = node_new();
    if (!c) goto fail;
    c->parent = self;
    c->min[0] = self->centre[0];
    c->max[0] = self->max[0];
    c->min[2] = self->min[2];
    c->max[2] = self->centre[2];

    c = self->child[2] = node_new();
    if (!c) goto fail;
    c->parent = self;
    c->min[0] = self->min[0];
    c->max[0] = self->centre[0];
    c->min[2] = self->centre[2];
    c->max[2] = self->max[2];

    c = self->child[3] = node_new();
    if (!c) goto fail;
    c->parent = self;
    c->min[0] = self->centre[0];
    c->max[0] = self->max[0];
    c->min[2] = self->centre[2];
    c->max[2] = self->max[2];

    level++;
    for (i = 0; i < 4; i++)
        if (!scene_graph_node_split(self->child[i], level))
            goto fail;
    return 1;

fail:
    scene_graph_node_destruct(self);
    return 0;
}

void world_free_scene_graph(struct World *self)
{
    scene_graph_node_destruct(&self->root);
}
