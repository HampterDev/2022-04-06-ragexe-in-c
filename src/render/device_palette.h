#ifndef RENDER_DEVICE_PALETTE_H
#define RENDER_DEVICE_PALETTE_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

struct C3dDevice;
void c3d_device_convert_palette(struct C3dDevice *device, DWORD *dst,
    const BYTE *src, int count);

#endif
