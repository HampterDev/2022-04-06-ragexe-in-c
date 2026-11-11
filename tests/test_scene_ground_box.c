/* custom: a leaf's ground box (world/scene_types.h), where with its models the frame's shadows can
   fall: the surfaces of its cells, walls included, the cells clamped to the ground */
#include "../src/world/scene_graph.c"
#include "../src/world/scene_lifecycle.c"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void c3d_ground_get_cell_pos(const struct C3dGround15 *ground, float x, float z, int *cx, int *cy)
{
    *cx = (int)floorf(x / ground->zoom) + ground->width / 2;
    *cy = (int)floorf(z / ground->zoom) + ground->height / 2;
}

void c3d_ground15_get_height_min_max(struct C3dGround15 *self, const int *cells, float *min, float *max)
{
    (void)self; (void)cells; *min = *max = 0.0f;
}

void c3d_attr_get_height_min_max(struct C3dAttr *self, const RECT *cells, float *low, float *high)
{
    (void)self; (void)cells; *low = *high = 0.0f;
}

void scene_graph_apply_bounds(struct SceneGraphNode *root, const struct RswBounds *bounds) { (void)root; (void)bounds; }
int scene_graph_node_insert(struct SceneGraphNode *root, struct C3dActor *actor, int level)
{
    (void)root; (void)actor; (void)level;
    return 0;
}

/* a surface from (x0, y0, z0) to (x1, y1, z1): a flat top where y0 equals y1, a wall otherwise */
static void surface(struct GndSurface *s, float x0, float y0, float z0, float x1, float y1, float z1)
{
    int i;

    memset(s, 0, sizeof *s);
    for (i = 0; i < 4; i++) {
        s->v[i].pos[0] = i & 1 ? x1 : x0;
        s->v[i].pos[1] = y0 == y1 ? y0 : i & 2 ? y1 : y0;
        s->v[i].pos[2] = y0 == y1 ? (i & 2 ? z1 : z0) : (i & 1 ? z1 : z0);
    }
}

static struct SceneGraphNode leaf(struct C3dGround15 *ground, float x0, float z0, float x1, float z1)
{
    struct SceneGraphNode node;

    scene_graph_node_construct(&node);
    node.min[0] = x0;
    node.min[2] = z0;
    node.max[0] = x1;
    node.max[2] = z1;
    scene_graph_node_set_ground(&node, ground, SCENE_GRAPH_DEPTH);
    return node;
}

int main(void)
{
    /* four cells in a row, 10 units each, from x -20 to 20: the first two flat at 0, the third
       raised 100 units (y -100, +y is down) behind a wall at x 0, the last without surfaces */
    struct GndSurface surfaces[4];
    struct GndCell cells[4];
    struct C3dGround15 ground;
    struct SceneGraphNode node;

    memset(cells, 0, sizeof cells);
    memset(&ground, 0, sizeof ground);
    ground.width = 4;
    ground.height = 1;
    ground.zoom = 10.0f;
    ground.cells = cells;
    surface(&surfaces[0], -20, 0, -5, -10, 0, 5);
    surface(&surfaces[1], -10, 0, -5, 0, 0, 5);
    surface(&surfaces[2], 0, 0, -5, 0, -100, 5);
    surface(&surfaces[3], 0, -100, -5, 10, -100, 5);
    cells[0].top = &surfaces[0];
    cells[1].top = &surfaces[1];
    cells[1].rightWall = &surfaces[2];
    cells[2].top = &surfaces[3];

    /* the second cell alone: its wall reaches the raised cell's height */
    node = leaf(&ground, -10, -5, -1, 4);
    assert(node.groundCell[0] == 1 && node.groundCell[2] == 1);
    assert(node.groundMin[0] == -10 && node.groundMax[0] == 0);
    assert(node.groundMin[1] == -100 && node.groundMax[1] == 0);
    assert(node.groundMin[2] == -5 && node.groundMax[2] == 5);
    /* the whole row and beyond: the cells are clamped to the ground */
    node = leaf(&ground, -60, -50, 60, 50);
    assert(node.groundMin[0] == -20 && node.groundMax[0] == 10);
    assert(node.groundMin[1] == -100 && node.groundMax[1] == 0);
    /* the last cell has no surfaces: the box stays empty */
    node = leaf(&ground, 10, -5, 19, 4);
    assert(node.groundMin[0] > node.groundMax[0]);
    /* a height that is not finite is left out */
    surfaces[0].v[0].pos[1] = -1e30f;
    node = leaf(&ground, -20, -5, -11, 4);
    assert(node.groundMin[1] == 0 && node.groundMax[1] == 0);
    puts("scene ground box: a leaf's surfaces with its walls, cells clamped to the ground, empty and unbounded heights passed");
    return 0;
}
