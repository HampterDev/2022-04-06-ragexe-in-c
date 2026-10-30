#include "work_pool_test_support.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void failures(void)
{
    struct WorkPoolMgr *pools;
    struct WorkPoolType invalid[2];
    unsigned i;
    LONG live;
    assert(!work_pool_create_with_types(NULL, 15));
    assert(!work_pool_create_with_types(pool_test_types, 0));
    assert(!work_pool_create_with_types(pool_test_types, SIZE_MAX));
    invalid[0] = invalid[1] = pool_test_types[0];
    assert(!work_pool_create_with_types(invalid, 2));
    invalid[0].initialize = NULL;
    assert(!work_pool_create_with_types(invalid, 1));
    invalid[0] = pool_test_types[0]; invalid[0].operations.run = NULL;
    assert(!work_pool_create_with_types(invalid, 1));
    invalid[0] = pool_test_types[0]; invalid[0].operations.done = NULL;
    assert(!work_pool_create_with_types(invalid, 1));
    invalid[0] = pool_test_types[0]; invalid[0].size = sizeof(struct AsyncWork) - 1;
    assert(!work_pool_create_with_types(invalid, 1));
    pool_test_fail_at = pool_test_attempts + 1;
    assert(!work_pool_mgr_create());
    assert(!pool_test_live && !pool_test_locks);
    pool_test_fail_at = 0;
    for (i = 1; i <= 15; ++i) {
        pool_test_lock_fail_at = pool_test_lock_attempts + i;
        assert(!work_pool_mgr_create());
        assert(!pool_test_live && !pool_test_locks);
    }
    pool_test_lock_fail_at = 0;
    pools = work_pool_mgr_create(); assert(pools && pool_test_locks == 15);
    live = pool_test_live;
    assert(!work_pool_mgr_acquire(pools, "missing"));
    assert(!work_pool_mgr_acquire(pools, NULL));
    assert(!work_pool_mgr_acquire(NULL, "request0"));
    for (i = 1; i <= 2; ++i) {
        pool_test_fail_at = pool_test_attempts + i;
        assert(!work_pool_mgr_acquire(pools, "request0"));
        assert(pool_test_live == live);
    }
    pool_test_fail_at = 0;
    work_pool_mgr_destroy(pools);
    work_pool_mgr_destroy(NULL);
    assert(!pool_test_live && !pool_test_locks);
}

static void reuse(void)
{
    struct WorkPoolMgr *pools = work_pool_mgr_create();
    struct TestWork *first, *second, *again;
    struct AsyncWork foreign;
    unsigned i;
    assert(pools);
    first = (struct TestWork *)work_pool_mgr_acquire(pools, "request0"); assert(first);
    memset(first->payload, 'a', sizeof first->payload);
    work_pool_mgr_release(pools, &first->base);
    work_pool_mgr_release(pools, &first->base);
    second = (struct TestWork *)work_pool_mgr_acquire(pools, "request0");
    assert(second && second != first);
    work_pool_mgr_release(pools, &second->base);
    again = (struct TestWork *)work_pool_mgr_acquire(pools, "request0");
    assert(again == first && again->payload[0] == 'a');
    work_pool_mgr_release(pools, &again->base);
    again = (struct TestWork *)work_pool_mgr_acquire(pools, "request0");
    assert(again == second);
    work_pool_mgr_release(pools, &again->base);
    work_pool_mgr_release(pools, &foreign);
    work_pool_mgr_release(NULL, &foreign);
    work_pool_mgr_release(pools, NULL);
    for (i = 0; i < 15; ++i) {
        struct AsyncWork *work = work_pool_mgr_acquire(pools, pool_test_types[i].name);
        assert(work);
        work_pool_mgr_release(pools, work);
    }
    work_pool_mgr_destroy(pools);
    assert(!pool_test_live && !pool_test_locks);
}

static DWORD WINAPI borrow_thread(void *argument)
{
    struct WorkPoolMgr *pools = argument;
    unsigned i;
    for (i = 0; i < 3000; ++i) {
        struct TestWork *work = (struct TestWork *)work_pool_mgr_acquire(pools, pool_test_types[i % 15].name);
        assert(work && InterlockedCompareExchange(&work->active, 1, 0) == 0);
        memset(work->payload, (unsigned char)i, sizeof work->payload);
        if (!(i % 7)) SwitchToThread();
        assert(work->payload[0] == (unsigned char)i && work->payload[112] == (unsigned char)i);
        assert(InterlockedExchange(&work->active, 0) == 1);
        work_pool_mgr_release(pools, &work->base);
    }
    return 0;
}

static void callbacks(void)
{
    struct WorkPoolType descriptors[2] = {pool_test_types[0], pool_test_types[1]};
    struct WorkPoolMgr *pools = work_pool_create_with_types(descriptors, 2);
    struct TestWork *short_request, *long_request;
    assert(pools);
    descriptors[0].operations.run = NULL;
    descriptors[1].operations.done = NULL;
    short_request = (struct TestWork *)work_pool_mgr_acquire(pools, "request0");
    long_request = (struct TestWork *)work_pool_mgr_acquire(pools, "request1");
    assert(short_request && long_request);
    assert(short_request->base.ops != &descriptors[0].operations);
    assert(short_request->base.ops->run(short_request) == 1);
    assert(long_request->base.ops->run(long_request) == 0);
    short_request->base.ops->done(short_request);
    long_request->base.ops->done(long_request);
    assert(short_request->payload[1] == 0xc3 && long_request->payload[1] == 0xc3);
    work_pool_mgr_release(pools, &short_request->base);
    work_pool_mgr_release(pools, &long_request->base);
    work_pool_mgr_destroy(pools);
}

static void concurrent(void)
{
    struct WorkPoolMgr *pools = work_pool_mgr_create();
    HANDLE threads[4];
    unsigned i;
    assert(pools);
    for (i = 0; i < 4; ++i) { threads[i] = CreateThread(NULL, 0, borrow_thread, pools, 0, NULL); assert(threads[i]); }
    assert(WaitForMultipleObjects(4, threads, TRUE, 30000) == WAIT_OBJECT_0);
    for (i = 0; i < 4; ++i) { DWORD code; assert(GetExitCodeThread(threads[i], &code) && code == 0); CloseHandle(threads[i]); }
    work_pool_mgr_destroy(pools);
    assert(!pool_test_live && !pool_test_locks);
}

static void manager_lifetime(void)
{
    unsigned i;
    for (i = 0; i < 100; ++i) {
        struct WorkPoolMgr *pools = work_pool_mgr_create();
        assert(pools && work_pool_mgr_acquire(pools, "request1"));
        work_pool_mgr_destroy(pools);
        assert(!pool_test_live && !pool_test_locks);
    }
}

int main(void)
{
    pool_test_begin();
    failures(); reuse(); callbacks(); concurrent(); manager_lifetime();
    pool_test_end();
    puts("Work pools: C ownership, 15 lock failures, both acquisition failures, FIFO reuse, duplicate/foreign returns, 12000 concurrent leases and 100 manager shutdown cycles passed");
    return 0;
}
