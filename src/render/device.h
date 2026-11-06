#ifndef RENDER_DEVICE_H
#define RENDER_DEVICE_H

#include "render/gfx.h"
#include "base.h"
#include <stddef.h>

struct C3dDevice {
    HWND hwnd;
    int width, height;
    RECT rcClient;
    RECT rcWindow;
    gfx_surface *primary;
    gfx_surface *back;
    gfx_surface *target;
    gfx_surface *zbuffer;
    void *dd;
    void *d3d;
    void *dev;
    BYTE devcaps[0xec];
    DWORD zbufferCaps;
    BYTE zformat[0x20];
    struct SurfaceDesc primaryDesc;
    int fullscreen;
    int bltToClient;
    int desktopBpp;
    gfx_surface *save;
    DWORD rShift, rBits;
    DWORD gShift, gBits;
    DWORD bShift, bBits;
    DWORD aShift, aBits;
    DWORD bpp;
    DWORD rMask, gMask, bMask, aMask;
    DWORD minTexW, minTexH;
    DWORD maxTexW, maxTexH;
    DWORD texAspect;
    BYTE bltStretch;
    BYTE tex1024;
    BYTE surfacesLost;
    DWORD colorKeyPixel;
    int faceCharsets[12];
    BYTE hexDigitFlags[0x140];
    signed char hexDigitValues[0x140];
};

extern struct C3dDevice g_3dDevice;

struct Renderer;

void c3d_device_clamp_texture_size(struct C3dDevice *self, unsigned *pw, unsigned *ph);
void c3d_device_save_frame(struct C3dDevice *self);
int c3d_device_check_lost(struct C3dDevice *self);
int c3d_device_clear(struct C3dDevice *self, DWORD colour);
int c3d_device_clear_z(struct C3dDevice *self);
RECT *window_client_rect_on_monitor(RECT *out, HWND hwnd);
int c3d_device_set_display_mode(struct C3dDevice *self, const struct SurfaceDesc *mode, int flags);
int c3d_device_create_environment(struct C3dDevice *self, const GUID *adapter, const GUID *device,
                                            const struct SurfaceDesc *mode, int flags);
int c3d_device_init(struct C3dDevice *self, HWND hwnd, const GUID *adapter, const GUID *device,
                               const struct SurfaceDesc *mode, int flags);
void c3d_device_enable_mipmap(struct C3dDevice *self);
struct Renderer *c3d_device_create_renderer(struct C3dDevice *self);
void c3d_device_delete_renderer(struct C3dDevice *self, struct Renderer *r);
int c3d_device_release_all(struct C3dDevice *self);
void device_flip_to_gdi(void);
void c3d_device_restore_frame(struct C3dDevice *self);
int c3d_device_restore_surfaces(struct C3dDevice *self);
void c3d_device_on_surfaces_lost(struct C3dDevice *self);
int c3d_device_show_frame(struct C3dDevice *self, char vsync);
int c3d_device_probe_texel_offset(struct C3dDevice *self);

#endif
