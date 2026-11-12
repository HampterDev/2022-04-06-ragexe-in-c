#ifndef RENDER_TEXTURE_H
#define RENDER_TEXTURE_H

#include "render/surface.h"

void texture_lock(struct Texture *texture);
void texture_unlock(struct Texture *texture);

struct Texture *texture_create(int width, int height, int format);
/* custom: a texture whose pixels are a render target's, width by height, which draws go into
   (gfx_set_render_target) and quads read as they read any texture; its pixels stay on the GPU. */
struct Texture *texture_create_target(int width, int height);
/* custom: a texture the device keeps as a layer of an array of its size (render/gfx.h,
   gfx_create_layer_texture), which only quads read: the sprite atlas's pages. */
struct Texture *texture_create_layer(int width, int height, int format);

struct Texture *texture_from_pixels(int width, int height, const DWORD *pixels,
    int format, int keyed, int full_size);
int texture_upload_pixels(struct Texture *texture, int x, int y, int width, int height,
                           const DWORD *pixels, int keyed, int pitch);

static inline void texture_destroy(struct Texture *texture)
{
    if (texture)
        surface_destroy(&texture->base);
}
static inline int texture_upload(struct Texture *texture, int x, int y, int width, int height,
                                 const DWORD *pixels, int keyed, int pitch)
{
    return texture && texture_upload_pixels(texture, x, y, width, height, pixels, keyed, pitch);
}

struct Texture *texture_init(struct Texture *texture, int width, int height, int format);
int texture_recreate(struct Texture *texture, int width, int height, int format);
int texture_update_mip_chain(struct Texture *texture, const RECT *rect);
int texture_blit_rect(struct Texture *texture, struct Surface *source, int sx, int sy, int sw, int sh,
                      int dx, int dy, int dw, int dh);

#endif
