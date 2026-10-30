#ifndef RESOURCES_BITMAP_H
#define RESOURCES_BITMAP_H

#include "resources/resource_ops.h"
#include "resources/bitmap_types.h"

struct BitmapRes *bitmap_resource_init(struct BitmapRes *self);
struct GifRes *gif_resource_init(struct GifRes *self);
void bitmap_resource_finish(struct BitmapRes *self);
void gif_resource_finish(struct GifRes *self);
void bitmap_resource_clear(struct BitmapRes *self);
void gif_resource_clear(struct GifRes *self);
BYTE bitmap_res_load(struct BitmapRes *self, const char *name);
BYTE gif_res_load(struct GifRes *self, const char *name);
BYTE bitmap_res_load_from_buffer(struct BitmapRes *self, const char *name,
                              const BYTE *data, int size, BYTE need24);
BYTE gif_res_load_from_buffer(struct GifRes *self, const char *name,
                           const BYTE *data, int size, BYTE need24);
BYTE gif_res_add_blank_frame(struct GifRes *self, int width, int height);
BYTE bitmap_res_make_gray_copy(struct BitmapRes *self, int mode);
void bitmap_res_free_gray_copy(struct BitmapRes *self);
DWORD bitmap_res_get_pixel(const struct BitmapRes *self, int x, int y);
BYTE gif_res_animate(struct GifRes *self);

BYTE bitmap_res_load_bmp_data(struct BitmapRes *self, const BYTE *data, int size, BYTE need24);
BYTE bitmap_res_load_tga_data(struct BitmapRes *self, const BYTE *data, int size);
BYTE bitmap_res_load_jpg_data(struct BitmapRes *self, const BYTE *data, int size);
BYTE bitmap_res_load_png_data(struct BitmapRes *self, const BYTE *data, int size);
BYTE gif_res_load_gif_data(struct GifRes *self, const BYTE *data, int size, BYTE need24);
#endif
