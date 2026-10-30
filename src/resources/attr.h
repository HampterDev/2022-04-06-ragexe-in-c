#ifndef RESOURCES_ATTR_H
#define RESOURCES_ATTR_H

#include "resources/resource_ops.h"

#include "resources/attr_types.h"
#include "resources/attr_queries.h"

struct C3dAttr *attribute_resource_init(struct C3dAttr *self);
struct Res *attribute_resource_create(void);
unsigned char c3d_attr_load(struct C3dAttr *self, const char *name);
void c3d_attr_destroy(struct C3dAttr *self);
void attribute_resource_clear(struct C3dAttr *self);
void attribute_resource_finish(struct C3dAttr *self);

struct AttrCell *c3d_attr_get_cell(const struct C3dAttr *self, int x, int y);
unsigned char c3d_attr_cell_walkable(struct C3dAttr *self, int x, int y);

#endif
