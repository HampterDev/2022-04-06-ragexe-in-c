#include "world/scene_graph.h"
#include <math.h>

struct RayPicker *ray_picker_construct(struct RayPicker *self)
{
    self->count = 0;
    self->maxT = 0;
    self->origin[0] = 0.0f;
    self->origin[1] = 0.0f;
    self->origin[2] = 0.0f;
    self->dir[0] = 0.0f;
    self->dir[1] = 0.0f;
    self->dir[2] = 0.0f;
    return self;
}

static int ray_picker_hits_box(struct RayPicker *self, const struct SceneGraphNode *node)
{
    float cy = (node->min[1] + node->max[1]) * 0.5f;
    float cx = (node->max[0] + node->min[0]) * 0.5f;
    float cz = (node->min[2] + node->max[2]) * 0.5f;
    float hy = node->max[1] - cy;
    float hx = node->max[0] - cx;
    float hz = node->max[2] - cz;
    float dy = cy - self->origin[1];
    float dx = cx - self->origin[0];
    float dz = cz - self->origin[2];
    float tmin = -SCENE_HEIGHT_HUGE, tmax = SCENE_HEIGHT_HUGE;
    float t1, t2, inv;

    if (fabsf(self->dir[0]) > 1e-5f) {
        inv = 1.0f / self->dir[0];
        t1 = (dx + hx) * inv;
        t2 = (dx - hx) * inv;
        if (t1 > t2) {
            tmin = t2 > tmin ? t2 : tmin;
            tmax = t1 < tmax ? t1 : tmax;
        } else {
            tmin = t1 > tmin ? t1 : tmin;
            tmax = t2 < tmax ? t2 : tmax;
        }
        if (tmin > tmax || 0.0f > tmax)
            return 0;
    } else {
        hx *= 0.7f;
        if (-dx - hx > 0.0f || 0.0f > hx - dx)
            return 0;
    }

    if (fabsf(self->dir[1]) > 1e-5f) {
        inv = 1.0f / self->dir[1];
        t1 = (dy + hy) * inv;
        t2 = (dy - hy) * inv;
        if (t1 > t2) {
            tmin = t2 > tmin ? t2 : tmin;
            tmax = t1 < tmax ? t1 : tmax;
        } else {
            tmin = t1 > tmin ? t1 : tmin;
            tmax = t2 < tmax ? t2 : tmax;
        }
        if (tmin > tmax || 0.0f > tmax)
            return 0;
    } else {
        hy *= 0.7f;
        if (-dy - hy > 0.0f || 0.0f > hy - dy)
            return 0;
    }

    if (fabsf(self->dir[2]) > 1e-5f) {
        inv = 1.0f / self->dir[2];
        t1 = (dz + hz) * inv;
        t2 = (dz - hz) * inv;
        if (t1 > t2) {
            tmin = t2 > tmin ? t2 : tmin;
            tmax = t1 < tmax ? t1 : tmax;
        } else {
            tmin = t1 > tmin ? t1 : tmin;
            tmax = t2 < tmax ? t2 : tmax;
        }
        if (tmin > tmax || 0.0f > tmax)
            return 0;
    } else {
        hz *= 0.7f;
        if (-dz - hz > 0.0f || 0.0f > hz - dz)
            return 0;
    }

    if (tmin > 0.0f)
        return tmin > self->maxT ? 0 : 1;
    return tmax > self->maxT ? 0 : 1;
}

static void ray_picker_collect(struct RayPicker *self, struct SceneGraphNode *node, int level)
{
    int i;

    if (level == SCENE_GRAPH_DEPTH) {
        self->hits[self->count++] = node;
        return;
    }
    for (i = 0; i < 4; i++)
        if (node->child[i] && ray_picker_hits_box(self, node->child[i]))
            ray_picker_collect(self, node->child[i], level + 1);
}

void ray_picker_pick_ray(struct RayPicker *self, const struct Ray3 *ray,
                                   struct SceneGraphNode *root)
{
    int i;

    self->dir[0] = ray->dir.x;
    self->dir[1] = ray->dir.y;
    self->dir[2] = ray->dir.z;
    self->count = 0;
    self->origin[0] = ray->origin.x;
    self->origin[1] = ray->origin.y;
    self->origin[2] = ray->origin.z;
    self->maxT = SCENE_HEIGHT_HUGE;
    if (!root) return;
    for (i = 0; i < 4; i++)
        if (root->child[i] && ray_picker_hits_box(self, root->child[i]))
            ray_picker_collect(self, root->child[i], 1);
}
