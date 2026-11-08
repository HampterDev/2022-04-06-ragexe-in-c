#include "render/math3d.h"
#include "resources/attr.h"
#include <string.h>

struct AttrCell *c3d_attr_get_cell(const struct C3dAttr *attr, int x, int y)
{
    if (!attr || !attr->cells || x < 0 || x >= attr->width || y < 0 || y >= attr->height)
        return NULL;
    return attr->cells + (size_t)y * (size_t)attr->width + (size_t)x;
}

float c3d_attr_get_height(struct C3dAttr *self, float x, float y)
{
    float zoom = (float)self->zoom;
    float px = (float)self->width * zoom * 0.5f + x;
    float py = (float)self->height * zoom * 0.5f + y;
    int cx = (int)(px / zoom), cy = (int)(py / zoom);
    struct AttrCell *cell;
    float left, right;

    if (cx < 0 || cx >= self->width || cy < 0 || cy >= self->height)
        return 0.0f;
    cell = c3d_attr_get_cell(self, cx, cy);
    if (!cell)
        return 0.0f;
    py -= (float)cy * zoom;
    px -= (float)cx * zoom;
    left = (cell->height[2] - cell->height[0]) * py / zoom + cell->height[0];
    right = (cell->height[3] - cell->height[1]) * py / zoom + cell->height[1];
    return px * (right - left) / zoom + left;
}

float c3d_attr_get_height_nearest(struct C3dAttr *self, float x, float y)
{
    float half_w = (float)self->width * (float)self->zoom * 0.5f;
    float half_h = (float)self->height * (float)self->zoom * 0.5f;
    float inset = (float)self->zoom * 0.5f;

    if (self->width <= 0 || self->height <= 0)
        return 0.0f;
    if (x < -half_w + inset)
        x = -half_w + inset;
    else if (x > half_w - inset)
        x = half_w - inset;
    if (y < -half_h + inset)
        y = -half_h + inset;
    else if (y > half_h - inset)
        y = half_h - inset;
    return c3d_attr_get_height(self, x, y);
}

void c3d_attr_get_height_min_max(struct C3dAttr *self, const RECT *rect, float *low,
                                       float *high)
{
    int x, y;

    *low = 1000000.0f;
    *high = -1000000.0f;
    for (x = rect->left; x <= rect->right; x++) {
        for (y = rect->top; y <= rect->bottom; y++) {
            struct AttrCell *cell;
            float m;
            int i;

            if (x < 0 || x >= self->width || y < 0 || y >= self->height)
                continue;
            cell = c3d_attr_get_cell(self, x, y);
            if (!cell)
                continue;
            m = *high;
            for (i = 0; i < 4; i++)
                if (cell->height[i] > m) {
                    *high = cell->height[i];
                    m = cell->height[i];
                }
            m = *low;
            for (i = 0; i < 4; i++)
                if (m > cell->height[i]) {
                    *low = cell->height[i];
                    m = cell->height[i];
                }
        }
    }
}

float ground_ray_distance(struct C3dAttr *self, const struct Ray3 *ray, int x, int y)
{
    float first = 999999.0f, result = 999999.0f;
    Vec3 v[4];
    float zoom, u, w, t;
    struct AttrCell *cell;
    int sx, sy;

    if (x < 0 || x >= self->width || y < 0 || y >= self->height)
        return 0.0f;
    cell = c3d_attr_get_cell(self, x, y);
    if (!cell)
        return 0.0f;
    zoom = (float)self->zoom;
    sx = x - self->width / 2;
    sy = y - self->height / 2;
    v[0].x = (float)sx * zoom;        v[0].y = cell->height[0]; v[0].z = (float)sy * zoom;
    v[1].x = (float)(sx + 1) * zoom;  v[1].y = cell->height[1]; v[1].z = (float)sy * zoom;
    v[2].x = (float)sx * zoom;        v[2].y = cell->height[2]; v[2].z = (float)(sy + 1) * zoom;
    v[3].x = (float)(sx + 1) * zoom;  v[3].y = cell->height[3]; v[3].z = (float)(sy + 1) * zoom;
    if (ray_triangle_hit(ray, &v[0], &v[1], &v[2], &u, &w, &t)) {
        first = t < 999999.0f ? t : 999999.0f;
        result = first;
    }
    if (ray_triangle_hit(ray, &v[1], &v[3], &v[2], &u, &w, &t))
        result = t < first ? t : first;
    return result;
}
