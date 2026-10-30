#ifndef RENDER_SURFACE_H
#define RENDER_SURFACE_H

#include "render/texture_types.h"

struct SurfaceOps {
    int (*upload)(struct Surface *surface, int x, int y, int width, int height,
                   const DWORD *pixels, int keyed, int pitch);
    void (*stretch)(struct Surface *surface, int x, int y, int width, int height);
};

extern const struct SurfaceOps surface_ops;
extern const struct SurfaceOps wallpaper_surface_ops;
extern const struct SurfaceOps texture_surface_ops;

void surface_reset(struct Surface *surface);
void surface_destroy(struct Surface *surface);
static inline int surface_upload(struct Surface *surface, int x, int y, int width, int height,
                                  const DWORD *pixels, int keyed, int pitch)
{
    return surface && surface->ops->upload(surface, x, y, width, height, pixels, keyed, pitch);
}
static inline void surface_stretch(struct Surface *surface, int x, int y, int width, int height)
{
    if (surface)
        surface->ops->stretch(surface, x, y, width, height);
}

struct Surface *surface_construct_sys_mem(struct Surface *surface, int width, int height);
int surface_recreate(struct Surface *surface, int width, int height);
struct C3dDevice;
struct Surface *c3d_device_new_wallpaper_surface(struct C3dDevice *device, unsigned width, unsigned height);

#endif
