#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/memory.h"

static size_t requested;
static unsigned failures, fail_next, releases;
static void *released;
static jmp_buf failure;
union AllocationAlignment { long double floating; long long integer; void *pointer; };

static void *recorded_malloc(size_t bytes)
{
    requested = bytes;
    if (fail_next) { fail_next = 0; return NULL; }
    return malloc(bytes);
}

static void recorded_free(void *memory)
{
    released = memory;
    ++releases;
    free(memory);
}

__declspec(noreturn) static void allocation_failed(void)
{
    ++failures;
    longjmp(failure, 1);
}

#define malloc recorded_malloc
#define free recorded_free
#define abort allocation_failed
#include "../src/core/memory.c"
#undef abort
#undef free
#undef malloc

int main(void)
{
    static const size_t sizes[] = {0, 1, 3, 8, 17, 64, 4095, 4096, 4097, 65537};
    size_t i;
    for (i = 0; i < sizeof sizes / sizeof sizes[0]; ++i) {
        size_t bytes = sizes[i];
        void *memory = mem_alloc(bytes);
        uintptr_t address = (uintptr_t)memory;
        assert(memory && requested == (bytes ? bytes : 1));
        assert(address % __alignof(union AllocationAlignment) == 0);
        memset(memory, 0x7b, bytes);
        if (bytes) assert(((unsigned char *)memory)[bytes - 1] == 0x7b);
        mem_free(memory);
        assert((uintptr_t)released == address);
        memory = mem_try_alloc(bytes);
        assert(memory && requested == (bytes ? bytes : 1));
        free(memory);
        memory = malloc(bytes ? bytes : 1);
        assert(memory);
        mem_free(memory);
    }
    mem_free(NULL);
    assert(!released && releases == 2 * i + 1);
    fail_next = 1;
    assert(mem_try_alloc(32) == NULL && !failures);
    fail_next = 1;
    if (!setjmp(failure)) {
        mem_alloc(32);
        assert(!"Required allocation must not return after failure");
    }
    assert(failures == 1 && requested == 32);
    puts("Application heap: C ownership, ordinary malloc/free interoperability, empty blocks, alignment and both failure policies passed");
    return 0;
}
