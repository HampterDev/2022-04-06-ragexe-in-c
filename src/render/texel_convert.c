#include "render/texel_convert.h"
#include <string.h>

static DWORD widen5(DWORD v)
{
    v &= 31;
    return v << 3 | v >> 2;
}

static DWORD widen6(DWORD v)
{
    v &= 63;
    return v << 2 | v >> 4;
}

DWORD texel_unpack(int fmt, DWORD p)
{
    switch (fmt) {
    case GFX_FMT_565:
        return 0xff000000u | widen5(p >> 11) << 16 | widen6(p >> 5) << 8 | widen5(p);
    case GFX_FMT_1555:
        return (p & 0x8000 ? 0xff000000u : 0) | widen5(p >> 10) << 16 | widen5(p >> 5) << 8 | widen5(p);
    case GFX_FMT_4444:
        return (p >> 12 & 15) * 17 << 24 | (p >> 8 & 15) * 17 << 16 | (p >> 4 & 15) * 17 << 8 | (p & 15) * 17;
    default:
        return p;
    }
}

DWORD texel_pack(int fmt, DWORD argb)
{
    DWORD a = argb >> 24, r = argb >> 16 & 0xff, g = argb >> 8 & 0xff, b = argb & 0xff;
    switch (fmt) {
    case GFX_FMT_565:  return r >> 3 << 11 | g >> 2 << 5 | b >> 3;
    case GFX_FMT_1555: return a >> 7 << 15 | r >> 3 << 10 | g >> 3 << 5 | b >> 3;
    case GFX_FMT_4444: return a >> 4 << 12 | r >> 4 << 8 | g >> 4 << 4 | b >> 4;
    default:           return argb;
    }
}

int texel_bytes(int fmt)
{
    return fmt == GFX_FMT_1555 || fmt == GFX_FMT_4444 || fmt == GFX_FMT_565 ? 2 : 4;
}

static DWORD read_texel(const unsigned char *p, int bytes)
{
    DWORD v = 0;
    memcpy(&v, p, (size_t)bytes);
    return v;
}

void texel_rows_to_bgra(int fmt, const void *src, long srcPitch, int w, int h, void *dst, long dstPitch)
{
    int bytes = texel_bytes(fmt), x, y;
    for (y = 0; y < h; y++) {
        const unsigned char *in = (const unsigned char *)src + (size_t)y * srcPitch;
        unsigned char *out = (unsigned char *)dst + (size_t)y * dstPitch;
        if (bytes == 4) {
            memcpy(out, in, (size_t)w * 4);
            continue;
        }
        for (x = 0; x < w; x++) {
            DWORD v = texel_unpack(fmt, read_texel(in + (size_t)x * 2, 2));
            memcpy(out + (size_t)x * 4, &v, 4);
        }
    }
}

void texel_rows_from_bgra(int fmt, const void *src, long srcPitch, int w, int h, void *dst, long dstPitch)
{
    int bytes = texel_bytes(fmt), x, y;
    for (y = 0; y < h; y++) {
        const unsigned char *in = (const unsigned char *)src + (size_t)y * srcPitch;
        unsigned char *out = (unsigned char *)dst + (size_t)y * dstPitch;
        if (bytes == 4) {
            memcpy(out, in, (size_t)w * 4);
            continue;
        }
        for (x = 0; x < w; x++) {
            DWORD v = texel_pack(fmt, read_texel(in + (size_t)x * 4, 4));
            out[x * 2] = (unsigned char)v;
            out[x * 2 + 1] = (unsigned char)(v >> 8);
        }
    }
}

void texel_rows_keyed_to_bgra(const void *src, long srcPitch, int w, int h, DWORD key, void *dst, long dstPitch)
{
    int x, y;
    for (y = 0; y < h; y++) {
        const unsigned char *in = (const unsigned char *)src + (size_t)y * srcPitch;
        unsigned char *out = (unsigned char *)dst + (size_t)y * dstPitch;
        for (x = 0; x < w; x++) {
            DWORD v = read_texel(in + (size_t)x * 4, 4);
            v = v == key ? v & 0xffffffu : v | 0xff000000u;
            memcpy(out + (size_t)x * 4, &v, 4);
        }
    }
}
