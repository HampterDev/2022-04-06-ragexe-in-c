#ifndef RENDER_TEXEL_CONVERT_H
#define RENDER_TEXEL_CONVERT_H

#include "platform/win_types.h"
#include "render/gfx_format.h"

/* The client's texels (render/gfx_format.h: GFX_FMT_1555, GFX_FMT_4444, GFX_FMT_565 and
   GFX_FMT_8888, as 16-bit words or A8R8G8B8 words) and the B8G8R8A8 bytes the GPU's textures hold,
   which are an A8R8G8B8 word's bytes. A channel of fewer than 8 bits widens by repeating its bits,
   as Direct3D 9 devices sample 16-bit textures; one 8 bits wide narrows by dropping its low bits,
   as the client's own conversions do. Rows are `pitch` bytes apart. */

/* A8R8G8B8 of one texel, and the texel of an A8R8G8B8 colour. */
DWORD texel_unpack(int fmt, DWORD texel);
DWORD texel_pack(int fmt, DWORD argb);
/* bytes per texel: 4 for GFX_FMT_8888, 2 for the others */
int texel_bytes(int fmt);

void texel_rows_to_bgra(int fmt, const void *src, long srcPitch, int w, int h, void *dst, long dstPitch);
void texel_rows_from_bgra(int fmt, const void *src, long srcPitch, int w, int h, void *dst, long dstPitch);
/* A system-memory surface's X8R8G8B8 rows: the colour key's texels transparent with their colour
   kept, the others opaque. */
void texel_rows_keyed_to_bgra(const void *src, long srcPitch, int w, int h, DWORD key, void *dst, long dstPitch);

#endif
