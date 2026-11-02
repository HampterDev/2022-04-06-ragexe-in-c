#define ENCODING_FIXTURE_IMPLEMENTATION
#include "encoding_fixture.h"
#include "platform/code_page.h"
#include <assert.h>

unsigned encoding_allocations, encoding_live, encoding_fail_allocation;
unsigned encoding_calls, encoding_fail_call;

void *encoding_test_malloc(size_t bytes)
{
    void *result;
    if (++encoding_allocations == encoding_fail_allocation)
        return NULL;
    result = malloc(bytes);
    if (result)
        ++encoding_live;
    return result;
}

void encoding_test_free(void *memory)
{
    if (memory) {
        assert(encoding_live);
        --encoding_live;
        free(memory);
        SetLastError(0xdead);
    }
}

int encoding_test_to_wide(unsigned int cp, const char *input, int bytes, uint16_t *out, int count)
{
    if (++encoding_calls == encoding_fail_call)
        return 0;
    return platform_code_page_to_utf16(cp, input, bytes, out, count);
}

int encoding_test_from_wide(unsigned int cp, const uint16_t *input, int count, char *out, int bytes)
{
    if (++encoding_calls == encoding_fail_call)
        return 0;
    return platform_code_page_from_utf16(cp, input, count, out, bytes);
}

void encoding_test_reset(void)
{
    assert(!encoding_live);
    encoding_allocations = encoding_fail_allocation = 0;
    encoding_calls = encoding_fail_call = 0;
}
