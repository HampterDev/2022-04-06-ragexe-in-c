#ifndef WORK_POOL_TEST_SUPPORT_H
#define WORK_POOL_TEST_SUPPORT_H

#include <windows.h>
#include <stddef.h>
#include "base.h"
#include "web/work.h"
#include "web/http_post.h"
#include "web/requests.h"
#include "web/work_pool_internal.h"
#include "web/work_manager.h"

struct TestWork {
    struct AsyncWork base;
    LONG active;
    unsigned kind;
    unsigned char payload[113];
    unsigned tail;
};

extern LONG pool_test_live, pool_test_attempts, pool_test_fail_at;
extern LONG pool_test_locks, pool_test_lock_attempts, pool_test_lock_fail_at;
extern LONG pool_test_initialized, pool_test_disposed;
extern struct WorkPoolType pool_test_types[15];

void pool_test_begin(void);
void pool_test_end(void);
void *pool_test_calloc(size_t count, size_t size);
void pool_test_free(void *memory);
BOOL WINAPI pool_test_lock_init(LPCRITICAL_SECTION lock, DWORD spins);
void WINAPI pool_test_lock_destroy(LPCRITICAL_SECTION lock);

#endif
