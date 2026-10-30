#include "render/device_palette.h"
#include "render/device.h"

void c3d_device_convert_palette(struct C3dDevice *self, DWORD *dst, const BYTE *src, int n)
{
    const BYTE *end = src + n * 4;
    for (; src != end; src += 4, dst++) {
        if (src[0] >= 0xfe && src[1] < 4 && src[2] >= 0xfe)
            *dst = self->colorKeyPixel;
        else
            *dst = (((((DWORD)(src[0] & 0xf8) + 0x100) << 5) + (src[1] & 0xf8)) << 2) + (src[2] >> 3);
    }
}
