#ifndef RESOURCES_ATTR_QUERIES_H
#define RESOURCES_ATTR_QUERIES_H

#define WIN32_LEAN_AND_MEAN
#include "render/math3d.h"
#include "platform/win_types.h"
struct C3dAttr;

float c3d_attr_get_height(struct C3dAttr *self, float x, float z);
/* custom: the height at the point of the map nearest to (x, z): c3d_attr_get_height answers 0
   outside the map, which can lie below the map's ground */
float c3d_attr_get_height_nearest(struct C3dAttr *self, float x, float z);
void c3d_attr_get_height_min_max(struct C3dAttr *self, const RECT *cells, float *low, float *high);
float ground_ray_distance(struct C3dAttr *self, const struct Ray3 *ray, int x, int y);

#endif
