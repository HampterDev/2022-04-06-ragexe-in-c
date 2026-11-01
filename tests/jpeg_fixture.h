#ifndef TEST_JPEG_FIXTURE_H
#define TEST_JPEG_FIXTURE_H
#include "resources/jpeg_codec.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct { unsigned char *base; size_t size; } jpeg_blocks[32];
static int jpeg_live, jpeg_allocations, jpeg_fail_allocation;
static int jpeg_reads, jpeg_writes;
static int jpeg_fail_read, jpeg_fail_write;
static int jpeg_width = 3, jpeg_height = 2;
static unsigned char jpeg_written[4096];
static size_t jpeg_written_stride;
static int jpeg_written_width, jpeg_written_height;
static const unsigned char jpeg_input[] = {0xff, 0xd8, 1, 2, 3, 0xff, 0xd9};

static void *jpeg_test_calloc(size_t count, size_t size)
{
    unsigned char *p;
    size_t bytes;
    int i;
    assert(!size || count <= SIZE_MAX / size);
    bytes = count * size;
    ++jpeg_allocations;
    if (jpeg_allocations == jpeg_fail_allocation)
        return NULL;
    assert(bytes < 1024 * 1024);
    p = malloc(bytes + 32);
    assert(p);
    memset(p, 0xa5, bytes + 32);
    memset(p + 16, 0, bytes);
    for (i = 0; i < 32 && jpeg_blocks[i].base; ++i) {}
    assert(i < 32);
    jpeg_blocks[i].base = p;
    jpeg_blocks[i].size = bytes;
    ++jpeg_live;
    return p + 16;
}

static void jpeg_test_free(void *memory)
{
    int i, j;
    if (!memory) return;
    for (i = 0; i < 32 && (!jpeg_blocks[i].base || jpeg_blocks[i].base + 16 != memory); ++i) {}
    assert(i < 32);
    for (j = 0; j < 16; ++j) {
        assert(jpeg_blocks[i].base[j] == 0xa5);
        assert(jpeg_blocks[i].base[16 + jpeg_blocks[i].size + j] == 0xa5);
    }
    free(jpeg_blocks[i].base);
    jpeg_blocks[i].base = NULL;
    --jpeg_live;
}

#define calloc jpeg_test_calloc
#define free jpeg_test_free
#include "../src/resources/jpeg_storage.c"
#undef calloc
#undef free

int jpeg_image_read(struct JpegImage *image, const void *data, size_t size)
{
    struct JpegImage next = {0};
    int x, y;
    assert(data == jpeg_input && size == sizeof jpeg_input);
    ++jpeg_reads;
    if (jpeg_fail_read || !jpeg_image_create(&next, jpeg_width, jpeg_height)) return 0;
    for (y = 0; y < jpeg_height; ++y) {
        unsigned char *row = next.pixels + (size_t)y * next.stride;
        for (x = 0; x < jpeg_width; ++x) {
            row[x * 3] = (unsigned char)(10 + x);
            row[x * 3 + 1] = (unsigned char)(20 + y);
            row[x * 3 + 2] = (unsigned char)(30 + x + y);
        }
    }
    next.pixels[0] = 254;
    next.pixels[1] = 3;
    next.pixels[2] = 255;
    jpeg_image_destroy(image);
    *image = next;
    return 1;
}

int jpeg_image_write(const struct JpegImage *image, FILE *file)
{
    size_t bytes = image->stride * (size_t)image->height;
    assert(file && bytes <= sizeof jpeg_written);
    ++jpeg_writes;
    jpeg_written_stride = image->stride;
    jpeg_written_width = image->width;
    jpeg_written_height = image->height;
    memcpy(jpeg_written, image->pixels, bytes);
    return !jpeg_fail_write;
}

static void jpeg_fixture_reset(void)
{
    assert(jpeg_live == 0);
    jpeg_allocations = jpeg_fail_allocation = 0;
    jpeg_reads = jpeg_writes = 0;
    jpeg_fail_read = jpeg_fail_write = 0;
    jpeg_width = 3;
    jpeg_height = 2;
    memset(jpeg_written, 0xcc, sizeof jpeg_written);
}
#endif
