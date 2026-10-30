#include "resources/emblem.h"
#include "fileio/res_mgr.h"
#include "resources/resource_factories.h"
#include <zlib.h>
#include <stdint.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

struct EmblemRes *emblem_resource_init(struct EmblemRes *self)
{
    res_construct(&self->res);
    self->res.ops = &emblem_resource_ops;
    gif_resource_init(&self->picture);
    return self;
}

struct Res *emblem_resource_create(void)
{
    struct EmblemRes *self = malloc(sizeof *self);
    return self ? &emblem_resource_init(self)->res : NULL;
}

BYTE emblem_res_load(struct EmblemRes *self, const char *name)
{
    struct File file;
    uint64_t bound;
    unsigned long size;
    BYTE *buffer = NULL, ok = 0;
    if (!self || !name) return 0;
    if (guild_mgr_is_bad_emblem(g_guildMgr, string_from_cstr(name)))
        return gif_res_add_blank_frame(&self->picture, 24, 24);
    bound = (uint64_t)g_guildMgr->emblem_max_size * 6 / 5;
    if (!bound || bound > INT_MAX) return 0;
    file_construct(&file);
    if (file_open(&file, name, 0, 1, 0)) {
        size = (unsigned long)bound;
        buffer = malloc(size);
        if (buffer) {
            if (file.size > 0 && file.buffer &&
                uncompress(buffer, &size, file.buffer, file.size) == 0 &&
                gif_res_load_from_buffer(&self->picture, "_emblem.bmp", buffer, (int)size, 1)) {
                ok = 1;
            } else {
                ok = gif_res_add_blank_frame(&self->picture, 24, 24);
                if (ok) guild_mgr_mark_bad_emblem(g_guildMgr, string_from_cstr(name));
            }
        }
        free(buffer);
        file_close(&file);
    }
    file_destruct(&file);
    return ok;
}

BYTE emblem_res_animate(struct EmblemRes *self)
{ return gif_res_animate(&self->picture); }
void emblem_resource_finish(struct EmblemRes *self)
{
    gif_resource_finish(&self->picture);
    res_destruct(&self->res);
}

static void emblem_resource_destroy(struct Res *res)
{
    emblem_resource_finish((struct EmblemRes *)res);
    free(res);
}
static void emblem_resource_process(struct Res *res)
{ emblem_res_animate((struct EmblemRes *)res); }
static int emblem_resource_resident(const struct Res *res)
{ return ((const struct EmblemRes *)res)->picture.animated; }
static int emblem_resource_load(struct Res *res, const char *name)
{ return emblem_res_load((struct EmblemRes *)res, name); }

const struct ResourceOps emblem_resource_ops = {
    .destroy = emblem_resource_destroy,
    .load = emblem_resource_load,
    .update = emblem_resource_process,
    .is_resident = emblem_resource_resident,
};
