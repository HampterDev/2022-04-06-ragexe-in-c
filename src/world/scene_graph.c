#include "world/model_types.h"
#include "world/ground.h"
#include "world/world.h"

#include <math.h>

static int attr_cell(float v, int zoom, int size)
{
    return (int)(float)floor((double)(v / (float)zoom)) + size / 2;
}

static void scene_graph_node_set_attr(struct SceneGraphNode *self, void *attr, int level)
{
    struct C3dAttr *a = attr;
    int i;

    self->needUpdate = 1;
    self->attr = attr;
    self->attrCell[0] = attr_cell(self->min[0], a->zoom, a->width);
    self->attrCell[1] = attr_cell(self->min[2], a->zoom, a->height);
    self->attrCell[2] = attr_cell(self->max[0], a->zoom, a->width);
    self->attrCell[3] = attr_cell(self->max[2], a->zoom, a->height);
    if (level == SCENE_GRAPH_DEPTH)
        return;
    level++;
    for (i = 0; i < 4; i++)
        scene_graph_node_set_attr(self->child[i], attr, level);
}

/* custom: the box around the surfaces of a leaf's cells (groundCell, both ends included), walls
   included: a wall rises from its cell's edge to the neighbour's, which may lie in the next leaf */
static void leaf_ground_box(struct SceneGraphNode *self)
{
    const struct C3dGround15 *g = self->ground;
    int x0 = self->groundCell[0] > 0 ? self->groundCell[0] : 0, y0 = self->groundCell[1] > 0 ? self->groundCell[1] : 0;
    int x1, y1, x, y, s, v, k;

    if (!g || !g->cells || g->width <= 0 || g->height <= 0)
        return;
    x1 = self->groundCell[2] < g->width - 1 ? self->groundCell[2] : g->width - 1;
    y1 = self->groundCell[3] < g->height - 1 ? self->groundCell[3] : g->height - 1;
    for (y = y0; y <= y1; y++)
        for (x = x0; x <= x1; x++) {
            const struct GndCell *cell = GND_CELL(g, x, y);
            const struct GndSurface *surface[3] = {cell->top, cell->rightWall, cell->frontWall};

            for (s = 0; s < 3; s++)
                for (v = 0; surface[s] && v < 4; v++)
                    for (k = 0; k < 3; k++) {
                        float p = surface[s]->v[v].pos[k];

                        if (!(p > -SCENE_HEIGHT_HUGE && p < SCENE_HEIGHT_HUGE))
                            continue;
                        self->groundMin[k] = p < self->groundMin[k] ? p : self->groundMin[k];
                        self->groundMax[k] = p > self->groundMax[k] ? p : self->groundMax[k];
                    }
        }
}

static void scene_graph_node_set_ground(struct SceneGraphNode *self, struct C3dGround15 *ground, int level)
{
    int i;

    self->needUpdate = 1;
    self->ground = ground;
    c3d_ground_get_cell_pos(ground, self->min[0], self->min[2], &self->groundCell[0], &self->groundCell[1]);
    c3d_ground_get_cell_pos(self->ground, self->max[0], self->max[2], &self->groundCell[2], &self->groundCell[3]);
    if (level == SCENE_GRAPH_DEPTH) {
        leaf_ground_box(self);
        return;
    }
    level++;
    for (i = 0; i < 4; i++)
        scene_graph_node_set_ground(self->child[i], ground, level);
}

static void leaf_widen_ancestors(struct SceneGraphNode *self)
{
    const float *leafMin = &self->min[1];
    const float *leafMax = &self->max[1];
    struct SceneGraphNode *n;

    for (n = self; n; n = n->parent) {
        float lo, hi;
        if (n->min[1] > *leafMin) {
            n->min[1] = *leafMin;
            lo = *leafMin;
        } else {
            lo = n->min[1];
        }
        if (*leafMax > n->max[1]) {
            n->max[1] = *leafMax;
            hi = *leafMax;
        } else {
            hi = n->max[1];
        }
        n->centre[1] = (lo + hi) * 0.5f;
        n->half[0] = n->max[0] - n->centre[0];
        n->half[2] = n->max[2] - n->centre[2];
        n->half[1] = hi - n->centre[1];
    }
}

static void leaf_ground_heights(struct SceneGraphNode *self)
{
    float groundMin, groundMax, attrMin, attrMax;

    RECT cells = {self->attrCell[0], self->attrCell[1], self->attrCell[2], self->attrCell[3]};
    c3d_ground15_get_height_min_max(self->ground, self->groundCell, &groundMin, &groundMax);
    c3d_attr_get_height_min_max(self->attr, &cells, &attrMin, &attrMax);
    self->max[1] = groundMax > attrMax ? groundMax : attrMax;
    self->min[1] = groundMin < attrMin ? groundMin : attrMin;
}

static void scene_graph_node_update_heights(struct SceneGraphNode *self, int level)
{
    int i;

    self->needUpdate = 0;
    if (level == SCENE_GRAPH_DEPTH) {
        size_t index;

        leaf_ground_heights(self);
        for (index = 0; index < self->objects.count; ++index) {
            const struct C3dActor *obj = self->objects.data[index];
            if (!obj)
                continue;
            if (self->min[1] > obj->bboxMin[1])
                self->min[1] = obj->bboxMin[1];
            if (obj->bboxMax[1] > self->max[1])
                self->max[1] = obj->bboxMax[1];
        }
        leaf_widen_ancestors(self);
        return;
    }
    for (i = 0; i < 4; i++)
        if (self->child[i]->needUpdate)
            scene_graph_node_update_heights(self->child[i], level + 1);
}

int world_build_scene_graph(struct World *self)
{
    struct C3dGround15 *ground;
    size_t index;
    int i;

    world_free_scene_graph(self);
    ground = self->ground;
    self->root.min[0] = (float)ground->width * ground->zoom * -0.5f;
    self->root.max[0] = (float)ground->width * ground->zoom * 0.5f - 1.0f;
    self->root.min[2] = (float)ground->height * ground->zoom * -0.5f;
    self->root.max[2] = (float)ground->height * ground->zoom * 0.5f - 1.0f;
    self->root.centre[0] = 0;
    self->root.centre[2] = 0;
    if (!scene_graph_node_split(&self->root, 0))
        return 0;

    for (index = 0; index < self->models.count; ++index) {
        struct C3dActor *obj = self->models.data[index];
        if (!scene_graph_node_insert(&self->root, obj, 0)) {
            world_free_scene_graph(self);
            return 0;
        }
    }

    self->root.needUpdate = 1;
    self->root.ground = ground;
    c3d_ground_get_cell_pos(ground, self->root.min[0], self->root.min[2],
                         &self->root.groundCell[0], &self->root.groundCell[1]);
    c3d_ground_get_cell_pos(self->root.ground, self->root.max[0], self->root.max[2],
                         &self->root.groundCell[2], &self->root.groundCell[3]);
    for (i = 0; i < 4; i++)
        scene_graph_node_set_ground(self->root.child[i], ground, 1);
    scene_graph_node_set_attr(&self->root, self->attr, 0);

    if (self->prebuilt) {
        scene_graph_apply_bounds(&self->root, self->prebuilt);
        return 1;
    }
    self->root.needUpdate = 0;
    for (i = 0; i < 4; i++)
        if (self->root.child[i]->needUpdate)
            scene_graph_node_update_heights(self->root.child[i], 1);
    return 1;
}
