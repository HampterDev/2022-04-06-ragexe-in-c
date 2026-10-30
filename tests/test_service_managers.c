#include "web/service_managers.h"
#include "service_manager_test_support.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static struct Allocation { unsigned char *raw; size_t size; int record; } allocations[512];
static int live, record_live, fail_record, record_attempts;
static int text_attempts, fail_text = -1;

static void *allocate(size_t size, int record)
{
    unsigned i;
    assert(size <= SIZE_MAX - 32);
    for (i = 0; i < 512 && allocations[i].raw; ++i) {}
    assert(i < 512);
    allocations[i].raw = malloc(size + 32); assert(allocations[i].raw);
    memset(allocations[i].raw, 0xa5, size + 32);
    allocations[i].size = size; allocations[i].record = record;
    ++live; record_live += record;
    return allocations[i].raw + 16;
}

static void release(void *memory, size_t size, int record)
{
    unsigned i, j;
    if (!memory) return;
    for (i = 0; i < 512 && (!allocations[i].raw || allocations[i].raw + 16 != memory); ++i) {}
    assert(i < 512 && allocations[i].record == record);
    assert(record || allocations[i].size == size);
    for (j = 0; j < 16; ++j) {
        assert(allocations[i].raw[j] == 0xa5);
        assert(allocations[i].raw[16 + allocations[i].size + j] == 0xa5);
    }
    free(allocations[i].raw); allocations[i].raw = NULL;
    --live; record_live -= record;
}

void *service_test_calloc(size_t count, size_t size)
{
    void *memory;
    ++record_attempts;
    if (fail_record) return NULL;
    assert(!size || count <= SIZE_MAX / size);
    memory = allocate(count * size, 1); memset(memory, 0, count * size);
    return memory;
}

void service_test_free(void *memory)
{
    assert(memory != g_accountUserDataMgr && memory != g_charUserDataMgr && memory != g_adventurerAgency && memory != g_emblemMgr && memory != g_twitterDataMgr);
    release(memory, 0, 1);
}

void *service_text_malloc(size_t size)
{
    if (text_attempts++ == fail_text) return NULL;
    return allocate(size, 0);
}

void service_text_free(void *memory)
{
    unsigned i;
    if (!memory) return;
    for (i = 0; i < 512 && (!allocations[i].raw || allocations[i].raw + 16 != memory); ++i) {}
    assert(i < 512);
    release(memory, allocations[i].size, 0);
}

void *mem_alloc(size_t size) { return allocate(size, 0); }
void mem_free(void *memory)
{ service_text_free(memory); }
void fatal_error(const char *message) { (void)message; assert(0); abort(); }

int main(void)
{
    int i, attempts;
    assert(!twitter_has_service(NULL) && !*twitter_auth_url(NULL));
    emblem_mgr_destroy(); twitter_destroy();
    assert(!account_user_data_is_loaded(NULL));
    assert(!account_user_data_pending_saves(NULL));
    assert(!adventurer_agency_is_recruiting(NULL));
    assert(!adventurer_agency_pending_requests(NULL));
    account_user_data_destroy(); char_user_data_destroy(); adventurer_agency_destroy();
    fail_record = 1;
    assert(!account_user_data_create() && !g_accountUserDataMgr);
    assert(!char_user_data_create() && !g_charUserDataMgr);
    assert(!adventurer_agency_create() && !g_adventurerAgency);
    assert(!emblem_mgr_create() && !g_emblemMgr);
    assert(!twitter_create() && !g_twitterDataMgr);
    assert(!live && record_attempts == 5);
    fail_record = 0;
    text_attempts = 0; fail_text = 0;
    assert(!account_user_data_create() && !g_accountUserDataMgr && !live);
    text_attempts = 0;
    assert(!char_user_data_create() && !g_charUserDataMgr && !live);
    for (i = 0; i < 2; ++i) {
        text_attempts = 0; fail_text = i;
        assert(!twitter_create() && !g_twitterDataMgr && !live);
    }
    fail_text = -1;
    service_urls_missing = 1;
    assert(!twitter_create() && !g_twitterDataMgr && !live);
    service_urls_missing = 0;
    for (i = 0; i < 100; ++i) {
        assert(account_user_data_create() && char_user_data_create() && adventurer_agency_create());
        assert(emblem_mgr_create() && twitter_create());
        assert(record_live == 5);
        attempts = record_attempts;
        assert(!account_user_data_create() && !char_user_data_create() && !adventurer_agency_create());
        assert(!emblem_mgr_create() && !twitter_create());
        assert(record_attempts == attempts);
        service_test_media_state();
        service_test_account_state(); service_test_character_state(); service_test_agency_state();
        account_user_data_destroy(); char_user_data_destroy(); adventurer_agency_destroy();
        emblem_mgr_destroy(); twitter_destroy();
        assert(!g_accountUserDataMgr && !g_charUserDataMgr && !g_adventurerAgency && !live && !record_live);
        account_user_data_destroy(); char_user_data_destroy(); adventurer_agency_destroy();
        emblem_mgr_destroy(); twitter_destroy();
    }
    puts("Service managers: five C owners, checked record/text construction, URL queries, long strings, 65 bookings and 100 shutdown cycles passed");
    return 0;
}
