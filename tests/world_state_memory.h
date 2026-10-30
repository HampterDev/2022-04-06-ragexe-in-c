#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "core/memory.h"
#include "core/fatal.h"

static struct { unsigned char *memory; size_t size; } blocks[2048];
static unsigned live_blocks;

void *mem_alloc(size_t size)
{
    unsigned i;
    unsigned char *p = malloc(size + 16);
    assert(p);
    memset(p, 0xa5, size + 16);
    for (i = 0; i < 2048 && blocks[i].memory; ++i) {}
    assert(i < 2048);
    blocks[i].memory = p;
    blocks[i].size = size;
    ++live_blocks;
    return p;
}

void mem_free(void *p)
{
    size_t size;
    unsigned i, j;
    if (!p) return;
    for (i = 0; i < 2048 && blocks[i].memory != p; ++i) {}
    assert(i < 2048);
    size = blocks[i].size;
    for (j = 0; j < 16; ++j) assert(blocks[i].memory[size + j] == 0xa5);
    free(p);
    blocks[i].memory = NULL;
    --live_blocks;
}

__declspec(noreturn) void fatal_error(const char *message)
{ (void)message; abort(); }
