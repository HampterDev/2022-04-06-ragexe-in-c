#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "owned_storage_test_allocator.h"
#define malloc checked_malloc
#define free checked_free
#include "../src/ui/chat/captcha_data.c"
#undef free
#undef malloc

#include "captcha_test_bitmap.h"

static void test_stream(void)
{
    struct CaptchaStream stream = {0};
    unsigned char chunk[1025];
    unsigned char *previous;
    size_t i;
    memset(chunk, 0x4d, sizeof chunk);
    assert(!captcha_stream_begin(&stream, 0) && !captcha_stream_begin(&stream, 65536));
    assert(captcha_stream_append(&stream, chunk, 1) == CAPTCHA_REJECTED);
    assert(captcha_stream_begin(&stream, 1025)); previous = stream.packed.bytes;
    assert(captcha_stream_append(&stream, chunk, 1) == CAPTCHA_PENDING);
    fail_at = allocation_calls + 1; assert(!captcha_stream_begin(&stream, 12)); fail_at = 0;
    assert(stream.packed.bytes == previous && stream.packed.size == 1025 && stream.received == 1 && previous[0] == 0x4d);
    assert(captcha_stream_append(&stream, chunk, 1025) == CAPTCHA_REJECTED);
    assert(captcha_stream_append(&stream, NULL, 1) == CAPTCHA_REJECTED);
    assert(captcha_stream_append(&stream, chunk, 0) == CAPTCHA_REJECTED);
    assert(stream.received == 1);
    assert(captcha_stream_append(&stream, chunk, 1024) == CAPTCHA_COMPLETE);
    assert(captcha_stream_append(&stream, chunk, 1) == CAPTCHA_REJECTED);
    assert(captcha_stream_begin(&stream, CAPTCHA_PACKED_LIMIT));
    for (i = 0; i < 63; ++i) assert(captcha_stream_append(&stream, chunk, 1024) == CAPTCHA_PENDING);
    assert(captcha_stream_append(&stream, chunk, 1023) == CAPTCHA_COMPLETE);
    assert(stream.received == CAPTCHA_PACKED_LIMIT);
    captcha_stream_clear(&stream); captcha_stream_clear(&stream); assert(!outstanding);
}

static void roundtrip(int custom, int bits, unsigned palette)
{
    struct CaptchaData packed = {0}, decoded = {0};
    size_t size, i, old_size;
    unsigned char *raw = make_bitmap(custom, bits, palette, &size), *old;
    assert(captcha_bitmap_valid(raw, size));
    for (i = 0; i < size; ++i) assert(!captcha_bitmap_valid(raw, i));
    assert(!captcha_bitmap_valid(NULL, size));
    assert(captcha_data_pack(&packed, raw, size));
    assert(packed.size && packed.size <= CAPTCHA_PACKED_LIMIT);
    assert(captcha_data_unpack(&decoded, &packed));
    assert(decoded.size == size && !memcmp(decoded.bytes, raw, size));
    old = packed.bytes; old_size = packed.size;
    fail_at = allocation_calls + 1; assert(!captcha_data_pack(&packed, raw, size)); fail_at = 0;
    assert(packed.bytes == old && packed.size == old_size);
    old = decoded.bytes; old_size = decoded.size;
    fail_at = allocation_calls + 1; assert(!captcha_data_unpack(&decoded, &packed)); fail_at = 0;
    assert(decoded.bytes == old && decoded.size == old_size);
    for (i = 0; i < packed.size; ++i) {
        struct CaptchaData truncated = {packed.bytes, i};
        assert(!captcha_data_unpack(&decoded, &truncated));
        assert(decoded.bytes == old && decoded.size == old_size);
    }
    packed.bytes[packed.size - 1] ^= 1; assert(!captcha_data_unpack(&decoded, &packed));
    assert(decoded.bytes == old); packed.bytes[packed.size - 1] ^= 1;
    captcha_data_clear(&packed); captcha_data_clear(&decoded); free(raw); assert(!outstanding);
}

static void test_invalid(void)
{
    size_t size;
    unsigned char *raw = make_bitmap(0, 8, 16, &size), *copy;
    struct CaptchaData packed = {0}, decoded = {0};
    uLongf compressed_size = CAPTCHA_PACKED_LIMIT;
    assert(captcha_bitmap_valid(raw, size));
    raw[54 + 16 * 4] = 16; assert(!captcha_bitmap_valid(raw, size)); raw[54 + 16 * 4] = 0;
    put32(raw + 46, 257); assert(!captcha_bitmap_valid(raw, size)); put32(raw + 46, 16);
    put32(raw + 10, 54); assert(!captcha_bitmap_valid(raw, size)); put32(raw + 10, 118);
    put32(raw + 30, 1); assert(!captcha_bitmap_valid(raw, size)); put32(raw + 30, 0);
    put16(raw + 26, 2); assert(!captcha_bitmap_valid(raw, size)); put16(raw + 26, 1);
    put32(raw + 18, 219); assert(!captcha_bitmap_valid(raw, size)); put32(raw + 18, 220);
    put32(raw + 22, (unsigned)-90); assert(!captcha_bitmap_valid(raw, size)); put32(raw + 22, 90);
    put32(raw + 14, 124); assert(!captcha_bitmap_valid(raw, size)); put32(raw + 14, 40);
    put32(raw + 2, 54); assert(!captcha_bitmap_valid(raw, size)); put32(raw + 2, (unsigned)size + 1);
    assert(!captcha_bitmap_valid(raw, size)); put32(raw + 2, (unsigned)size);
    assert(captcha_bitmap_valid(raw, size)); free(raw);
    raw = make_bitmap(1, 8, 16, &size);
    raw[21 + 16 * 4] = 16; assert(!captcha_bitmap_valid(raw, size)); raw[21 + 16 * 4] = 0;
    put32(raw + 17, 257); assert(!captcha_bitmap_valid(raw, size)); put32(raw + 17, 16);
    raw[16] = 2; assert(!captcha_bitmap_valid(raw, size)); raw[16] = 1;
    assert(captcha_bitmap_valid(raw, size)); free(raw);

    copy = calloc(1, CAPTCHA_BITMAP_LIMIT + 1); assert(copy);
    packed.bytes = checked_malloc(CAPTCHA_PACKED_LIMIT); assert(packed.bytes);
    assert(compress(packed.bytes, &compressed_size, copy, CAPTCHA_BITMAP_LIMIT + 1) == Z_OK);
    packed.size = compressed_size;
    assert(!captcha_data_unpack(&decoded, &packed) && !decoded.bytes);
    compressed_size = CAPTCHA_PACKED_LIMIT;
    assert(compress(packed.bytes, &compressed_size, copy, 128) == Z_OK);
    packed.size = compressed_size;
    assert(!captcha_data_unpack(&decoded, &packed));
    captcha_data_clear(&packed); free(copy); assert(!outstanding);
}

int main(void)
{
    assert(sizeof BMP_CLIENT_MAGIC == 13);
    test_stream();
    roundtrip(0, 24, 0); roundtrip(0, 8, 0); roundtrip(0, 8, 16);
    roundtrip(1, 24, 0); roundtrip(1, 8, 0); roundtrip(1, 8, 16);
    test_invalid();
    puts("Captcha data: bounded chunks, rollback, six bitmap forms, real zlib roundtrips, truncation/corruption and decode limits passed");
    return 0;
}
