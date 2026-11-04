#include "resources/attr.h"
#include "platform/error_message.h"
#include "std/string.h"
#include "resources/resource_ops.h"
#include "fileio/resource_api.h"
#include "fileio/res_mgr.h"
#include "resources/resource_factories.h"
#include "fileio/file_api.h"
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "world/mode.h"

struct C3dAttr *attribute_resource_init(struct C3dAttr *self)
{
    memset(self, 0, sizeof *self);
    res_construct(&self->res);
    self->res.ops = &attribute_resource_ops;
    self->zoom = 5;
    return self;
}

struct Res *attribute_resource_create(void)
{
    struct C3dAttr *self = malloc(sizeof *self);
    if (!self)
        return NULL;
    attribute_resource_init(self);
    return &self->res;
}

void attribute_resource_clear(struct C3dAttr *self)
{
    free(self->cells);
    self->cells = NULL;
    self->width = self->height = 0;
}

void attribute_resource_finish(struct C3dAttr *self)
{
    attribute_resource_clear(self);
    res_destruct(&self->res);
}

unsigned char c3d_attr_load(struct C3dAttr *self, const char *name)
{
    struct File file;
    struct AttrCell *cells = NULL;
    char magic[4];
    unsigned char version[2], ok = 0;
    int width, height;
    size_t count, bytes, i;

    file_construct(&file);
    if (!file_open(&file, name, 0, 1, 0)
        || !file_read(&file, magic, sizeof magic))
        goto done;
    if (memcmp(magic, "GRAT", sizeof magic) != 0) {
        error_msg("Block Attr 이 지원되지 않는 옛버젼의 GAT "
                 "파일입니다. 새 버젼의 RPGSTUDIO "
                 "에서 교정하세요", 0);
        goto done;
    }
    if (!file_read(&file, version, sizeof version))
        goto done;
    if (version[0] != 1 || version[1] > 3) {
        error_msg("C3dAttr::Unsupported Version", 0);
        ok = 1;
        goto done;
    }
    if (!file_read(&file, &width, sizeof width)
        || !file_read(&file, &height, sizeof height)
        || width < 0 || height < 0)
        goto done;
    if (width && (size_t)height > SIZE_MAX / (size_t)width)
        goto done;
    count = (size_t)width * (size_t)height;
    if (count > INT_MAX / sizeof *cells)
        goto done;
    bytes = count * sizeof *cells;
    if (file.position < 0 || file.size < file.position
        || bytes > (size_t)(file.size - file.position))
        goto done;
    if (count) {
        cells = malloc(bytes);
        if (!cells || !file_read(&file, cells, (int)bytes))
            goto done;
    }
    if (version[1] == 3)
        for (i = 0; i < count; ++i)
            cells[i].type &= 0x7fffffff;
    attribute_resource_clear(self);
    self->cells = cells;
    self->width = width;
    self->height = height;
    cells = NULL;
    ok = 1;
done:
    free(cells);
    file_destruct(&file);
    return ok;
}

BYTE c3d_attr_cell_walkable(struct C3dAttr *self, int x, int y)
{
    struct AttrCell *cell = c3d_attr_get_cell(self, x, y);
    if (!cell)
        return 0;
    return cell->type != 1 && cell->type != 5;
}

void c3d_attr_destroy(struct C3dAttr *self)
{
    attribute_resource_finish(self);
    free(self);
}
static void attribute_resource_destroy(struct Res *resource)
{
    c3d_attr_destroy((struct C3dAttr *)resource);
}
static int attribute_resource_load(struct Res *resource, const char *name)
{
    return c3d_attr_load((struct C3dAttr *)resource, name);
}
const struct ResourceOps attribute_resource_ops = {
    .destroy = attribute_resource_destroy,
    .load = attribute_resource_load,
};
