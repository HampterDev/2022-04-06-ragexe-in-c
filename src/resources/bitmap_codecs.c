#include "resources/bitmap_internal.h"
#include "resources/bitmap_format.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static unsigned image_u16(const BYTE *p)
{ return (unsigned)p[0] | (unsigned)p[1] << 8; }
static uint32_t image_u32(const BYTE *p)
{ return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static DWORD image_bgr(const BYTE *p)
{ return 0xff000000u | (DWORD)p[2] << 16 | (DWORD)p[1] << 8 | p[0]; }
static int image_span(size_t size, size_t offset, size_t count)
{ return offset <= size && count <= size - offset; }

BYTE bitmap_res_load_bmp_data(struct BitmapRes *self, const BYTE *data, int size, BYTE need24)
{
    int width, height, bottom_up = 1, custom;
    unsigned bits, colors = 0;
    size_t offset, stride, count, x, y;
    const BYTE *palette = NULL;
    DWORD *pixels, table[256];
    if (!self || !data || size < 17)
        return 0;
    custom = !memcmp(data, BMP_CLIENT_MAGIC, 12);
    if (custom) {
        uint32_t dimensions = (image_u32(data + 12) >> 1) - 0x1929300;
        width = (int)(dimensions >> 16);
        height = (int)(dimensions & 65535) - 1;
        if (data[16] > 1) return 0;
        bits = data[16] ? 8 : 24;
        offset = 17;
        if (bits == 8) {
            if (!image_span(size, offset, 4)) return 0;
            colors = image_u32(data + offset);
            if (!colors) colors = 256;
            if (colors > 256 || !image_span(size, offset + 4, colors * 4)) return 0;
            palette = data + offset + 4;
            offset += 4 + colors * 4;
        }
    } else {
        uint32_t header, compression, raw_height;
        if (size < 54 || data[0] != 'B' || data[1] != 'M') return 0;
        header = image_u32(data + 14);
        if (header < 40 || !image_span(size, 14, header)) return 0;
        width = (int32_t)image_u32(data + 18);
        raw_height = image_u32(data + 22);
        if (raw_height == 0x80000000u) return 0;
        bottom_up = (int32_t)raw_height >= 0;
        height = bottom_up ? (int)raw_height : -(int32_t)raw_height;
        bits = image_u16(data + 28);
        compression = image_u32(data + 30);
        if (image_u16(data + 26) != 1 || compression || (bits != 8 && bits != 24)) return 0;
        offset = image_u32(data + 10);
        if (bits == 8) {
            colors = image_u32(data + 46);
            if (!colors) colors = 256;
            if (colors > 256 || !image_span(size, 14 + (size_t)header, colors * 4)) return 0;
            palette = data + 14 + header;
        }
        if (offset < 14 + (size_t)header + colors * 4) return 0;
    }
    count = bitmap_pixel_count(width, height);
    if (!count || (need24 && (width != 24 || height != 24))) return 0;
    stride = ((size_t)width * (bits / 8) + 3) & ~(size_t)3;
    if (!image_span(size, offset, custom && bits == 24 ? count * 3 : stride * height)) return 0;
    pixels = bitmap_allocate_pixels(width, height);
    if (!pixels) return 0;
    if (custom && bits == 24) {
        const BYTE *gr = data + offset, *blue = gr + count * 2;
        for (x = 0; x < count; ++x)
            pixels[x] = 0xff000000u | (DWORD)gr[2 * x + 1] << 16 | (DWORD)gr[2 * x] << 8 | blue[x];
    } else {
        if (bits == 8)
            for (x = 0; x < 256; ++x)
                table[x] = x >= colors ? 0xff000000u
                         : custom ? 0xff000000u | (DWORD)palette[colors * 2 + x * 2 + 1] << 16 |
                                    (DWORD)palette[x * 2] << 8 | palette[x * 2 + 1]
                         : image_bgr(palette + x * 4);
        for (y = 0; y < (size_t)height; ++y) {
            const BYTE *row = data + offset + y * stride;
            DWORD *out = pixels + (bottom_up ? height - 1 - y : y) * width;
            for (x = 0; x < (size_t)width; ++x) {
                if (bits == 24) out[x] = image_bgr(row + x * 3);
                else out[x] = table[row[x]];
            }
        }
    }
    bitmap_replace_pixels(self, pixels, width, height, 0);
    return 1;
}

BYTE bitmap_res_load_tga_data(struct BitmapRes *self, const BYTE *data, int size)
{
    int width, height;
    unsigned bytes, type;
    size_t count, offset, written = 0;
    DWORD *pixels;
    if (!self || !data || size < 18 || data[1]) return 0;
    type = data[2];
    if ((type != 2 && type != 10) || (data[16] != 24 && data[16] != 32)) return 0;
    width = image_u16(data + 12);
    height = image_u16(data + 14);
    count = bitmap_pixel_count(width, height);
    if (!count) return 0;
    bytes = data[16] / 8;
    offset = 18 + data[0];
    if (!image_span(size, offset, 0)) return 0;
    pixels = bitmap_allocate_pixels(width, height);
    if (!pixels) return 0;
    while (written < count) {
        size_t run = count - written, i;
        int repeat = 0;
        if (type == 10) {
            BYTE packet;
            if (!image_span(size, offset, 1)) goto invalid;
            packet = data[offset++];
            run = (packet & 127) + 1;
            repeat = (packet & 128) != 0;
            if (run > count - written) goto invalid;
        }
        if (!image_span(size, offset, (repeat ? 1 : run) * bytes)) goto invalid;
        for (i = 0; i < run; ++i, ++written) {
            const BYTE *source = data + offset + (repeat ? 0 : i * bytes);
            size_t x = written % width, y = height - 1 - written / width;
            DWORD pixel = image_bgr(source);
            if (bytes == 4) pixel = (pixel & 0xffffff) | (DWORD)source[3] << 24;
            pixels[y * width + x] = pixel;
        }
        offset += (repeat ? 1 : run) * bytes;
    }
    bitmap_replace_pixels(self, pixels, width, height, 1);
    return 1;
invalid:
    free(pixels);
    return 0;
}
