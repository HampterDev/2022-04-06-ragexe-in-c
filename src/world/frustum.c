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

void view_frustum_build(struct ViewFrustum *self, float tanX, float tanY,
                                   const float *m, struct SceneGraphNode *root)
{
    const float z = 0.0f;
    float nx = -tanX, ny = -tanY;

    self->plane[0][0] = m[0] * nx + m[1] * z + m[2];
    self->plane[0][1] = nx * m[3] + m[4] * z + m[5];
    self->plane[0][2] = m[6] * nx + m[7] * z + m[8];
    self->plane[0][3] = m[9] * nx + m[10] * z + m[11] + z;
    copy_normal(self, 0);

    self->plane[1][0] = m[0] * tanX + m[1] * z + m[2];
    self->plane[1][1] = tanX * m[3] + m[4] * z + m[5];
    self->plane[1][2] = m[6] * tanX + m[7] * z + m[8];
    self->plane[1][3] = m[9] * tanX + m[10] * z + m[11] + z;
    copy_normal(self, 1);

    self->plane[2][0] = m[1] * ny + m[0] * z + m[2];
    self->plane[2][1] = ny * m[4] + m[3] * z + m[5];
    self->plane[2][2] = m[7] * ny + m[6] * z + m[8];
    self->plane[2][3] = m[10] * ny + m[9] * z + m[11] + z;
    copy_normal(self, 2);

    self->plane[3][0] = m[1] * tanY + m[0] * z + m[2];
    self->plane[3][1] = tanY * m[4] + m[3] * z + m[5];
    self->plane[3][2] = m[7] * tanY + m[6] * z + m[8];
    self->plane[3][3] = m[10] * tanY + m[9] * z + m[11] + z;
    copy_normal(self, 3);

    self->plane[4][0] = m[1] * z + m[0] * z + m[2];
    self->plane[4][1] = m[4] * z + m[3] * z + m[5];
    self->plane[4][2] = m[7] * z + m[6] * z + m[8];
    self->plane[4][3] = m[10] * z + m[9] * z + m[11] - RENDER_NEAR_Z;
    copy_normal(self, 4);

    self->plane[5][0] = m[1] * z + m[0] * z - m[2] * 1.0f;
    self->plane[5][1] = m[4] * z + m[3] * z - m[5] * 1.0f;
    self->plane[5][2] = m[7] * z + m[6] * z - m[8] * 1.0f;
    self->plane[5][3] = m[10] * z + m[9] * z - m[11] * 1.0f + RENDER_FAR_Z;
    copy_normal(self, 5);

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
