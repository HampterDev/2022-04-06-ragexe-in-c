#include "std/std.h"
#include "ui/ui_std.h"
#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *container_test_vector_allocate(size_t bytes);

static struct Block { unsigned char *raw, *data; size_t bytes, prefix; } blocks[128];
static unsigned live, allocations, failures;
static jmp_buf failed;

__declspec(noreturn) static void container_failed(void)
{
    ++failures;
    longjmp(failed, 1);
}

void *mem_alloc(size_t bytes)
{
    unsigned i;
    struct Block *block;
    uintptr_t address;
    assert(bytes && bytes < 1000000);
    for (i = 0; i < 128 && blocks[i].raw; ++i) {}
    assert(i < 128);
    block = &blocks[i];
    block->raw = malloc(bytes + 64);
    assert(block->raw);
    address = ((uintptr_t)block->raw + 23) & ~(uintptr_t)7;
    if (!(address & 31)) address += 8;
    block->data = (unsigned char *)address;
    block->prefix = block->data - block->raw;
    block->bytes = bytes;
    memset(block->raw, 0xa7, block->prefix);
    memset(block->data, 0x5b, bytes);
    memset(block->data + bytes, 0x7a, 16);
    ++live; ++allocations;
    return block->data;
}

void mem_free(void *memory)
{
    unsigned i;
    size_t j;
    struct Block *block;
    if (!memory) return;
    for (i = 0; i < 128 && blocks[i].data != memory; ++i) {}
    assert(i < 128 && blocks[i].raw && live);
    block = &blocks[i];
    for (j = 0; j < block->prefix; ++j) assert(block->raw[j] == 0xa7);
    for (j = 0; j < 16; ++j) assert(block->data[block->bytes + j] == 0x7a);
    free(block->raw);
    memset(block, 0, sizeof *block);
    --live;
}

__declspec(noreturn) void fatal_error(const char *message)
{ (void)message; container_failed(); }

#define abort container_failed
#include "../src/std/container_memory.c"
#undef abort

static void check_block(void *memory, size_t bytes)
{
    unsigned i;
    assert(memory && (uintptr_t)memory % 8 == 0 && (uintptr_t)memory % 32 != 0);
    for (i = 0; i < 128 && blocks[i].data != memory; ++i) {}
    assert(i < 128 && blocks[i].bytes == bytes);
    memset(memory, 0x4c, bytes);
}

static void check_producers(void)
{
    static const size_t sizes[] = {32, 4064, 4096, 4128, 8192, 65536};
    size_t i;
    assert(!std_allocate_bytes(0) && !std_allocate(0, 8, 10));
    assert(!container_test_vector_allocate(0));
    mem_free(NULL);
    for (i = 0; i < sizeof sizes / sizeof sizes[0]; ++i) {
        size_t bytes = sizes[i];
        void *blocks_to_free[4];
        unsigned j;
        struct Vec vector;
        blocks_to_free[0] = std_allocate_bytes(bytes);
        blocks_to_free[1] = std_allocate(bytes / 8, 8, SIZE_MAX / 8);
        blocks_to_free[2] = container_test_vector_allocate(bytes);
        blocks_to_free[3] = std_allocate_bytes(bytes);
        for (j = 0; j < 4; ++j) check_block(blocks_to_free[j], bytes);
        mem_free(blocks_to_free[0]);
        vector.first = blocks_to_free[1];
        vector.last = vector.end = (char *)vector.first + bytes;
        vec_free(&vector);
        assert(!vector.first && !vector.last && !vector.end);
        mem_free(blocks_to_free[2]);
        vector.first = blocks_to_free[3];
        vector.last = vector.end = (char *)vector.first + bytes;
        vec_free(&vector);
        assert(!vector.first && !vector.last && !vector.end);
        assert(!live);
    }
}

static void check_values(void)
{
    struct Vec vector = {0};
    struct String text, copied;
    char value[8192];
    unsigned i;
    memset(value, 'x', sizeof value);
    for (i = 0; i < 1600; ++i)
        vec_push(&vector, &i, sizeof i);
    for (i = 0; i < 1600; ++i) assert(((unsigned *)vector.first)[i] == i);
    vec_free(&vector);
    for (i = 0; i < 2; ++i) {
        string_init(&text);
        string_assign_cstr(&text, "한글");
        assert(!strcmp(string_cstr(&text), "한글"));
        string_assign(&text, value, sizeof value);
        assert(text.size == sizeof value && !memcmp(string_cstr(&text), value, sizeof value));
        string_copy(&copied, &text);
        string_free(&text);
        assert(copied.size == sizeof value && !memcmp(string_cstr(&copied), value, sizeof value));
        string_free(&copied);
    }
    assert(!live);
}

int main(void)
{
    check_producers();
    check_values();
    if (!setjmp(failed)) { std_allocate(2, SIZE_MAX, SIZE_MAX); assert(!"multiplication overflow"); }
    if (!setjmp(failed)) { std_allocate(3, 8, 2); assert(!"element limit"); }
    assert(failures == 2 && !live && allocations > 40);
    puts("Container memory: four former allocation paths, unsized mixed frees, unaligned-to-32 buffers, guarded growth, large mail/string ownership and overflow passed");
    return 0;
}
