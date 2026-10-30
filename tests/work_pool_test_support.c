#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "work_pool_test_support.h"

#define calloc pool_test_calloc
#define free pool_test_free
#define InitializeCriticalSectionAndSpinCount pool_test_lock_init
#define DeleteCriticalSection pool_test_lock_destroy
#include "../src/web/work_pool.c"
#undef DeleteCriticalSection
#undef InitializeCriticalSectionAndSpinCount
#undef free
#undef calloc

LONG pool_test_live, pool_test_attempts, pool_test_fail_at;
LONG pool_test_locks, pool_test_lock_attempts, pool_test_lock_fail_at;
LONG pool_test_initialized, pool_test_disposed;

static CRITICAL_SECTION allocation_lock;
static struct Allocation { unsigned char *raw; size_t size; } allocations[1024];

void pool_test_begin(void) { InitializeCriticalSection(&allocation_lock); }
void pool_test_end(void)
{
    assert(!pool_test_live && !pool_test_locks);
    assert(pool_test_initialized == pool_test_disposed);
    DeleteCriticalSection(&allocation_lock);
}

void *pool_test_calloc(size_t count, size_t size)
{
    unsigned i;
    size_t bytes;
    assert(!size || count <= SIZE_MAX / size);
    bytes = count * size;
    assert(bytes <= SIZE_MAX - 64);
    EnterCriticalSection(&allocation_lock);
    if (++pool_test_attempts == pool_test_fail_at) {
        LeaveCriticalSection(&allocation_lock);
        return NULL;
    }
    for (i = 0; i < 1024 && allocations[i].raw; ++i) {}
    assert(i < 1024);
    allocations[i].raw = malloc(bytes + 64); assert(allocations[i].raw);
    allocations[i].size = bytes;
    memset(allocations[i].raw, 0xa5, bytes + 64);
    memset(allocations[i].raw + 32, 0, bytes);
    ++pool_test_live;
    LeaveCriticalSection(&allocation_lock);
    return allocations[i].raw + 32;
}

void pool_test_free(void *memory)
{
    unsigned i, j;
    if (!memory) return;
    EnterCriticalSection(&allocation_lock);
    for (i = 0; i < 1024 && (!allocations[i].raw || allocations[i].raw + 32 != memory); ++i) {}
    assert(i < 1024);
    for (j = 0; j < 32; ++j) {
        assert(allocations[i].raw[j] == 0xa5);
        assert(allocations[i].raw[32 + allocations[i].size + j] == 0xa5);
    }
    free(allocations[i].raw); allocations[i].raw = NULL;
    --pool_test_live;
    LeaveCriticalSection(&allocation_lock);
}

BOOL WINAPI pool_test_lock_init(LPCRITICAL_SECTION lock, DWORD spins)
{
    if (InterlockedIncrement(&pool_test_lock_attempts) == pool_test_lock_fail_at)
        return FALSE;
    assert(InitializeCriticalSectionAndSpinCount(lock, spins));
    InterlockedIncrement(&pool_test_locks);
    return TRUE;
}

void WINAPI pool_test_lock_destroy(LPCRITICAL_SECTION lock)
{
    assert(InterlockedDecrement(&pool_test_locks) >= 0);
    DeleteCriticalSection(lock);
}

struct LargeTestWork { struct TestWork common; unsigned char extra[8193]; };

static void initialize_small(void *pointer)
{
    struct TestWork *work = pointer;
    assert(!work->active && !work->tail);
    work->kind = 1; work->tail = 0x12345678;
    InterlockedIncrement(&pool_test_initialized);
}

static void initialize_large(void *pointer)
{
    struct LargeTestWork *work = pointer;
    initialize_small(&work->common);
    work->common.kind = 2;
    memset(work->extra, 0x6d, sizeof work->extra);
}

static void dispose_work(void *pointer)
{
    struct TestWork *work = pointer;
    assert(!work->active && work->tail == 0x12345678);
    if (work->kind == 2) {
        struct LargeTestWork *large = pointer;
        size_t i;
        for (i = 0; i < sizeof large->extra; ++i) assert(large->extra[i] == 0x6d);
    }
    work->tail = 0;
    InterlockedIncrement(&pool_test_disposed);
}

static char run_small(void *pointer)
{
    struct TestWork *work = pointer;
    assert(work->kind == 1);
    work->payload[0] = 21;
    return 1;
}

static char run_large(void *pointer)
{
    struct TestWork *work = pointer;
    assert(work->kind == 2);
    work->payload[0] = 34;
    return 0;
}

static void complete_work(void *pointer)
{
    struct TestWork *work = pointer;
    assert(work->payload[0] == (work->kind == 1 ? 21 : 34));
    work->payload[1] = 0xc3;
}

#define SMALL(n) {"request" #n, sizeof(struct TestWork), initialize_small, dispose_work, {run_small, complete_work}}
#define LARGE(n) {"request" #n, sizeof(struct LargeTestWork), initialize_large, dispose_work, {run_large, complete_work}}
struct WorkPoolType pool_test_types[15] = {
    SMALL(0), LARGE(1), SMALL(2), LARGE(3), SMALL(4), LARGE(5), SMALL(6), LARGE(7),
    SMALL(8), LARGE(9), SMALL(10), LARGE(11), SMALL(12), LARGE(13), SMALL(14)
};

struct WorkPoolMgr *work_pool_mgr_create(void)
{
    return work_pool_create_with_types(pool_test_types, 15);
}
