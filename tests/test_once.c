#include "core/once.h"
#include <assert.h>
#include <process.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static INIT_ONCE shared_once = INIT_ONCE_STATIC_INIT;
static INIT_ONCE nested_once = INIT_ONCE_STATIC_INIT;
static HANDLE start_event;
static LONG builds, nested_builds;
static unsigned payload[256];

static void on_exit(void)
{
    assert(builds == 1);
    assert(nested_builds == 1);
    puts("once: concurrent publication, nested initialization and teardown passed");
}

static unsigned __stdcall worker(void *unused)
{
    unsigned pass, i;
    (void)unused;
    assert(WaitForSingleObject(start_event, 5000) == WAIT_OBJECT_0);
    for (pass = 0; pass < 100; ++pass) {
        if (once_begin(&shared_once)) {
            InterlockedIncrement(&builds);
            if (once_begin(&nested_once)) {
                InterlockedIncrement(&nested_builds);
                once_complete(&nested_once);
            }
            for (i = 0; i < 256; ++i)
                payload[i] = i * i + 37;
            Sleep(5);
            assert(atexit(on_exit) == 0);
            once_complete(&shared_once);
        }
        for (i = 0; i < 256; ++i)
            assert(payload[i] == i * i + 37);
    }
    return 0;
}

int main(void)
{
    HANDLE threads[32];
    unsigned i;
    start_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    assert(start_event != NULL);
    for (i = 0; i < 32; ++i) {
        threads[i] = (HANDLE)(uintptr_t)_beginthreadex(NULL, 0, worker, NULL, 0, NULL);
        assert(threads[i] != NULL);
    }
    assert(SetEvent(start_event));
    assert(WaitForMultipleObjects(32, threads, TRUE, 10000) == WAIT_OBJECT_0);
    assert(builds == 1 && nested_builds == 1);
    for (i = 0; i < 32; ++i)
        CloseHandle(threads[i]);
    CloseHandle(start_event);
    return 0;
}
