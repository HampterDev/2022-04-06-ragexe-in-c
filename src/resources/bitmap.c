#include "resources/bitmap_internal.h"
#include "fileio/res_mgr.h"
#include "resources/resource_factories.h"
#include <stdlib.h>
#include <string.h>
#include "platform/clock.h"
#include "core/cstr.h"

struct BitmapRes *bitmap_resource_init(struct BitmapRes *self)
{
    memset(self, 0, sizeof *self);
    res_construct(&self->res);
    self->res.ops = &bitmap_resource_ops;
    return self;
}

struct GifRes *gif_resource_init(struct GifRes *self)
{
    memset(self, 0, sizeof *self);
    bitmap_resource_init(&self->bmp);
    self->bmp.res.ops = &gif_resource_ops;
    return self;
}

struct Res *bitmap_resource_create(void)
{
    struct BitmapRes *self = malloc(sizeof *self);
    return self ? &bitmap_resource_init(self)->res : NULL;
}

struct Res *gif_resource_create(void)
{
    struct GifRes *self = malloc(sizeof *self);
    return self ? &gif_resource_init(self)->bmp.res : NULL;
}

void bitmap_resource_finish(struct BitmapRes *self)
{
    bitmap_resource_clear(self);
    res_destruct(&self->res);
}

void gif_resource_finish(struct GifRes *self)
{
    gif_resource_clear(self);
    res_destruct(&self->bmp.res);
}

BYTE bitmap_res_load_from_buffer(struct BitmapRes *self, const char *name,
                              const BYTE *data, int size, BYTE need24)
{
    const char *ext;
    if (!self || !name || !data || size <= 0 || !(ext = strrchr(name, '.')))
        return 0;
    if (!cstr_compare_nocase(ext, ".bmp")) return bitmap_res_load_bmp_data(self, data, size, need24);
    if (!cstr_compare_nocase(ext, ".tga")) return bitmap_res_load_tga_data(self, data, size);
    if (!cstr_compare_nocase(ext, ".jpg")) return bitmap_res_load_jpg_data(self, data, size);
    if (!cstr_compare_nocase(ext, ".png")) return bitmap_res_load_png_data(self, data, size);
    return 0;
}

BYTE gif_res_load_from_buffer(struct GifRes *self, const char *name,
                           const BYTE *data, int size, BYTE need24)
{
    struct BitmapRes image = {0};
    if (!self || !data || size <= 0)
        return 0;
    if (size >= 3 && !memcmp(data, "GIF", 3))
        return gif_res_load_gif_data(self, data, size, need24);
    if (!bitmap_res_load_from_buffer(&image, name, data, size, need24))
        return 0;
    if (need24 && (image.width != 24 || image.height != 24)) {
        bitmap_resource_clear(&image);
        return 0;
    }
    gif_resource_clear(self);
    bitmap_replace_pixels(&self->bmp, image.pixels, image.width, image.height, image.hasAlpha);
    return 1;
}

BYTE bitmap_res_load(struct BitmapRes *self, const char *name)
{
    struct File file;
    BYTE ok = 0;
    file_construct(&file);
    if (name && file_open(&file, name, 0, 1, 0)) {
        ok = bitmap_res_load_from_buffer(self, name, file.buffer, file.size, 0);
        file_close(&file);
    }
    file_destruct(&file);
    return ok;
}

BYTE gif_res_load(struct GifRes *self, const char *name)
{
    struct File file;
    BYTE ok = 0;
    file_construct(&file);
    if (name && file_open(&file, name, 0, 1, 0)) {
        ok = gif_res_load_from_buffer(self, name, file.buffer, file.size, 0);
        file_close(&file);
    }
    file_destruct(&file);
    return ok;
}

BYTE gif_res_animate(struct GifRes *self)
{
    DWORD now;
    self->changed = 0;
    if (self->frames.count <= 1 || self->curFrame >= self->frames.count)
        return 0;
    now = platform_ticks_ms();
    if ((DWORD)(now - self->lastTime) < self->frames.data[self->curFrame].delay * 10u)
        return 0;
    self->lastTime = now;
    self->curFrame = (unsigned short)((self->curFrame + 1u) % self->frames.count);
    self->bmp.pixels = self->frames.data[self->curFrame].pixels;
    bitmap_res_free_gray_copy(&self->bmp);
    self->changed = 1;
    return 1;
}

static void bitmap_resource_destroy(struct Res *res)
{
    bitmap_resource_finish((struct BitmapRes *)res);
    free(res);
}
static int bitmap_resource_load(struct Res *res, const char *name)
{ return bitmap_res_load((struct BitmapRes *)res, name); }
static void gif_resource_destroy(struct Res *res)
{
    gif_resource_finish((struct GifRes *)res);
    free(res);
}
static void gif_resource_process(struct Res *res)
{ gif_res_animate((struct GifRes *)res); }
static int gif_resource_resident(const struct Res *res)
{ return ((const struct GifRes *)res)->animated; }
static int gif_resource_load(struct Res *res, const char *name)
{ return gif_res_load((struct GifRes *)res, name); }

const struct ResourceOps bitmap_resource_ops = {
    .destroy = bitmap_resource_destroy,
    .load = bitmap_resource_load,
};

const struct ResourceOps gif_resource_ops = {
    .destroy = gif_resource_destroy,
    .load = gif_resource_load,
    .update = gif_resource_process,
    .is_resident = gif_resource_resident,
};
