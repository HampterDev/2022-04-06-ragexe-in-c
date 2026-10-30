#ifndef TEST_JPEG_FIXTURE_H
#define TEST_JPEG_FIXTURE_H
#include "resources/jpeg_codec.h"
#include "platform/ijl_api.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct { unsigned char *base; size_t size; } jpeg_blocks[32];
static int jpeg_live, jpeg_allocations, jpeg_fail_allocation;
static int jpeg_inits, jpeg_frees, jpeg_headers, jpeg_reads, jpeg_writes, jpeg_active;
static int jpeg_fail_init, jpeg_fail_header, jpeg_fail_read, jpeg_fail_write;
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

static int __stdcall jpeg_test_init(struct IjlProps *props)
{
    size_t i;
    const unsigned char *bytes = (const unsigned char *)props;
    for (i = 0; i < sizeof *props; ++i) assert(bytes[i] == 0);
    ++jpeg_inits;
    if (jpeg_fail_init) return -1;
    assert(!jpeg_active);
    jpeg_active = 1;
    props->useProps = 12345;
    return 0;
}

static int __stdcall jpeg_test_free_props(struct IjlProps *props)
{
    assert(jpeg_active && props->useProps == 12345);
    ++jpeg_frees;
    jpeg_active = 0;
    return 0;
}

static int __stdcall jpeg_test_read(struct IjlProps *props, int operation)
{
    int x, y;
    size_t stride;
    assert(jpeg_active && props->useProps == 12345);
    assert(props->jpgBytes == jpeg_input && props->jpgSizeBytes == sizeof jpeg_input);
    if (operation == IJL_JBUFF_READPARAMS) {
        ++jpeg_headers;
        props->jpgWidth = jpeg_width;
        props->jpgHeight = jpeg_height;
        return jpeg_fail_header ? -2 : 0;
    }
    assert(operation == IJL_JBUFF_READWHOLEIMAGE);
    ++jpeg_reads;
    assert(props->dibWidth == jpeg_width && props->dibHeight == jpeg_height);
    assert(props->dibColor == IJL_BGR && props->dibChannels == 3);
    stride = ((size_t)jpeg_width * 3 + 3) & ~(size_t)3;
    assert(props->dibPadBytes == stride - (size_t)jpeg_width * 3);
    for (y = 0; y < jpeg_height; ++y) {
        unsigned char *row = props->dibBytes + (size_t)y * stride;
        for (x = 0; x < jpeg_width; ++x) {
            row[3*x] = (unsigned char)(10 + x);
            row[3*x+1] = (unsigned char)(20 + y);
            row[3*x+2] = (unsigned char)(30 + x + y);
        }
    }
    props->dibBytes[0] = 254;
    props->dibBytes[1] = 3;
    props->dibBytes[2] = 255;
    return jpeg_fail_read ? -3 : 0;
}

static int __stdcall jpeg_test_write(struct IjlProps *props, int operation)
{
    size_t bytes;
    assert(jpeg_active && props->useProps == 12345);
    assert(operation == IJL_JFILE_WRITEWHOLEIMAGE);
    assert(props->jpgFile && props->jpgFile[0]);
    assert(props->dibColor == IJL_BGR && props->dibChannels == 3);
    assert(props->dibWidth == props->jpgWidth && props->dibHeight == props->jpgHeight);
    ++jpeg_writes;
    jpeg_written_stride = (size_t)props->dibWidth * 3 + props->dibPadBytes;
    jpeg_written_width = props->jpgWidth;
    jpeg_written_height = props->jpgHeight;
    bytes = jpeg_written_stride * (size_t)jpeg_written_height;
    assert(bytes <= sizeof jpeg_written);
    memcpy(jpeg_written, props->dibBytes, bytes);
    return jpeg_fail_write ? -4 : 0;
}

#define calloc jpeg_test_calloc
#define free jpeg_test_free
#define ijlInit jpeg_test_init
#define ijlFree jpeg_test_free_props
#define ijlRead jpeg_test_read
#define ijlWrite jpeg_test_write
#include "../src/resources/jpeg_codec.c"
#undef calloc
#undef free
#undef ijlInit
#undef ijlFree
#undef ijlRead
#undef ijlWrite

static void jpeg_fixture_reset(void)
{
    assert(jpeg_live == 0 && jpeg_active == 0);
    jpeg_allocations = jpeg_fail_allocation = 0;
    jpeg_inits = jpeg_frees = jpeg_headers = jpeg_reads = jpeg_writes = 0;
    jpeg_fail_init = jpeg_fail_header = jpeg_fail_read = jpeg_fail_write = 0;
    jpeg_width = 3;
    jpeg_height = 2;
    memset(jpeg_written, 0xcc, sizeof jpeg_written);
}
#endif
