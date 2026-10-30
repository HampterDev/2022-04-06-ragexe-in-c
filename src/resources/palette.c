#include "resources/palette.h"
#include "resources/resource_factories.h"
#include "resources/resource_ops.h"
#include "fileio/res_mgr.h"
#include "fileio/file.h"
#include "render/device.h"
#include "render/device_palette.h"
#include "render/sprite_atlas.h"
#include "render/renderer.h"
#include <stdlib.h>
#include <string.h>

struct Res *palette_resource_create(void)
{
    struct PaletteRes *palette = calloc(1, sizeof *palette);
    if (!palette)
        return NULL;
    res_construct(&palette->res);
    palette->res.ops = &palette_resource_ops;
    return &palette->res;
}

static int palette_resource_load(struct Res *resource, const char *name)
{
    struct PaletteRes *palette = (struct PaletteRes *)resource;
    struct File file;
    BYTE raw[sizeof palette->raw];
    DWORD colors[256];
    int ok = 0;
    if (!name)
        return 0;
    file_construct(&file);
    if (file_open(&file, name, CFILE_MODE_READ, 1, 0) &&
        file.size >= (int)sizeof raw && file_read(&file, raw, sizeof raw)) {
        c3d_device_convert_palette(&g_3dDevice, colors, raw, 256);
        sprite_atlas_invalidate_palette(g_renderer, palette->palette);
        memcpy(palette->raw, raw, sizeof raw);
        memcpy(palette->palette, colors, sizeof colors);
        ok = 1;
    }
    file_destruct(&file);
    return ok;
}

static void palette_resource_destroy(struct Res *resource)
{
    sprite_atlas_invalidate_palette(g_renderer, ((struct PaletteRes *)resource)->palette);
    free(resource);
}

const struct ResourceOps palette_resource_ops = {
    .destroy = palette_resource_destroy,
    .load = palette_resource_load,
};
