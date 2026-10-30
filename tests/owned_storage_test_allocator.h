#ifndef OWNED_STORAGE_TEST_ALLOCATOR_H
#define OWNED_STORAGE_TEST_ALLOCATOR_H

struct Allocation { size_t size; unsigned marker; unsigned char padding[24]; };
static unsigned allocation_calls, fail_at, outstanding;

static void *checked_malloc(size_t size)
{
    struct Allocation *block;
    unsigned char *data;
    if (++allocation_calls == fail_at)
        return NULL;
    block = malloc(sizeof *block + size + 16);
    assert(block);
    block->size = size;
    block->marker = 0x8af04d31;
    memset(block->padding, 0xa6, sizeof block->padding);
    data = (unsigned char *)(block + 1);
    memset(data, 0xcb, size);
    memset(data + size, 0x6a, 16);
    ++outstanding;
    return data;
}

static void checked_free(void *memory)
{
    struct Allocation *block;
    unsigned i;
    if (!memory) return;
    block = (struct Allocation *)memory - 1;
    assert(block->marker == 0x8af04d31);
    for (i = 0; i < sizeof block->padding; ++i) assert(block->padding[i] == 0xa6);
    for (i = 0; i < 16; ++i) assert(((unsigned char *)memory)[block->size + i] == 0x6a);
    block->marker = 0;
    assert(outstanding);
    --outstanding;
    free(block);
}

#endif
