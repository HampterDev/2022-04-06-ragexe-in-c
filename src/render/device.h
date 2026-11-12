#ifndef RENDER_DEVICE_H
#define RENDER_DEVICE_H

#include "render/gfx.h"
#include "platform/window_handle.h"
#include "base.h"
#include <stddef.h>

struct C3dDevice {
    PlatformWindow hwnd;
    int width, height;
    gfx_surface *target;  /* the screen (render/gfx.h, gfx_create_screen) */
    void *dev;            /* the GPU device, while there is one */
    gfx_surface *save;    /* the frame c3d_device_save_frame keeps */
    DWORD rShift, rBits;
    DWORD gShift, gBits;
    DWORD bShift, bBits;
    DWORD bpp;
    DWORD rMask, gMask, bMask;
    DWORD minTexW, minTexH;
    DWORD maxTexW, maxTexH;
    DWORD texAspect;
    DWORD colorKeyPixel;
    const wchar_t *faceNamesB[12];
    const wchar_t *faceNamesA[12];
    int faceCharsets[12];
};

extern struct C3dDevice g_3dDevice;

struct Renderer;

void c3d_device_clamp_texture_size(struct C3dDevice *self, unsigned *pw, unsigned *ph);
void c3d_device_save_frame(struct C3dDevice *self);
int c3d_device_clear(struct C3dDevice *self, DWORD colour);
int c3d_device_clear_z(struct C3dDevice *self);
int c3d_device_init(struct C3dDevice *self, PlatformWindow hwnd);
void c3d_device_enable_mipmap(struct C3dDevice *self);
struct Renderer *c3d_device_create_renderer(struct C3dDevice *self);
void c3d_device_delete_renderer(struct C3dDevice *self, struct Renderer *r);
int c3d_device_release_all(struct C3dDevice *self);
void c3d_device_restore_frame(struct C3dDevice *self);
int c3d_device_show_frame(struct C3dDevice *self, char vsync);

#endif
