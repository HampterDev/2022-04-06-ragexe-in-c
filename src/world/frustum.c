#include "world/frustum.h"
#include "render/render_api.h"
#include <string.h>

#include <math.h>
static void view_frustum_collect(struct ViewFrustum *self, struct SceneGraphNode *node, int level, unsigned char inside);

#define ABSF(x) ((float)fabs((double)(x)))

struct ViewFrustum *view_frustum_construct(struct ViewFrustum *self)
{
    memset(self, 0, sizeof *self);
    return self;
}

static void copy_normal(struct ViewFrustum *f, int i)
{
    f->normal[i][0] = f->plane[i][0];
    f->normal[i][1] = f->plane[i][1];
    f->normal[i][2] = f->plane[i][2];
}

/* custom: plane i is a * (view x or y, by axis) + b * (view depth) + c >= 0 in world coordinates;
   the view's x, y and depth are the columns 0, 1 and 2 of the view matrix (rows of three). */
static void view_plane(struct ViewFrustum *f, int i, const float *m, int axis, float a, float b, float c)
{
    int k;

    for (k = 0; k < 4; k++)
        f->plane[i][k] = a * m[k * 3 + axis] + b * m[k * 3 + 2];
    f->plane[i][3] += c;
    copy_normal(f, i);
}

/* custom: the original built the four sides from one tangent each way, the screen's half size over
   its scale. Its centre is the half size rounded down, so on a screen of odd width or height the
   right or bottom side stopped a pixel short of the edge and culled what showed only there; the
   window could not take such a size in the original, and can since it resizes. The sides are
   where the projection puts the screen's edges. */
void view_frustum_build(struct ViewFrustum *self, const struct FrustumScreen *s,
                                   const float *m, struct SceneGraphNode *root)
{
    view_plane(self, 0, m, 0, s->proj[0], s->centre[0], 0.0f);
    view_plane(self, 1, m, 0, -s->proj[0], s->size[0] - s->centre[0], 0.0f);
    view_plane(self, 2, m, 1, s->proj[1], s->centre[1], 0.0f);
    view_plane(self, 3, m, 1, -s->proj[1], s->size[1] - s->centre[1], 0.0f);
    view_plane(self, 4, m, 0, 0.0f, 1.0f, -RENDER_NEAR_Z);
    view_plane(self, 5, m, 0, 0.0f, -1.0f, RENDER_FAR_Z);

    view_frustum_clear(self);
    if (root)
        view_frustum_collect(self, root, 0, 0);
}

static int node_vs_planes(const struct ViewFrustum *f, const struct SceneGraphNode *n)
{
    unsigned char crossing = 0;
    int i;

    for (i = 0; i < 6; i++) {
        const float *nv = f->normal[i];
        float reach = ABSF(n->half[0] * nv[0]) + ABSF(nv[1] * n->half[1]) + ABSF(n->half[2] * nv[2]);
        float dist = n->centre[0] * nv[0] + nv[1] * n->centre[1] + n->centre[2] * nv[2] + f->plane[i][3];
        if (dist > reach)
            continue;
        if (!(dist > -reach))
            return FRUSTUM_OUTSIDE;
        crossing = 1;
    }
    return crossing ? FRUSTUM_CROSSING : FRUSTUM_INSIDE;
}

int view_frustum_test_obb(struct ViewFrustum *self, const struct FrustumObb *b)
{
    unsigned char crossing = 0;
    int i;

    for (i = 0; i < 6; i++) {
        const float *nv = self->normal[i];
        float reach = ABSF((nv[0] * b->axis[0][0] + nv[1] * b->axis[0][1] + nv[2] * b->axis[0][2]) * b->extent[0])
                    + ABSF((nv[0] * b->axis[1][0] + nv[1] * b->axis[1][1] + nv[2] * b->axis[1][2]) * b->extent[1])
                    + ABSF((nv[0] * b->axis[2][0] + nv[1] * b->axis[2][1] + nv[2] * b->axis[2][2]) * b->extent[2]);
        float dist = nv[0] * b->centre[0] + nv[1] * b->centre[1] + nv[2] * b->centre[2] + self->plane[i][3];
        if (dist > reach)
            continue;
        if (!(dist > -reach))
            return FRUSTUM_OUTSIDE;
        crossing = 1;
    }
    return crossing ? FRUSTUM_CROSSING : FRUSTUM_INSIDE;
}

static void view_frustum_collect(struct ViewFrustum *self, struct SceneGraphNode *node,
                                     int level, unsigned char inside)
{
    int i;

    if (level == RSW_TREE_DEPTH) {
        if (inside)
            self->inside[self->inside_count++] = node;
        else
            self->crossing[self->crossing_count++] = node;
        return;
    }
    for (i = 0; i < 4; i++) {
        struct SceneGraphNode *c = node->child[i];
        int r;

        if (!c)
            continue;
        if (inside) {
            view_frustum_collect(self, c, level + 1, inside);
            continue;
        }
        r = node_vs_planes(self, c);
        if (r == FRUSTUM_OUTSIDE)
            continue;
        view_frustum_collect(self, c, level + 1, r == FRUSTUM_INSIDE);
    }
}

void view_frustum_clear(struct ViewFrustum *self)
{
    self->inside_count = 0;
    self->crossing_count = 0;
}
