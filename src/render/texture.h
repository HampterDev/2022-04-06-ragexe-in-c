#ifndef RENDER_TEXTURE_H
#define RENDER_TEXTURE_H

#include "render/surface.h"

void texture_lock(struct Texture *texture);
void texture_unlock(struct Texture *texture);

struct Texture *texture_create(int width, int height, int format);

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
int texture_create_bump(struct Texture *texture, int width, int height);
int texture_update_mip_chain(struct Texture *texture, const RECT *rect);
int texture_copy_rect_from(struct Texture *texture, gfx_surface *source, RECT *rect);
int texture_blit_rect(struct Texture *texture, struct Surface *source, int sx, int sy, int sw, int sh,
                      int dx, int dy, int dw, int dh);

#endif
