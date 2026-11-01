#include "ui/chat/captcha_data.h"
#include "resources/bitmap_format.h"
#include <zlib.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void captcha_data_clear(struct CaptchaData *data)
{
    free(data->bytes);
    *data = (struct CaptchaData){0};
}

void captcha_stream_clear(struct CaptchaStream *stream)
{
    captcha_data_clear(&stream->packed);
    stream->received = 0;
}

int captcha_stream_begin(struct CaptchaStream *stream, size_t size)
{
    unsigned char *bytes;
    if (!size || size > CAPTCHA_PACKED_LIMIT) return 0;
    bytes = malloc(size);
    if (!bytes) return 0;
    captcha_stream_clear(stream);
    stream->packed.bytes = bytes;
    stream->packed.size = size;
    return 1;
}

enum CaptchaAppend captcha_stream_append(struct CaptchaStream *stream, const void *chunk, size_t size)
{
    if (!chunk || !size || size > CAPTCHA_CHUNK_LIMIT || !stream->packed.bytes
        || stream->received > stream->packed.size || size > stream->packed.size - stream->received)
        return CAPTCHA_REJECTED;
    memcpy(stream->packed.bytes + stream->received, chunk, size);
    stream->received += size;
    return stream->received == stream->packed.size ? CAPTCHA_COMPLETE : CAPTCHA_PENDING;
}

static uint16_t bitmap_u16(const unsigned char *p)
{
    return (uint16_t)(p[0] | (uint16_t)p[1] << 8);
}
static uint32_t bitmap_u32(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

int captcha_bitmap_valid(const void *bitmap, size_t size)
{
    const unsigned char *bytes = bitmap;
    size_t offset, stride, palette_count = 0;
    unsigned bits;
    size_t row, column;
    if (!bytes || size < 17 || size > CAPTCHA_BITMAP_LIMIT) return 0;
    if (!memcmp(bytes, BMP_CLIENT_MAGIC, 12)) {
        uint32_t dimensions = (bitmap_u32(bytes + 12) >> 1) - 0x1929300u;
        if ((dimensions >> 16) != CAPTCHA_WIDTH || (dimensions & 0xffff) != CAPTCHA_HEIGHT + 1) return 0;
        if (bytes[16] == 0) {
            return size >= 17 + CAPTCHA_WIDTH * CAPTCHA_HEIGHT * 3;
        }
        if (bytes[16] != 1 || size < 21) return 0;
        palette_count = bitmap_u32(bytes + 17);
        if (!palette_count) palette_count = 256;
        if (palette_count > 256) return 0;
        offset = 21 + palette_count * 4;
        bits = 8;
    } else {
        uint32_t file_size;
        if (size < 54 || bitmap_u16(bytes) != 0x4d42 || bitmap_u32(bytes + 14) != 40
            || bitmap_u32(bytes + 18) != CAPTCHA_WIDTH || bitmap_u32(bytes + 22) != CAPTCHA_HEIGHT
            || bitmap_u16(bytes + 26) != 1 || bitmap_u32(bytes + 30) != 0) return 0;
        bits = bitmap_u16(bytes + 28);
        if (bits == 8) {
            palette_count = bitmap_u32(bytes + 46);
            if (!palette_count) palette_count = 256;
            if (palette_count > 256) return 0;
        } else if (bits != 24) return 0;
        offset = 54 + palette_count * 4;
        if (bitmap_u32(bytes + 10) != offset) return 0;
        file_size = bitmap_u32(bytes + 2);
        if (file_size && (file_size > size || file_size < offset + ((CAPTCHA_WIDTH * bits + 31) / 32) * 4 * CAPTCHA_HEIGHT)) return 0;
    }
    stride = ((CAPTCHA_WIDTH * bits + 31) / 32) * 4;
    if (offset > size || stride * CAPTCHA_HEIGHT > size - offset) return 0;
    if (bits == 8 && palette_count < 256) {
        for (row = 0; row < CAPTCHA_HEIGHT; ++row)
            for (column = 0; column < CAPTCHA_WIDTH; ++column)
                if (bytes[offset + row * stride + column] >= palette_count) return 0;
    }
    return 1;
}

int captcha_data_unpack(struct CaptchaData *out, const struct CaptchaData *packed)
{
    struct CaptchaData next;
    uLongf length = CAPTCHA_BITMAP_LIMIT;
    if (!packed || !packed->bytes || !packed->size || packed->size > CAPTCHA_PACKED_LIMIT) return 0;
    next.bytes = malloc(CAPTCHA_BITMAP_LIMIT);
    if (!next.bytes) return 0;
    if (uncompress(next.bytes, &length, packed->bytes, (uLong)packed->size) != Z_OK
        || !captcha_bitmap_valid(next.bytes, length)) {
        free(next.bytes);
        return 0;
    }
    next.size = length;
    captcha_data_clear(out);
    *out = next;
    return 1;
}
